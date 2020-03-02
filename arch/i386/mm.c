#include <kern/inc.h>
#include <arch/i386/memlayout.h>

// Phsical memory top(pa) and kernel end(va)
void *PHYSTOP, *kend;

void
mm_init() 
{
    cprintf("PHYSTOP: 0x%x\n", PHYSTOP);
    cprintf("kend: 0x%x\n", kend);
    free_range(kend, P2V(PHYSTOP));
}
