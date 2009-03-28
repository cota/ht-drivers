/**
 * @file cdcmBoth.c
 *
 * @brief Contains functions that should be used both, by Linux and LynxOS.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 21/07/2006
 *
 * This module should be compiled both with Linux and Lynx drivers.
 * Depending on the kernel - different actions will be taken.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version
 */
#include "cdcmBoth.h"

#ifdef __linux__
#include <asm/uaccess.h>
#include "cdcmDrvr.h"
#else /* __Lynx__ */
#include  <kernel.h>
#endif


/**
 * @brief Should be used to pass user data into kernel address space.
 *
 * @param to   - destination address, in kernel space
 * @param from - source address, in user space
 * @param size - number of bytes to copy.
 *
 * @return Number of bytes that could not be copied. On success,
 *         this will be zero.
 * @return SYSERR (-1) - in case of any error.
 */
int cdcm_copy_from_user(void *to, void *from, int size)
{
#ifdef __linux__

  if (copy_from_user(to, from, size))
    return(SYSERR); /* -EFAULT on Linux */


#else  /* __Lynx__ */


  /* Even though Lynx can access user space memory directly, we do the memcpy
   * because assigning to = from directly would make the code much more
   * complicated -- since it would copy under Linux but not under LynxOS.
   */

  /* secure checking */
  if (rbounds((unsigned long) from) < size)
    return(SYSERR); /* -1 */

  memcpy(to, from, size);

#endif
  return(0);
}


/**
 * @brief Should be used to pass data from kernel to user address space.
 *
 * @param to   - destination address, in user space
 * @param from - source address, in kernel space
 * @param size - number of bytes to copy
 *
 * @return Returns number of bytes that could not be copied.
 *         On success, this will be zero.
 * @return SYSERR (-1) - in case of any error.
 */
int cdcm_copy_to_user(void *to, void *from, int size)
{
#ifdef __linux__

  if (copy_to_user(to, from, size))
    return(SYSERR); /* -EFAULT on Linux */

#else  /* __Lynx__ */

  /* secure checking */
  if (wbounds((unsigned long) to) < size)
    return(SYSERR); /* -1 */

  memcpy(to, from, size);

#endif
  return(0);
}


#if 0
/**
 * @brief Helps to initialize user-address space pointers.
 *
 * @param usr_addr - address from user space
 * @param size     - size to validate
 * @param action   - DIR_READ, DIR_WRITE, or both
 *
 * Like that it's possible to perform direct pointer dereferencing afterwards.
 *
 * @return
 */
char* cdcm_init_usr_ptr(unsigned int usr_addr, int size, int action)
{

#ifdef __linux__

#else  /* __Lynx__ */

  /* secure checkings first */
  if (action & DIR_READ)
    if (rbounds((unsigned long) usr_addr) < size)
      return(SYSERR); /* -1 */
  if (action & DIR_WRITE)
    if (wbounds((unsigned long) usr_addr) < size)
      return(SYSERR); /* -1 */

#endif /* !__linux__ */
}
#endif
