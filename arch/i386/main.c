#include <arch/i386/inc.h>

static void boot_aps();
void mp_main();

void kernel_main() {

    cons_init();

    mm_init();
    mp_init();
    trap_init();

    pic_init();
    lapic_init();
    ioapic_init();

    boot_aps();
    while (1);
}

// While boot_aps is booting a given CPU, it communicates the per-core
// stack pointer that should be loaded by mpentry.S to that CPU in
// this variable.
void *mpentry_kstack;

static void
boot_aps()
{
    extern uint8_t mpentry_start[], mpentry_end[];

    // Write entry code to unused memory at MPENTRY_PADDR
    memmove(P2V(MPENTRY_PADDR), mpentry_start, mpentry_end - mpentry_start);

    // Boot each AP one at a time
    for (int i = 1; i < ncpu; i ++) {
        // Tell mpentry.S what stack to use 
        mpentry_kstack = percpu_kstacks[i] + KSTKSIZE;
        // Start the CPU at mpentry_start
        lapic_startap(cpus[i].apicid, MPENTRY_PADDR);
        // Wait for the CPU to finish some basic setup in mp_main()
        while(cpus[i].status != CPU_STARTED)
            ;
    }
    // Clear the identical map: [0, 4MB) -> [0, 4MB)
    // Since all CPU have been in higher half
    entry_pgdir[0] = 0;
}

// Setup code for APs
void
mp_main()
{
    seg_init();
    lapic_init();
    idt_init();

    cprintf("CPU(idx=%d, apicid=%d) initialization finished.\n", cpuidx(), thiscpu()->apicid);
	xchg(&thiscpu()->status, CPU_STARTED); // tell boot_aps() we're up
    while(1) ;
}
