/**
 * @file cdcmLynxAPI.c
 *
 * @brief Standart LynxOS API functions are located here.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 12/02/2007
 *
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version $Id: cdcmLynxAPI.c,v 1.3 2009/01/09 10:26:03 ygeorgie Exp $
 */
#include "cdcmDrvr.h"
#include "cdcmMem.h"

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern int cdcm_err;		/* global error */
extern cdcmStatics_t cdcmStatT; /* CDCM statics table */


/**
 * @brief LynxOs bus error trap mechanism. !TODO!
 *
 * @param none
 *
 * @return 
 */
int recoset(void)
{
  return(0); /* Assumes no bus errors */
}


/**
 * @brief LynxOs bus error trap mechanism. !TODO!
 *
 * @param none
 *
 * @return void
 */
void noreco(void)
{
  return;
}


/**
 * @brief LynxOs service call wrapper.
 *
 * @param err - 
 *
 * @return 
 */
int pseterr(int err)
{ 
  cdcm_err = err; 
  return(err); 
}


/**
 * @brief Allocate memory.
 *
 * @param size - memory size to allocate in bytes
 *
 * LynxOs service call wrapper.
 * Depending on the request memory size - different allocation methods
 * are used. If size is less then 128Kb - kmalloc, vmalloc otherwice.
 * 128kb - for code is to be completely portable (ldd3).
 *
 * @return allocated memory pointer - if success.
 * @return NULL                     - if fails.
 */
char* sysbrk(unsigned long size)
{
  char *mem;

  if ( !(mem = (B2KB(size) > CDCM_MEM_BOUND)? vmalloc(size):kmalloc(size, GFP_KERNEL)) ) {
    PRNT_ERR("Can't allocate memory size %lu bytes long", size);
    return(NULL);
  }
  return(mem);
}


/**
 * @brief Free allocated memory
 *
 * @param cp   - memory pointer to free
 * @param size - memory size
 *
 * LynxOs service call wrapper. We don't use CDCM memory handler as it's
 * not our business in this case.
 */
void sysfree(char *cp, unsigned long size)
{
  if (B2KB(size) > CDCM_MEM_BOUND)
    vfree(cp);
  else
    kfree(cp);
}


/**
 * @brief LynxOs service call wrapper.
 *
 * @param buf    - 
 * @param format - 
 * @param ...    - 
 *
 * Read ksprintf manpage for more info.
 *
 * @return
 */
int ksprintf(char *buf,char *format, ...)
{
  int rc;
  va_list kspargs;
  
  va_start(kspargs, format);
  rc = vsprintf(buf, format, kspargs);
  va_end(kspargs);

  return(rc);
}


/**
 * @brief LynxOs service call wrapper.
 *
 * @param pntr - memory address to check
 *
 * @return 
 */
long rbounds(unsigned long pntr)
{
  cdcmm_t *mbptr = cdcm_mem_find_block((char*)pntr); /* mem block pointer */
  
  if (mbptr && (mbptr->cmFlg & _IOC_WRITE))
    return(mbptr->cmSz);

  return(0);
}


/**
 * @brief LynxOs service call wrapper.
 *
 * @param 
 * @param pntr - memory address to check
 *
 * @return 
 */
long wbounds(unsigned long pntr)
{
  cdcmm_t *mbptr = cdcm_mem_find_block((char*)pntr); /* mem block pointer */

  if (mbptr && (mbptr->cmFlg & _IOC_READ))
    return(mbptr->cmSz);

  return(0);
}

