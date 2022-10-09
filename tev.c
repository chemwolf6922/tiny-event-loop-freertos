#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "tev.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "list.h"
#include "cHeap/heap.h"
#include "map/map.h"


/* structs */

typedef struct
{
    // event fifo
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t semaphore;
    list_handle_t fifo;
    // events
    map_handle_t events;
    // timers is a minimum heap
    heap_handle_t timers;
    map_handle_t timer_handles;
    tev_timeout_handle_t timer_handle_seed;
} tev_t;

typedef int64_t timestamp_t;
typedef struct
{
    timestamp_t target;
    tev_timeout_handle_t handle;
    void(*handler)(void* ctx);
    void *ctx;
} tev_timeout_t;

typedef struct
{
    tev_event_handler_t handler;
    void* handler_ctx;
} tev_event_t;

typedef struct
{
    tev_event_handle_t handle;
    void* data;
    int len;
} tev_event_data_t;

/* pre defined functions */
static tev_timeout_handle_t get_next_timer_handle(tev_t* tev);
static timestamp_t get_now_ms(void);
static bool compare_timeout(void* A,void* B);
static tev_event_data_t* tev_event_data_new(tev_event_handle_t handle, void* data, int len);
static void tev_event_data_free(tev_event_data_t* event);
static void tev_event_data_free_with_ctx(void* data, void* ctx);

/* Flow control */

tev_handle_t tev_create_ctx(void)
{
    tev_t *tev = malloc(sizeof(tev_t));
    if(!tev)
        goto error;
    memset(tev,0,sizeof(tev_t));

    tev->mutex = NULL;
    tev->semaphore = NULL;
    tev->fifo = NULL;
    tev->timers = NULL;
    tev->timer_handles = NULL;
    tev->events = NULL;
    tev->timer_handle_seed = NULL;

    // create fifo
    tev->fifo = list_new();
    if(!tev->fifo)
        goto error;
    tev->mutex = xSemaphoreCreateMutex();
    if(!tev->mutex)
        goto error;
    tev->semaphore = xSemaphoreCreateCounting(INT_MAX,0);

    // create event map
    tev->events = map_create();
    if(!tev->events)
        goto error;
    // create timer heap
    tev->timers = heap_create(compare_timeout);
    if(!tev->timers)
        goto error;
    // create timer handle map
    tev->timer_handles = map_create();
    if(!tev->timer_handles)
        goto error;

    return (tev_handle_t)tev;
error:
    if(tev)
    {
        if(tev->semaphore)
            vSemaphoreDelete(tev->semaphore);
        if(tev->mutex)
            vSemaphoreDelete(tev->mutex);
        if(tev->fifo)
            list_free(tev->fifo,NULL,NULL);
        if(tev->timer_handles != NULL)
            map_delete(tev->timer_handles,NULL,NULL);
        if(tev->timers != NULL)
            heap_free(tev->timers,NULL);
        if(tev->events != NULL)
            map_delete(tev->events,NULL,NULL);
        free(tev);
    }
    return NULL;
}

static void free_with_ctx(void* ptr,void* ctx)
{
    free(ptr);
}

void tev_free_ctx(tev_handle_t handle)
{
    tev_t* tev = (tev_t*)handle;
    if(tev)
    {
        if(tev->timers)
            heap_free(tev->timers,free);
        if(tev->timer_handles)
            map_delete(tev->timer_handles,NULL,NULL);
        if(tev->events)
            map_delete(tev->events,free_with_ctx,NULL);
        if(tev->fifo)
            list_free(tev->fifo,tev_event_data_free_with_ctx,NULL);
        if(tev->semaphore)
            vSemaphoreDelete(tev->semaphore);
        if(tev->mutex)
            vSemaphoreDelete(tev->mutex);
        free(tev);
    }
}


void tev_main_loop(tev_handle_t handle)
{
    tev_t *tev = (tev_t *)handle;
    if(!tev)
        return;
    
    int next_timeout;
    for(;;)
    {
        next_timeout = 0;
        // process due timers
        if(heap_get_length(tev->timers)!=0)
        {
            timestamp_t now = get_now_ms();
            for(;;)
            {
                tev_timeout_t *p_timeout = heap_get(tev->timers);
                if(p_timeout == NULL)
                    break;
                if(p_timeout->target <= now)
                {
                    // heap does not manage values like array do.
                    heap_pop(tev->timers);
                    map_remove(tev->timer_handles,&p_timeout->handle,sizeof(tev_timeout_handle_t));
                    if(p_timeout->handler != NULL)
                        p_timeout->handler(p_timeout->ctx);
                    free(p_timeout);
                }
                else
                {
                    next_timeout = p_timeout->target - now;
                    break;
                }
            }
        }
        // are there any files to listen to
        if(next_timeout == 0 && map_get_length(tev->events)!=0)
        {
            next_timeout = -1;
        }
        // wait for timers & fds
        if(next_timeout == 0)
        {
            // neither timer nor fds exist
            break;
        }
        
        // timeout ms to tick
        uint32_t next_timeout_tick = 0;
        if(next_timeout == -1)
            next_timeout_tick = portMAX_DELAY;
        else
            next_timeout_tick = next_timeout/portTICK_PERIOD_MS;
        
        int has_event = xSemaphoreTake(tev->semaphore,next_timeout_tick);

        // handle event
        if(has_event == pdTRUE)
        {
            tev_event_data_t* event_data = NULL;
            xSemaphoreTake(tev->mutex,portMAX_DELAY);
            event_data = list_shift(tev->fifo);
            xSemaphoreGive(tev->mutex);
            if(event_data)
            {
                tev_event_t* event = map_get(tev->events,&(event_data->handle),sizeof(event_data->handle));
                if(event)
                {
                    if(event->handler)
                        event->handler(event_data->data,event_data->len,event->handler_ctx);
                }
                tev_event_data_free(event_data);
            }
        }
    }
}



