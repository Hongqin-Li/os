#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void test_ipc() {
    int nchild = 10;
    int cid[nchild];
    for (int i = 0; i < nchild; i ++) {
        int pid = fork();
        if (!pid) {
            int get = recvi();
            cprintf("child %d recv: %d\n", i, get);
            exit();
        }
        else {
            cid[i] = pid;
        }
    }
    for (int i = 0; i < nchild; i ++) {
        sendi(cid[i], i + 1);
        //cprintf("parent send i: %d\n", i);
    }
}
