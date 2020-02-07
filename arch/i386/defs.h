#ifndef ARCH_I386_DEFS_H
#define ARCH_I386_DEFS_H

#include <inc/types.h>
#include <arch/i386/memlayout.h>
#include <arch/i386/mmu.h>

// Maximum number of CPUs
#define NCPU  8

// Values of status in struct Cpu
enum {
	CPU_UNUSED = 0,
	CPU_STARTED,
	CPU_HALTED,
};

// Per-CPU state
struct cpu {
	uint8_t apicid;                 // Local APIC ID
	volatile unsigned status;       // The status of the CPU
	struct context *scheduler;		// swtch() here to enter scheduler
	struct taskstate ts;            // Used by x86 to find stack for interrupt
	struct segdesc gdt[NSEGS];		// x86 global descriptor table
	struct proc *proc;				// The process running on this cpu or null
	int32_t ncli;					// Depth of pushcli nesting
	int32_t intena;					// Were interrupts enabled before pushcli?
};

// mp.c
extern struct cpu cpus[NCPU];
extern int ncpu;                    // Total number of CPUs in the system
extern struct cpu *bootcpu;         // The boot-strap processor (BSP)
extern volatile uint32_t *lapic;    // Physical MMIO address of the local APIC

// Per-CPU kernel stacks
extern unsigned char percpu_kstacks[NCPU][KSTKSIZE];


void mp_init(void);
int cpuidx();
struct cpu *thiscpu();

// lapic.c
void lapic_init();
void lapic_startap(uint8_t apicid, uint32_t addr);
void lapic_eoi();
int lapicid();

// ioapic.c

// trap.c
void trap_init();
void idt_init();

#endif
