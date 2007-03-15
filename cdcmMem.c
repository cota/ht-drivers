/* $Id: cdcmMem.c,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmMem.c
; Module Descr:	 All CDCM memory handling is located here
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: Feb, 2007
; Author:	 Georgievskiy Yuryy, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmMem.c: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 2.0   ygeorgie   14/03/07   Production release, CVS controlled.   
; 1.0	ygeorgie   17/02/07   Initial version.
*/

#include "cdcmDrvr.h"
#include "cdcmMem.h"

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern int cdcm_err;		/* global error */
extern cdcmStatics_t cdcmStatT; /* CDCM statics table */

static int cdcm_alloc_blocks = 0; /* amount of allocated memory blocks */
static int cdcm_claimed_blocks = 0; /* how many already used */

/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_mem_init
 * DESCRIPTION: CDCM memory handler initialization.
 * RETURNS:	0       - we cool
 *		-ENOMEM - we are not.
 *-----------------------------------------------------------------------------
 */
int
cdcm_mem_init(void)
{
  int cntr;
  cdcmm_t *amem; /* allocated memory */
  
  for (cntr = 0; cntr < CDCM_ALLOC_DEF; cntr++) {
    if ( !(amem = kzalloc(sizeof(cdcmm_t), GFP_KERNEL)) ) {
      PRNT_ABS_ERR("Can't alloc CDCM memory handle");
      cdcm_mem_cleanup_all();	/* roll back */
      return(-ENOMEM);
    }
    list_add(&amem->cm_list, &cdcmStatT.cdcm_mem_list_head);
    ++cdcm_alloc_blocks;
  }
  return(0);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_mem_free
 * DESCRIPTION: Release previously claimed block and free allocated memory.
 * RETURNS:	0 - success.
 *-----------------------------------------------------------------------------
 */
int
cdcm_mem_free(
	      cdcmm_t *pntr)	/*  */
{
  if (!pntr)
    return(0);

  //PRNT_DBG("Dealloc mem @0x%x, Size %u bytes, Flgs 0x%x\n", (unsigned int)pntr->cmPtr, (unsigned int)pntr->cmSz, (unsigned int)pntr->cmFlg);

  if (B2KB(pntr->cmSz) > CDCM_MEM_BOUND)
    vfree(pntr->cmPtr);
  else
    kfree(pntr->cmPtr);
  
  /* reset */
#if 0

  NOTE - Initialization like this is zeroing out _all_ the rest!
         I.e. it is _not_ correct!!!

  *pntr = (cdcmm_t) {
    .cmPtr = NULL,
    .cmSz  = 0,
    .cmFlg = 0
  };
#endif
  pntr->cmPtr = NULL;
  pntr->cmSz  = 0;
  pntr->cmFlg = 0;

  --cdcm_claimed_blocks;

  return(0);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_mem_cleanup_all
 * DESCRIPTION: Cleanup all memory blocks and their allocated memory.
 * RETURNS:	0  - if success
 *	       -1  - if fails
 *-----------------------------------------------------------------------------
 */
int
cdcm_mem_cleanup_all(void)
{
  cdcmm_t *memP, *tmpP;

  /* free allocated memory */
  list_for_each_entry_safe(memP, tmpP, &cdcmStatT.cdcm_mem_list_head, cm_list) {
    if (memP->cmPtr) {
      CDCM_DBG("Releasing allocated memory chunk %p\n", memP);
      cdcm_mem_free(memP);
    }
    
    list_del(&memP->cm_list);
    --cdcm_alloc_blocks;
    kfree(memP);
  }
  
  return(0);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_mem_alloc
 * DESCRIPTION: Allocates memory to be used in IOCTL operations. Depending on
 *		the request memory size - different allocation methods are
 *		used. If size is less then 128Kb - kmalloc, vmalloc otherwice.
 *		128kb - for code is to be completely portable (ldd3).
 *		Moreover by default memory is considered to be a short term
 *		usage (i.e. CDCM_M_SHORT_TERM) and will not be included in
 *		the linked list of allocated memory chunks!
 * RETURNS:	-ENOMEM               - if fails.
 *		memory handle pointer - if success.
 *-----------------------------------------------------------------------------
 */
cdcmm_t*
cdcm_mem_alloc(
	       size_t size, /* size in bytes */
	       int    flgs) /* memory flags */
{
  cdcmm_t *amem = cdcm_mem_get_free_block();

  if (!amem) {
    PRNT_ABS_ERR("Can't allocate CDCM memory handle");
    return(ERR_PTR(-ENOMEM));
  }
  
  if ( !(amem->cmPtr = (B2KB(size) > CDCM_MEM_BOUND) ? vmalloc(size) : kmalloc(size, GFP_KERNEL)) ) {
    PRNT_ERR("Can't alloc mem size %u bytes long", size);
    return(ERR_PTR(-ENOMEM));
  }

  ++cdcm_claimed_blocks;
  amem->cmSz  = size;
  amem->cmFlg = flgs;

  //PRNT_DBG("Alloc mem @0x%x, Size %u bytes, Flgs 0x%x\n", (unsigned int)amem->cmPtr, (unsigned int)size, (unsigned int)flgs);

  return(amem);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_mem_find_block
 * DESCRIPTION: Search CDCM memory descriptor using memory pointer.
 * RETURNS:	pointer to found entry - if success.
 *		NULL                   - if fails
 *-----------------------------------------------------------------------------
 */
cdcmm_t*
cdcm_mem_find_block(
		    char *mptr)	/* allocated memory pointer */
{
  cdcmm_t *tmp;
  
  list_for_each_entry(tmp, &cdcmStatT.cdcm_mem_list_head, cm_list) {
    if (tmp->cmPtr == mptr)
      return(tmp);
  }

  return(NULL);	/* not found */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_mem_get_free_block
 * DESCRIPTION: Search for free CDCM memory block.
 * RETURNS:	pointer to free block - if success.
 *		NULL                  - if fails
 *-----------------------------------------------------------------------------
 */
cdcmm_t*
cdcm_mem_get_free_block(void)
{
  cdcmm_t *tmp;

  if (cdcm_alloc_blocks == cdcm_claimed_blocks)
    cdcm_mem_init();	/* allocate new portion */

  list_for_each_entry(tmp, &cdcmStatT.cdcm_mem_list_head, cm_list) {
    if (tmp->cmPtr == NULL) /* free one */
      return(tmp);
  }

  return(NULL);	/* nomally not reached */
}
