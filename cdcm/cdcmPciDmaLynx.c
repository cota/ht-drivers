/**
 * @file cdcmPciDmaLynx.c
 *
 * @brief CDCM PCI DMA implementation for LynxOS
 *
 * @author Copyright (C) 2009 CERN CO/HT Emilio G. Cota
 * 					<emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifdef __Lynx__

#include "cdcmPciDma.h"
#include <kernel.h>

cdcm_dma_t cdcm_pci_map(void *handle, void *addr, size_t size, int write)
{
  unsigned long ptr;
  ptr = (unsigned long)addr;
  if (ptr < PHYSBASE)
    ptr = (unsigned long)get_phys((kaddr_t)addr);
  return (cdcm_dma_t)(ptr - PHYSBASE);
}

void cdcm_pci_unmap(void *handle, cdcm_dma_t dma_addr, int size, int write)
{
  return;
}

int cdcm_pci_mmchain_lock(void *handle, struct cdcm_dmabuf *dma, int write,
			  int pid, void *buf, unsigned long size,
			  struct dmachain *out)
{
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
}

int cdcm_pci_mem_unlock(void *handle, struct cdcm_dmabuf *dma, int pid,
			int dirty)
{
  return mem_unlock(pid, dma->user_buf, dma->size, dirty);
}

#endif	/* __Lynx__ */