/* Timeout */

static tev_timeout_handle_t get_next_timer_handle(tev_t* tev)
{
    tev->timer_handle_seed++;
    if(tev->timer_handle_seed==NULL)
        tev->timer_handle_seed = (void*)1;
    return tev->timer_handle_seed;
}

static timestamp_t get_now_ms(void)
{
    return ((timestamp_t)xTaskGetTickCount()) * (timestamp_t)portTICK_PERIOD_MS;
}

static bool compare_timeout(void* A,void* B)
{
    tev_timeout_t* timeout_A = (tev_timeout_t*)A;
    tev_timeout_t* timeout_B = (tev_timeout_t*)B;
    return timeout_A->target > timeout_B->target;
}

tev_timeout_handle_t tev_set_timeout(tev_handle_t handle, tev_timeout_handler_t handler, void *ctx, int64_t timeout_ms)
{
    if(handle == NULL)
        return NULL;
    tev_t *tev = (tev_t *)handle;
    timestamp_t target = get_now_ms() + timeout_ms;
    tev_timeout_t* new_timeout = malloc(sizeof(tev_timeout_t));
    if(!new_timeout)
        return NULL;
    new_timeout->target = target;
    new_timeout->ctx = ctx;
    new_timeout->handler = handler;
    new_timeout->handle = get_next_timer_handle(tev);
    if(!heap_add(tev->timers,new_timeout))
    {
        free(new_timeout);
        return NULL;
    }
    if(map_add(tev->timer_handles,&new_timeout->handle,sizeof(tev_timeout_handle_t),new_timeout)==NULL)
    {
        heap_delete(tev->timers,new_timeout);
        free(new_timeout);
        return NULL;
    }
    return new_timeout->handle;
}

static bool match_by_data_ptr(void* data, void* arg)
{
    return data == arg;
}

int tev_clear_timeout(tev_handle_t tev_handle, tev_timeout_handle_t handle)
{
    if(tev_handle == NULL)
        return -1;
    tev_t * tev = (tev_t *)tev_handle;
    tev_timeout_t* timeout = map_remove(tev->timer_handles,&handle,sizeof(tev_timeout_handle_t));
    if(timeout)
    {
        if(heap_delete(tev->timers,timeout)) 
            free(timeout);
    }
    return 0;
}

/* Event handler */

static tev_event_data_t* tev_event_data_new(tev_event_handle_t handle, void* data, int len)
{
    tev_event_data_t* event_data = NULL;

    event_data = malloc(sizeof(tev_event_data_t));
    if(!event_data)
        goto error;
    memset(event_data,0,sizeof(tev_event_data_t));

    event_data->handle = handle;
    if(data != NULL && len != 0)
    {
        event_data->data = malloc(len);
        if(!event_data->data)
            goto error;
        memcpy(event_data->data,data,len);
    }

    return event_data;
error:
    if(event_data)
    {
        if(event_data->data)
            free(event_data->data);
        free(event_data);
    }
    return NULL;
} 

static void tev_event_data_free(tev_event_data_t* event)
{
    if(event)
    {
        if(event->data)
            free(event->data);
        free(event);
    }
}

static void tev_event_data_free_with_ctx(void* data, void* ctx)
{
    tev_event_data_free((tev_event_data_t*)data);
}

tev_event_handle_t tev_set_event_handler(tev_handle_t handle, tev_event_handler_t handler, void* ctx)
{
    tev_event_t* event = NULL;
    bool event_added = false;

    tev_t *tev = (tev_t *)handle;
    if(!tev)
        goto error;

    /* create fd_handler */
    event = malloc(sizeof(tev_event_t));
    if(!event)
        goto error;
    memset(event,0,sizeof(tev_event_t));
    event->handler = handler;
    event->handler_ctx = ctx;

    /* add event to map */
    if(map_add(tev->events,&event,sizeof(event),event) == NULL)
        goto error;
    event_added = true;

    return (tev_event_handle_t)event;
error:
    if(event)
    {
        if(event_added)
            map_remove(tev->events,&event,sizeof(event));
        free(event);
    }
    return NULL;
}

int tev_clear_event_handler(tev_handle_t handle, tev_event_handle_t event_handle)
{
    tev_t* tev = (tev_t*)handle;
    if(!tev)
        goto error;

    tev_event_t* event = map_remove(tev->events,&event_handle,sizeof(event_handle));
    if(event)
        free(event);

    return 0;
error:
    return -1;
}

int tev_send_event(tev_handle_t handle, tev_event_handle_t event, void* data, int len)
{
    int ret = 0;
    /* This should not be called from ISR */
    if(xPortIsInsideInterrupt()==pdTRUE)
        return -1;

    tev_t* tev = (tev_t*)handle;
    if(!tev)
        return -1;

    tev_event_data_t* event_data = tev_event_data_new(event,data,len);
    if(!event_data)
        return -1;

    xSemaphoreTake(tev->mutex,portMAX_DELAY);
    
    if(list_push(tev->fifo,event_data) < 0)
    {
        tev_event_data_free(event_data);
        ret = -1;
    }

    xSemaphoreGive(tev->semaphore);
    xSemaphoreGive(tev->mutex);

    return ret;
}
