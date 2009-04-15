/*
 * vme_dma.h -
 *
 * Copyright (c) 2009 Sebastien Dugue
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _VME_DMA_H
#define _VME_DMA_H

#include "vmebus.h"

/**
 * struct dma_channel - Internal data structure representing a DMA channel
 * @lock: Lock protecting the busy flag
 * @busy: Busy flag
 * @num: Channel number
 * @desc: DMA transfer descriptor currently used
 * @chained: Chained (1) / Direct (0) transfer
 * @sgl: Scatter gather list of userspace pages for the transfer
 * @sg_pages: Number of pages in the scatter gather list
 * @sg_mapped: Number of pages mapped onto the PCI bus
 * @hw_desc: List of hardware descriptors
 * @wait: Wait queue for the DMA channel
 *
 */
struct dma_channel {
	struct mutex		lock;
	unsigned int		busy;
	unsigned int		num;
	struct vme_dma		desc;
	int			chained;
	struct scatterlist	*sgl;
	int			sg_pages;
	int			sg_mapped;
	struct list_head	hw_desc_list;
	wait_queue_head_t	wait;
};

/**
 * struct hw_desc_entry - Hardware descriptor
 * @list: Descriptors list
 * @va: Virtual address of the descriptor
 * @phys: Bus address of the descriptor
 *
 *  This data structure is used internally to keep track of the hardware
 * descriptors that are allocated in order to free them when the transfer
 * is done.
 */
struct hw_desc_entry {
	struct list_head	list;
	void			*va;
	dma_addr_t		phys;
};


#endif /* _VME_DMA_H */
