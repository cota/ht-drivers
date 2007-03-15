/* $Id: cdcmInfoT.h,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmInfoT.h
; Module Descr:	 All Linux Info Files that are created, based on the following
;		 convention:
;		 First comes the standart CDCM header 'cdcmHeader' that 
;		 contains all needed service information. Next, that is comming
;		 is the actual user-defined description of every device (i.e.
;		 user-style info table). It's not interpreted by CDCM.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: May, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmInfoT.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   01/06/06   Initial version.
*/

#ifndef _CDCM_INFO_TABLE_H_INCLUDE_
#define _CDCM_INFO_TABLE_H_INCLUDE_

#define CDCM_MAGIC_IDENT (('C'<<24)|('D'<<16)|('C'<<8)|'M')

/* info file header */
typedef struct _cdcmInfoHeader {
  int cih_signature; /* 'CDCM' */
  int cih_sz;	     /* info file size */
  int cih_crc;	     /* checksum */
  int cih_mpd;       /* minor devices amount per module */
} cdcmHeader; 

/* general info table */
typedef struct _cdcmInfoTable {
  cdcmHeader cdcmit_sig; /* standart info file header */
  void *cdcmit_usrData;	 /* user info tables */
} cdcmInfoT;

#endif /* _CDCM_INFO_TABLE_H_INCLUDE_ */
