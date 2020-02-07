#include <inc/types.h>
#include <inc/list.h>
#include <inc/bitmap.h>
#include <inc/string.h>

#include <kern/mm/buddy.h>
#include <kern/console.h>

struct buddy_page {
    uint32_t magic;
    uint32_t order;
    struct list_head list;
};

// Buddy System Layout
// +--------+------------+--------+-------------+
// | header | freelist[] | bitmap | page region |
// +--------+------------+--------+-------------+

#define BUDDY_SYSTEM(name, order)       \
struct {                                \
    struct buddy_system header;  \
    struct list_head freelist[order+1];  \
    BITMAP(bitmap, 1<<(order+1));        \
    uint32_t magic;                     \
} name

#define BUDDY_MAGIC     0xabcdbdbd


// Parent idx in bitmap tree
#define PARENT(i)     ((i+1)/2-1)

#define PGSIZE 4096
#define BUDDY_PGSIZE(order) ((1<<(order))*PGSIZE)

// Page index and bitmap index
#define BUDDY_PID(hdr, p)   (((uint32_t)(p) - (hdr)->start)/PGSIZE)
#define BUDDY_BID(hdr, p)   (((((uint32_t)(p) + (hdr)->end - 2*(hdr)->start) / PGSIZE) >> (p)->order) - 1)

static void buddy_test(struct buddy_system *);

static void
buddy_stat(void *p) {
    struct buddy_system *hdr = p;
    cprintf("***** buddy system stat\n");
    cprintf("overall range: [0x%x, 0x%x)\n", hdr, hdr->end);
    cprintf("pages range: [0x%x, 0x%x)\n", hdr->start, hdr->end);
    cprintf("max order: %d\n", hdr->max_order);

    uint32_t maxd = hdr->max_order;
    BUDDY_SYSTEM(*bsp, maxd) = (void *)hdr;
    assert(bsp->magic == BUDDY_MAGIC);

    int npage = 0;
    for (int i = 0; i <= maxd; i ++) {
        cprintf("freelist[%d]: ", i);
        struct buddy_page *bp;
        LIST_FOREACH_ENTRY(bp, &bsp->freelist[i], list) {
            assert(bp->magic == BUDDY_MAGIC);
            cprintf("0x%x -> ", bp);
            npage += (1<<i);
        }
        cprintf("\n");
    }
    cprintf("free/total npages: %d/%d\n", npage, (hdr->end - hdr->start) / PGSIZE);
    
}

struct buddy_system *
buddy_init(void *startp, void *endp) 
{
    uint32_t start = (uint32_t)startp;
    uint32_t end = (uint32_t)endp;
    uint32_t size = end - start;

    // Find the max order
    uint32_t maxd;
    for (maxd = 0; maxd < 100; maxd ++) {
        BUDDY_SYSTEM(*bsp, maxd);
        uint32_t tsize = ROUNDUP(start + sizeof(*bsp), PGSIZE) + BUDDY_PGSIZE(maxd) - start;
        if (tsize > size) {
            maxd -= 1;
            break;
        }
    }

    BUDDY_SYSTEM(*bsp, maxd) = startp;
    cprintf("maxd: %d\n", maxd);

    // Init magic, header, bitmap and freelist
    bsp->magic = BUDDY_MAGIC;

    bsp->header.max_order = maxd;
    bsp->header.start = ROUNDUP((uint32_t)bsp + sizeof(*bsp), PGSIZE);
    bsp->header.end = bsp->header.start + BUDDY_PGSIZE(maxd);

    memset(bsp->bitmap, 0, sizeof(bsp->bitmap));

    for (uint32_t i = 0; i <= maxd; i ++)
        list_init(&bsp->freelist[i]);

    // The first whole page
    struct buddy_page *first = (struct buddy_page *)bsp->header.start;
    first->order = maxd;
    first->magic = BUDDY_MAGIC;
    list_push_back(&bsp->freelist[maxd], &first->list);

    buddy_test((void *)bsp);
    return (struct buddy_system *)bsp;
}

void *
buddy_alloc(struct buddy_system *hdr, uint32_t sz) 
{
    uint32_t maxd = hdr->max_order;
    BUDDY_SYSTEM(*bsp, maxd) = (void *)hdr;
    assert(bsp->magic == BUDDY_MAGIC);

    uint32_t d = 0;
    while (d <= maxd && BUDDY_PGSIZE(d) < sz)
        d ++;

    uint32_t fd = d;
    while (fd <= maxd && list_empty(&bsp->freelist[fd])) 
        fd ++;

    if (fd > maxd || list_empty(&bsp->freelist[fd]))
        return 0;
    assert(fd >= d);

    // Remove from list
    struct buddy_page *p = CONTAINER_OF(list_front(&bsp->freelist[fd]), struct buddy_page, list);
    assert(p->magic == BUDDY_MAGIC);
    list_drop(&p->list);

    while (1) {
        bitmap_set(bsp->bitmap, BUDDY_BID(hdr, p), 1);
        if (fd == d) 
            break;
        fd --;
        // Split
        struct buddy_page *right = (struct buddy_page *)((uint32_t)p + BUDDY_PGSIZE(fd));
        right->order = p->order = fd;
        right->magic = BUDDY_MAGIC;
        list_push_back(&bsp->freelist[fd], &right->list);
    }

    assert(fd == d);
    return p;
}

// Free physical memory pointed by va_ptr.
// Return 0 if succeed else 1.
int 
buddy_free(struct buddy_system *hdr, void *va_ptr) 
{
    uint32_t va = (uint32_t)va_ptr;
    if (va < hdr->start || va >= hdr->end)
        return -1;

    uint32_t maxd = hdr->max_order;
    BUDDY_SYSTEM(*bsp, maxd) = (void *)hdr;
    assert(bsp->magic == BUDDY_MAGIC);

    // Skip zeros from bottom of the bitmap tree
    uint32_t bi = (1<<maxd) - 1 + BUDDY_PID(hdr, va);
    uint32_t d = 0;
    while (bi && !bitmap_get(bsp->bitmap, bi)) {
        bi = PARENT(bi);
        d ++;
    }

    assert(bitmap_get(bsp->bitmap, bi));

    // Merge if buddy page is free
    struct buddy_page *p = va_ptr;
    for (; ; d ++, bi = PARENT(bi)) {
        bitmap_set(bsp->bitmap, bi, 0);

        // If buddy page not exists or buddy page is in use.
        if (!bi || bitmap_get(bsp->bitmap, (bi&1) ? bi+1: bi-1))
            break;

        struct buddy_page *bp;
        bp = (struct buddy_page *)((bi&1) ? (uint32_t)p + BUDDY_PGSIZE(d)
                                          : (uint32_t)p - BUDDY_PGSIZE(d));
        assert(bp->magic == BUDDY_MAGIC);
        list_drop(&bp->list);
        p = MIN(bp, p);
    }
    p->order = d;
    p->magic = BUDDY_MAGIC;
    list_push_back(&bsp->freelist[d], &p->list);
    return 0;
}

static void 
buddy_test(struct buddy_system *hdr) {
    static uint32_t size = 17;
    int n = 10;
    char *p[n];

    cprintf("allocating\n");
    for (int i = 0; i < n; i ++) {
        size = (size * 19) % (BUDDY_PGSIZE(3));
        cprintf("size: %d\n", size);
        p[i] = buddy_alloc(hdr, size);
        if (!p[i]) break;
    }
    buddy_stat(hdr);

    cprintf("freeing\n");
    for (int i = 0; i < n && p[i]; i ++)
        buddy_free(hdr, p[i]);
    buddy_stat(hdr);

}
