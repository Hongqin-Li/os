#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
child(int i) 
{
    cprintf("in child idx: %d\n", i);
    exit();
}

void
test_fork()
{
    cprintf("##### test fork begin\n");
    for (int i = 0; i < 10; i ++) {
        int pid = fork();
        if(!pid) {
            child(i);
            break;
        }
        else cprintf("in parent: child pid %x\n", pid);
    }
    cprintf("##### test fork end\n");
    //exit();
}
