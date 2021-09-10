CC?=gcc
AR?=ar
BUILD_DIR?=$(shell pwd)/build/

CFLAGS=-Og
LDFLAGS=
LIBS=$(BUILD_DIR)libarray.a

LIBSRCS=tev.c
APPSRCS=test.c $(LIBSRCS) 

all:$(BUILD_DIR)app lib

lib:$(BUILD_DIR)libtev.a

$(BUILD_DIR)%.o:%.c $(BUILD_DIR)%.d | $(BUILD_DIR)
	$(CC) -MT $@ -MMD -MP -MF $(BUILD_DIR)$*.d $(CFLAGS) -o $@ -c $<

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)app:$(patsubst %.c,$(BUILD_DIR)%.o,$(APPSRCS)) $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)libtev.a:$(patsubst %.c,$(BUILD_DIR)%.o,$(LIBSRCS)) $(LIBS)
	$(AR) -rcs -o $@ $^

$(BUILD_DIR)libarray.a:cArray
	$(MAKE) -C $< BUILD_DIR=$(BUILD_DIR)

$(patsubst %.c,$(BUILD_DIR)%.d,$(APPSRCS)):
include $(patsubst %.c,$(BUILD_DIR)%.d,$(APPSRCS))

clean:
	rm -rf $(BUILD_DIR)