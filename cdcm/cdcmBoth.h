/**
 * @file cdcmBoth.h
 *
 * @brief header file with helpers for both Linux and Lynx
 *
 * Copyright (c) 2006-2009 CERN
 * @author Yury Georgievskiy <yury.georgievskiy@cern.ch>
 *
 * Copyright (c) 2009 CERN
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifndef _CDCM_BOTH_H_INCLUDE_
#define _CDCM_BOTH_H_INCLUDE_

#ifdef __linux__

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/fs.h>

#include <asm/byteorder.h>

#include "cdcmDrvr.h"


/*
 * Endianness definitions
 */
#if defined(__LITTLE_ENDIAN) && defined(__BIG_ENDIAN)
# error CDCM: Fix asm/byteorder.h to define one endianness.
#endif

#if !defined(__LITTLE_ENDIAN) && !defined(__BIG_ENDIAN)
# error CDCM: Fix asm/byteorder.h to define one endianness.
#endif

#if defined(__LITTLE_ENDIAN)
#define CDCM_LITTLE_ENDIAN 1234
#else /* __BIG_ENDIAN */
#define CDCM_BIG_ENDIAN 4321
#endif

/*
 * ms_to_ticks, ticks_to_ms conversion
 */
#define cdcm_ticks_to_ms(x) ((x) * 1000 / HZ)
#define cdcm_ms_to_ticks(x) ((x) * HZ / 1000)

/*
 * CDCM Spinlocks: wrap Linux' spinlocks
 */
typedef struct {
	spinlock_t lock;
} cdcm_spinlock_t;

#define cdcm_spin_lock_init(cdcm) spin_lock_init(&(cdcm)->lock)

#define cdcm_spin_lock_irqsave(cdcm, flags)		\
		spin_lock_irqsave(&(cdcm)->lock, flags)

#define cdcm_spin_unlock_irqrestore(cdcm, flags)		\
		spin_unlock_irqrestore(&(cdcm)->lock, flags)

#define __CDCM_SPINLOCK_UNLOCKED(name) {			\
		.lock = __SPIN_LOCK_UNLOCKED(name.lock), }

/*
 * Mutexes: wrap Linux' mutexes
 */
struct cdcm_mutex {
	struct mutex mutex;
};

#define __CDCM_MUTEX_INIT(name) \
	(struct cdcm_mutex) {	.mutex = DEFINE_MUTEX(name.mutex) }
#define cdcm_mutex_init(cdcm)	mutex_init(&(cdcm)->mutex)
#define cdcm_mutex_lock(cdcm)	mutex_lock(&(cdcm)->mutex)
#define cdcm_mutex_unlock(cdcm)	mutex_unlock(&(cdcm)->mutex)
#define cdcm_mutex_lock_interruptible(cdcm) \
		mutex_lock_interruptible(&(cdcm)->mutex)



#else /* LynxOS */


#include <kernel.h>
#include <conf.h>

/*
 * Endianness definitions
 */
#if defined(__LITTLE_ENDIAN__) && defined(__BIG_ENDIAN__)
# error CDCM: Conflicting endianities defined in this machine.
#endif

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
# error CDCM: Conflicting endianities defined in this machine.
#endif

#if defined(__LITTLE_ENDIAN__)
#define CDCM_LITTLE_ENDIAN 1234
#else /* __BIG_ENDIAN__ */
#define CDCM_BIG_ENDIAN 4321
#endif

/*
 * ms_to_ticks, ticks_to_ms conversion
 */
#define cdcm_ticks_to_ms(x) ((x) * 1000 / TICKSPERSEC)
#define cdcm_ms_to_ticks(x) ((x) * TICKSPERSEC / 1000)

/*
 * CDCM Spinlocks: mimic Linux' spinlocks
 * In Lynx, we just use the only available primitives (disable/restore)
 * We use 'int foo' in Lynx to avoid compiler warnings (i.e. unused struct)
 */
typedef struct {
	/* no individual locks in LynxOS */
	int foo;
} cdcm_spinlock_t;

/*
 * Note: foo is just used to actually do something on the struct,
 * but it has no logical significance. As it is now, 'foo' refers
 * to the lock's state: 0 (unlocked), 1 (locked), but really, there's
 * no code that should ever need to rely on this value. So simply ignore it.
 */
#define _lynx_lock_init(cdcm)		\
	do {				\
		(cdcm)->foo = 0;	\
	} while (0)			\

#define __CDCM_SPINLOCK_UNLOCKED(lockname) { \
		.foo = 0, }

#define _lynx_lock(cdcm, flags)	\
	do {				\
		disable(flags);		\
		(cdcm)->foo = 1;	\
	} while (0)

#define _lynx_unlock(cdcm, flags)	\
	do {				\
		(cdcm)->foo = 0;	\
		restore(flags);		\
	} while (0)

#define cdcm_spin_lock_init(cdcm)		  _lynx_lock_init(cdcm)
#define cdcm_spin_lock_irqsave(cdcm, flags)	  _lynx_lock(cdcm, flags)
#define cdcm_spin_unlock_irqrestore(cdcm, flags)  _lynx_unlock(cdcm, flags)

/*
 * Mutexes: make them Linux-like, calling ssignal/swait in Lynx
 */
struct cdcm_mutex {
	int sem;
};

#define __CDCM_MUTEX_INIT(name) \
	(struct cdcm_mutex) {	.sem = 1 }
#define cdcm_mutex_init(cdcm) 		do { (cdcm)->sem = 1; } while (0)
#define cdcm_mutex_lock(cdcm) 		swait(&(cdcm)->sem, SEM_SIGIGNORE)
#define cdcm_mutex_unlock(cdcm) 	ssignal(&(cdcm)->sem)
#define cdcm_mutex_lock_interruptible(cdcm) \
		swait(&(cdcm)->sem, SEM_SIGABORT)

#endif /* !__linux__ */



/*
 * Common Linux and Lynx
 */

#define CDCM_DEFINE_SPINLOCK(x) \
		cdcm_spinlock_t x = __CDCM_SPINLOCK_UNLOCKED(x)
/* statically define a mutex */
#define CDCM_MUTEX(x) struct cdcm_mutex x = __CDCM_MUTEX_INIT(x)

/*
 * The extern declaration of these functions is mainly for Lynx, since For Linux,
 * they need to be declared as extern before being used. For Linux,
 * they're declared (as non-extern) in cdcmLynxAPI.h.
 */
extern int ksprintf(char *buf, char *format, ...);
extern void usec_sleep(unsigned long usecs);
extern int scount(int *semaphore);

/*
 * Common function signatures
 */
int cdcm_copy_from_user(void *, void *, int);
int cdcm_copy_to_user(void *, void *, int);

#ifdef __Lynx__
#define CDCM_LOOP_AGAIN
#endif

#endif /* _CDCM_BOTH_H_INCLUDE_ */
