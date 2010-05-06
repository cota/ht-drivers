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
 * @version
 */
#include "cdcmDrvr.h"
#include "cdcmMem.h"

/* external crap */
extern int cdcm_err;		/* global error */
extern cdcmStatics_t cdcmStatT; /* CDCM statics table */


/**
 * @brief LynxOs bus error trap mechanism. TODO.
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
 * @brief LynxOs bus error trap mechanism. TODO.
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
	struct cdcm_mem_header *buf = (struct cdcm_mem_header *)pntr;

	buf--;

	return buf->flags & _IOC_WRITE ? buf->size : 0;
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
	struct cdcm_mem_header *buf = (struct cdcm_mem_header *)pntr;

	buf--;

	return buf->flags & _IOC_READ ? buf->size : 0;
}

/**
 * @brief Allocate memory.
 *
 * @param size - memory size to allocate in bytes
 *
 * Depending on the request memory size - different allocation methods
 * are used. If size is less then 128Kb - kmalloc, vmalloc otherwise.
 * 128kb - for code is to be completely portable (ldd3).
 *
 * @return allocated memory pointer - if success.
 * @return NULL                     - if fails.
 */

char* sysbrk(unsigned long size)
{
	return cdcm_mem_alloc(size, 0);
}


/**
 * @brief Free allocated memory
 *
 * @param cp   - memory pointer to free
 * @param size - memory size
 *
 * @return void
 */
void sysfree(char *cp, unsigned long size)
{
	return cdcm_mem_free(cp);
}
