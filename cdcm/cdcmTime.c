/**
 * @file cdcmTime.c
 *
 * @brief timing-related stuff, mainly timers and semaphores
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

#include <linux/time.h>
#include <linux/delay.h>
#include "list_extra.h" /* for extra handy list operations */
#include "cdcmTime.h"
#include "cdcmThread.h"
#include "cdcmLynxDefs.h"

extern cdcmStatics_t cdcmStatT;	/* declared in the cdcmDrvr.c module */

static DEFINE_SPINLOCK(sem_list_lock); /* protect access to cdcm_sem_list */

/**
 * @brief busy loop for at least the requested number of microseconds
 *
 * @param usecs - microseconds
 */
void usec_sleep(unsigned long usecs)
{
       udelay(usecs);
}

/**
 * @brief Generic timer callback.
 *
 * @param arg - Index of the timer data. Assumed to be a valid one.
 */
static void cdcm_timer_cb(unsigned long arg)
{
	cdcmt_t *my_data = &cdcmStatT.cdcm_timer[arg];

	del_timer(&my_data->ct_timer);
	my_data->ct_on = 0;
	my_data->ct_uf(my_data->ct_arg); /* call user payload */
}

/**
 * @brief LynxOs service call wrapper.
 *
 * @param sec - current time in seconds is placed here
 *
 * @return fraction of a second in nanoseconds.
 *         @e sec will hold current time in seconds.
 */
int nanotime(unsigned long *sec)
{
	struct timespec curtime;

	curtime = current_kernel_time();

	*sec = (unsigned long)curtime.tv_sec;

	return (int)curtime.tv_nsec;
}

static inline void cdcm_timer_init(tpp_t func, void *arg, int interval, int tid)
{
	unsigned long expires = jiffies + msecs_to_jiffies(10 * interval);

	cdcmStatT.cdcm_timer[tid] = (cdcmt_t) {
		.ct_on = current->pid,
		.ct_timer = TIMER_INITIALIZER(cdcm_timer_cb, expires, tid),
		.ct_uf  = func,
		.ct_arg = arg
	};
}

/**
 * @brief LynxOs service call wrapper.
 *
 * @param func     - function to call
 * @param arg      - argument for function handler
 * @param interval - timeout interval (10 millisecond granularity)
 *
 * @return non-negative timeout id - if there is a timer available.
 * @return SYSERR (i.e. -1)        - otherwise.
 */
int timeout(tpp_t func, void *arg, int interval)
{
	int tid; /* timeout ID */

	for (tid = 0; tid < MAX_CDCM_TIMERS; tid++) {

		if (cdcmStatT.cdcm_timer[tid].ct_on)
			continue;

		/* not used, initialise it */
		cdcm_timer_init(func, arg, interval, tid);

		init_timer(&cdcmStatT.cdcm_timer[tid].ct_timer);
		add_timer(&cdcmStatT.cdcm_timer[tid].ct_timer);

		return tid + 1;
	}

	PRNT_ERR(cdcmStatT.cdcm_ipl, "No more timers available (MAX %d)",
		 MAX_CDCM_TIMERS);
	return SYSERR;
}

/**
 * @brief LynxOs service call wrapper.
 *
 * @param id - timeout id
 *
 * @return OK - on success
 * @return SYSERR - on failure
 */
int cancel_timeout(int id)
{
	int tid = id - 1; /* remember that the user received tid + 1 */

	if (tid < 0 || tid >= MAX_CDCM_TIMERS) {
		PRNT_ERR(cdcmStatT.cdcm_ipl, "No such timer: %d", id);
		return SYSERR;
	}

	if (!cdcmStatT.cdcm_timer[tid].ct_on)
		return OK; /* it had already expired */

	del_timer_sync(&cdcmStatT.cdcm_timer[tid].ct_timer);
	cdcmStatT.cdcm_timer[tid].ct_on = 0;

	return OK;
}

static struct cdcm_semaphore *__get_sema(int *user_sem)
{
	struct cdcm_semaphore *sema;

	list_for_each_entry(sema, &cdcmStatT.cdcm_sem_list_head, sem_list)
		if (sema->user_sem == user_sem)
			return sema;

	/* the semaphore hasn't been used until now */
	sema = kmalloc(sizeof(struct cdcm_semaphore), GFP_ATOMIC);
	if (sema == NULL) {
		PRNT_ABS_WARN("Couldn't allocate new semaphore");
		return NULL;
	}

	/* initialise the semaphore */
	sema->user_sem = user_sem;
	cdcm_sem_init(&sema->sem, *user_sem);
	list_add(&sema->sem_list, &cdcmStatT.cdcm_sem_list_head);

	return sema;
}

