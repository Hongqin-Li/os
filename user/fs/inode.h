#ifndef FS_INODE_H
#define FS_INODE_H

#include <inc/types.h>

#define MAX_FILENAME_LEN  24
struct dirent {
    int inum;
    char name[MAX_FILENAME_LEN];
};

#define T_NONE        0x00
#define T_FILE        0x01
#define T_DIR         0x02
#define T_PIPE        0x03
#define T_DEV         0x04
#define T_MOUNT       0x08

#define DEV_RAMDISK  0

struct inode {
    int dev, inum;
    int valid;
    int ref;
    struct inodeop *ops;
};

typedef int (*reclaim_t)  (struct inode *ip); // Called when inode has no ref.
typedef int (*alloc_t)    (int type);         // Create new inode
typedef int (*read_t)     (struct inode *ip, char *buf, uint32_t off, uint32_t n);
typedef int (*write_t)    (struct inode *ip, char *buf, uint32_t off, uint32_t n);
typedef int (*unlink_t)   (struct inode *ip);
typedef int (*dirlink_t)  (struct inode *dp, char *name, struct inode *ip);
typedef int (*dirlookup_t)(struct inode *dp, char *name);

struct inodeop {
    alloc_t     alloc;
    reclaim_t   reclaim;
    read_t      read;
    write_t     write;
    unlink_t    unlink;
    dirlink_t   dirlink;
    dirlookup_t dirlookup;
};

struct inode *  iget(int dev, int inum);
void            iput(struct inode *ip);

void fs_init();

int read(struct inode *ip, char *buf, uint32_t off, uint32_t n);
int write(struct inode *ip, char *buf, uint32_t off, uint32_t n);
int dirlink(struct inode *dp, char *name, struct inode *ip);

struct inode *namei(char *path);

// Different file systems
extern struct inodeop ramdisk_ops;

#endif

