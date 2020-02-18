#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys.h>

void test_ipc() {
    int nchild = 2;
    int cid[nchild];
    for (int i = 0; i < nchild; i ++) {
        int pid = fork();
        if (!pid) {
            char buf[10];
            recvs(buf, 10);
            cprintf("child %d recv: %s\n", i, buf);
            exit();
        }
        else {
            cid[i] = pid;
        }
    }
    for (int i = 0; i < nchild; i ++) {
        cprintf("cid[%d]: %x\n", i, cid[i]);
        sends(cid[i], "hello", 6);
    }
}
