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
extern cdcmStatics_t cdcmStatT; /* CDCM statics table */

/**
 * @brief Release previously allocated memory
 *
 * @param pntr - mem pointer to release, returned by @e cdcm_mem_alloc.
 */
void cdcm_mem_free(void *addr)
{
	struct cdcm_mem_header *buf = addr;
	ssize_t realsize;

	buf--;
	realsize = buf->size + sizeof(*buf);

	if (B2KB(realsize) > MEM_BOUND)
		vfree(buf);
	else
		kfree(buf);
}

/**
 * @brief Allocates memory.
 *
 * @param size - size in @b bytes
 * @param flags - memory flags (see @e cdcmm_t definition
 *               for more info on the possible flags)
 *
 * @return -ENOMEM                  - if fails.
 * @return allocated memory pointer - if success.
 */
void *cdcm_mem_alloc(ssize_t size, int flags)
{
	struct cdcm_mem_header *buf;
	size_t realsize = size + sizeof(*buf);

	/*
	 * For large buffers we use vmalloc, because kmalloc allocates
	 * physically contiguous pages.
	 */
	if (B2KB(realsize > MEM_BOUND))
		buf = vmalloc(realsize);
	else
		buf = kmalloc(realsize, GFP_KERNEL);

	if (buf == NULL) {
		PRNT_ERR(cdcmStatT.cdcm_ipl, "Can't alloc 0x%x bytes", realsize);
		return ERR_PTR(-ENOMEM);
	}
	buf->size = size;
	buf->flags = flags;
	return ++buf;
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
	return (char *)get_zeroed_page(GFP_KERNEL | GFP_DMA);
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
