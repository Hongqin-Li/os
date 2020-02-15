// Physical memory allocator based on buddy system
#include <kern/console.h>
#include <kern/mm/buddy.h>
#include <kern/locks.h>
#include <memlayout.h>

void *PHYSTOP, *kend;
static struct buddy_system *bsp;
static struct spinlock memlock;

//#define DEBUG

#define MAXN  300
static void *pool[MAXN];

void
mm_init() 
{
    cprintf("PHYSTOP: 0x%x\n", PHYSTOP);
    cprintf("kend: 0x%x\n", kend);
    bsp = buddy_init(kend, P2V(PHYSTOP));
}

// Allocate sz size of physical memory.
// Returns 0 if failed else a pointer.
void *
kalloc(size_t sz)
{
    spinlock_acquire(&memlock);
    void *p = buddy_alloc(bsp, sz);

    #ifdef DEBUG
    cprintf("kalloc: p: 0x%x, sz: %d\n", p, sz);
    assert(p);
    int alloc = 0;
    for (int i = 0; i < MAXN; i ++) {
        if (!pool[i]) {
            pool[i] = p;
            alloc = 1;
            break;
        }
    }
    assert(alloc);
    #endif

    spinlock_release(&memlock);
    return p;
}

// Free the physical memory pointed at by v.
void
kfree(void *va)
{
    spinlock_acquire(&memlock);

    #ifdef DEBUG
    cprintf("kfree: va: 0x%x\n", va);
    int valid = 0;
    for (int i = 0; i < MAXN; i ++) {
        if (pool[i] == va) {
            valid = 1;
            pool[i] = 0;
            break;
        }
    }
    assert(valid);
    #endif

    buddy_free(bsp, va);
    spinlock_release(&memlock);
}


