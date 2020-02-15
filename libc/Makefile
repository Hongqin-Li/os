# GNU Makefile doc: https://www.gnu.org/software/make/manual/html_node/index.html
CC := gcc -m32 -Werror -gstabs -fno-pie -fno-pic -fno-stack-protector -static -fno-builtin -nostdlib -Wl,--build-id=none

# The headers
INC_DIRS := ../inc

BUILD_DIR := ../obj/libc

LIBC_A := $(BUILD_DIR)/libc.a

all: $(LIBC_A)

# Automatically find sources and headers
SRCS := $(shell find ./ -name "*.c" -or -name "*.S")
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
-include $(DEPS)

CFLAGS := $(addprefix -I,$(INC_DIRS)) -MMD -MP

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD_DIR)/%.S.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIBC_A): $(OBJS)
	ar rcs $@ $^

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
