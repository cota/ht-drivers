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


#ifdef __Lynx__

#define CDCM_LOOP_AGAIN

#endif

#endif /* _CDCM_BOTH_H_INCLUDE_ */
