// Physical memory allocator based on buddy system
#include <kern/console.h>
#include <kern/mm/buddy.h>
#include <arch/i386/memlayout.h>

void *PHYSTOP, *kend;
static struct buddy_system *bsp;

void
mm_init() 
{
    cprintf("PHYSTOP: 0x%x\n", PHYSTOP);
    cprintf("kend: 0x%x\n", kend);
    bsp = buddy_init(kend, P2V(PHYSTOP));
}

// Free the physical memory pointed at by v.
void
kfree(void *va)
{
    buddy_free(bsp, va);
}

// Allocate sz size of physical memory.
// Returns 0 if failed else a pointer.
void *
kalloc(size_t sz)
{
    buddy_alloc(bsp, sz);
}
