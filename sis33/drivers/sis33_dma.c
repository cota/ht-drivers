/**
 * sis33_dma.c
 * Common DMA code for SIS33xx modules
 *
 * In most cases, direct I/O mapping of SIS devices isn't possible, mainly
 * as a consequence of each module taking 128MB of address space. Since
 * we are likely to talk to the VME bus across a PCI bridge, we would
 * rapidly exhaust all mapping windows on the bridge if we wanted to map
 * several modules. Note that mapping them together in a single 'huge' window
 * isn't usually an option, either.
 * So what we do is to set up a DMA transfer for each register read or write.
 * Yes, it's ugly and unsafe, but with this hack we can fill up a VME crate
 * with ADCs and use them.
 *
 * Copyright (c) 2009 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <vmebus.h>
#include "sis33core.h"

static int
__sis33_dma(struct device *dev, unsigned int vme_addr, enum vme_address_modifier am,
	    void *addr, ssize_t size, int to_user)
{
	struct vme_dma desc;
	struct vme_dma_attr *vme;
	struct vme_dma_attr *pci;
	int ret;

	memset(&desc, 0, sizeof(desc));

	vme = &desc.src;
	pci = &desc.dst;

	desc.dir = VME_DMA_FROM_DEVICE;
	desc.length = size;

	desc.ctrl.pci_block_size	= VME_DMA_BSIZE_2048;
	desc.ctrl.pci_backoff_time	= VME_DMA_BACKOFF_0;
	desc.ctrl.vme_block_size	= VME_DMA_BSIZE_2048;
	desc.ctrl.vme_backoff_time	= VME_DMA_BACKOFF_0;

	pci->addru = 0;
	pci->addrl = (unsigned int)addr;

	vme->addru = 0;
	vme->addrl = vme_addr;
	vme->am = am;
	vme->data_width = VME_D32;

	dev_dbg(dev, "DMA 0x%8x size: 0x%x\n", vme_addr, desc.length);
	if (to_user)
		ret = vme_do_dma(&desc);
	else
		ret = vme_do_dma_kernel(&desc);
	return ret;
}

/**
 * sis33_dma_read_mblt - read a block of data to a kernel bufferfrom VME using MBLT-DMA
 * @dev:	device to read from
 * @vme_addr:	VME address to start reading from
 * @addr:	address to write to
 * @size:	length of the block (in bytes) to be read
 *
 * returns 0 on success, negative error code on failure
 */
int sis33_dma_read_mblt(struct device *dev, unsigned int vme_addr, void __kernel *addr, ssize_t size)
{
	return __sis33_dma(dev, vme_addr, VME_A32_USER_MBLT, (void __force *)addr, size, 0);
}
EXPORT_SYMBOL_GPL(sis33_dma_read_mblt);

/**
 * sis33_dma_read_mblt_user - read a block of data to user-space from VME using MBLT-DMA
 * @dev:	device to read from
 * @vme_addr:	VME address to start reading from
 * @addr:	address to write to
 * @size:	length of the block (in bytes) to be read
 *
 * returns 0 on success, negative error code on failure
 */
int sis33_dma_read_mblt_user(struct device *dev, unsigned int vme_addr, void __user *addr, ssize_t size)
{
	return __sis33_dma(dev, vme_addr, VME_A32_USER_MBLT, (void __force *)addr, size, 1);
}
EXPORT_SYMBOL_GPL(sis33_dma_read_mblt_user);

/**
 * sis33_dma_read32be_blt - read a block of 32-be data to kernel space from VME using BLT-DMA
 * @dev:	device to read from
 * @vme_addr:	VME address to start reading from
 * @addr:	kernel address to write to
 * @elems:	number of elements (u32's) to be read
 *
 * returns 0 on success, negative error code on failure
 */
int sis33_dma_read32be_blt(struct device *dev, unsigned int vme_addr, u32 __kernel *addr, unsigned int elems)
{
	int ret;
	int i;

	ret = __sis33_dma(dev, vme_addr, VME_A32_USER_BLT, (void __force *)addr, elems * sizeof(u32), 0);
	if (ret)
		return ret;
	for (i = 0; i < elems; i++)
		addr[i] = be32_to_cpu(addr[i]);
	return 0;
}
EXPORT_SYMBOL_GPL(sis33_dma_read32be_blt);

/**
 * sis33_dma_read - read a double word from VME using DMA
 * @dev:	device to read from
 * @vme_addr:	VME address to read from
 *
 * returns the value read
 */
unsigned int sis33_dma_read(struct device *dev, unsigned int vme_addr)
{
	u32 val;
	struct vme_dma desc;
	struct vme_dma_attr *vme;
	struct vme_dma_attr *pci;

	memset(&desc, 0, sizeof(desc));

	vme = &desc.src;
	pci = &desc.dst;

	desc.dir = VME_DMA_FROM_DEVICE;
	desc.length = sizeof(u32);

	desc.ctrl.pci_block_size	= VME_DMA_BSIZE_64;
	desc.ctrl.pci_backoff_time	= VME_DMA_BACKOFF_0;
	desc.ctrl.vme_block_size	= VME_DMA_BSIZE_64;
	desc.ctrl.vme_backoff_time	= VME_DMA_BACKOFF_0;

	pci->addru = 0;
	pci->addrl = (unsigned int)&val;

	vme->addru = 0;
	vme->addrl = vme_addr;
	vme->am = VME_A32_USER_DATA_SCT;
	vme->data_width = VME_D32;

	/* @todo bail out gracefully here */
	if (vme_do_dma_kernel(&desc))
		dev_warn(dev, "vme_do_dma failed on address 0x%08x. Status: 0x%8x\n", vme_addr, desc.status);
	val = be32_to_cpu(val);
	dev_dbg(dev, "DMAr:\t0x%8x\tval: 0x%8x\n", vme_addr, val);

	return val;
}
EXPORT_SYMBOL_GPL(sis33_dma_read);

/**
 * sis33_dma_write - write a double word to VME using DMA
 * @dev:	device to write to
 * @vme_addr:	VME address to write to
 * @val:	value to be written
 */
void sis33_dma_write(struct device *dev, unsigned int vme_addr, unsigned int val)
{
	struct vme_dma desc;
	struct vme_dma_attr *vme;
	struct vme_dma_attr *pci;
	u32 val_be;

	val_be = cpu_to_be32(val);

	memset(&desc, 0, sizeof(desc));

	pci = &desc.src;
	vme = &desc.dst;

	desc.dir = VME_DMA_TO_DEVICE;
	desc.length = sizeof(u32);

	desc.ctrl.pci_block_size	= VME_DMA_BSIZE_64;
	desc.ctrl.pci_backoff_time	= VME_DMA_BACKOFF_0;
	desc.ctrl.vme_block_size	= VME_DMA_BSIZE_64;
	desc.ctrl.vme_backoff_time	= VME_DMA_BACKOFF_0;

	pci->addru = 0;
	pci->addrl = (unsigned int)&val_be;

	vme->addru = 0;
	vme->addrl = vme_addr;
	vme->am = VME_A32_USER_DATA_SCT;
	vme->data_width = VME_D32;

	dev_dbg(dev, "DMAw:\t\t0x%08x\tval: 0x%8x\n", vme_addr, val);

	/* @todo bail out gracefully here */
	if (vme_do_dma_kernel(&desc)) {
		dev_warn(dev, "vme_do_dma failed on address 0x%08x value 0x%08x. Status: 0x%8x\n",
			vme_addr, val, desc.status);
	}
	return;
}
EXPORT_SYMBOL_GPL(sis33_dma_write);
