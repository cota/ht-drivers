/**
 * \file libvmebus.h
 * \brief VME Bus access user library interface
 * \author Sébastien Dugué
 * \date 04/02/2009
 *
 * This library gives userspace applications access to the VME bus
 *
 * Copyright (c) 2009 \em Sébastien \em Dugué
 *
 * \par License:
 *      This program is free software; you can redistribute  it and/or
 *      modify it under  the terms of  the GNU General  Public License as
 *      published by the Free Software Foundation;  either version 2 of the
 *      License, or (at your option) any later version.
 *
 */

#ifndef _LIBVMEBUS_H_INCLUDE_
#define _LIBVMEBUS_H_INCLUDE_

#include <vmebus.h>

/**
 * \brief Swap a 16-bit halfword bytewise
 * \param val The 16-bit halfword to swap
 *
 * \return The swapped value
 *
 */
static inline unsigned short swapbe16(unsigned short val)
{
	return (((val & 0xff00) >> 8) | ((val & 0xff) << 8));
}

/**
 * \brief Swap a 32-bit word bytewise
 * \param val The 32-bit word to swap
 *
 * \return The swapped value
 *
 */
static inline unsigned int swapbe32(unsigned int val)
{
	return (((val & 0xff000000) >> 24) | ((val & 0xff0000) >> 8) |
		((val & 0xff00) << 8) | ((val & 0xff) << 24));
}


extern int vme_bus_error_check(struct vme_mapping *desc);
extern int vme_bus_error_check_clear(struct vme_mapping *desc, __u64 address);

/* VME address space mapping - CES library emulation */
extern unsigned long find_controller(unsigned long vmeaddr, unsigned long len,
				     unsigned long am, unsigned long offset,
				     unsigned long size,
				     struct pdparam_master *param);
extern unsigned long return_controller(struct vme_mapping *desc);

/* VME address space mapping */
extern void *vme_map(struct vme_mapping *, int);
extern int vme_unmap(struct vme_mapping *, int);

/* DMA access */
extern int vme_dma_read(struct vme_dma *);
extern int vme_dma_write(struct vme_dma *);

#endif	/* _LIBVMEBUS_H_INCLUDE_ */
