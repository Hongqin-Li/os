#ifndef KERN_LOCKS_H
#define KERN_LOCKS_H

#include <inc/types.h>

// Spin lock
struct spinlock {
    int locked;
};
// Ticket lock
struct ticketlock {
    uint32_t now, last;
};
// MCS lock
struct mcs_node {
    bool waiting;
    struct mcs_node *next;
};
struct mcslock {
    struct mcs_node *tail;
};

void spinlock_acquire(struct spinlock *lk);
void spinlock_release(struct spinlock *lk);

// ticket lock
void ticketlock_acquire(struct ticketlock* lk);
void ticketlock_release(struct ticketlock *lk);

void mcslock_acquire(struct mcslock *lk, struct mcs_node *node);
void mcslock_release(struct mcslock *lk, struct mcs_node *node);


#endif
