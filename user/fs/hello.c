#include <stdio.h>
#include <unistd.h>

extern void test();

void
child() 
{
    cprintf("in child\n");
}

void
umain(int argc, char **argv)
{
    cprintf("hello");
    int pid = fork();
    if(pid) {
        child();
    }
    else cprintf("in parent: %x\n", pid);
}
