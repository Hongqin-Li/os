
#include <arch/i386/inc.h>
#include <kern/console.h>

struct stackframe {
  struct stackframe* ebp;
  uint32_t eip;
};

void
trace(uint32_t max_frames)
{
    struct stackframe *stk;
    asm volatile("movl %%ebp,%0" : "=r"(stk));
    cprintf("Stack trace:\n");
    for(uint32_t i = 0; stk && i < max_frames; i ++) {
        cprintf("0x%x, ", stk->eip);
        stk = stk->ebp;
    }
    cprintf("end\n");
}
