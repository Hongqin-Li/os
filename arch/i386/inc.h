#ifndef ARCH_I386_INC_H
#define ARCH_I386_INC_H

#include <inc/types.h>
#include <inc/string.h>
#include <inc/list.h>
#include <inc/sys.h>

#include <arch/i386/x86.h>
#include <arch/i386/memlayout.h>
#include <arch/i386/mmu.h>

#include <kern/console.h>
#include <kern/locks.h>

#define PROC_MAGIC 0xabcdcccc
enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, WAITING, ZOMBIE };
struct proc {
    int magic;
    pde_t *pgdir;

    int state;
    struct list_head hlist;
    struct list_head wait_list;
    struct list_head pos;       // Scheduled by whom, either empty
                                // or in ready_list, or in zombie_list
    struct mailbox *mailbox;

    struct list_head page_list;
    struct context *context;    // Value of the kernel stack pointer
    struct trapframe *tf;
};

// Large Prime Number: https://planetmath.org/goodhashtableprimes
#define PROC_BUCKET_SIZE     769
#define PROC_HASH(x)         (((uint32_t)x) % PROC_BUCKET_SIZE)
#define PROC_EXISTS(p) (list_find(&ptable.hlist[PROC_HASH(p)], &(p)->hlist) && (p)->magic == PROC_MAGIC)

struct ptable {
    struct spinlock lock;
    struct list_head hlist[PROC_BUCKET_SIZE];    // Hash Map of all proc
    struct list_head ready_list;            // list of runnable proc 
    struct list_head zombie_list;           // list of zombie proc
};

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
void test_pgdir(pde_t *pgdir);
void seg_init();
void tss_init();
pte_t *pgdir_walk(pde_t *pgdir, const void *va, int32_t alloc);
pde_t *vm_fork(pde_t *pgdir);
void vm_alloc(pde_t *pgdir, uint32_t va, uint32_t len);
void vm_free(pde_t *pgdir);
void vm_switch(pde_t *pgdir);
int uvm_check(pde_t *pgdir, char *s, uint32_t len);


// mm.c
extern void *PHYSTOP; // maximum physical memory address(pa)
extern void *kend;    // kernel end address(va)
void mm_init();
void *kalloc(size_t);
void kfree(void *);

// proc.c
extern struct ptable ptable;
struct proc *thisproc();
struct proc *proc_alloc();
void proc_init();
void proc_stat();
void scheduler();
void sleep();
void wakeup(struct proc *);
void swtchp(struct proc *);
void exit();
int fork();
void yield(struct proc *);
struct proc *serve();

// user.c
extern struct proc *kbd_proc;
void user_init();
void user_intr();

// ipc.c
void ipc_init(struct proc *);

// syscall.c
int32_t syscall(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5);

#endif
