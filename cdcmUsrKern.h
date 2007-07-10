/* $Id: cdcmUsrKern.h,v 1.1 2007/07/10 06:06:59 ygeorgie Exp $ */
/*
; Module Name:	cdcmUsrKern.h
; Module Descr:	General constants and predefined names used both by the
;		user-space and driver are located here.
; Date:         July, 2007.
; Author:       Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmUsrKern.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 2.0   ygeorgie   10/07/07   Production release, CVS controlled.
; 1.0	ygeorgie   04/07/07   Initial version.
*/
#ifndef _CDCM_USER_KERNEL_H_INCLUDE_
#define _CDCM_USER_KERNEL_H_INCLUDE_

#define CDCM_PREFIX      "cdcm_"         /* CDCM prefix */
#define CDCM_DEV_NM_FMT  CDCM_PREFIX"%s" /* device name format */
#define CDCM_DNL         32              /* device name length */
#define CDCM_IPATH_LEN   64              /* info file path lengths */

#endif /* _CDCM_USER_KERNEL_H_INCLUDE_ */
