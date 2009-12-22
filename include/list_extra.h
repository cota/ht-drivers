/**
 * @file list_extra.h
 *
 * @brief Extra handy list utils.
 *
 * @author Yury Georgievskiy. CERN AB/CO.
 *
 * @date May, 2008
 *
 * Can be used by the driver and in the user space.
 *
 * @version 1.0  ygeorgie  15/05/2008  Creation date.
 */
#ifndef _LIST_EXTRA_H_INCLUDE_
#define _LIST_EXTRA_H_INCLUDE_

#if defined (__linux__) && defined(__KERNEL__)
/* for kernel only */
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/version.h>
#endif

#if !defined(__KERNEL__) && !defined(__LYNXOS)
/* for user-space only */
#define kfree free
#endif

//!< A way to declare list in a header file
#define DECLARE_GLOB_LIST_HEAD(list_name) \
_DECL struct list_head list_name _INIT(LIST_HEAD_INIT(list_name))

/**
 * list_capacity - how many elements in the list
 *
 * @head: the list to test.
 */
static inline int list_capacity(struct list_head *head)
{
	int cntr = 0;
	struct list_head *at = head;

	while (at->next != head) {
		at = at->next;
		++cntr;
	}

	return cntr;
}

/**
 * list_next - pointer to the next element in the list
 *
 * @head: current list element
 */
static inline struct list_head* list_next(struct list_head *head)
{
	return head->next;
}


/**
 * list_prev - pointer to the previous element in the list
 *
 * @head: current list element
 */
static inline struct list_head* list_prev(struct list_head *head)
{
	return head->prev;
}


/**
 * list_idx - get element of the list
 *
 * @head: the list in question
 * @idx:  list element index (starting from 0) to get
 */
static inline struct list_head* list_idx(struct list_head *head, int idx)
{
	int cntr = 0;
	struct list_head *at = NULL;

	list_for_each(at, head) {
		if (cntr++ == idx)
			break;
	}
	return at;
}


/**
 * list_free_all_safe - delete and free all elements safe from the list
 *
 * All list elements should be allocated entities only!
 *
 * @head:   the head for your list
 * @type:   the type of the struct this is embedded in
 * @member: the name of the list_struct within the struct
 */
#define list_free_all_safe(head, type, member)			\
do {								\
	type *__element;					\
	struct list_head *__list, *__safe;			\
								\
	list_for_each_safe(__list, __safe, head) {		\
		__element = list_entry(__list, type, member);	\
		list_del(&__element->member);			\
		kfree(__element);				\
	}							\
 } while (0)


#ifndef list_last_entry
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)
#endif

#endif /* _LIST_EXTRA_H_INCLUDE_ */
