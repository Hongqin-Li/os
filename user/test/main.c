#include <stdio.h>
extern void test_fork();
extern void test_ipc();

void
umain(int argc, char **argv) 
{
    //test_fork();
    test_ipc();
}

