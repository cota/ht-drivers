/* $Id: cdcmBoth.h,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmBoth.h
; Module Descr:	 Contains definitions that are used both, by LynxOS and Linux
;		 drivers. It should be included in the user part of the driver.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: Jul, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmBoth.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   21/07/06   Initial version.
*/

#ifndef _CDCM_BOTH_H_INCLUDE_
#define _CDCM_BOTH_H_INCLUDE_

int cdcm_copy_from_user(char *, char *, int);
int cdcm_copy_to_user(char *, char *, int);

#ifdef __Lynx__

#define CDCM_LOOP_AGAIN

#endif

#endif /* _CDCM_BOTH_H_INCLUDE_ */
