#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
test_malloc() {
    int n = 2;
    char *p[n];
    for (int i = 0; i < n; i ++) {
        p[i] = malloc(20);
        cprintf("test_malloc: malloc ret %x\n", p[i]);
    }
    for (int i = 0; i < n; i ++) {
        free(p[i]);
    }
}
