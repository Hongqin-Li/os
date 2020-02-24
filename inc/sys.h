#ifndef INC_SYS_H
#define INC_SYS_H

#define USTKTOP 0xD0000000

#ifndef __ASSEMBLER__
#include <bitmap.h>
#include <string.h>

enum {
    USER_KBD = 0, 
    USER_VGA, 
    USER_VFS,
    USER_UNIX,
    NUSERS
};

struct mailbox {
    BITMAP_STATIC(irq, 32);
    int len;
    char content[512];
} __attribute__((packed));

#define USER_PID(i) ((int *)((struct mailbox *)USTKTOP)->content)[i]

int sys_send(int pid, int cnt);
int sys_recv(int pid, int cnt);

// User util functions
static int 
sends(int pid, char *s, int len) {
    struct mailbox *mb = (void *)USTKTOP;
    memmove(mb->content, s, len);
    return sys_send(pid, len);
}
static void
recvs(char *buf, int len) {
    int sender = sys_recv(0, len);
    struct mailbox *mb = (void *)USTKTOP;
    memmove(buf, mb->content, mb->len);
}
#endif

#endif
