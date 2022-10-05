#ifndef __LIST_H
#define __LIST_H

typedef void* list_handle_t;

list_handle_t list_new();
void list_free(list_handle_t list,void(*free_with_ctx)(void* data, void* ctx),void* free_ctx);

int list_get_length(list_handle_t list);

// tail operations
int list_push(list_handle_t list, void* data);
void* list_pop(list_handle_t list);
void* list_peek_tail(list_handle_t list);

// head operations
int list_unshift(list_handle_t list, void* data);
void* list_shift(list_handle_t list);
void* list_peek_head(list_handle_t list);


#endif
