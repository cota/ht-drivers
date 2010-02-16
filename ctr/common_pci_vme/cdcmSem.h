#ifndef __CDCM_SEM_H
#define __CDCM_SEM_H

#include <linux/version.h>
#include <linux/kthread.h>

#define SEM_SIGRETRY 0
#define SEM_SIGIGNORE 1
#define SEM_SIGABORT 2

#define SREAD 4

int	swait(int *sem, int flag);
int	tswait(int *sem, int flag, int interval);
int	ssignal(int *sem);
int	ssignaln(int *sem, int count);
void	sreset(int *sem);
int	scount(int *sem);

void	cdcm_free_semaphores(void);

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

static inline int cdcm_list_size(struct list_head *head)
{
	int cntr = 0;
	struct list_head *at = head;

	while (at->next != head) {
		at = at->next;
		++cntr;
	}

	return cntr;
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

#endif /* __CDCM_SEM_H */
