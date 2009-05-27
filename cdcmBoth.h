/**
 * @file cdcmBoth.h
 *
 * @brief Contains definitions that are used both, by LynxOS and Linux drivers.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 21/07/2006
 *
 * It should be included in the user part of the driver.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version
 */
#ifndef _CDCM_BOTH_H_INCLUDE_
#define _CDCM_BOTH_H_INCLUDE_

#ifdef __linux__

#include <asm/byteorder.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>

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

#else /* LynxOS */

#include <conf.h>


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

#endif /* __linux__ */


/* ms_to_ticks, ticks_to_ms conversion */

/* Pay attention to precedence in these operations!
 * if you divide first, you'll always return 0.
 * Therefore the  macros below will return 0 for sub-tick delays.
 */
#ifdef __linux__
#define cdcm_ticks_to_ms(x) ((x) * 1000 / HZ)
#define cdcm_ms_to_ticks(x) ((x) * HZ / 1000)
#else /* Lynx */
#define cdcm_ticks_to_ms(x) ((x) * 1000 / TICKSPERSEC)
#define cdcm_ms_to_ticks(x) ((x) * TICKSPERSEC / 1000)
#endif /* !__linux__ */


int cdcm_copy_from_user(void *, void *, int);
int cdcm_copy_to_user(void *, void *, int);
/*
 * The extern declaration of these functions is mainly for Lynx, since For Linux,
 * they need to be declared as extern before being used. For Linux,
 * they're declared (as non-extern) in cdcmLynxAPI.h.
 */
extern int ksprintf(char *buf, char *format, ...);
extern void usec_sleep(unsigned long usecs);
extern int scount(int *semaphore);

/*
 * CDCM Spinlocks: mimic Linux' spinlocks and rwLocks.
 * In Lynx, we just use the only available primitives (disable/restore)
 * We use 'int foo' in Lynx to avoid compiler warnings (unused struct)
 */
typedef struct {
#ifdef __linux__
	spinlock_t lock;
#else
	/* no individual locks in LynxOS */
	int foo;
#endif
} cdcm_spinlock_t;

typedef struct {
#ifdef __linux__
	rwlock_t rwlock;
#else
	int foo;
#endif
} cdcm_rwlock_t;

#ifdef __linux__
#define cdcm_spin_lock_init(cdcm) spin_lock_init(&(cdcm)->lock)

#define cdcm_spin_lock_irqsave(cdcm, flags)		\
		spin_lock_irqsave(&(cdcm)->lock, flags)

#define cdcm_spin_unlock_irqrestore(cdcm, flags)		\
		spin_unlock_irqrestore(&(cdcm)->lock, flags)

#define __CDCM_SPINLOCK_UNLOCKED(name) {			\
		.lock = __SPIN_LOCK_UNLOCKED(name.lock), }

#define cdcm_rwlock_init(cdcm) rwlock_init(&(cdcm)->rwlock)

#define __CDCM_RW_LOCK_UNLOCKED(name) {				\
		.rwlock = __RW_LOCK_UNLOCKED(name.rwlock), }
/*
 * in {read,write}_lock, use flags as a dummy value.
 */
#define cdcm_read_lock(cdcm, flags)		\
	do {					\
		read_lock(&(cdcm)->rwlock);	\
		flags = 1;			\
	} while (0)

#define cdcm_read_unlock(cdcm, flags)		\
	do {					\
		flags = 0;			\
		read_unlock(&(cdcm)->rwlock);	\
	} while (0)

#define cdcm_write_lock(cdcm, flags)		\
	do {					\
		write_lock(&(cdcm)->rwlock);	\
		flags = 1;			\
	} while (0)

#define cdcm_write_unlock(cdcm, flags)		\
	do {					\
		flags = 0;			\
		write_unlock(&(cdcm)->rwlock);	\
	} while (0)

#define cdcm_read_lock_irqsave(cdcm, flags)			\
		read_lock_irqsave(&(cdcm)->rwlock, flags)

#define cdcm_read_unlock_irqrestore(cdcm, flags)		\
		read_unlock_irqrestore(&(cdcm)->rwlock, flags)

#define cdcm_write_lock_irqsave(cdcm, flags)			\
		write_lock_irqsave(&(cdcm)->rwlock, flags)

#define cdcm_write_unlock_irqrestore(cdcm, flags)		\
		write_unlock_irqrestore(&(cdcm)->rwlock, flags)

