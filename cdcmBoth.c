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

int cdcm_pci_mmchain_lock(void *handle, struct cdcm_dmabuf *dma, int write,
			  int pid, void *buf, unsigned long size,
			  struct dmachain *out)
{
#ifdef __linux__
  /* FIXME: improve error handling: deallocate stuff when needed. */
  /* FIXME: No dynamic allocation here --> It should go inside the CDCM */
  /* FIXME: check DMA capabilities of the PCI device; barf if it can't
   *        DMA up to 4GB (i.e. 32bits).
   * FIXME: Barf if the user's pages are above 4GB (32bits addresses)
   * Another get-around for this would be to copy the 'above-4GB' user
   * buffer to kernel space, and DMA from there to the device. But
   * then the performance impact would be pretty serious. This is the
   * so-called bounce-buffers approach. See ivtv-udma.c in /drivers.
   * BUG: With big malloced buffers from user space (say, more than 8MB)
   * get_user_pages seems to go crazy -- it returns many pointers to the same
   * page!
   */

  int i;
  int err;
  unsigned int len;
  long first, last, data;
  struct cdcm_dev_info *cast = (struct cdcm_dev_info*)handle;
  struct scatterlist *sgentry;

  if (write) dma->direction = PCI_DMA_FROMDEVICE;
  else dma->direction = PCI_DMA_TODEVICE;

  data = (unsigned long)buf;
  first = ( data         & PAGE_MASK) >> PAGE_SHIFT;
  last  = ((data+size-1) & PAGE_MASK) >> PAGE_SHIFT;
  dma->offset = data & ~PAGE_MASK;
  dma->tail = 1 + ((data + size - 1) & ~PAGE_MASK);
  dma->nr_pages = last - first + 1;
  dma->pages = kmalloc(dma->nr_pages * sizeof(struct page *), GFP_KERNEL);
  if (NULL == dma->pages) return SYSERR;

  /* get_user_pages_fast would be cleaner here, but it's still fairly recent. */
  down_read(&current->mm->mmap_sem);
  err = get_user_pages(current, current->mm,
		       data & PAGE_MASK, dma->nr_pages,
		       write==1, 0, dma->pages, NULL);
  up_read(&current->mm->mmap_sem);

  if (err != dma->nr_pages) {
    printk("xmemDrvr: failed to map user pages, returned %d, expected %d.\n",
	     err, dma->nr_pages);
    printk("Check that the user buffer is page-aligned.");
    return -EINVAL;
  }

  dma->sglist = kcalloc(dma->nr_pages, sizeof(struct scatterlist), GFP_KERNEL);
  if (NULL == dma->sglist) return SYSERR;

  sg_init_table(dma->sglist, dma->nr_pages);

  /* DMA to HIGHMEM is risky */
  //  if (PageHighMem(dma->pages[0])) return SYSERR;
  len = PAGE_SIZE - dma->offset;
  if (dma->nr_pages == 1) len = dma->tail - dma->offset;
  sg_set_page(&dma->sglist[0], dma->pages[0], len, dma->offset);

  /* Now with the rest (we know they're aligned) */
  for (i=1; i < dma->nr_pages; i++) {
    if (NULL == dma->pages[i]) return SYSERR;
    //    if (PageHighMem(dma->pages[i])) return SYSERR;
    /* the last page probably won't be filled completely */
    if (i == dma->nr_pages - 1) len = dma->tail;
    else len = PAGE_SIZE;
    sg_set_page(&dma->sglist[i], dma->pages[i], len, 0);
  }

  dma->sglen = pci_map_sg(cast->di_pci, dma->sglist, dma->nr_pages,
			  dma->direction);
  if (dma->sglen <= 0) return SYSERR;

  sg_mark_end(&dma->sglist[dma->sglen-1]);

  /*
   * Read http://lwn.net/Articles/256368/ for the change below.
   */
  for_each_sg(dma->sglist, sgentry, dma->sglen, i) {
    out[i].address = sg_dma_address(sgentry);
    out[i].count = sg_dma_len(sgentry);
  }
  return dma->sglen;

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

int cdcm_pci_mem_unlock(void *handle, struct cdcm_dmabuf *dma, int pid,
			int dirty)
{
#ifdef __linux__
	struct cdcm_dev_info *cast = (struct cdcm_dev_info*)handle;

  if (dma->nr_pages && dma->sglist)
    pci_unmap_sg(cast->di_pci, dma->sglist, dma->nr_pages, dma->direction);
  __cdcm_clear_dma(dma, dirty);
  return OK;
#else

  return mem_unlock(pid, dma->user_buf, dma->size, dirty);

#endif
}

int __cdcm_clear_dma(struct cdcm_dmabuf *dma, int dirty) {
#ifdef __linux__
  if(dma->pages) {
    int i;

    for (i=0; i < dma->nr_pages; i++) {
      if (dirty && !PageReserved(dma->pages[i])) SetPageDirty(dma->pages[i]);
      page_cache_release(dma->pages[i]);
    }
  kfree(dma->pages);
  dma->pages = NULL;
  }

  if (dma->sglist) {
    kfree(dma->sglist);
    dma->sglist = NULL;
    dma->sglen = 0;
  }

  if (dma->vmalloc) {
    vfree(dma->vmalloc);
    dma->vmalloc = NULL;
  }

  if (dma->bus_addr) dma->bus_addr = 0;

  dma->direction = DMA_NONE;
  dma->offset = 0;
  dma->nr_pages = 0;

  return OK;
#else /* LynxOS */
  return OK;
#endif
}
