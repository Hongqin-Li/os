# GAS syntax: https://sourceware.org/binutils/docs/as/

#include <arch/i386/mmu.h>
#include <arch/i386/memlayout.h>

#define RELOC(x) ((x)-KERNBASE)

.text
# '_start' specifies the ELF entry point.  
.globl		_start
_start:
	mov     $(RELOC(bootstack+KSTKSIZE)), %esp
    push    %ebx
    push    %eax  
    call    multiboot_init

    # kend = end
    movl    $end, RELOC(kend)

    # We have hard-coded a page table with an 
    # indentical map: [0, 4MB) -> [0, 4MB)
    # (See entrypgdir.c)

    # Now we are going to map more as shown below, 
    # [KERNBASE, KERNBASE+PHYSTOP) -> [0, PHYSTOP)
    # [DEVSPACE, 0) -> [DEVSPACE, 0)
    # We should also provide kend(kernel end)
    # to our physical page allocator
    ###### Create Page Table Start ######

    #define REG_PA      %edi 
    #define REG_PA_END  %esi
    #define REG_PDX     %edx
    #define REG_END     %esp
    #define REG_TMP     %ebp
    #define REG_CNT     %ecx

    mov     $(RELOC(end)), REG_END
    mov     $0, REG_CNT

pgdir_init:
    cmp     $1, REG_CNT
    je      map2
    cmp     $2, REG_CNT
    je      update_kend
map1: 
    mov     $(KERNBASE>>PDXSHIFT), REG_PDX
    mov     $0, REG_PA
    mov     RELOC(PHYSTOP), REG_PA_END
    mov     $RELOC(DEVSPACE), REG_PA_END
    jmp     loop
map2:
    mov     $(DEVSPACE>>PDXSHIFT), REG_PDX
    mov     $DEVSPACE, REG_PA
    mov     $0xfffffffe, REG_PA_END
    jmp     loop

loop:
    cmp     REG_PA, REG_PA_END
    jle     set_zero
    test    $0x3ff000, REG_PA
    jne     set_entry

    mov     REG_END, REG_TMP
    or      $(PTE_P|PTE_W), REG_TMP
    movl    REG_TMP, RELOC(entry_pgdir)(, REG_PDX, 4)
    add     $1, REG_PDX

set_entry:
    # Non-writable for kernel code
    mov     REG_PA, REG_TMP
    cmp     $EXTMEM, REG_PA
    jl      writable
    cmp     $(RELOC(etext)), REG_PA
    jge     writable
    jmp     nonwritable
writable:
    orl     $(PTE_W), REG_TMP
nonwritable:
    orl     $(PTE_P), REG_TMP
    mov     REG_TMP, (REG_END)

    add     $4, REG_END
    add     $PGSIZE, REG_PA
    jmp     loop

set_zero:
    mov     $0, REG_PA
    test    $0xfff, REG_END
    je      set_zero_out
    movl    REG_PA, (REG_END)
    add     $4, REG_END
    jmp     set_zero
set_zero_out:
    add     $1, REG_CNT
    jmp     pgdir_init

update_kend:
    add     $KERNBASE, REG_END
    movl    REG_END, RELOC(kend)

    ###### Create Page Table End ######

    # Turn on paging
	movl    $(RELOC(entry_pgdir)), %ecx
	movl    %ecx, %cr3
	movl    %cr0, %ecx
	orl	    $(CR0_PE|CR0_PG|CR0_WP), %ecx
	movl    %ecx, %cr0

	# Now paging is enabled, but we can still running at a low EIP
	# since we have an identical map of low address
    mov     $relocated, %eax
    jmp     *%eax
relocated:

	# Set the stack pointer and call into C
	mov     $(bootstack+KSTKSIZE), %esp
    call    kernel_main

	# Should never get here, but in case we do, just spin.
spin:	
    hlt
    jmp	spin

# Usage: loadgdt(gdt, sizeof(gdt)-1)
.global loadgdt
loadgdt:
    mov     4(%esp), %eax
    mov     %eax, gdtdesc+2
    mov     8(%esp), %eax
    mov     %ax, gdtdesc
    lgdt    gdtdesc
    ljmp    $(SEG_KCODE<<3), $reload_cs
reload_cs:
    mov     $(SEG_KDATA<<3), %ax    # Our data segment selector
    mov     %ax, %ds                # -> DS: Data Segment
    mov     %ax, %es                # -> ES: Extra Segment
    mov     %ax, %ss                # -> SS: Stack Segment
    mov     $0, %ax                 # Zero segments not ready for use
    mov     %ax, %fs                # -> FS
    mov     %ax, %gs                # -> GS
    ret
test:
    push    %eax
    pop     %eax
    ret

.data
gdtdesc:
    .word   0   # Limit 
    .long   0   # Base

# size of KSTKSIZE, align 4096
.comm bootstack, KSTKSIZE, 4096
