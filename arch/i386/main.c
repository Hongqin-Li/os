#include <inc/types.h>
#include <inc/string.h>
#include <inc/multiboot.h>

#include <arch/i386/defs.h>
#include <arch/i386/x86.h>
#include <arch/i386/memlayout.h>

#include <kern/console.h>
#include <kern/kalloc.h>
#include <kern/mm/buddy.h>

uint32_t PHYSTOP;
uint32_t kend;

void kernel_main() {
    extern char end[];
    cons_init();
    cprintf("kernel_main: %x\n", kernel_main);
    cprintf("PHYSTOP: 0x%x\n", PHYSTOP);
    cprintf("kernel end: 0x%x\n", kend);

    //struct buddy_system *bsp = buddy_init(end, P2V(4*1024*1024));
    //free_range(end, P2V(4*1024*1024));

    trap_init();
    while(1) ;
}

// While boot_aps is booting a given CPU, it communicates the per-core
// stack pointer that should be loaded by mpentry.S to that CPU in
// this variable.
void *mpentry_kstack;

static void
boot_aps(void)
{
	extern unsigned char mpentry_start[], mpentry_end[];
	void *code;
	struct cpu *c;

	// Write entry code to unused memory at MPENTRY_PADDR
	code = P2V(MPENTRY_PADDR);
	memmove(code, mpentry_start, mpentry_end - mpentry_start);

	// Boot each AP one at a time
	for (c = cpus; c < cpus + ncpu; c++) {
		if (c == cpus + cpuidx())  // We've started already.
			continue;

		// Tell mpentry.S what stack to use 
		mpentry_kstack = percpu_kstacks[c - cpus] + KSTKSIZE;
		// Start the CPU at mpentry_start
		lapic_startap(c->apicid, V2P(code));
		// Wait for the CPU to finish some basic setup in mp_main()
		while(c->status != CPU_STARTED)
			;
	}
}

// Setup code for APs
void
mp_main(void)
{
	// TODO: Your code here.
	// You need to initialize something.

	xchg(&thiscpu()->status, CPU_STARTED); // tell boot_aps() we're up
	for (;;);
}
