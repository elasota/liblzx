#pragma once

struct list
{
	struct list *prev;
	struct list *next;
};

void list_init(struct list *list);
void list_remove(struct list *element);
void list_add_tail(struct list *list, struct list *element);
void list_add_before(struct list *element, struct list *to_add);
int list_empty(const struct list *list);

#define LIST_ENTRY(elem, type, field) \
    ((type *)((char *)(elem) - (uintptr_t)(&((type *)0)->field)))
    
#define LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, list, type, field) \
    for ((cursor) = LIST_ENTRY((list)->next, type, field), \
         (cursor2) = LIST_ENTRY((cursor)->field.next, type, field); \
         &(cursor)->field != (list); \
         (cursor) = (cursor2), \
         (cursor2) = LIST_ENTRY((cursor)->field.next, type, field))

#define LIST_FOR_EACH_ENTRY(elem, list, type, field) \
    for ((elem) = LIST_ENTRY((list)->next, type, field); \
         &(elem)->field != (list); \
         (elem) = LIST_ENTRY((elem)->field.next, type, field))
