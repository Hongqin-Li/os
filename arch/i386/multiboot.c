
#include <inc/types.h>
#include <arch/i386/multiboot.h>

struct {
    uint32_t magic;
    uint32_t arch;
    uint32_t len;
    uint32_t checksum;
} mbhdr __attribute__((section(".multiboot"))) __attribute__((aligned(8)));


void mb_init(uint32_t magic, uint32_t addr)
{
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        while (1);
    }
}
