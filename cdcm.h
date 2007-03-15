/* $Id: cdcm.h,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcm.h
; Module Descr:	 CDCM driver header file module. Exported for the user.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: Feb, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcm.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   02/06/06   Initial version.
*/

#ifndef _CDCM_H_INCLUDE_
#define _CDCM_H_INCLUDE_

#include <linux/types.h>	/* for dev_t */
#include "cdcmUsr.h"

/* definitions, that should be visible both in user and kernel spaces */
#define CDCM_DEV_NAME "cdcm-"CDCM_USR_DRVR_NAME
#define CDCM_DNL 16 /* name length of each device */

typedef struct tagMODINFO {
  char info_dname[CDCM_DNL]; /* device name */
  int  info_major;           /* major device number */
  int  info_minor;           /* minor device number */
} modinfo_t;


/* Private IOCTL codes:
 *
 * CDCM_SRVIO_GET_MOD_AMOUNT -> get amount of found devices
 * CDCM_SRVIO_GET_MOD_INFO   -> get device names and their numbers
 *				NOTE - we don't use dev_t to pass device
 *				number, because it's of different size in
 *				the kernel and user space (4bytes and 8bytes
 *				respectively). We'll have definition mismatch.
 *
 */

#define CDCM_IOC_MAGIC 'c'

#define CDCM_SRVIO_GET_MOD_AMOUNT _IOR(CDCM_IOC_MAGIC, 197, int)
#define CDCM_SRVIO_GET_MOD_INFO   _IOR(CDCM_IOC_MAGIC, 198, modinfo_t)

#endif /* _CDCM_H_INCLUDE_ */
