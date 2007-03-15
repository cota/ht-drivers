/* $Id: cdcmMem.h,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmMem.h
; Module Descr:	 All CDCM memory handling definitions are located here.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: Feb, 2007
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmMem.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 2.0   ygeorgie   14/03/07   Production release, CVS controlled.   
; 1.0	ygeorgie   17/02/07   Initial version.
*/
#ifndef _CDCM_MEM_H_INCLUDE_
#define _CDCM_MEM_H_INCLUDE_

#define CDCM_ALLOC_DEF 16     /* how many mem block allocated at one shot */
#define CDCM_MEM_BOUND 128*KB /* what to use kmalloc or vmalloc */
/* 
   NOTE: that memory allocation method depends on requested mem size.
         If size is less then 128Kb - than kmalloc is used.
	 If size is bigger - than vmalloc is used.
*/
#define CDCM_M_SHORT_TERM (1<<3) /* short term memory */
#define CDCM_M_LONG_TERM  (1<<4) /* long term memory */
#define CDCM_M_IOCTL      (1<<5) /* claimed by ioctl call */

typedef struct _cdcmMem {
  struct list_head cm_list;     /* linked list */
  char   *cmPtr; /* pointer to allocated memory */
  size_t  cmSz;	 /* allocated size in bytes */
  int     cmFlg; /* memory flags. See CDCM_M_xxx definitions.
		    Also _IOC_NONE (1U),  _IOC_WRITE (2U) and  _IOC_READ (4U)
		    are set as a flags */
} cdcmm_t;

int      cdcm_mem_init(void);
int      cdcm_mem_free(cdcmm_t*);
int      cdcm_mem_cleanup_all(void);
cdcmm_t* cdcm_mem_alloc(size_t, int);
cdcmm_t* cdcm_mem_find_block(char*);
cdcmm_t* cdcm_mem_get_free_block(void);

#endif /* _CDCM_MEM_H_INCLUDE_ */
