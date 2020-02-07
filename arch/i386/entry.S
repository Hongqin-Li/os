# GAS syntax: https://sourceware.org/binutils/docs/as/

#include <arch/i386/mmu.h>
#include <arch/i386/memlayout.h>
#include <inc/multiboot.h>

#define	RELOC(x) ((x) - KERNBASE)

#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN|MULTIBOOT_MEMORY_INFO)
#define CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

.text
multiboot_header:
    .align MULTIBOOT_HEADER_ALIGN
    .long MULTIBOOT_HEADER_MAGIC
    .long MULTIBOOT_HEADER_FLAGS
    .long CHECKSUM

# '_start' specifies the ELF entry point.  
.globl		_start
_start:
    # Check magic and fetch memory info
    cmp     $MULTIBOOT_BOOTLOADER_MAGIC, %eax
    jne     spin

    # PHYSTOP = 1M + (mbt->mem_upper)K
    mov     8(%ebx), %ecx
    shl     $10, %ecx
    add     $0x100000, %ecx
    mov     $(RELOC(PHYSTOP)), %edx
    mov     %ecx, (%edx)

    # kend = end
    mov     $(RELOC(kend)), %ebp
    movl    $end, (%ebp)

    # %edi: RELOC(kend)
    # %ebx: pdx
    # %eax: pgt
    # %edx: va
    # %esi: pa | flags

    # %esp: temp

    mov     $KERNBASE, %edx
    mov     $0, %esi

    jmp     RELOC(map)
mapret:
    add     $PGSIZE, %edx
    add     $PGSIZE, %esi

map:
    # map va(edx) -> pa(esi)
    mov     %edx, %ebx
    shr     $PDXSHIFT, %ebx
    and     $0x3ff, %ebx
	mov     RELOC(entry_pgdir)(, %ebx, 4), %eax     # eax = entry_pgdir[PDX(va)]
    test    $PTE_P, %eax
    jne     RELOC(alloc_end)

alloc_begin:

    # memset(edi, 0, PGSIZE)
    # edi += PGSIZE
    mov     $PGSIZE, %ecx
    mov     $0, %al
    cld
    rep     stosb

    # entry_pgdir[pdx] = kalloc() | PTE_P
    movl    %edi, %eax
    # FIXME: flag
    or      $(PTE_P|PTE_W), %eax
    movl    %eax, RELOC(entry_pgdir)(, %ebx, 4)


alloc_end:
    and     $0xfffff000, %eax
    shr     $PTXSHIFT, %edx
    and     $0x3ff, %edx
    # FIXME
    or      $(PTE_P|PTE_W), %esi
	mov     %esi, (%eax, %edx, 4)     # eax = pgtable[PTX(va)]

    jmp     RELOC(mapret)


    # Set page directory
	movl    $(RELOC(entry_pgdir)), %ecx
	movl    %ecx, %cr3
	# Turn on paging.
	movl    %cr0, %ecx
	orl	    $(CR0_PE|CR0_PG|CR0_WP), %ecx
	movl    %ecx, %cr0

	# Now paging is enabled, but we're still running at a low EIP
	# (why is this okay?).  

	# Set the stack pointer and call into C
	movl	$(bootstack+KSTKSIZE), %esp
    mov     $kernel_main, %eax
    jmp     *%eax

	# Should never get here, but in case we do, just spin.
spin:	
    hlt
    jmp	spin

# size of KSTKSIZE, align 4096
.comm bootstack, KSTKSIZE, 4096

