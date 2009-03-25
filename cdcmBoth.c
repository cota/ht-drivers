/**
 * @file cdcmBoth.c
 *
 * @brief Contains functions that should be used both, by Linux and LynxOS.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 21/07/2006
 *
 * This module should be compiled both with Linux and Lynx drivers.
 * Depending on the kernel - different actions will be taken.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version
 */
#include "cdcmBoth.h"

#ifdef __linux__
#include <asm/uaccess.h>
#include "cdcmDrvr.h"
#else /* __Lynx__ */
#include  <kernel.h>
#endif


/**
 * @brief Should be used to pass user data into kernel address space.
 *
 * @param to   - destination address, in kernel space
 * @param from - source address, in user space
 * @param size - number of bytes to copy.
 *
 * @return Number of bytes that could not be copied. On success,
 *         this will be zero.
 * @return SYSERR (-1) - in case of any error.
 */
int cdcm_copy_from_user(void *to, void *from, int size)
{
#ifdef __linux__

  if (copy_from_user(to, from, size))
    return(SYSERR); /* -EFAULT on Linux */


#else  /* __Lynx__ */


  /* Even though Lynx can access user space memory directly, we do the memcpy
   * because assigning to = from directly would make the code much more
   * complicated -- since it would copy under Linux but not under LynxOS.
   */

  /* secure checking */
  if (rbounds((unsigned long) from) < size)
    return(SYSERR); /* -1 */

  memcpy(to, from, size);

#endif
  return(0);
}


/**
 * @brief Should be used to pass data from kernel to user address space.
 *
 * @param to   - destination address, in user space
 * @param from - source address, in kernel space
 * @param size - number of bytes to copy
 *
 * @return Returns number of bytes that could not be copied.
 *         On success, this will be zero.
 * @return SYSERR (-1) - in case of any error.
 */
int cdcm_copy_to_user(void *to, void *from, int size)
{
#ifdef __linux__

  if (copy_to_user(to, from, size))
    return(SYSERR); /* -EFAULT on Linux */

#else  /* __Lynx__ */

  /* secure checking */
  if (wbounds((unsigned long) to) < size)
    return(SYSERR); /* -1 */

  memcpy(to, from, size);

#endif
  return(0);
}


#if 0
/**
 * @brief Helps to initialize user-address space pointers.
 *
 * @param usr_addr - address from user space
 * @param size     - size to validate
 * @param action   - DIR_READ, DIR_WRITE, or both
 *
 * Like that it's possible to perform direct pointer dereferencing afterwards.
 *
 * @return
 */
char* cdcm_init_usr_ptr(unsigned int usr_addr, int size, int action)
{

#ifdef __linux__

#else  /* __Lynx__ */

  /* secure checkings first */
  if (action & DIR_READ)
    if (rbounds((unsigned long) usr_addr) < size)
      return(SYSERR); /* -1 */
  if (action & DIR_WRITE)
    if (wbounds((unsigned long) usr_addr) < size)
      return(SYSERR); /* -1 */

#endif /* !__linux__ */
}
#endif

/**
 * @brief maps a virtual memory region for PCI DMA
 *
 * @param handle CDCM device
 * @param addr virtual address
 * @param size size of the region
 * @param write 1 --> PCI DMA to memory; 0 otherwise.
 *
 * Note that this function may fail
 *
 * @return address to dma to/from - on success
 * @return 0 - on failure
 */
cdcm_dma_t cdcm_pci_map(void *handle, void *addr, size_t size, int write)
{
#ifdef __linux__
  int direction;
  cdcm_dma_t dma_handle;
  struct cdcm_dev_info *cast = (struct cdcm_dev_info*)handle;

  if (write) direction = PCI_DMA_FROMDEVICE;
  else direction = PCI_DMA_TODEVICE;

  dma_handle = pci_map_single(cast->di_pci, (void *)addr, size, direction);
  if (cdcm_pci_dma_maperror(cast->di_pci, dma_handle)) {
    PRNT_ABS_ERR("PCI DMA mapping failed");
    return 0;
  }
  return dma_handle;

#else
  unsigned long ptr;
  ptr = (unsigned long)addr;
  if (ptr < PHYSBASE)
    ptr = (unsigned long)get_phys((kaddr_t)addr);
  return (cdcm_dma_t)(ptr - PHYSBASE);

#endif
}


