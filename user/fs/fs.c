#include <stdio.h>
#include <string.h>
#include <kern/locks.h>
#include <fs/inode.h>

int rootinum;

// Initialize VFS
void fs_init() 
{
    char *s = "This is a test file\n";
    char buf[20];
    struct inode *root, *file;

    rootinum = ramdisk_ops.alloc(T_DIR);
    root = iget(DEV_RAMDISK, rootinum);

    file = iget(DEV_RAMDISK, root->ops->alloc(T_FILE));
    dirlink(root, "test.txt", file);
    iput(file);

    file = namei("/test.txt");
    assert(file);
    write(file, s, 0, strlen(s) + 1);
    read(file, buf, 0, strlen(s) + 1);
    iput(file);

    cprintf("fs read: %s", buf);
}

#define NICACHE 100
struct {
    //struct spinlock lock;
    struct inode inode[NICACHE];
} icache;

// Find the inode with number inum on device dev
// and return the in-memory copy.
// O(NICACHE), TODO: optimize to O(1) by hashing
struct inode *
iget(int dev, int inum) 
{
    struct inode *free_ip = 0;
    for (struct inode *ip = icache.inode; ip < icache.inode + NICACHE; ip ++) {
        if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
            ip->ref += 1;
            return ip;
        }
        if (!free_ip && !ip->ref) 
            free_ip = ip;
    }
    assert(free_ip);

    free_ip->dev = dev;
    free_ip->inum = inum;
    free_ip->ref = 1;
    //TODO load virtual ops according to dev
    assert(dev == DEV_RAMDISK);
    free_ip->ops = &ramdisk_ops;

    return free_ip;
}

// Drop a reference to an in-memory inode.
void
iput(struct inode *ip)
{
    ip->ref--;
    if (ip->ref == 0) 
        ip->ops->reclaim(ip);
}

int
read(struct inode *ip, char *buf, uint32_t off, uint32_t n) 
{
    assert(ip->ops && ip->ops->read)
    return ip->ops->read(ip, buf, off, n);
}

int
write(struct inode *ip, char *buf, uint32_t off, uint32_t n)
{
    assert(ip->ops && ip->ops->write)
    return ip->ops->write(ip, buf, off, n);
}

int
dirlink(struct inode *dp, char *name, struct inode *ip)
{
    assert(dp->ops && dp->ops->write)
    return dp->ops->dirlink(dp, name, ip);
}

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static char *
skipelem(char *path, char *name)
{
    char *s;
    int len;

    while (*path == '/') 
        path ++;
    if (*path == 0) 
        return 0;
    s = path;
    while (*path != '/' && *path != 0)
        path++;
    len = path - s;
    if (len >= MAX_FILENAME_LEN)
        memmove(name, s, MAX_FILENAME_LEN);
    else {
        memmove(name, s, len);
        name[len] = 0;
    }
    while(*path == '/')
        path++;
    return path;
}

// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Must be called inside a transaction since it calls iput().
static struct inode*
namex(char *path, int nameiparent, char *name)
{
    struct inode *ip;
    int next;

    if(*path == '/')
        ip = iget(DEV_RAMDISK, rootinum);
    else
        panic("namex: relative namex not implemented\n");

    while ((path = skipelem(path, name)) != 0) {
        cprintf("names: path: %s\n", path);
        if (nameiparent && *path == '\0') {
            // Stop one level early.
            return ip;
        }
        if ((next = ip->ops->dirlookup(ip, name)) == -1) {
            iput(ip);
            return 0;
        }
        iput(ip);
        // FIXME swtch fs if it's a mount point
        ip = iget(DEV_RAMDISK, next);
    }
    if (nameiparent) {
        iput(ip);
        return 0;
    }
    return ip;
}

struct inode *
namei(char *path)
{
    char name[MAX_FILENAME_LEN];
    return namex(path, 0, name);
}

struct inode *
nameiparent(char *path, char *name)
{
    return namex(path, 1, name);
}


