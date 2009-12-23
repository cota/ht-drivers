/*
 * vme_cesif - Emulation for the LynxOS CES driver
 *
 * Copyright (c) 2009 Sebastien Dugue
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

/*
 *  This file provides emulation for the find_controller(), return_controller(),
 * vme_intset() and vme_intclr() interfaces of the LynxOS CES driver.
 */

#define DEBUG
#include <linux/device.h>

#include "vmebus.h"
#include "vme_bridge.h"

/**
 * find_controller() - Maps a VME address space into the PCI address space
 * @vmeaddr: VME physical start address of the mapping
 * @len: Window size (must be a multiple of 64k)
 * @am: VME address modifier
 * @offset: Offset in the mapping for read access test (Not used)
 * @size: Data width
 * @param: VME mapping parameters
 *
 *  This function is an emulation of the CES driver functionality on LynxOS.
 *
 *  The CES function interface does not give all the needed VME parameters, so
 * the following choices were made and may have to be tweaked.
 *
 *  - if read prefetch is enabled the the prefetch size is set to 2 cache lines
 *  - the VME address and size are automatically aligned on 64k if needed
 *  - the VME address is limited to 32 bits
 *
 *  The kernel allocated mapping descriptor address (cookie) is stored in
 * the pdparam_master sgmin field for use by return_controller().
 *
 * @return virtual address of the mapping - if success.
 * @return -1                             - on failure.
 */

unsigned long find_controller(unsigned long vmeaddr, unsigned long len,
			      unsigned long am, unsigned long offset,
			      unsigned long size, struct pdparam_master *param)
{
	struct vme_mapping *desc;
	int rc = 0;

	/* Allocate our mapping descriptor */
	if ((desc = kzalloc(sizeof(struct vme_mapping), GFP_KERNEL)) == NULL) {
		printk(KERN_ERR PFX "%s - "
		       "Failed to allocate mapping descriptor\n", __func__);
		return -1;
	}

	/* Now fill it with the parameters we got */
	if (param->rdpref) {
		desc->read_prefetch_enabled = 1;
		desc->read_prefetch_size = VME_PREFETCH_2;
	}

	switch (size) {
	case 2:
		desc->data_width = VME_D16;
		break;
	case 4:
		desc->data_width = VME_D32;
		break;
	default:
		printk(KERN_ERR PFX "%s - Unsupported data width %ld\n",
		       __func__, size);
		rc = -1;
		goto out_free;
		break;
	}

	desc->am = am;
	desc->bcast_select = 0;

	/*
	 * Note: no rounding up/down for size and address at this point,
	 * since that's taken care of when creating the window (if any).
	 */
	desc->sizel = len;
	desc->sizeu = 0;

	desc->vme_addrl = vmeaddr;
	desc->vme_addru = 0;

	/*
	 * Now we're all set up, just call the mapping function. We force
	 * window creation if no existing mapping can be found.
	 */
	if ((rc = vme_find_mapping(desc, 1))) {
		printk(KERN_ERR PFX "%s - "
		       "Failed (rc is %d) to find a mapping "
		       "for VME addr: %.8lx Size:%8lx AM: %.2lx\n",
		       __func__, rc, vmeaddr, len, am);
		rc = -1;
		goto out_free;
	}

	printk(KERN_DEBUG PFX "%s - "
	       "Mapping found, VME addr: %.8lx "
	       "Size:%.8lx AM: %.2lx mapped at %p\n",
	       __func__, vmeaddr, len, am, desc->kernel_va);

	return (unsigned long)desc->kernel_va;

out_free:
	kfree(desc);

	return rc;
}
EXPORT_SYMBOL_GPL(find_controller);

/**
 * @brief Release a VME mapping
 *
 * @param logaddr - CPU logical address returned by @ref find_controller()
 * @param len     - size of the mapped window in bytes.
 *
 *  This function is an emulation of the CES driver functionality on LynxOS.
 *
 * @return  0 - on success.
 * @return -1 - if fails.
 */
unsigned long return_controller(unsigned logaddr, unsigned len)
{
	struct vme_mapping *desc = find_vme_mapping_from_addr(logaddr);
	int err;

	if (!desc) {
		printk(KERN_ERR PFX "%s - mapping not found @ 0x%x",
			__func__, logaddr);
		return -1;
	}

	err = vme_release_mapping(desc, 1);
	if (!err)
		return 0;

	printk(KERN_ERR PFX "%s - failed to remove mapping @ 0x%x (err=%d)\n",
		__func__, logaddr, err);
	return -1;
}
EXPORT_SYMBOL_GPL(return_controller);

/**
 * vme_intset() - Install an interrupt handler for the given vector
 * @vec: Interrupt vector
 * @handler: Handler function
 * @arg: Handler argument
 * @sav: Unused
 *
 *  This function is an emulation of the CES driver functionality on LynxOS.
 *
 * Returns 0 on success, or a standard kernel error
 */
int vme_intset(int vec, int (*handler)(void *), void *arg, void *sav)
{
	return vme_request_irq(vec, handler, arg, NULL);
}
EXPORT_SYMBOL_GPL(vme_intset);

/**
 * vme_intclr() - Uninstall the interrupt handler for the given vector
 * @vec: Interrupt vector
 * @sav: Unused
 *
 *  This function is an emulation of the CES driver functionality on LynxOS.
 *
 * Returns 0 on success, or a standard kernel error
 */
int vme_intclr(int vec, void *sav)
{
	return vme_free_irq(vec);
}
EXPORT_SYMBOL_GPL(vme_intclr);