#else /* lynx */

/*
 * Note: foo is just used to actually do something on the struct,
 * but it has no logical significance. As it is now, 'foo' refers
 * to the lock's state: 0 (unlocked), 1 (locked), but really, there's
 * no code that should ever need to rely on this value. So simply ignore it.
 */
# define _lynx_lock_init(cdcm)		\
	do {				\
		(cdcm)->foo = 0;	\
	} while (0)			\

# define __CDCM_SPINLOCK_UNLOCKED(lockname) { \
		.foo = 0, }

# define _lynx_lock(cdcm, flags)	\
	do {				\
		disable(flags);		\
		(cdcm)->foo = 1;	\
	} while (0)

# define _lynx_unlock(cdcm, flags)	\
	do {				\
		(cdcm)->foo = 0;	\
		restore(flags);		\
	} while (0)

# define cdcm_spin_lock_init(cdcm)		  _lynx_lock_init(cdcm)
# define cdcm_spin_lock_irqsave(cdcm, flags)	  _lynx_lock(cdcm, flags)
# define cdcm_spin_unlock_irqrestore(cdcm, flags) _lynx_unlock(cdcm, flags)

# define cdcm_rwlock_init(cdcm)		_lynx_lock_init(cdcm)
# define __CDCM_RW_LOCK_UNLOCKED(x)	__CDCM_SPINLOCK_UNLOCKED(x)

# define cdcm_read_lock_irqsave(cdcm, flags)	   _lynx_lock(cdcm, flags)
# define cdcm_read_unlock_irqrestore(cdcm, flags)  _lynx_unlock(cdcm, flags)

# define cdcm_write_lock_irqsave(cdcm, flags)	   _lynx_lock(cdcm, flags)
# define cdcm_write_unlock_irqrestore(cdcm, flags) _lynx_unlock(cdcm, flags)

# define cdcm_read_lock(cdcm, flags)	_lynx_lock(cdcm, flags)
# define cdcm_read_unlock(cdcm, flags)	_lynx_unlock(cdcm, flags)

# define cdcm_write_lock(cdcm, flags)	_lynx_lock(cdcm, flags)
# define cdcm_write_unlock(cdcm, flags)	_lynx_unlock(cdcm, flags)

#endif /* !__linux__ */

#define CDCM_DEFINE_SPINLOCK(x) \
		cdcm_spinlock_t x = __CDCM_SPINLOCK_UNLOCKED(x)
#define CDCM_DEFINE_RWLOCK(x) \
		cdcm_rwlock_t x = __CDCM_RW_LOCK_UNLOCKED(x)

/*
 * Mutexes: make them Linux-like, calling ssignal/swait in Lynx
 */
struct cdcm_mutex {
#ifdef __linux__
	struct mutex mutex;
#else
	int sem;
#endif
};

#ifdef __linux__
#define __CDCM_MUTEX_INIT(name) \
	(struct cdcm_mutex) {	.mutex = DEFINE_MUTEX(name.mutex) }
#define cdcm_mutex_init(cdcm)	mutex_init(&(cdcm)->mutex)
#define cdcm_mutex_lock(cdcm)	mutex_lock(&(cdcm)->mutex)
#define cdcm_mutex_unlock(cdcm)	mutex_unlock(&(cdcm)->mutex)
#define cdcm_mutex_lock_interruptible(cdcm) \
		mutex_lock_interruptible(&(cdcm)->mutex)
#else
# define __CDCM_MUTEX_INIT(name) \
	(struct cdcm_mutex) {	.sem = 1 }
# define cdcm_mutex_init(cdcm) 		do { (cdcm)->sem = 1; } while (0)
# define cdcm_mutex_lock(cdcm) 		swait(&(cdcm)->sem, SEM_SIGIGNORE)
# define cdcm_mutex_unlock(cdcm) 	ssignal(&(cdcm)->sem)
# define cdcm_mutex_lock_interruptible(cdcm) \
		swait(&(cdcm)->sem, SEM_SIGABORT)
#endif /* !__linux__ */

/* statically define a mutex */
#define CDCM_MUTEX(x) struct cdcm_mutex x = __CDCM_MUTEX_INIT(x)


#ifdef __Lynx__

#define CDCM_LOOP_AGAIN

#endif

#endif /* _CDCM_BOTH_H_INCLUDE_ */
