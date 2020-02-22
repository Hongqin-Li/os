#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern void fs_init();

void
umain(int argc, char **argv)
{
    cprintf("fs: hello\n");
    fs_init();
}
