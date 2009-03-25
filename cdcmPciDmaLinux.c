/**
 * @file cdcmPciDmaLinux.c
 *
 * @brief CDCM PCI DMA implementation for Linux
 *
 * @author Copyright (C) 2009 CERN CO/HT Emilio G. Cota
 * 					<emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include "cdcmDrvr.h"
#include "cdcmPciDma.h"

cdcm_dma_t cdcm_pci_map(void *handle, void *addr, size_t size, int write)
{
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
}

void cdcm_pci_unmap(void *handle, cdcm_dma_t dma_addr,
			int size, int write)
{
  int direction;
  struct cdcm_dev_info *cast = (struct cdcm_dev_info*)handle;

  if (write) direction = PCI_DMA_FROMDEVICE;
  else direction = PCI_DMA_TODEVICE;

  return pci_unmap_single(cast->di_pci, dma_addr, size, direction);
}

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

int cdcm_pci_mmchain_lock(void *handle, struct cdcm_dmabuf *dma, int write,
			  int pid, void *buf, unsigned long size,
			  struct dmachain *out)
{
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
}

int cdcm_pci_mem_unlock(void *handle, struct cdcm_dmabuf *dma, int pid,
			int dirty)
{
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
}
