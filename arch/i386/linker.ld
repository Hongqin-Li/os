ENTRY (_start)

KERNLINK = 0xF0100000;
KERNBASE = 0xF0000000;

SECTIONS
{
	/* The kernel will live at 3GB + 1MB in the virtual address space, */
	/* which will be mapped to 1MB in the physical address space. */
	/* Note that we page-align the sections. */
	. = KERNLINK;
	/* Add a symbol that indicates the start address of the kernel. */
	_kernel_start = .;
	.text ALIGN (4K) : AT (ADDR (.text) - KERNBASE)
	{
        *(.multiboot_header)
		*(.text)
	}
    PROVIDE(etext = .);

	.rodata ALIGN (4K) : AT (ADDR (.rodata) - KERNBASE)
	{
		*(.rodata)
	}
    
    PROVIDE(data = .);
	.data ALIGN (4K) : AT (ADDR (.data) - KERNBASE)
	{
		*(.data)
	}
    PROVIDE(edata = .);

	.bss ALIGN (4K) : AT (ADDR (.bss) - KERNBASE)
	{
		*(.bss)
	}
    . = ALIGN(0x1000);
    PROVIDE(end = .);

	/* Conventionally, Unix linkers provide pseudo-symbols
	 * etext, edata, and end, at the end of the text, data, and bss.
	 * For the kernel mapping, we need the address at the beginning
	 * of the data section, but that's not one of the conventional
	 * symbols, because the convention started before there was a
	 * read-only rodata section between text and data. */
}
