#ifndef __TINY_EVENT_LOOP_H
#define __TINY_EVENT_LOOP_H

#include <stdint.h>

#define TINY_EVENT_LOOP_VERSION "1.2.0"
/**
 * Notice: 
 * As an event loop, this lib is not, will not be and should not be thread safe.
 * Use fifos & read handlers to inject events into the event loop.
*/

/* Flow control */

typedef void *tev_handle_t;

tev_handle_t tev_create_ctx(void);
void tev_main_loop(tev_handle_t tev);
void tev_free_ctx(tev_handle_t tev);

/* Timeout */

typedef void *tev_timeout_handle_t;
typedef void(*tev_timeout_handler_t)(void* ctx);

tev_timeout_handle_t tev_set_timeout(tev_handle_t tev, tev_timeout_handler_t handler, void *ctx, int64_t timeout_ms);
int tev_clear_timeout(tev_handle_t tev, tev_timeout_handle_t timeout);

/* Event handler */

typedef void *tev_event_handle_t;
typedef void(*tev_event_handler_t)(void* data, int len, void* ctx);

tev_event_handle_t tev_set_event_handler(tev_handle_t tev, tev_event_handler_t handler, void* ctx);
int tev_clear_event_handler(tev_handle_t tev, tev_event_handle_t event);
/* This should not be used in ISR */
int tev_send_event(tev_handle_t tev, tev_event_handle_t event, void* data, int len);


#endif