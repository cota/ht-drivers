/* $Id: cdcmLynxAPI.c,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmLynxAPI.c
; Module Descr:	 Standart LynxOS API functions are located here.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: Feb, 2007
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmLynxAPI.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 2.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 1.0	ygeorgie   12/02/07   Initial version.
*/

#include "cdcmDrvr.h"
#include "cdcmMem.h"

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern int cdcm_err;		/* global error */
extern cdcmStatics_t cdcmStatT; /* CDCM statics table */


/*-----------------------------------------------------------------------------
 * FUNCTION:    recoset
 * DESCRIPTION: LynxOs bus error trap mechanism. NOT IMPLEMENTED.
 * RETURNS:     
 *-----------------------------------------------------------------------------
 */
int
recoset(void)
{
  return(0); /* Assumes no bus errors */
}

			
/*-----------------------------------------------------------------------------
 * FUNCTION:    noreco
 * DESCRIPTION: LynxOs bus error trap mechanism. NOT IMPLEMENTED.
 * RETURNS:     
 *-----------------------------------------------------------------------------
 */
void
noreco(void)
{
  return;
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    pseterr
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:     
 *-----------------------------------------------------------------------------
 */
int
pseterr(
	int err) /*  */
{ 
  cdcm_err = err; 
  return(err); 
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    sysbrk
 * DESCRIPTION: LynxOs service call wrapper.
 *		NOTE: Depending on the request memory size - different
 *		allocation methods are used. If size is less then 
 *		128Kb - kmalloc, vmalloc otherwice.
 *		128kb - for code is to be completely portable (ldd3).
 * RETURNS:     memory pointer - if success.
 *		NULL           - if fails.
 *-----------------------------------------------------------------------------
 */
char*
sysbrk(
       unsigned long size) /* size in bytes */
{
  char *mem;

  if ( !(mem = (B2KB(size) > CDCM_MEM_BOUND)? vmalloc(size):kmalloc(size, GFP_KERNEL)) ) {
    PRNT_ERR("Can't allocate memory size %lu bytes long", size);
    return(NULL);
  }
  return(mem);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    sysfree
 * DESCRIPTION: LynxOs service call wrapper. I don't use CDCM memory handler
 *		as it's not our business in this case.
 * RETURNS:     void
 *-----------------------------------------------------------------------------
 */
void
sysfree(
	char *cp,	    /* memory pointer */
	unsigned long size) /* memory size */
{
  if (B2KB(size) > CDCM_MEM_BOUND)
    vfree(cp);
  else
    kfree(cp);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    ksprintf
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:     void
 *-----------------------------------------------------------------------------
 */
int
ksprintf(
	 char *buf,		/*  */
	 char *format,		/*  */
	 ...)
{
  int rc;
  va_list kspargs;
  
  va_start(kspargs, format);
  rc = vsprintf(buf, format, kspargs);
  va_end(kspargs);

  return(rc);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    rbounds
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
long
rbounds(
	unsigned long pntr)	/* memory address to check */
{
  cdcmm_t *mbptr = cdcm_mem_find_block((char*)pntr); /* mem block pointer */
  
  if (mbptr && (mbptr->cmFlg & _IOC_WRITE))
    return(mbptr->cmSz);

  return(0);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    wbounds
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
long
wbounds(
	unsigned long pntr)	/* memory address to check */
{
  cdcmm_t *mbptr = cdcm_mem_find_block((char*)pntr); /* mem block pointer */

  if (mbptr && (mbptr->cmFlg & _IOC_READ))
    return(mbptr->cmSz);

  return(0);
}

