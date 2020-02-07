#ifndef INC_LIST_H
#define INC_LIST_H

struct list_head 
{
    struct list_head *next, *prev; 
};

static inline void
__list_del(struct list_head *prev, struct list_head *next) 
{
    next->prev = prev;
    prev->next = next;
}


// API
static inline void
list_init(struct list_head *head) 
{
    head->next = head->prev = head;
}

static inline int 
list_empty(struct list_head *head) 
{
    return head->next == head;
}
static inline struct list_head *
list_front(struct list_head *head)
{
    return head->next;
}
static inline struct list_head *
list_back(struct list_head *head) 
{
    return head->prev;
}

static inline void 
list_insert(struct list_head *new, struct list_head *prev, struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}
static inline void 
list_push_front(struct list_head *head, struct list_head *new) 
{
    list_insert(new, head, head->next);
}
static inline void 
list_push_back(struct list_head *head, struct list_head *new)
{
    list_insert(new, head->prev, head);
}

static inline void
list_drop(struct list_head *item) 
{
    __list_del(item->prev, item->next);
}

static inline void
list_pop_front(struct list_head *head)
{
    list_drop(list_front(head));
}

static inline void
list_pop_back(struct list_head *head)
{
    list_drop(list_back(head));
}

#define LIST_FOREACH_ENTRY(pos, head, member) \
    for(pos = CONTAINER_OF(list_front(head), typeof(*pos), member); \
        &pos->member != (head); \
        pos = CONTAINER_OF(pos->member.next, typeof(*pos), member))

#endif

