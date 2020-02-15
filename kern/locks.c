#include <inc/types.h>
#include <kern/locks.h>
#include <kern/console.h>

void 
spinlock_acquire(struct spinlock *lk) {
    while(lk->locked || __sync_lock_test_and_set(&lk->locked, 1))
        ;
}

void 
spinlock_release(struct spinlock *lk) {
    if (!lk->locked)
        panic("spinlock_release: not locked\n");
    __sync_lock_test_and_set(&lk->locked, 0);
}


// ticket lock
void 
ticketlock_acquire(struct ticketlock* lk) {
    uint32_t me = __sync_add_and_fetch(&lk->last, 1);
    while (lk->now != me) ;
}
void 
ticketlock_release(struct ticketlock *lk) {
    __sync_add_and_fetch(&lk->now, 1);
}

void 
mcslock_acquire(struct mcslock *lk, struct mcs_node *node) {

    node->next = 0;
    struct mcs_node* prev = __sync_lock_test_and_set(&lk->tail, node);//atomic add self into the queue
    if (prev) {
        node->waiting = 1;
        prev->next = node;
        while (node->waiting) ;
    }
}

void
mcslock_release(struct mcslock *lk, struct mcs_node *node) {

    if (!node->next && __sync_bool_compare_and_swap(&lk->tail, node, 0)) 
        return;// No one waiting

    while (!node->next) ;

    node->next->waiting = 0;
}