/**
 * @brief finds a cdcm-semaphore given a pointer to a user's semaphore
 *
 * @param sem - user's-defined semaphore
 *
 * @return semaphore - on success
 * @return NULL - if a new semaphore cannot be allocated
 */
static struct cdcm_semaphore *get_sema(int *user_sem)
{
	struct cdcm_semaphore *sema;
	unsigned long flags;

	spin_lock_irqsave(&sem_list_lock, flags);
	sema = __get_sema(user_sem);
	spin_unlock_irqrestore(&sem_list_lock, flags);

	return sema;
}

/*
 * a task is added as a waiter to the tail of the semaphore's wait_list;
 * then it just loops and gets woken-up by the scheduler (how often
 * this happens depends on @timeout), to check if it's been marked to wake-up.
 */
static inline int __cdcm_down_common(struct cdcm_sem *sem, long state,
				     long timeout)
{
	struct task_struct *task = current;
	struct cdcm_sem_waiter waiter;

	list_add_tail(&waiter.list, &sem->wait_list);
	waiter.task = task;
	waiter.up = 0;

	for (;;) {
		if (signal_pending_state(state, task))
			goto interrupted;
		if (timeout <= 0)
			goto timed_out;
		__set_task_state(task, state);
		spin_unlock_irq(&sem->lock);
		timeout = schedule_timeout(timeout);
		spin_lock_irq(&sem->lock);
		/*
		 * In Lynx a kthread might want to wait on a semaphore;
		 * we check here for the case where the sleeping thread
		 * has been marked to be stopped. If that's the case,
		 * we wake up the thread (which will be then killed).
		 */
		if (waiter.up || kthread_should_stop())
			return 0;
	}

 timed_out:
	list_del(&waiter.list);
	return -ETIME;

 interrupted:
	list_del(&waiter.list);
	return -EINTR;
}

