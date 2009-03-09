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
	/* mem block pointer */
	cdcmm_t *mbptr = cdcm_mem_find_block((char*)pntr);

	if (mbptr && (mbptr->cmFlg & _IOC_WRITE))
		return mbptr->cmSz;

	return 0;
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
	 /* mem block pointer */
	cdcmm_t *mbptr = cdcm_mem_find_block((char*)pntr);

	if (mbptr && (mbptr->cmFlg & _IOC_READ))
		return mbptr->cmSz;

	return 0;
}

