// Multiboot2 Specification: https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
#include <inc/types.h>
#include <arch/i386/inc.h>
#include <multiboot.h>

struct {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
} multiboot_header 
__attribute__((section(".multiboot"))) __attribute__((aligned(4))) = {
    MULTIBOOT_HEADER_MAGIC,
    MULTIBOOT_INFO_MEMORY,
    -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_INFO_MEMORY), 
};

struct {
    uint32_t magic;
    uint32_t arch;
    uint32_t len;
    uint32_t checksum;
    uint32_t end[2];
} multiboot2_header 
__attribute__((section(".multiboot2"))) __attribute__((aligned(8))) = {

    MULTIBOOT2_HEADER_MAGIC,
    MULTIBOOT_ARCHITECTURE_I386,
    sizeof(multiboot_header), 
    -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + sizeof(multiboot_header)), 
    {0, 8} // End Tag
};

#define RELOC(sym) *(typeof(&(sym)))(((void *)&(sym)) - KERNBASE)

// Called by entry.S with flat page mapping
// Use RELOC(sym) to refer to global symbols
void multiboot_init(uint32_t magic, uint32_t addr)
{
    // Multiboot 1
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        RELOC(PHYSTOP) = (void *)(1024*1024 + 1024*((struct multiboot_info *)addr)->mem_upper);
    }
    // Multiboot 2
    else if (magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        uint32_t total_size = *(uint32_t *)addr;
        struct multiboot_tag *tag;
        for (tag = (struct multiboot_tag *) (addr + 8);
            tag->type != MULTIBOOT_TAG_TYPE_END;
            tag = (struct multiboot_tag *) ((uint8_t *) tag + ((tag->size + 7) & ~7))) {

            switch(tag->type) {
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: 
                RELOC(PHYSTOP) = (void *)(1024*1024 + 1024*((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);
                break;

            case MULTIBOOT_TAG_TYPE_ACPI_OLD:
            case MULTIBOOT_TAG_TYPE_ACPI_NEW:
                //uint32_t top = ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper;
                break;
            }
        }
    }
    else {
        while(1);
    }
}
