/**
 * @file cdcmIo.h
 *
 * @brief 
 *
 * @author Emilio Garcia Cota. CERN AB/CO
 *
 * @date Created on 09/01/2009
 *
 * <long description>
 *
 * @version $Id: cdcmIo.h,v 1.1 2009/01/09 10:14:33 ygeorgie Exp $
 */
#ifndef _CDCM_IO_H_INCLUDE_
#define _CDCM_IO_H_INCLUDE_

#ifdef __linux__
#include <asm/io.h>        /* for memory barrier macros */
#include <asm/byteorder.h> /* for endianness handling */
#include <linux/types.h>   /* for fixed-size types */
#else /* LynxOS */
#include <sys/types.h>     /* for fixed-size types */
#include <io.h>

#if defined(__powerpc__)
#include <port_ops_ppc.h>
#elif defined(__x86__)
#include <port_ops_x86.h>
#else
#include <port_ops.h>
#endif

#endif /* __linux__ */


/* memory barriers
 * Note that although in some platforms these barriers are nop, we should
 * call them to ensure ordering when necessary to ensure portability.
 *
 * From the Linux kernel:
 * mb() prevents loads _and_ stores being reordered across this point.
 * rmb() prevents loads being reordered across this point.
 * wmb() prevents stores being reordered across this point.
 */
#ifdef __linux__

#define cdcm_mb()   mb()
#define cdcm_rmb()  rmb()
#define cdcm_wmb()  wmb()

#else /* Lynx @ powerpc */
/* 
 * See arch/powerpc/include/asm/system.h @ Linux kernel
 * See also the section "Memory Barrier Instructions" here:
 * http://www.ibm.com/developerworks/eserver/articles/powerpc.html
 */
#define cdcm_mb()   __asm__ __volatile__ ("sync" : : : "memory")
#define cdcm_rmb()  __asm__ __volatile__ ("sync" : : : "memory")
#define cdcm_wmb()  __asm__ __volatile__ ("sync" : : : "memory")
#endif /* __linux__ */



/*
 * Little/Big endian conversion
 */
#ifdef __linux__

#define cdcm_le16_to_cpu(x) (le16_to_cpu(x))
#define cdcm_le32_to_cpu(x) (le32_to_cpu(x))
#define cdcm_le64_to_cpu(x) (le64_to_cpu(x))
#define cdcm_cpu_to_le16(x) (cpu_to_le16(x))
#define cdcm_cpu_to_le32(x) (cpu_to_le32(x))
#define cdcm_cpu_to_le64(x) (cpu_to_le64(x))

#define cdcm_be16_to_cpu(x) (be16_to_cpu(x))
#define cdcm_be32_to_cpu(x) (be32_to_cpu(x))
#define cdcm_be64_to_cpu(x) (be64_to_cpu(x))
#define cdcm_cpu_to_be16(x) (cpu_to_be16(x))
#define cdcm_cpu_to_be32(x) (cpu_to_be32(x))
#define cdcm_cpu_to_be64(x) (cpu_to_be64(x))

#else /* LynxOS */

/* Swapping functions, first for PowerPC, then in general */
#ifdef __powerpc__


static inline u_int16_t __cdcm_swab16(u_int16_t value)
{
  u_int16_t result;

  __asm__("rlwimi %0,%1,8,16,23"
	  : "=r" (result)
	  : "r" (value), "0" (value >> 8));
  return result;
}

static inline  u_int32_t __cdcm_swab32(u_int32_t value)
{
  u_int32_t result;

  __asm__("rlwimi %0,%1,24,16,23\n\t"
	  "rlwimi %0,%1,8,8,15\n\t"
	  "rlwimi %0,%1,24,0,7"
	  : "=r" (result)
	  : "r" (value), "0" (value >> 24));
  return result;
}

static inline u_int64_t __cdcm_swab64(u_int64_t x)
{
  u_int32_t h = x >> 32;
  u_int32_t l = x & ((1ULL<<32)-1);
  return (((u_int64_t)__cdcm_swab32(l)) << 32) | ((u_int64_t)(__cdcm_swab32(h)));
}


#else /* not __powerpc__, then we use pure C functions */


static inline u_int16_t __cdcm_swab16(u_int16_t x)
{
  return x<<8 | x>>8;
}

static inline u_int32_t __cdcm_swab32(u_int32_t x)
{
  return x<<24 | x>>24 |
    (x & (u_int32_t)0x0000ff00UL)<<8 |
    (x & (u_int32_t)0x00ff0000UL)>>8;
}

