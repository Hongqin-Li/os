#include <arch/i386/inc.h>
#include <arch/i386/traps.h>

void mp_main();
static void boot_aps();
static int finish;

void kernel_main() {

    cons_init();
    test_pgdir(entry_pgdir);

    mm_init();
    acpi_init();
    trap_init();

    seg_init(); // GDT
    idt_init(); // IDT

    pic_init();
    lapic_init();
    ioapic_init();

    ioapic_enable(IRQ_KBD, 0);

    proc_init();

    boot_aps();
    sched_init();
    user_init();
    scheduler();
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
    for (int i = 0; i < ncpu; i ++) {
        if (i == cpuidx()) 
            continue;
        mpentry_kstack = percpu_kstacks[i] + KSTKSIZE;
        lapic_startap(cpus[i].apicid, MPENTRY_PADDR);
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
    idt_init();
    lapic_init();
    sched_init();

	xchg(&thiscpu()->status, CPU_STARTED); // tell boot_aps() we're up
    cprintf("CPU(idx=%d, apicid=%d) initialization finished.\n", cpuidx(), thiscpu()->apicid);
    scheduler();
}
