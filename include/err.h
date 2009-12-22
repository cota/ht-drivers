/**
 * @file err.h
 *
 * @brief Handy error handling for userspace.
 *
 * @author Taken from Linux Kernel (GPL2)
 *
 * @date 09/07/2008
 */
#ifndef _LINUX_ERR_H
#define _LINUX_ERR_H

/*
 * Kernel pointers have redundant information, so we can use a
 * scheme where we can return either an error code or a dentry
 * pointer with the same return value.
 *
 * This should be a per-architecture thing, to allow different
 * error and pointer decisions.
 */

/* __builtin_expect(A, B) evaluates to A, but notifies the compiler that
   the most likely value of A is B.  This feature was added at some point
   between 2.95 and 3.0.  Let's use 3.0 as the lower bound for now.  */
#define GCC_VERSION (__GNUC__ * 10000 \
			+ __GNUC_MINOR__ * 100 \
			+ __GNUC_PATCHLEVEL__)
#if (GCC_VERSION < 30000)
#define __builtin_expect(a, b) (a)
#endif

//#define __builtin_expect(expr, val) (expr)
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define MAX_ERRNO	4095

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

#endif /* _LINUX_ERR_H */