static inline u_int64_t __cdcm_swab64(u_int64_t x)
{
  return x<<56 | x>>56 |
    (x & (u_int64_t)0x000000000000ff00ULL)<<40 |
    (x & (u_int64_t)0x0000000000ff0000ULL)<<24 |
    (x & (u_int64_t)0x00000000ff000000ULL)<< 8 |
    (x & (u_int64_t)0x000000ff00000000ULL)>> 8 |
    (x & (u_int64_t)0x0000ff0000000000ULL)>>24 |
    (x & (u_int64_t)0x00ff000000000000ULL)>>40;
}

#endif /* !__powerpc__ */

#define cdcm_be16_to_cpu(x) ((u_int16_t)(x))
#define cdcm_be32_to_cpu(x) ((u_int32_t)(x))
#define cdcm_be64_to_cpu(x) ((u_int64_t)(x))
#define cdcm_cpu_to_be16(x) ((u_int16_t)(x))
#define cdcm_cpu_to_be32(x) ((u_int32_t)(x))
#define cdcm_cpu_to_be64(x) ((u_int64_t)(x))

#define cdcm_le16_to_cpu(x) __cdcm_swab16((u_int16_t)(x))
#define cdcm_le32_to_cpu(x) __cdcm_swab32((u_int32_t)(x))
#define cdcm_le64_to_cpu(x) __cdcm_swab64((u_int64_t)(x))
#define cdcm_cpu_to_le16(x) ((u_int16_t)__cdcm_swab16(x))
#define cdcm_cpu_to_le32(x) ((u_int32_t)__cdcm_swab32(x))
#define cdcm_cpu_to_le64(x) ((u_int64_t)__cdcm_swab64(x))

#endif /* !__linux__ */

/* I/O functions. */
#ifdef __linux__

/* We just call the kernel functions */

#define cdcm_ioread8(A)      ioread8(A)
#define cdcm_ioread16(A)     ioread16(A)
#define cdcm_ioread32(A)     ioread32(A)

#define cdcm_iowrite8(V, A)  iowrite8(V, A)
#define cdcm_iowrite16(V, A) iowrite16(V, A)
#define cdcm_iowrite32(V, A) iowrite32(V, A)

#else /* LynxOS -- I/O functions */

/* 
 * here is better to define functions to ensure consistency when calling
 * these functions.
 */

static inline unsigned int cdcm_ioread8(void *addr)
{
  unsigned int ret = __inb((__port_addr_t)addr);
  cdcm_mb();
  return ret;
}
static inline unsigned int cdcm_ioread16(void *addr)
{
  unsigned int ret = __inw((__port_addr_t)addr);
  cdcm_mb();
  return ret;
}
static inline unsigned int cdcm_ioread32(void *addr)
{
  unsigned int ret = __inl((__port_addr_t)addr);
  cdcm_mb();
  return ret;
}
static inline void cdcm_iowrite8(u_int8_t val, void *addr)
{
  __outb((__port_addr_t)addr, (unsigned char)val);
  cdcm_mb();
}
static inline void cdcm_iowrite16(u_int16_t val, void *addr)
{
  __outw((__port_addr_t)addr, (unsigned short)val);
  cdcm_mb();
}
static inline void cdcm_iowrite32(u_int32_t val, void *addr)
{
  __outl((__port_addr_t)addr, (unsigned long)val);
  cdcm_mb();
}

#endif /* !__linux__ */

/*
 * I/O macros for both platforms
 * These combine the macros above to cover the most common I/O cases
 */
#define cdcm_ioread16be(A) cdcm_be16_to_cpu(cdcm_ioread16((A)))
#define cdcm_ioread32be(A) cdcm_be32_to_cpu(cdcm_ioread32((A)))
#define cdcm_ioread64be(A) cdcm_be64_to_cpu(cdcm_ioread64((A)))

#define cdcm_ioread16le(A) cdcm_le16_to_cpu(cdcm_ioread16((A)))
#define cdcm_ioread32le(A) cdcm_le32_to_cpu(cdcm_ioread32((A)))
#define cdcm_ioread64le(A) cdcm_le64_to_cpu(cdcm_ioread64((A)))

#define cdcm_iowrite16be(V, A) cdcm_iowrite16(cdcm_cpu_to_be16((V)), (A))
#define cdcm_iowrite32be(V, A) cdcm_iowrite32(cdcm_cpu_to_be32((V)), (A))
#define cdcm_iowrite64be(V, A) cdcm_iowrite64(cdcm_cpu_to_be64((V)), (A))

#define cdcm_iowrite16le(V, A) cdcm_iowrite16(cdcm_cpu_to_le16((V)), (A))
#define cdcm_iowrite32le(V, A) cdcm_iowrite32(cdcm_cpu_to_le32((V)), (A))
#define cdcm_iowrite64le(V, A) cdcm_iowrite64(cdcm_cpu_to_le64((V)), (A))

#endif /* _CDCM_IO_H_INCLUDE_ */
