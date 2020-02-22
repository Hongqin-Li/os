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

GRUB_CFG := menuentry "sos" { multiboot /boot/$(notdir $(KERN_ELF)) }

all: $(IMG)

##### User C Library
LIBC_A := $(BUILD_DIR)/libc/libc.a
$(LIBC_A): FORCE
	make -C libc
FORCE:
# https://www.gnu.org/software/make/manual/html_node/Force-Targets.html

# Automatically find sources and headers
SRCS := $(shell find $(SRC_DIRS) -name *.c -or -name *.S)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
-include $(DEPS)

CC += -I. -Iarch/i386 -Iinc -Iuser/ -MMD -MP

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $<
$(BUILD_DIR)/%.S.o: %.S
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $<

##### User ELFs
USER_DIRS := $(shell find user/ -maxdepth 1 -mindepth 1 -type d)
USER_ELFS := $(USER_DIRS:%=$(BUILD_DIR)/%.elf)

# library should be at last!
.SECONDEXPANSION:
$(BUILD_DIR)/user/%.elf: user/user.ld user/entry.S $$(addsuffix .o,$$(addprefix $(BUILD_DIR)/,$$(shell find user/% -name "*.c"))) $(LIBC_A)
	$(CC) -I. -T $< -o $@ $(filter-out $<,$^) 
	objdump -S -D $@ > $(basename $@).asm

$(KERN_ELF): $(ARCH_DIR)/linker.ld $(OBJS) $(USER_ELFS)
	$(LD) -o $@ -T $< $(OBJS) -b binary $(USER_ELFS)
	objdump -S -D $@ > $(basename $@).asm

$(IMG): $(KERN_ELF)
	mkdir -p sysroot/boot/grub
	cp $< sysroot/boot/$(notdir $<)
	echo $(GRUB_CFG) > sysroot/boot/grub/grub.cfg
	grub-mkrescue -o $@ sysroot

RAM := 4 # MB
NCPU := 4
qemu: $(KERN_ELF) 
	qemu-system-i386 -kernel $< -serial mon:stdio -m $(RAM) -smp $(NCPU)
qemu-nox: $(KERN_ELF) 
	qemu-system-i386 -kernel $< -serial mon:stdio -m $(RAM) -smp $(NCPU) -nographic
qemu-img: $(IMG)
	qemu-system-i386 -cdrom $< -serial mon:stdio -m $(RAM) -smp $(NCPU)
qemu-gdb: $(IMG)
	qemu-system-i386 -kernel $< -serial mon:stdio -m $(RAM) -smp $(NCPU) -S -gdb tcp::1234
gdb: 
	gdb -n -x .gdbinit

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
