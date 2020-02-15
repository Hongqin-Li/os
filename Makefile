# GNU Makefile doc: https://www.gnu.org/software/make/manual/html_node/index.html

# -gstab enable debug info
# -fno-pie -fno-pic remove .got and data.rel sections, `objdump -t obj/kernel.o | sort` to see the difference
# -MMD -MP generate .d files
# -Wl,--build-id=none to remove .note.gnu.build-id section, making the multiboot header in first 4KB
CC := gcc -m32
CC += -Werror -gstabs 
CC += -fno-pie -fno-pic -fno-stack-protector 
CC += -static -fno-builtin -nostdlib
CC += -Wl,--build-id=none

LD := ld -m elf_i386

# FIXME: build with different target architecture
ARCH_DIR := ./arch/i386
KERN_DIR := ./kern

SRC_DIRS := $(ARCH_DIR) $(KERN_DIR)
BUILD_DIR = obj

KERN_ELF := $(BUILD_DIR)/kernel.o
IMG := $(BUILD_DIR)/sos.iso

GRUB_CFG := menuentry "sos" { multiboot2 /boot/$(notdir $(KERN_ELF)) }

all: $(IMG)

##### User C Library
LIBC_A := $(BUILD_DIR)/libc/libc.a
$(LIBC_A): 
	make -C libc

##### Kernel ELF Start

# Automatically find sources and headers
SRCS := $(shell find $(SRC_DIRS) -name *.c -or -name *.S)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
-include $(DEPS)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# CC += -I. $(INC_FLAGS) -MMD -MP
CC += -I. -Iarch/i386 -Iinc -MMD -MP

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $<
$(BUILD_DIR)/%.S.o: %.S
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $<

##### User ELFs
USER_DIRS := $(shell find user/ -maxdepth 1 -mindepth 1 -type d)
USER_ELFS := $(USER_DIRS:%=$(BUILD_DIR)/%.elf)
GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

.SECONDEXPANSION:
# link gcc library to support int64_t division operation
$(BUILD_DIR)/user/%.elf: $(LIBC_A) $$(addsuffix .o,$$(addprefix $(BUILD_DIR)/,$$(shell find user/% -name "*.c")))
	$(CC) -I. -T user/user.ld -o $@ user/entry.S $^ $(GCC_LIB)
	objdump -S -D $@ > $(basename $@).asm

$(KERN_ELF): $(ARCH_DIR)/linker.ld $(OBJS) $(USER_ELFS)
	# $(CC) -o $@ -T $< $(OBJS) 
	$(LD) -o $@ -T $< $(OBJS) -b binary $(USER_ELFS)
	objdump -S -D $@ > $(basename $@).asm

$(IMG): $(KERN_ELF)
	mkdir -p sysroot/boot/grub
	cp $< sysroot/boot/$(notdir $<)
	echo $(GRUB_CFG) > sysroot/boot/grub/grub.cfg
	grub-mkrescue -o $@ sysroot


# Hardware
RAM := 4 # MB
NCPU := 4

qemu: $(IMG) 
	qemu-system-i386 -cdrom $< -serial mon:stdio -m $(RAM) -smp $(NCPU)
qemu-nox: $(IMG) 
	qemu-system-i386 -cdrom $< -serial mon:stdio -m $(RAM) -smp $(NCPU) -nographic
qemu-img: $(IMG)
	qemu-system-i386 -cdrom $< -serial mon:stdio -m $(RAM) -smp $(NCPU)
qemu-gdb: $(IMG)
	qemu-system-i386 -cdrom $< -serial mon:stdio -m $(RAM) -smp $(NCPU) -S -gdb tcp::1234
gdb: 
	gdb -n -x .gdbinit

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