/**
 * cdcm_pci_unmap_dma - return to the CPU the PCI-DMA memory block
 * previously granted to the owner of the handle.
 *
 * @param handle: CDCM device
 * @param dma_addr: address of the dma mapping to be unmapped
 * @param size: size of the DMA buffer
 * @param direction: PCI-DMA flags
 *
 * To be called from the interrupt handler.
 *
 */
void cdcm_pci_unmap(void *handle, cdcm_dma_t dma_addr,
			int size, int write)
{
#ifdef __linux__
  int direction;
  struct cdcm_dev_info *cast = (struct cdcm_dev_info*)handle;

  if (write) direction = PCI_DMA_FROMDEVICE;
  else direction = PCI_DMA_TODEVICE;

  return pci_unmap_single(cast->di_pci, dma_addr, size, direction);
#else  /* __Lynx__ */
  return ;
#endif
}

#ifdef __linux__
/**
 * @brief initialise a dma buffer
 *
 * @param dma - CDCM DMA buffer
 * @param user - user's buffer's address to be mapped
 *
 * Initialise the following values of @ref dma: uaddrl, offset, tail,
 * nr_pages, direction.
 */
static void cdcm_init_dmabuf(struct cdcm_dmabuf *dma, void *userbuf,
			     unsigned long size, int write)
{
  unsigned long first, last;

  dma->uaddrl = (unsigned long)userbuf;
  first = (dma->uaddrl & PAGE_MASK) >> PAGE_SHIFT;
  last  = ((dma->uaddrl + size - 1) & PAGE_MASK) >> PAGE_SHIFT;

  dma->offset = dma->uaddrl & ~PAGE_MASK;
  dma->tail = 1 + ((dma->uaddrl + size - 1) & ~PAGE_MASK);
  dma->nr_pages = last - first + 1;

  if (write)
    dma->direction = PCI_DMA_FROMDEVICE;
  else
    dma->direction = PCI_DMA_TODEVICE;
}

/**
 * @brief find user physical pages and map them into a sg list
 *
 * @param - CDCM DMA buffer
 *
 * Note that the sglist might have less entries than nr_pages, since
 * contiguous physical pages are put into the same sg entry.
 *
 * @return @ref dma->nr_pages - on success
 * @return <= 0 - on failure
 */
static int cdcm_map_user_pages(struct cdcm_dmabuf *dma)
{
  int i, write, retval, len;

  dma->pages = kmalloc(dma->nr_pages * sizeof(struct page *), GFP_KERNEL);
  if (dma->pages == NULL)
    return -ENOMEM;

  if (dma->direction == PCI_DMA_FROMDEVICE)
    write = 1;
  else
    write = 0;

  down_read(&current->mm->mmap_sem);
  retval = get_user_pages(current, current->mm, dma->uaddrl & PAGE_MASK,
		      dma->nr_pages, dma->direction == PCI_DMA_FROMDEVICE,
		      0, dma->pages, NULL);
  up_read(&current->mm->mmap_sem);

  if (retval < 0)
    goto out_free;

  if (retval < dma->nr_pages) {
    for (i = 0; i < retval; i++)
      page_cache_release(dma->pages[i]);
    retval = -ENOMEM;
    goto out_free;
  }

  sg_init_table(dma->sglist, dma->nr_pages);

  /* @todo FIXME: support DMA to HIGHMEM */
  len = PAGE_SIZE - dma->offset;

  if (dma->nr_pages == 1)
    len = dma->tail - dma->offset;

  /* pin the first page */
  sg_set_page(&dma->sglist[0], dma->pages[0], len, dma->offset);

  /* Now with the rest (we know they're aligned) */
  for (i = 1; i < dma->nr_pages; i++) {
    if (dma->pages[i] == NULL) {
      PRNT_ABS_ERR("dma->pages[%d] pointing to NULL", i);
      goto out_free;
    }
    /* the last page probably won't be filled completely */
    if (i == dma->nr_pages - 1)
      len = dma->tail;
    else
      len = PAGE_SIZE;

    sg_set_page(&dma->sglist[i], dma->pages[i], len, 0);
  }

 out_free:
    kfree(dma->pages);
    return retval;
}
#endif /* linux */

