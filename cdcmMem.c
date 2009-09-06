/**
 * @file cdcmMem.c
 *
 * @brief All @b CDCM memory handling is located here.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 17/02/2007
 *
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version
 */
#include "general_drvr.h"	/* various defs */
#include "cdcmDrvr.h"
#include "cdcmMem.h"

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern int cdcm_err;		/* global error */
extern cdcmStatics_t cdcmStatT; /* CDCM statics table */

static int cdcm_alloc_blocks   = 0; /* amount of allocated memory blocks */
static int cdcm_claimed_blocks = 0; /* how many already used */

static cdcmm_t* cdcm_mem_get_free_block(void);


/**
 * @brief CDCM memory handler initialization.
 *
 * @param void
 *
 * @return 0       - we cool
 * @return -ENOMEM - we are not.
 */
int cdcm_mem_init(void)
{
	int cntr;
	cdcmm_t *amem; /* allocated memory */

	for (cntr = 0; cntr < CDCM_ALLOC_DEF; cntr++) {
		if ( !(amem = kzalloc(sizeof(*amem), GFP_KERNEL)) ) {
			PRNT_ABS_ERR("Can't alloc CDCM memory handle");
			return -ENOMEM;
		}
		list_add(&amem->cm_list, &cdcmStatT.cdcm_mem_list_head);
		++cdcm_alloc_blocks;
	}

	return 0;
}


/**
 * @brief Release previously claimed block and free allocated memory.
 *
 * @param pntr - mem pointer to release, returned by @e cdcm_mem_alloc.
 *
 * @return 0 - always success.
 */
int cdcm_mem_free(char *pntr)
{
	cdcmm_t *mm = NULL;

	if (!pntr || !(mm = cdcm_mem_find_block(pntr)))
		return 0;
	/*
	PRNT_DBG(cdcmStatT.cdcm_ipl, "Dealloc mem @0x%x, Size %u bytes,"
	"Flgs 0x%x\n", (unsigned int)pntr->cmPtr, (unsigned int)pntr->cmSz,
	(unsigned int)pntr->cmFlg);
	*/

	if (B2KB(mm->cmSz) > MEM_BOUND)
		vfree(mm->cmPtr);
	else
		kfree(mm->cmPtr);

	/* reset */
#if 0
	/*
	  NOTE - Initialization like this is zeroing out _all_ the rest
	  in the cdcmm_t to which mm points to! I.e. it is _not_ correct!!!
	*/
	*mm = (cdcmm_t) {
		.cmPtr = NULL,
		.cmSz  = 0,
		.cmFlg = 0
	};
#endif

	mm->cmPtr = NULL;
	mm->cmSz  = 0;
	mm->cmFlg = 0;

	--cdcm_claimed_blocks;

	return 0;
}


/**
 * @brief Cleanup all memory blocks and their allocated memory.
 *
 * @param void
 *
 * @return  0 - if success
 * @return -1 - if fails
 */
int cdcm_mem_cleanup_all(void)
{
	cdcmm_t *memP, *tmpP;

	/* free allocated memory */
	list_for_each_entry_safe(memP, tmpP, &cdcmStatT.cdcm_mem_list_head,
				 cm_list) {
		if (memP->cmPtr) {
			PRNT_DBG(cdcmStatT.cdcm_ipl, "Releasing allocated"
				 "memory chunk %p\n", memP);
			cdcm_mem_free(memP->cmPtr);
		}

		list_del(&memP->cm_list);
		--cdcm_alloc_blocks;
		kfree(memP);
	}

	return 0;
}


/**
 * @brief Allocates memory.
 *
 * @param size - size in @b bytes
 * @param flgs - memory flags (see @e cdcmm_t definition
 *               for more info on the possible flags)
 *
 * Depending on the request memory size - different allocation methods are
 * used. If size is less then 128Kb - kmalloc, vmalloc otherwise.
 * 128kb - for code is to be completely portable (ldd3).
 * Moreover by default memory is considered to be a short term
 * usage (i.e. CDCM_M_SHORT_TERM) and will not be included in
 * the linked list of allocated memory chunks!
 *
 * @return -ENOMEM                  - if fails.
 * @return allocated memory pointer - if success.
 */
