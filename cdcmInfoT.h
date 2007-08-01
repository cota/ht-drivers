/* $Id: cdcmInfoT.h,v 1.2 2007/08/01 15:07:20 ygeorgie Exp $ */
/*
; Module Name:	 cdcmInfoT.h
; Module Descr:	 All Linux Info Files that are created, based on the following
;		 convention:
;		 First comes the standart CDCM header 'cdcm_hdr_t' that 
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
; 4.0   ygeorgie   01/08/07   Full Lynx-like installation behaviour.
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   01/06/06   Initial version.
*/

#ifndef _CDCM_INFO_TABLE_H_INCLUDE_
#define _CDCM_INFO_TABLE_H_INCLUDE_

#include "cdcm.h"

/* signature */
#define CDCM_MAGIC_IDENT (('C'<<24)|('D'<<16)|('C'<<8)|'M')

#define MAX_VME_SLOT 21 /* how many slots in the VME crate */

/* CDCM info header. Parameter for module_init entry point */
typedef struct _cdcmInfoHeader {
  int  cih_signature; /* 'CDCM' */
  int  cih_grp_bits;  /* declared groups bits. only [0-20]bits are valid */
  int  cih_crc;       /* checksum */
  /* general module name. It will be used to buildup names
     for /proc/devices, irq's, groups, etc... */
  char cih_dnm[CDCM_DNL - sizeof(CDCM_PREFIX)]; 
} cdcm_hdr_t;

/* Module description
   Possible LUN (Logical Unit Numbers) are [0 -20]
   Each module belongs to the logical group. Even if there is no logical
   grouping in the current design, each device will belong to it's own group.
   So minimum configuration is one group - one device
 */
typedef struct _cdcmModDescr {
  int md_gid; /* group ID to which module belongs [1 - 21] */
  int md_lun; /* Logical Unit Number */
  int md_mpd; /* Minor amount Per current Device (channels) */

  int md_ivec; /* interrupt vector */
  int md_ilev; /* interrupt level */

  /* VME specific */
  unsigned int md_vme1addr; /* first base adress */
  unsigned int md_vme2addr; /* second base adress */
  
  /* PCI specific */
  unsigned int md_pciVenId; /* vendor ID */
  unsigned int md_pciDevId; /* device ID */

  struct list_head md_list; /* group module linked list */
} cdcm_md_t;

/* Group description.
   Possible group numbers are [1 - 21]
*/
typedef struct {
  struct list_head grp_list; /* claimed group list */
  struct list_head mod_list; /* list of modules of the current group */
  int grp_num; /* group number (starting from 1) */
  int mod_am;  /* module amount in the group */
} cdcm_grp_t;

#endif /* _CDCM_INFO_TABLE_H_INCLUDE_ */
