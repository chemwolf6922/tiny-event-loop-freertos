#include "list.h"
#include <stdlib.h>
#include <string.h>

struct list_node_s
{
    struct list_node_s* prev;
    struct list_node_s* next;
    void* data;
};

typedef struct list_node_s list_node_t;

typedef struct
{
    int length;
    list_node_t* head;
    list_node_t* tail;
} list_t;

static list_node_t* list_node_new();
static void list_node_free(list_node_t* node,void(*free_with_ctx)(void* data, void* ctx), void* free_ctx);

list_handle_t list_new()
{
    list_t* list = NULL;
    
    list = malloc(sizeof(list_t));
    if(!list)
        goto error;
    memset(list,0,sizeof(list_t));

    list->length = 0;
    list->head = NULL;
    list->tail = NULL;

    return (list_handle_t)list;
error:
    if(list)
        free(list);
    return NULL;
}

void list_free(list_handle_t handle,void(*free_with_ctx)(void* data, void* ctx),void* free_ctx)
{
    list_t* list = (list_t*)handle;
    if(list)
    {   
        list_node_t* node = list->head;
        while(node != NULL)
        {
            list_node_t* c_node = node;
            node = c_node->next;
            if(free_with_ctx)
                free_with_ctx(c_node->data,free_ctx);
            free(c_node);
        }
        free(list);
    }
}

int list_get_length(list_handle_t handle)
{
    list_t* list = (list_t*)handle;
    if(!list)
        goto error;

    return list->length;
error:
    return -1;
}

// tail operations
int list_push(list_handle_t handle, void* data)
{
    list_node_t* node = NULL;

    list_t* list = (list_t*)handle;
    if(!list)
        goto error;
    
    // create node
    node = list_node_new();
    if(!node)
        goto error;

    // assign data
    node->data = data;
    
    // connect node
    if(list->head == NULL)
        list->head = node;
    if(list->tail != NULL)
    {
        list->tail->next = node;
        node->prev = list->tail;
    }
    list->tail = node;
    list->length ++;

    return 0;
error:
    if(node)
        list_node_free(node,NULL,NULL);
    return -1;
}

void* list_pop(list_handle_t handle)
{
    list_t* list = (list_t*)handle;
    if(!list)
        goto error;

    if(!list->tail)
        goto error;

    list_node_t* node = list->tail;
    void* data = node->data;
    if(node->prev == NULL)
    {
        list->head = NULL;
        list->tail = NULL;
    }
    else
    {
        list->tail = node->prev;
        list->tail->next = NULL;
    }

    list_node_free(node,NULL,NULL);
    list->length--;

    return data;
error:
    return NULL;
}

void* list_peek_tail(list_handle_t handle)
{
    list_t* list = (list_t*)handle;
    if(!list)
        goto error;

    if(!list->tail)
        goto error;

    return list->tail->data;
error:
    return NULL;
}

// head operations
int list_unshift(list_handle_t handle, void* data)
{
    list_node_t* node = NULL;

    list_t* list = (list_t*)handle;
    if(!list)
        goto error;
    
    // create node
    node = list_node_new();
    if(!node)
        goto error;

    // assign data
    node->data = data;
    
    // connect node
    if(list->tail == NULL)
        list->tail = node;
    if(list->head != NULL)
    {
        list->head->prev = node;
        node->next = list->head;
    }
    list->head = node;
    list->length ++;

    return 0;
error:
    if(node)
        list_node_free(node,NULL,NULL);
    return -1;
}

void* list_shift(list_handle_t handle)
{
    list_t* list = (list_t*)handle;
    if(!list)
        goto error;

    if(!list->head)
        goto error;

    list_node_t* node = list->head;
    void* data = node->data;
    if(node->next == NULL)
    {
        list->head = NULL;
        list->tail = NULL;
    }
    else
    {
        list->head = node->next;
        list->head->prev = NULL;
    }

    list_node_free(node,NULL,NULL);
    list->length--;

    return data;
error:
    return NULL;
}

void* list_peek_head(list_handle_t handle)
{
        list_t* list = (list_t*)handle;
    if(!list)
        goto error;

    if(!list->head)
        goto error;

    return list->head->data;
error:
    return NULL;
}

static list_node_t* list_node_new()
{
    list_node_t* node = NULL;

    node = malloc(sizeof(list_node_t));
    if(!node)
        goto error;
    memset(node,0,sizeof(list_node_t));

    node->data = NULL;
    node->prev = NULL;
    node->next = NULL;

    return node;
error:
    return NULL;
}

static void list_node_free(list_node_t* node,void(*free_with_ctx)(void* data, void* ctx), void* free_ctx)
{
    if(node)
    {
        if(free_with_ctx)
            free_with_ctx(node->data,free_ctx);
        free(node);
    }
}


// // test
// #include <stdio.h>

// #define list_forEach(l,n) for( \
//     (n) = (l)->head; \
//     (n) != NULL; \
//     (n) = (n)->next)

// #define list_display(l) { \
//     list_node_t* n17v06qc = NULL; \
//     list_forEach(l,n17v06qc) \
//     { \
//         if(n17v06qc->data) \
//             printf("%i ",*(int*)n17v06qc->data); \
//     } \
//     puts(""); \
// }

// int main(int argc, char const *argv[])
// {
//     int data[] = {0,1,2,3,4,5};
//     list_handle_t list = list_new();
//     if(!list)
//         goto error;

//     for(int i=0;i<sizeof(data)/sizeof(data[0]);i++)
//     {
//         list_push(list,&data[i]);
//     }

//     list_display((list_t*)list);

//     list_unshift(list,list_pop(list));

//     list_display((list_t*)list);

//     list_push(list,list_shift(list));

//     list_display((list_t*)list);

//     printf("%d\n",*(int*)list_peek_head(list));

//     printf("%d\n",*(int*)list_peek_tail(list));

//     printf("%d\n",list_get_length(list));

//     for(int i=0;i<10;i++)
//     {
//         list_pop(list);
//         list_shift(list);
//     }

//     list_display((list_t*)list);

//     printf("%d\n",list_get_length(list));

//     list_free(list,NULL,NULL);
//     return 0;
// error:
//     return -1;
// }

