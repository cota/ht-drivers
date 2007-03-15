/* $Id: cdcmBoth.c,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmBoth.c
; Module Descr:	 Contains functions that should be used both, by Linux and 
;		 LynxOS. This module should be compiled both with Linux and 
;		 Lynx drivers. Depending on the kernel - different actions will
;		 be taken.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Date:          Jul, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmBoth.c: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   21/07/06   Initial version.
*/

#ifdef __linux__
#include <asm/uaccess.h>
#include "cdcmDrvr.h"
#else /* __Lynx__ */
#include  <kernel.h>
#endif


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_copy_from_user
 * DESCRIPTION: This function should be used to pass user data into kernel
 *		address space.
 * RETURNS:	Returns number of bytes that could not be copied.
 *		On success, this will be zero.
 *		SYSERR (-1) - in case of any error.
 *-----------------------------------------------------------------------------
 */
int
cdcm_copy_from_user(
		    char *to,	/* destination address, in kernel space */
		    char *from,	/* source address, in user space */
		    int   size)	/* number of bytes to copy. */
{
#ifdef __linux__

  if (copy_from_user(to, from, size))
    return(SYSERR); /* -EFAULT on Linux */
  

#else  /* __Lynx__ */
  
  /* secure checking */
  if (rbounds((unsigned long) from) < size)
    return(SYSERR); /* -1 */

  memcpy(to, from, size);

#endif
  return(0);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_copy_to_user
 * DESCRIPTION: This function should be used to pass data from kernel to user
 *		address space.
 * RETURNS:	Returns number of bytes that could not be copied.
 *		On success, this will be zero.
 *		SYSERR (-1) - in case of any error.
 *-----------------------------------------------------------------------------
 */
int
cdcm_copy_to_user(
		  char *to,   /* destination address, in user space */
		  char *from, /* source address, in kernel space */
		  int   size) /* number of bytes to copy */
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
/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_init_usr_ptr
 * DESCRIPTION: Should help to initialize user-address space pointers, so that
 *		it's possible to perform direct pointer dereferencing 
 *		afterwards.
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
char*
cdcm_init_usr_ptr(
		  unsigned int usr_addr, /* address from user space */
		  int size,	/* size to validate */
		  int action)	/* DIR_READ, DIR_WRITE, or both */
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
  
#endif
}
#endif
