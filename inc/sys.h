#ifndef INC_SYS_H
#define INC_SYS_H

#define USTKTOP 0xD0000000

#ifndef __ASSEMBLER__
#include <bitmap.h>
#include <string.h>


struct mailbox {
    BITMAP_STATIC(irq, 32);
    int sender;
    char content[512];
} __attribute__((packed));

int sys_send(int pid, int irq);
int sys_recv(int irq);

// User util functions
static int 
sends(int pid, char *s, int len) {
    struct mailbox *mb = (void *)USTKTOP;
    memmove(mb->content, s, len);
    return sys_send(pid, 0);
}
static void
recvs(char *buf, int len) {
    sys_recv(0);
    struct mailbox *mb = (void *)USTKTOP;
    memmove(buf, mb->content, len);
}
#endif

#endif
