// Physical memory allocator based on buddy system
#include <kern/console.h>
#include <kern/locks.h>
#include <kern/mm.h>
#include <memlayout.h>

void *PHYSTOP, *kend;
static struct buddy_system *bsp;
static struct freelist freelist;
static struct spinlock memlock;

#define DEBUG

#define MAXN  300
static void *pool[MAXN];

void
mm_init() 
{
    cprintf("PHYSTOP: 0x%x\n", PHYSTOP);
    cprintf("kend: 0x%x\n", kend);
    //bsp = buddy_init(kend, P2V(PHYSTOP));
    free_range(&freelist, kend, P2V(PHYSTOP));
}

// Allocate sz size of physical memory.
// Returns 0 if failed else a pointer.
void *
kalloc(size_t sz)
{
    spinlock_acquire(&memlock);
    //void *p = buddy_alloc(bsp, sz);
    void *p = freelist_alloc(&freelist);
    assert(p)

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
    cprintf("kfree: va: 0x%x, ", va);
    int nalloc = 0;
    int valid = 0;
    for (int i = 0; i < MAXN; i ++) {
        if (!valid && pool[i] == va) {
            valid = 1;
            pool[i] = 0;
        }
        else if (pool[i]) nalloc ++;
    }
    assert(valid);
    cprintf("nalloc: %d\n", nalloc);
    #endif

    freelist_free(&freelist, va);
    //buddy_free(bsp, va);
    spinlock_release(&memlock);
}


