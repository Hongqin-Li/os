#ifndef ARCH_I386_INC_H
#define ARCH_I386_INC_H

#include <inc/types.h>
#include <inc/string.h>
#include <inc/sys.h>

#include <arch/i386/x86.h>
#include <arch/i386/memlayout.h>
#include <arch/i386/mmu.h>

#include <kern/inc.h>

// Maximum number of CPUs
#define NCPU  8

// Values of status in struct cpu
enum { CPU_UNUSED = 0, CPU_STARTED, CPU_HALTED,};

// Per-CPU state
struct cpu {
	uint8_t apicid;                 // Local APIC ID
	volatile unsigned status;       // The status of the CPU
	struct proc scheduler;         // swtchp() here to enter scheduler
	struct taskstate ts;            // Used by x86 to find stack for interrupt
	struct segdesc gdt[NSEGS];      // x86 global descriptor table
	struct proc *proc;              // The process running on this cpu or null
	//int32_t ncli;                   // Depth of pushcli nesting
	//int32_t intena;                 // Were interrupts enabled before pushcli?
};

// cpu.c
extern struct cpu cpus[NCPU];
extern int ncpu;                    // Total number of CPUs in the system
extern struct cpu *bootcpu;         // The boot-strap processor (BSP)
extern unsigned char percpu_kstacks[NCPU][KSTKSIZE];
int cpuidx();
struct cpu *thiscpu();

// lapic.c
extern volatile uint32_t *lapic;    // Physical MMIO address of the local APIC
void    lapic_init();
void    lapic_startap(uint8_t apicid, uint32_t addr);
void    lapic_eoi();
int     lapicid();

// ioapic.c
void    ioapic_init();
void    ioapic_enable(int, int);

// picirq.c
void    pic_init();

// acpi.c
void    acpi_init();

// trap.c
void    trap_init();
void    idt_init();

// vm.c
extern pde_t entry_pgdir[NPDENTRIES];
void seg_init();
void tss_init();
void test_pgdir(pde_t *pgdir);
pte_t *pgdir_walk(pde_t *pgdir, const void *va, int32_t alloc);
pde_t *vm_fork(pde_t *pgdir);

int uvm_check(struct vm *vm, char *s, uint32_t len);

// mm.c
extern void *PHYSTOP; // maximum physical memory address(pa)
extern void *kend;    // kernel end address(va)
void mm_init();

// proc.c
void sched_init();
int fork();

// user.c
extern struct proc *utable[NUSERS];
void user_init();
void user_intr();

// ipc.c
void ipc_init(struct proc *);

// syscall.c
int32_t syscall(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5);

#endif
