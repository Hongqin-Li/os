#ifndef KERN_INC_H
#define KERN_INC_H

#include <inc/elf.h>
#include <inc/types.h>
#include <inc/bitmap.h>
#include <inc/list.h>

#define PGSIZE 4096

// In kern/spinlock.c
struct spinlock {
    volatile int locked;
};
void acquire(struct spinlock *);
void release(struct spinlock *);

// In kern/mm.c
void free_range(void *start, void *end);
void *kalloc(size_t sz);
void kfree(void *v);

// Large Prime Number: https://planetmath.org/goodhashtableprimes
#define PROC_BUCKET_SIZE     769
#define PROC_HASH(x)         (((uint32_t)x) % PROC_BUCKET_SIZE)
#define PROC_EXISTS(p) (list_find(&ptable.hlist[PROC_HASH(p)], &(p)->hlist) && (p)->magic == PROC_MAGIC)
#define PROC_MAGIC 0xabcdcccc

struct proc {
    int magic;
    int size;

    struct list_head hlist;
    struct list_head wait_list;
    struct list_head pos;       // Scheduled by whom, either empty
                                // or in ready_list, or in zombie_list
    struct mailbox *mailbox;

    // Architexture dependent part
    struct vm       *vm;        // Virtual memory or address space
    struct context  *context;   // Context
};

struct ptable {
    struct spinlock lock;
    struct list_head hlist[PROC_BUCKET_SIZE];   // Hash Map of all proc
    struct list_head ready_list;                // list of runnable proc 
    struct list_head zombie_list;               // list of zombie proc
};

// In kern/proc.c
extern struct ptable ptable;
void         proc_init();
void         proc_stat();
void         sched();
void         exit();                         // Exit current process
void         sleep();
void         wakeup(struct proc *);
void         yield(struct proc *);
struct proc *serve();
struct proc *spawn(struct elfhdr *);    // Create a new process specified by elf
void *       sbrk(int);

// In kern/ipc.c
int send(int, int);
int recv(int, int);

// In kern/console.c
#define BACKSPACE 0x100
#define assert(x)  { if (!(x)) panic("%s:%d: assertion failed.\n", __FILE__, __LINE__);  }
extern int panicked;
void cprintf(char *fmt, ...);
void panic(char *fmt, ...);

// In arch/xxx/console.c
void cons_init();
void consputc(int c);

// In arch/xxx/proc.c
struct proc *thisproc();                // Get current process
struct proc *thisched();                // Get current scheduler
struct proc *proc_alloc(uint32_t, int);
struct proc *spawnx(struct elfhdr *, int);
void         swtch(struct proc *p);     // Switch to process p, including context and vm
void         reap(struct proc *p);      // Reap a process
void         scheduler();

// In arch/XXX/vm.c
struct vm *vm_init();
void       vm_switch(struct vm *);
void       vm_alloc(struct vm *, uint32_t, uint32_t);
int        vm_dealloc(struct vm *, uint32_t, uint32_t);
void       vm_free(struct vm *);

#endif
