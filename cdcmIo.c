/**
 * @file cdcmIo.c
 *
 * @brief 
 *
 * @author Emilio Garcia Cota. CERN AB/CO
 *
 * @date Created on 09/01/2009
 *
 * <long description>
 *
 * @version $Id: cdcmIo.c,v 1.1 2009/01/09 10:14:33 ygeorgie Exp $
 */
#include "cdcmIo.h"

/* I/O functions */

#ifndef __linux__ /* LynxOS */


/* Swapping functions, first for PowerPC, then in general */
#ifdef __powerpc__


static __inline__ u_int16_t __cdcm_swab16(u_int16_t value)
{
  u_int16_t result;

  __asm__("rlwimi %0,%1,8,16,23"
	  : "=r" (result)
	  : "r" (value), "0" (value >> 8));
  return result;
}

static __inline__  u_int32_t __cdcm_swab32(u_int32_t value)
{
  u_int32_t result;

  __asm__("rlwimi %0,%1,24,16,23\n\t"
	  "rlwimi %0,%1,8,8,15\n\t"
	  "rlwimi %0,%1,24,0,7"
	  : "=r" (result)
	  : "r" (value), "0" (value >> 24));
  return result;
}

static __inline__ u_int64_t __cdcm_swab64(u_int64_t x)
{
  u_int32_t h = x >> 32;
  u_int32_t l = x & ((1ULL<<32)-1);
  return (((u_int64_t)__cdcm_swab32(l)) << 32) | ((u_int64_t)(__cdcm_swab32(h)));
}


#else /* not __powerpc__, then we use pure C functions */


static __inline__ u_int16_t __cdcm_swab16(u_int16_t x)
{
  return x<<8 | x>>8;
}

static __inline__ u_int32_t __cdcm_swab32(u_int32_t x)
{
  return x<<24 | x>>24 |
    (x & (u_int32_t)0x0000ff00UL)<<8 |
    (x & (u_int32_t)0x00ff0000UL)>>8;
}

static __inline__ u_int64_t __cdcm_swab64(u_int64_t x)
{
  return x<<56 | x>>56 |
    (x & (u_int64_t)0x000000000000ff00ULL)<<40 |
    (x & (u_int64_t)0x0000000000ff0000ULL)<<24 |
    (x & (u_int64_t)0x00000000ff000000ULL)<< 8 |
    (x & (u_int64_t)0x000000ff00000000ULL)>> 8 |
    (x & (u_int64_t)0x0000ff0000000000ULL)>>24 |
    (x & (u_int64_t)0x00ff000000000000ULL)>>40;
}

#endif /* __powerpc__ */ 

#endif /* !__linux__ */
