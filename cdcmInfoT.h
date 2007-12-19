/* $Id: cdcmInfoT.h,v 1.3 2007/12/19 09:02:05 ygeorgie Exp $ */
/**
 * @file cdcmInfoT.h
 *
 * @brief Module info table definitions.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date May, 2006
 *
 * All Linux Info Files that are created, based on the following convention:
 * First comes the standart CDCM header @b cdcm_hdr_t that contains all needed
 * service information.
 * Next, that is comming is the actual user-defined description of every device
 * (i.e. user-style info table). It's not interpreted by @b CDCM.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version 4.0  ygeorgie  01/08/2007  Full Lynx-like installation behaviour.
 * @version 3.0  ygeorgie  14/03/2007  Production release, CVS controlled.
 * @version 2.0  ygeorgie  27/07/2006  First working release.
 * @version 1.0  ygeorgie  01/06/2006  Initial version.
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
  int  cih_grp_bits;  /* declared groups bits. only [0-20] bits are valid */
  int  cih_crc;       /* checksum */
  /* general module name. It will be used to buildup names
     for /proc/devices, irq's, groups, etc... */
  char cih_dnm[CDCM_DNL - sizeof(CDCM_PREFIX)]; 
} cdcm_hdr_t;

/* parameters that are global for all the groups */
typedef struct {
  void *glob_opaque; /* global data, common for all the groups */
} cdcm_glob_t;

/* Group description. Possible group numbers are [1 - 21] */
typedef struct {
  struct list_head grp_list; /* claimed group list */
  struct list_head mod_list; /* list of modules of the current group */

  int grp_num;      /* group number [1 - 21] */
  int mod_amount;   /* module amount in the group */
  void *grp_opaque; /* group-specific data */
  cdcm_glob_t *grp_glob; /* global params pointer */
} cdcm_grp_t;

/* Module description. Possible LUN (Logical Unit Numbers) are [0 - 20]
   Each module belongs to the logical group. Even if there is no logical
   grouping in the current design, each device will belong to it's own group.
   So minimum configuration is one group - one device (module)
 */
typedef struct _cdcmModDescr {
  struct list_head md_list; /* group module linked list */

  int   md_lun;    /* Logical Unit Number [0 - 20] */
  int   md_mpd;    /* Minor amount Per current Device (channels) */
  int   md_ivec;   /* interrupt vector */
  int   md_ilev;   /* interrupt level */
  int   md_iflags; /* module info flags */
  char *md_transp; /* transparent module parameter (string format only) */

  /* VME specific */
  unsigned int md_vme1addr; /* first base adress */
  unsigned int md_vme2addr; /* second base adress */
  
  /* PCI specific */
  unsigned int md_pciVenId; /* vendor ID */
  unsigned int md_pciDevId; /* device ID */

  void       *md_opaque; /* module-specific data */
  cdcm_grp_t *md_owner;  /* group to which module belongs */
} cdcm_md_t;

/* housekeeping defs */
#define GET_GLOBAL_CONTEXT(modp) (modp->md_owner->grp_glob)
#define GET_GROUP_CONTEXT(modp)  (modp->md_owner)

#endif /* _CDCM_INFO_TABLE_H_INCLUDE_ */
