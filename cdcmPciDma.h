/**
 * @file cdcmPciDma.h
 *
 * @brief CDCM DMA-PCI capabilities
 *
 * This header file is common for Linux and LynxOS; the makefile
 * should compile only the corresponding 'c' source file.
 *
 * @author Copyright (C) 2009 CERN CO/HT Emilio G. Cota
 * 					<emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _CDCM_DMA_PCI_H_INCLUDE_
#define _CDCM_DMA_PCI_H_INCLUDE_


#ifdef __linux__

#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/scatterlist.h>

#define cdcm_dma_t dma_addr_t

/** @brief Assimilates in a portable way LynxOS' struct dmachain.
 *
 * @ref address is the (portable) address to be used,
 * and @ref count is the size of the corresponding chunk at @ref address
 */
struct dmachain {
  cdcm_dma_t address;
  unsigned short count;
};

/** @brief stores scatter-gather mappings information
 *
 * Note that more fields could be added--as it is now it covers
 * DMA from user-space to/from a PCI device
 */
struct cdcm_dmabuf {
  /* for userland buffer */
  int			offset;  /* within the first page of the buffer */
  int			tail;	 /* within the last page of the buffer */
  struct page		**pages; /* array of physical pages */
  unsigned long		uaddrl;  /* address of the user's buffer */

  /* common */
  struct scatterlist	*sglist;   /* scatter-gather list */
  int			sglen;	   /* number of elems in sglist */
  int			nr_pages;  /* phys pages of the user's buffer */
  int			direction; /* direction flag of the PCI_DMA transfer */
};

/**
 * @brief check if a DMA mapping was successful
 *
 * @param pdev - PCI device
 * @param handle - DMA operation handle
 *
 * @return 0 if the DMA mapping was successful
 * @return !0 otherwise
 */
static inline int cdcm_pci_dma_maperror(struct pci_dev *pdev, cdcm_dma_t handle)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
  return pci_dma_mapping_error(pdev, handle);
#else
  return pci_dma_mapping_error(handle);
#endif /* kernel 2.6.27, see commit 8d8bb39b9eba32dd70e87fd5ad5c5dd4ba118e06 */
}


#else  /* __Lynx__ */

#include <sys/types.h>
#include <mem.h>
#define cdcm_dma_t unsigned char* /* Valid for Power PC */

struct cdcm_dmabuf {
  char *user_buf;
  unsigned long size;
};

#endif /* !__linux__ */



/*
 * Common CDCM DMA PCI functions
 */

/**
 * @brief maps a virtual memory region for PCI DMA
 *
 * @param handle - CDCM device
 * @param addr - virtual address
 * @param size - size of the region
 * @param write - 1 --> PCI DMA to memory; 0 otherwise.
 *
 * Note that this function may fail
 *
 * @return address to dma to/from - on success
 * @return 0 - on failure
 */
cdcm_dma_t cdcm_pci_map(void *handle, void *addr, size_t size, int write);

/**
 * @brief return to the CPU the PCI-DMA memory block previously granted
 * to the owner of the handle.
 *
 * @param handle: CDCM device
 * @param dma_addr: address of the dma mapping to be unmapped
 * @param size: size of the DMA buffer
 * @param write - 1 --> PCI DMA to memory; 0 otherwise.
 *
 * To be called from the interrupt handler
 */
void cdcm_pci_unmap(void *handle, cdcm_dma_t dma_addr, int size, int write);

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
			  struct dmachain *out);

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
			int dirty);

#endif /* _CDCM_DMA_PCI_H_INCLUDE_ */
