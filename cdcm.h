/* $Id: cdcm.h,v 1.2 2007/08/01 15:07:20 ygeorgie Exp $ */
/*
; Module Name:	 cdcm.h
; Module Descr:	 CDCM driver header file module. Exported for the user.
;		 Definitions, that should be visible both in user and kernel
;		 space are located here.
; Creation Date: Feb, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcm.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 4.0   ygeorgie   01/08/07   Full Lynx-like installation behaviour.
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   02/06/06   Initial version.
*/

#ifndef _CDCM_H_INCLUDE_
#define _CDCM_H_INCLUDE_

#ifndef __KERNEL__
/* Get necessary definitions from system and kernel headers. */
#include <sys/types.h>
#include <sys/ioctl.h>
#endif

#include <cdcm/cdcmUsrKern.h>

/* Private IOCTL codes:
 *
 * CDCM_CDV_INSTALL    -> register new device logical group in the
 *			  driver. Called each time cdv_install() do.
 * CDCM_CDV_UNINSTALL  -> unloading major device (one of the logical
 *			  groups). Called each time cdv_uninstall() do.
 * CDCM_GET_GRP_MOD_AM -> get device amount of the specified group.
 */

#define CDCM_IOCTL_MAGIC 'c'

#define CDCM_IO(nr)      _IO(CDCM_IOCTL_MAGIC, nr)
#define CDCM_IOR(nr,sz)  _IOR(CDCM_IOCTL_MAGIC, nr, sz)
#define CDCM_IOW(nr,sz)  _IOW(CDCM_IOCTL_MAGIC, nr, sz)
#define CDCM_IOWR(nr,sz) _IOWR(CDCM_IOCTL_MAGIC, nr, sz)

#define CDCM_CDV_INSTALL    CDCM_IOW(197, char[PATH_MAX])
#define CDCM_CDV_UNINSTALL  CDCM_IOW(198, int)
#define CDCM_GET_GRP_MOD_AM CDCM_IOW(199, int)

#endif /* _CDCM_H_INCLUDE_ */
