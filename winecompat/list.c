#include <stddef.h>

#include "wine/list.h"

void list_init(struct list *list)
{
	list->next = list;
	list->prev = list;
}

void list_remove(struct list *element)
{
	element->next->prev = element->prev;
	element->prev->next = element->next;
}

void list_add_tail(struct list *list, struct list *element)
{
	list_add_before(list, element);
}

void list_add_before(struct list *element, struct list *to_add)
{
	to_add->next = element;
	to_add->prev = element->prev;
	element->prev->next = to_add;
	element->prev = to_add;
}

int list_empty(const struct list *list)
{
	return list->next == list;
}

