#include <inc/types.h>
#include <inc/syscall.h>

#include <arch/i386/mmu.h>
#include <arch/i386/memlayout.h>
#include <arch/i386/x86.h>
#include <arch/i386/traps.h>

//#include <arch/i386/mp.h>
//#include <arch/i386/apic.h>

//#include <kern/syscall.h>

#include <kern/console.h>
//#include <kern/ui/kbd.h>

extern uint32_t vectors[]; // in vectors.S: array of 256 entry pointers

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];

// Initialize the interrupt descriptor table.
void
trap_init(void)
{
    int cs = SEG_SELECTOR(SEG_KCODE, 0, 0); //kernel code segment selector

    for (int i = 0; i < ARRAY_SIZE(idt); i ++) 
        SETGATE(idt[i], 0, cs, vectors[i], 0);

    SETGATE(idt[T_SYSCALL], 1, cs, vectors[T_SYSCALL], DPL_USER);
}

void
idt_init(void)
{
	lidt(idt, sizeof(idt));
}

void
trap(struct trapframe *tf)
{
    switch(tf->trapno) {
        case T_SYSCALL:
            //tf->eax = syscall(tf->eax, tf->edx, tf->ecx, tf->ebx, tf->edi, tf->esi);
            break;

        case T_IRQ0 + IRQ_TIMER:
            //lapic_eoi();
            break;

        case T_IRQ0 + IRQ_KBD:
            //kbd_intr();
            //lapic_eoi();
            break;

        case T_IRQ0 + IRQ_IDE:
            //ide_intr();
            //lapic_eoi();
            break;

        default: 
            cprintf("tf number: %d\n", tf->trapno);
            panic("tf not implemented.\n");
    }
}

