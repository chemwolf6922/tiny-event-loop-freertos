#include <stdio.h>
#include "tev.h"

void init_script(void);

static tev_handle_t tev = NULL;

int main(int argc, char const *argv[])
{
    tev = tev_create_ctx();

    /* user code */
    init_script();

    tev_main_loop(tev);
    tev_free_ctx(tev);
    return 0;
}

void periodic_print_hello(void* ctx)
{
    printf("hello\n");
    tev_set_timeout(tev,periodic_print_hello,NULL,1000);
}

void init_script(void)
{
    tev_set_timeout(tev,periodic_print_hello,NULL,1000);
}