static noinline void __cdcm_down(struct cdcm_sem *sem)
{
	__cdcm_down_common(sem, TASK_UNINTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}

static noinline int __cdcm_down_interruptible(struct cdcm_sem *sem)
{
	return __cdcm_down_common(sem, TASK_INTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}

static noinline int __cdcm_down_timeout(struct cdcm_sem *sem, long jiffies)
{
	return __cdcm_down_common(sem, TASK_UNINTERRUPTIBLE, jiffies);
}

static inline void cdcm_down(struct cdcm_sem *sem)
{
	unsigned long flags;

	spin_lock_irqsave(&sem->lock, flags);
	if (likely(sem->count > 0))
		sem->count--;
	else
		__cdcm_down(sem);
	spin_unlock_irqrestore(&sem->lock, flags);
}

static inline int cdcm_down_interruptible(struct cdcm_sem *sem)
{
	unsigned long flags;
	int result = 0;

	spin_lock_irqsave(&sem->lock, flags);
	if (likely(sem->count > 0))
		sem->count--;
	else
		result = __cdcm_down_interruptible(sem);
	spin_unlock_irqrestore(&sem->lock, flags);

	return result;
}

static inline int cdcm_down_timeout(struct cdcm_sem *sem, long jiffies)
{
	unsigned long flags;
	int result = 0;

	spin_lock_irqsave(&sem->lock, flags);
	if (likely(sem->count > 0))
		sem->count--;
	else
		result = __cdcm_down_timeout(sem, jiffies);
	spin_unlock_irqrestore(&sem->lock, flags);

	return result;
}

/**
 * @brief try to acquire the semaphore, without waiting
 * @return 0 - if the semaphore has been acquired successfully
 * @return 1 - if it cannot be acquired
 */
static inline int cdcm_down_trylock(struct cdcm_sem *sem)
{
	unsigned long flags;
	int count;

	spin_lock_irqsave(&sem->lock, flags);
	count = sem->count - 1;
	if (likely(count >= 0))
		sem->count = count;
	spin_unlock_irqrestore(&sem->lock, flags);

	return count < 0;
}

static noinline void __cdcm_up_waiter(struct cdcm_sem *sem)
{
	struct cdcm_sem_waiter *waiter = list_first_entry(&sem->wait_list,
						struct cdcm_sem_waiter, list);
	list_del(&waiter->list);
	waiter->up = 1;
	wake_up_process(waiter->task);
}

/*
 * 'up' has a very simple implementation:
 * - if there are no waiters, increment count
 * - if there are waiters, wake the first waiter in the FIFO up
 */
static inline void __cdcm_up(struct cdcm_sem *sem)
{
	if (likely(list_empty(&sem->wait_list)))
		sem->count++;
	else
		__cdcm_up_waiter(sem);
}

static inline void cdcm_up(struct cdcm_sem *sem)
{
	unsigned long flags;

	spin_lock_irqsave(&sem->lock, flags);
	__cdcm_up(sem);
	spin_unlock_irqrestore(&sem->lock, flags);
}

int swait(int *user_sem, int sig)
{
	int result = 0;
	struct cdcm_semaphore *sema = get_sema(user_sem);

	if (sema == NULL)
		return SYSERR;

	switch (sig) {
	case SEM_SIGIGNORE:
		cdcm_down(&sema->sem);
		break;
	case SEM_SIGABORT:
	case SEM_SIGRETRY:
	/* yes, I know SIGRETRY != SIGABORT, but we don't use SIGRETRY.. */
		result = cdcm_down_interruptible(&sema->sem);
		break;
	default:
		result = SYSERR;
	}

	return result ? SYSERR : OK;
}

/*
 * Lynx' tswait: swait with timeout
 */
int tswait(int *user_sem, int sig, int interval)
{
	struct cdcm_semaphore *sema;
	unsigned long expires = msecs_to_jiffies(10 * interval);
	int ret;

	if (user_sem == NULL) {
		msleep(10 * interval);
		return TSWAIT_TIMEDOUT;
	}

	sema = get_sema(user_sem);
	if (sema == NULL)
		return SYSERR;

	if (!interval)
		return cdcm_down_trylock(&sema->sem);
	else if (interval < 0)
		return swait(user_sem, sig);

	ret = cdcm_down_timeout(&sema->sem, expires); /* non-interruptible */
	switch (ret) {
	case 0:
		return OK;
	case -ETIME:
		return TSWAIT_TIMEDOUT;
	case -EINTR:
		return TSWAIT_ABORTED;
	default:
		return SYSERR;
	}
}

int ssignal(int *user_sem)
{
	struct cdcm_semaphore *sema = get_sema(user_sem);

	if (sema == NULL)
		return SYSERR;

	cdcm_up(&sema->sem);
	return OK;
}

/*
 * atomically 'up' the semaphore 'count' times
 */
int ssignaln(int *user_sem, int count)
{
	struct cdcm_semaphore *sema = get_sema(user_sem);
	unsigned long flags;
	int i;

	if (sema == NULL)
		return SYSERR;

	spin_lock_irqsave(&sema->sem.lock, flags);
	for (i = 0; i < count; i++)
		__cdcm_up(&sema->sem);
	spin_unlock_irqrestore(&sema->sem.lock, flags);

	return OK;
}

/*
 * if there are no waiters, simply set the count to 0.
 * if there are waiters, wake them all up.
 */
static inline void __sreset(struct cdcm_sem *sem)
{
	int i;

	if (!sem->count) {
		int waiters = list_capacity(&sem->wait_list);

		for (i = 0; i < waiters; i++)
			__cdcm_up_waiter(sem);
	} else
		sem->count = 0;
}

void sreset(int *user_sem)
{
	struct cdcm_semaphore *sema = get_sema(user_sem);
	unsigned long flags;

	if (sema == NULL)
		return;

	spin_lock_irqsave(&sema->sem.lock, flags);
	__sreset(&sema->sem);
	spin_unlock_irqrestore(&sema->sem.lock, flags);
}

/*
 * Note: count is an unsigned int and represents how many tasks can acquire
 * the semaphore. Therefore if count is bigger than 0, there are no waiters.
 * If count is zero, then there may (or may not) be waiters; the number of
 * elements in wait_list provides the answer.
 */
static inline int __scount(struct cdcm_sem *sem)
{
	if (sem->count > 0)
		return sem->count;
	else
		return -1 * list_capacity(&sem->wait_list);
}

int scount(int *user_sem)
{
	struct cdcm_semaphore *sema = get_sema(user_sem);
	unsigned long flags;
	int result;

	if (sema == NULL)
		return SYSERR;

	spin_lock_irqsave(&sema->sem.lock, flags);
	result = __scount(&sema->sem);
	spin_unlock_irqrestore(&sema->sem.lock, flags);

	return result;
}

/**
 * @brief Cleanup all allocated semaphores.
 */
void cdcm_sema_cleanup_all(void)
{
	struct cdcm_semaphore *sem, *tmp;

	/* free allocated memory */
	list_for_each_entry_safe(sem, tmp, &cdcmStatT.cdcm_sem_list_head,sem_list) {
		list_del(&sem->sem_list);
		kfree(sem);
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
/* was not exported before this */
void set_normalized_timespec(struct timespec *ts, time_t sec, long nsec)
{
        while (nsec >= NSEC_PER_SEC) {
                nsec -= NSEC_PER_SEC;
                ++sec;
        }
        while (nsec < 0) {
                nsec += NSEC_PER_SEC;
                --sec;
        }
        ts->tv_sec = sec;
        ts->tv_nsec = nsec;
}
#endif /* 2.6.26 */
