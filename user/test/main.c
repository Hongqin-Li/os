#include <stdio.h>
extern void test_fork();
extern void test_ipc();
extern void test_malloc();

void
umain(int argc, char **argv) 
{
    //test_fork();
    test_ipc();
    test_malloc();
}