/**
 * @brief map a user's buffer for DMA
 *
 * @param handle - CDCM handle
 * @param dma - DMA buffer handle; needed as a reference to the mapping
 * @param write - 1 --> PCI DMA to memory; 0 otherwise
 * @param pid - process ID of the caller
 * @param buf - user's buffer
 * @param size - size of the user's buffer
 * @param out - array of dma addresses to be filled
 *
 * The physical pages of @ref buf are put in @ref out; note that these pages
 * are mapped for dma and locked in memory.
 * Note also that due to memory overcommit, this function might be fooled
 * by the kernel, retrieving the same physical pages all over again. To avoid
 * this, user-space code could either:
 * - write to the buffer beforehand (eg. zeroeing it)
 * or
 * - mlock() the buffer
 *
 * @return number of items in the @ref out array - on success
 * @return SYSERR - on failure
 */
int cdcm_pci_mmchain_lock(void *handle, struct cdcm_dmabuf *dma, int write,
			  int pid, void *buf, unsigned long size,
			  struct dmachain *out)
{
#ifdef __linux__
  /* FIXME: check DMA capabilities of the PCI device; barf if it can't
   *        DMA up to 4GB (i.e. 32bits).
   * FIXME: Barf if the user's pages are above 4GB (32bits addresses)
   * Another get-around for this would be to copy the 'above-4GB' user
   * buffer to kernel space, and DMA from there to the device. But
   * then the performance impact would be pretty serious. This is the
   * so-called bounce-buffers approach. See ivtv-udma.c in /drivers.
   */
  struct cdcm_dev_info *cast = (struct cdcm_dev_info*)handle;
  struct scatterlist *sgentry;
  int i, rc;

  cdcm_init_dmabuf(dma, buf, size, write);

  dma->sglist = kcalloc(dma->nr_pages, sizeof(struct scatterlist), GFP_KERNEL);
  if (NULL == dma->sglist) {
    PRNT_ABS_ERR("Cannot allocate space for dma->sglist");
    goto out_err;
  }

  rc = cdcm_map_user_pages(dma);

  if (rc != dma->nr_pages) {
    PRNT_ABS_ERR("failed to map user pages, returned %d, expected %d",
	     rc, dma->nr_pages);
    goto out_free_sglist;
  }

  dma->sglen = pci_map_sg(cast->di_pci, dma->sglist, dma->nr_pages,
			  dma->direction);
  if (dma->sglen <= 0) {
    PRNT_ABS_ERR("pci_map_sg failed");
    goto out_free_sglist;
  }

  sg_mark_end(&dma->sglist[dma->sglen-1]);

  for_each_sg(dma->sglist, sgentry, dma->sglen, i) {
    out[i].address = sg_dma_address(sgentry);
    out[i].count = sg_dma_len(sgentry);
  }

  return dma->sglen;

 out_free_sglist:
  kfree(dma->sglist);
 out_err:
  return SYSERR;

#else

  int err;
  int i;
  dma->user_buf = (char *)buf;
  dma->size = size;
  err = mem_lock(pid, dma->user_buf, dma->size);
  if (err < 0) return SYSERR;
  err = mmchain(out, dma->user_buf, dma->size);
  for (i=0; i < err; i++) {
    /* get_phys here would be redundant */
    out[i].address -= PHYSBASE;
  }
  return err;

#endif
}

/**
 * @brief clean and unlock the dma mappings
 *
 * @param handle - CDCM device handle
 * @param dma - CDCM DMA handle
 * @param pid - process pid
 * @param dirty - 'mark pages as dirty' flag
 *
 * @return OK - on success
 * @return SYSERR - on failure
 */
int cdcm_pci_mem_unlock(void *handle, struct cdcm_dmabuf *dma, int pid,
			int dirty)
{
#ifdef __linux__
  struct cdcm_dev_info *cast = (struct cdcm_dev_info*)handle;
  int i;

  pci_unmap_sg(cast->di_pci, dma->sglist, dma->sglen, dma->direction);

  for (i = 0; i < dma->sglen; i++) {
    struct page *page = sg_page(&dma->sglist[i]);

    if (dirty && !PageReserved(page))
      SetPageDirty(page);
    page_cache_release(page);
  }

  kfree(dma->sglist);

  return OK;
#else

  return mem_unlock(pid, dma->user_buf, dma->size, dirty);

#endif
}