char* cdcm_mem_alloc(size_t size, int flgs)
{
	cdcmm_t *amem = cdcm_mem_get_free_block();

	if (!amem) {
		PRNT_ABS_ERR("Can't allocate CDCM memory handle");
		return(ERR_PTR(-ENOMEM));
	}

	if ( !(amem->cmPtr = (B2KB(size) > MEM_BOUND) ?
	       vmalloc(size) :
	       kmalloc(size, GFP_KERNEL)) ) {
		PRNT_ERR(cdcmStatT.cdcm_ipl,
			 "Can't alloc mem size %u bytes long", size);
		return(ERR_PTR(-ENOMEM));
	}

	++cdcm_claimed_blocks;
	amem->cmSz  = size;
	amem->cmFlg = flgs;

	/*
	  PRNT_DBG(cdcmStatT.cdcm_ipl, "Alloc mem @0x%x, Size %u bytes,"
	  "Flgs 0x%x\n", (unsigned int)amem->cmPtr, (unsigned int)size,
	  (unsigned int)flgs);
	*/

	return amem->cmPtr;
}


/**
 * @brief Search CDCM memory descriptor using memory pointer.
 *
 * @param mptr - allocated memory pointer
 *
 * @return pointer to found entry - if success.
 * @return NULL                   - if fails.
 */
cdcmm_t* cdcm_mem_find_block(char *mptr)
{
	cdcmm_t *tmp;

	list_for_each_entry(tmp, &cdcmStatT.cdcm_mem_list_head, cm_list) {
		if (tmp->cmPtr == mptr)
			return tmp;
	}

	return NULL;	/* not found */
}


/**
 * @brief Search for free CDCM memory block.
 *
 * @param none
 *
 * @return pointer to free block - if success.
 * @return NULL                  - if fails.
 */
static cdcmm_t* cdcm_mem_get_free_block(void)
{
	cdcmm_t *tmp;

	if (cdcm_alloc_blocks == cdcm_claimed_blocks)
		cdcm_mem_init();	/* allocate new portion */

	list_for_each_entry(tmp, &cdcmStatT.cdcm_mem_list_head, cm_list) {
		if (tmp->cmPtr == NULL) /* free one */
			return tmp;
	}

	return NULL;	/* nomally not reached */
}


/**
 * get_phys - get physical address of a virtual address !TODO!
 *
 * @param vaddr: virtual address
 *
 * returns the physical address onto which the given virtual address is mapped.
 * Some hardware devices may require a corresponding physical address
 * if they, for example, access physical memory directly (DMA) without the
 * benefit of the MMU. The value returned by get_phys minus PHYSBASE may be used
 * for such purposes.
 *
 * @return
 */
char *get_phys(long vaddr)
{
	return 0;
}


/**
 * mem_lock - guarantees address range is in real memory
 *
 * @param tjob: process ID that wants to lock memory
 * @param start:virtual start address, in bytes
 * @param size: size of the region of memory to lock, in bytes
 *
 * mem_lock guarantees that a region of virtual memory is locked into physical
 * memory. mem_lock returns OK if it can lock all the requested memory
 * otherwise it will return SYSERR.
 *
 * @return
 */
int mem_lock(int tjob, char *start, unsigned long size)
{
	return OK;
}


/**
 * mem_unlock - frees real memory for swapping !TODO!
 *
 * @param tjob: process ID that wants to lock memory
 * @param start: is the virtual start address, in bytes
 * @param size: is the size of the region of memory to lock, in bytes
 * @param dirty: if dirty is non 0 then the entire section of memory from
 * start to size will be marked as dirty. Dirty pages will be written to disk
 * by the VM mechanism in the kernel before they are released.
 *
 * mem_unlock unlocks the previously locked region of virtual memory which
 * allows the memory to be swapped. mem_unlock returns OK if the region
 * has been successfully unlocked otherwise it will return SYSERR
 *
 * @return
 */


/**
 * get1page - get one page of physical memory
 *
 * allocates one page of physical memory and returns a virtual address
 * which can be used to access it.
 *
 * @return
 */
char *get1page()
{
	return (char *)get_zeroed_page(GFP_KERNEL);
}

/**
 * @brief free a page allocated with get1page()
 *
 * @param addr - address of the page
 */
void free1page(void *addr)
{
	return free_page((unsigned long)addr);
}
