#include <arch/i386/inc.h>

struct cpu cpus[NCPU];
int ncpu;
// Per-CPU kernel stacks
unsigned char percpu_kstacks[NCPU][KSTKSIZE] __attribute__ ((aligned(PGSIZE)));

int 
cpuidx() 
{
    return thiscpu() - cpus;
}

struct cpu *
thiscpu() 
{
    int apicid = lapicid();
    for (struct cpu *c = cpus; c < cpus + ncpu; c ++)
        if (c->apicid == apicid)
            return c;
    panic("unknown apicid");
}
