CFLAGS?=-O3
override CFLAGS+=-MMD -MP
override CFLAGS+=-L$(FREERTOS_INCLUDE_DIR)
LDFLAGS?=
STATIC_LIB=libtev.a
LIB_SRC=tev.c list.c

LIB_MAP=map/libmap.a
LIB_HEAP=cHeap/libheap.a
LIBS=$(LIB_HEAP) $(LIB_MAP)

all:lib

lib:$(STATIC_LIB)

$(STATIC_LIB):$(patsubst %.c,%.o,$(LIB_SRC)) $(LIBS)
	for lib in $(LIBS); do \
		$(AR) -x $$lib; \
	done
	$(AR) -rcs -o $@ *.o

$(LIB_HEAP):
	$(MAKE) -C cHeap lib

$(LIB_MAP):
	$(MAKE) -C map lib USE_SIMPLE_HASH=y

%.oo:%.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o:%.c
	$(CC) $(CFLAGS) -c $<

-include $(TEST_SRC:.c=.d)

clean:
	$(MAKE) -C cHeap clean
	$(MAKE) -C map clean
	rm -f *.oo *.o *.d test $(STATIC_LIB) 
