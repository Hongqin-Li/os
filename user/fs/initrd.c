#include <fs/inode.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NRDINODES    20

#define GET_RIP(ip) \
({ \
    assert(ip->dev == DEV_RAMDISK && ip->inum < NRDINODES); \
    struct initrd_inode *rip = &rdinode[ip->inum];          \
    assert(rip->type != T_NONE);                            \
    rip;    \
})

struct initrd_inode {
    int nlink;
    int type;
    uint32_t size;
    char *data;
};

static struct initrd_inode rdinode[NRDINODES];

int initrd_alloc(int type);
int initrd_reclaim(struct inode *ip);
int initrd_read(struct inode *ip, char *buf, uint32_t off, uint32_t n);
int initrd_write(struct inode *ip, char *buf, uint32_t off, uint32_t n);
int initrd_unlink(struct inode *ip);
int initrd_dirlink(struct inode *dp, char *name, struct inode *ip);
int initrd_dirlookup(struct inode *dp, char *name);

struct inodeop ramdisk_ops = {
    .alloc      = initrd_alloc,
    .reclaim    = initrd_reclaim,
    .read       = initrd_read,
    .write      = initrd_write,
    .unlink     = initrd_unlink,
    .dirlink    = initrd_dirlink,
    .dirlookup  = initrd_dirlookup,
};

void
initrd_init() 
{
    //file[i] = 
}

int
initrd_reclaim(struct inode *ip) {
    struct initrd_inode *rip = GET_RIP(ip);
    if (rip->nlink == 0) {
        free(rip->data);
    }
    rip->type = T_NONE;
}

// Allocate a file of specific type.
// Return -1 if failed else inum of allocated file.
int 
initrd_alloc(int type)
{
    assert(type);
    for (int i = 0; i < NRDINODES; i ++) {
        if (rdinode[i].type == T_NONE) {
            rdinode[i].size = 0;
            rdinode[i].type = type;
            return i;
        }
    }
    panic("initrd_ialloc: not more inodes.\n");
    return -1;
}

int 
initrd_read(struct inode *ip, char *buf, uint32_t off, uint32_t n)
{
    struct initrd_inode *rip = GET_RIP(ip);
    uint32_t size = rip->size;

    if(off > size || off + n < off) 
        return -1;

    memmove(buf, rip->data + off, MIN(rip->size - off, n));
    return n;
}

int 
initrd_write(struct inode *ip, char *buf, uint32_t off, uint32_t n)
{
    struct initrd_inode *rip = GET_RIP(ip);
    uint32_t size = rip->size;
    if (off > rip->size || off + n < off) 
        return -1;

    if (off + n > rip->size) {
        void *p = realloc(rip->data, off + n);
        if (!p) return -1;
        rip->data = p;
        rip->size = off + n;
    }
    memmove(rip->data + off, buf, n);
    return n;
}

// Lookup subfile with name in directory dp
// Return -1 if failed else inum of the subfile
int
initrd_dirlookup(struct inode *dp, char *name)
{
    struct initrd_inode *rip = GET_RIP(dp);
    cprintf("initrd_dirlookup: dp %d, rip(type=%d)\n", dp->inum, rip->type);
    if (rip->type != T_DIR) 
        return -1;
    uint32_t off = 0;
    for (struct dirent de; initrd_read(dp, (char *)&de, off, sizeof(de)) == sizeof(de); off += sizeof(de)) {
        if (strncmp(name, de.name, MAX_FILENAME_LEN) == 0) {
            return de.inum;
        }
    }
    return -1;
}

// Write a new directory entry (name, ip->inum)
// into the directory dp and increment link of ip.
// Return -1 if failed else 0.
int
initrd_dirlink(struct inode *dp, char *name, struct inode *ip)
{
    struct initrd_inode *rip = GET_RIP(dp);
    // Check that name is not present.
    if (initrd_dirlookup(dp, name) != -1) {
        cprintf("dirlink name already present.\n");
        return -1;
    }
    struct initrd_inode *sip = GET_RIP(ip);
    sip->nlink ++;

    uint32_t off = 0;
    struct dirent de = {0};
    for (; initrd_read(dp, (char *)&de, off, sizeof(de)) == sizeof(de); off += sizeof(de)) 
        // Found an empty dirent
        if (de.inum == 0) break;

    assert(de.inum == 0 || off == rip->size);

    de.inum = ip->inum;
    strncpy(de.name, name, MAX_FILENAME_LEN);
    assert(initrd_write(dp, (char*)&de, off, sizeof(de)) == sizeof(de));

    cprintf("dirlink: dirinum %d, name %s, inum %d(nlink=%d)\n", dp->inum, name, ip->inum, sip->nlink);

    return 0;
}

// Decrement link to ip by 1
// Return #link after unlink.
int
initrd_unlink(struct inode *ip)
{
    struct initrd_inode *rip = GET_RIP(ip);
    assert(rip->nlink > 0);
    return -- rip->nlink;
}

