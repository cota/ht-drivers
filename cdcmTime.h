/**
 * @file cdcmTime.h
 *
 * @brief timing-related stuff, header file
 *
 * Copyright (c) 2006-2009 CERN
 * @author Yury Georgievskiy <yury.georgievskiy@cern.ch>
 *
 * Copyright (c) 2009 CERN
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * Almost all the code related to semaphores has been taken
 * from the Linux Kernel:
 * Copyright (c) 2008 Intel Corporation
 * Author: Matthew Wilcox <willy@linux.intel.com>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifndef _CDCM_TIME_H_INCLUDE_
#define _CDCM_TIME_H_INCLUDE_

#include "cdcmDrvr.h"

/*
 * This semaphore's implementation is the same as introduced in
 * mainline 2.6.26 (by Matthew Wilcox)
 * Since our current stable version is 2.6.24, we open-code
 * this implementation here.
 */

/*
 * Semaphores: some notes on the implementation:
 *
 * The spinlock controls access to the other members of the semaphore.
 * up() can be called from interrupt context, so we have to disable
 * interrupts when taking the lock.  It turns out various parts
 * of the kernel expect to be able to use down() on a semaphore in
 * interrupt context when they know it will succeed, so we have to use
 * irqsave variants for down() and down_interruptible().
 *
 * The ->count variable represents how many more tasks can acquire this
 * semaphore.  If it's zero, there may be tasks waiting on the wait_list.
 */
struct cdcm_sem {
	spinlock_t		lock;
	unsigned int		count;
	struct list_head	wait_list;
};

/*
 * In CDCM we keep a linked list of semaphores; each of them point to the
 * user's semaphore and have the 'real' cdcm semaphore (described above).
 */
struct cdcm_semaphore {
	int			*user_sem;
	struct list_head	sem_list;
	struct cdcm_sem		sem;
};

/*
 * A waiting task will be added to a cdcm_sem's wait_list.
 * When the task is woken up by the scheduler, it checks the 'up'
 * field; if it's true, then it leaves the semaphore, otherwise
 * it sleeps again.
 */
struct cdcm_sem_waiter {
	struct list_head list;
	struct task_struct *task;
	int up;
};

#define __CDCM_SEM_INITIALIZER(name, n)					\
{									\
	.lock		= __SPIN_LOCK_UNLOCKED((name).lock),		\
	.count		= n,						\
	.wait_list	= LIST_HEAD_INIT((name).wait_list),		\
}

static inline void cdcm_sem_init(struct cdcm_sem *sem, int val)
{
	spin_lock_init(&sem->lock);
	sem->count = val;
	INIT_LIST_HEAD(&sem->wait_list);
}

/*
 * Note: the following is needed by our current semaphore's implementation.
 * In 2.6.26 signal_pending_state() was added.
 * In 2.6.25, the implementation is as in 2.6.29 (see below).
 * In 2.6.24, we have a much simpler implementation below.
 *
 * see commit 364d3c13c17f45da6d638011078d4c4d3070d719 and commit
 * 5b2becc8cffdccdd60c63099f592ddd35aa6c34f . 364d introduces
 * signal_pending_state, and 5b2b uses it in down_common().
 */
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,25)

#include <linux/sched.h>
static inline int signal_pending_state(long state, struct task_struct *p)
{
	if (!(state & (TASK_INTERRUPTIBLE | TASK_WAKEKILL)))
		return 0;
	if (!signal_pending(p))
		return 0;

	return (state & TASK_INTERRUPTIBLE) || __fatal_signal_pending(p);
}

#endif /* 2.6.25 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24)

#include <linux/sched.h>
static inline int signal_pending_state(long state, struct task_struct *p)
{
	return state & TASK_INTERRUPTIBLE && signal_pending(p);
}

#endif /* 2.6.24 */

void cdcm_sema_cleanup_all(void);

#endif /* _CDCM_TIME_H_INCLUDE_ */
