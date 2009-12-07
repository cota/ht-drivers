/*
 * vme_dma.c - PCI-VME bridge DMA management
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
 *  This file provides the PCI-VME bridge DMA management code.
 */

#include <linux/pagemap.h>

#include "vmebus.h"
#include "vme_bridge.h"
#include "vme_dma.h"


struct dma_channel channels[TSI148_NUM_DMA_CHANNELS];

/*
 * Used for synchronizing between DMA transfer using a channel and
 * module exit
 */
wait_queue_head_t channel_wait[TSI148_NUM_DMA_CHANNELS];

void handle_dma_interrupt(int channel_mask)
{
	if (channel_mask & 1)
		wake_up(&channels[0].wait);

	if (channel_mask & 2)
		wake_up(&channels[1].wait);

	account_dma_interrupt(channel_mask);
}



/**
 * sgl_map_user_pages() - Pin user pages and put them into a scatter gather list
 * @sgl: Scatter gather list to fill
 * @nr_pages: Number of pages
 * @uaddr: User buffer address
 * @count: Length of user buffer
 * @rw: Direction (0=read from userspace / 1 = write to userspace)
 *
 *  This function pins the pages of the userspace buffer and fill in the
 * scatter gather list.
 */
static int sgl_map_user_pages(struct scatterlist *sgl,
			      const unsigned int nr_pages, unsigned long uaddr,
			      size_t length, int rw)
{
	int rc;
	int i;
	struct page **pages;

	if ((pages = kmalloc(nr_pages * sizeof(struct page *),
			     GFP_KERNEL)) == NULL)
		return -ENOMEM;

	/* Get user pages for the DMA transfer */
	down_read(&current->mm->mmap_sem);
	rc = get_user_pages(current, current->mm, uaddr, nr_pages, rw, 0,
			    pages, NULL);
	up_read(&current->mm->mmap_sem);

	if (rc < 0)
		/* We completely failed to get the pages */
		goto out_free;

	if (rc < nr_pages) {
		/* Some pages were pinned, release these */
		for (i = 0; i < rc; i++)
			page_cache_release(pages[i]);

		rc = -ENOMEM;
		goto out_free;
	}

	/* Populate the scatter/gather list */
	sg_init_table(sgl, nr_pages);

	/* Take a shortcut here when we only have a single page transfer */
	if (nr_pages > 1) {
		unsigned int off = uaddr & ~PAGE_MASK;
		unsigned int len = PAGE_SIZE - off;

		sg_set_page (&sgl[0], pages[0], len, off);
		length -= len;

		for (i = 1; i < nr_pages; i++) {
			sg_set_page (&sgl[i], pages[i],
				     (length < PAGE_SIZE) ? length : PAGE_SIZE,
				     0);
			length -= PAGE_SIZE;
		}
	} else
		sg_set_page (&sgl[0], pages[0], length, uaddr & ~PAGE_MASK);

out_free:
	/* We do not need the pages array anymore */
	kfree(pages);

	return nr_pages;
}

/**
 * sgl_unmap_user_pages() - Release the scatter gather list pages
 * @sgl: The scatter gather list
 * @nr_pages: Number of pages in the list
 * @dirty: Flag indicating whether the pages should be marked dirty
 *
 */
static void sgl_unmap_user_pages(struct scatterlist *sgl,
				 const unsigned int nr_pages, int dirty)
{
	int i;

	for (i = 0; i < nr_pages; i++) {
		struct page *page = sg_page(&sgl[i]);

		if (dirty && !PageReserved(page))
			SetPageDirty(page);

		page_cache_release (page);
	}
}

/**
 * vme_dma_setup() - Setup a DMA transfer
 * @desc: DMA channel to setup
 *
 *  Setup a DMA transfer.
 *
 *  Returns 0 on success, or a standard kernel error code on failure.
 */
static int vme_dma_setup(struct dma_channel *channel)
{
	int rc = 0;
	struct vme_dma *desc = &channel->desc;
	unsigned int length = desc->length;
	unsigned int uaddr;
	int nr_pages;

	/* Create the scatter gather list */
	uaddr = (desc->dir == VME_DMA_TO_DEVICE) ?
		desc->src.addrl : desc->dst.addrl;

	/* Check for overflow */
	if ((uaddr + length) < uaddr)
		return -EINVAL;

	nr_pages = ((uaddr & ~PAGE_MASK) + length + ~PAGE_MASK) >> PAGE_SHIFT;

	if ((channel->sgl = kmalloc(nr_pages * sizeof(struct scatterlist),
				    GFP_KERNEL)) == NULL)
		return -ENOMEM;

	/* Map the user pages into the scatter gather list */
	channel->sg_pages = sgl_map_user_pages(channel->sgl, nr_pages, uaddr,
					       length,
					       (desc->dir==VME_DMA_FROM_DEVICE));
	if (channel->sg_pages <= 0) {
		rc = channel->sg_pages;
		goto out_free_sgl;
	}

	/* Map the sg list entries onto the PCI bus */
	channel->sg_mapped = pci_map_sg(vme_bridge->pdev, channel->sgl,
					channel->sg_pages, desc->dir);

	rc = tsi148_dma_setup(channel);


	if (rc)
		goto out_unmap_sgl;

	return 0;

out_unmap_sgl:
	pci_unmap_sg(vme_bridge->pdev, channel->sgl, channel->sg_mapped,
		     desc->dir);

	sgl_unmap_user_pages(channel->sgl, channel->sg_pages, 0);

out_free_sgl:
	kfree(channel->sgl);

	return rc;
}

/**
 * vme_dma_start() - Start a DMA transfer
 * @channel: DMA channel to start
 *
 */
static void vme_dma_start(struct dma_channel *channel)
{
	/* Not much to do here */
	tsi148_dma_start(channel);
}

/**
 * vme_do_dma() - Do a DMA transfer
 * @desc: DMA transfer descriptor
 *
 *  This function first checks the validity of the user supplied DMA transfer
 * parameters. It then tries to find an available DMA channel to do the
 * transfer, setups that channel and starts the DMA.
 *
 *  Returns 0 on success, or a standard kernel error code on failure.
 */
int vme_do_dma(struct vme_dma *desc)
{
	int rc = 0;
	struct dma_channel *channel;
	int i;

	/* First check the transfer length */
	if (!desc->length) {
		printk(KERN_ERR PFX "%s: Wrong length %d\n",
		       __func__, desc->length);
		return -EINVAL;
	}

	/* Check the transfer direction validity */
	if ((desc->dir != VME_DMA_FROM_DEVICE) &&
	    (desc->dir != VME_DMA_TO_DEVICE)) {
		printk(KERN_ERR PFX "%s: Wrong direction %d\n",
		       __func__, desc->dir);
		return -EINVAL;
	}

	/* Check we're within a 32-bit address space */
	if (desc->src.addru || desc->dst.addru) {
		printk(KERN_ERR PFX "%s: Addresses are not 32-bit\n", __func__);
		return -EINVAL;
	}

	/* Find an available channel */
	for (i = 0; i < TSI148_NUM_DMA_CHANNELS; i++) {
		if (mutex_lock_interruptible(&channels[i].lock))
			return -ERESTARTSYS;

		channel = &channels[i];

		if (!channel->busy)
			break;

		mutex_unlock(&channel->lock);
	}

	if (i == TSI148_NUM_DMA_CHANNELS) {
		/* No channel available, try again later */
		return -EBUSY;
	}

	/*
	 * We found a free channel, mark it as busy and release the
	 * channel mutex as no one will bother us now */
	channel->busy = 1;
	memcpy(&channel->desc, desc, sizeof(struct vme_dma));

	mutex_unlock(&channel->lock);

	/* Setup the DMA transfer */
	rc = vme_dma_setup(channel);

	if (rc)
		goto out_release_channel;

	/* Start the DMA transfer */
	vme_dma_start(channel);

	/* Wait for DMA completion */
	rc = wait_event_interruptible(channel->wait,
				 !tsi148_dma_busy(channel));

	desc->status = tsi148_dma_get_status(channel);

	/* Now do some cleanup and we're done */
	tsi148_dma_release(channel);

	pci_unmap_sg(vme_bridge->pdev, channel->sgl, channel->sg_mapped,
		     desc->dir);

	sgl_unmap_user_pages(channel->sgl, channel->sg_pages, 0);

	kfree(channel->sgl);

out_release_channel:
	/* Finally release the channel busy flag */
	mutex_lock(&channel->lock);
	channel->busy = 0;
	mutex_unlock(&channel->lock);

	/* Signal we're done in case we're in module exit */
	wake_up(&channel_wait[channel->num]);

	return rc;
}
EXPORT_SYMBOL_GPL(vme_do_dma);

/**
 * vme_dma_ioctl() - ioctl file method for the VME DMA device
 * @file: Device file descriptor
 * @cmd: ioctl number
 * @arg: ioctl argument
 *
 *  Currently the VME DMA device supports the following ioctl:
 *
 *    VME_IOCTL_START_DMA
 */
long vme_dma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	struct vme_dma desc;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case VME_IOCTL_START_DMA:
		/* Get the DMA transfer descriptor */
		if (copy_from_user(&desc, (void *)argp, sizeof(struct vme_dma)))
			return -EFAULT;

		/* Do the DMA */
		rc = vme_do_dma(&desc);

		if (rc)
			return rc;

		/*
		 * Copy back the DMA transfer descriptor containing the DMA
		 * updated status.
		 */
		if (copy_to_user((void *)argp, &desc, sizeof(struct vme_dma)))
			return -EFAULT;

		break;

	default:
		rc = -ENOIOCTLCMD;
	}


	return rc;
}

/**
 * vme_dma_exit() - Release DMA management resources
 *
 *
 */
void __devexit vme_dma_exit(void)
{
	int i;

	/*
	 * We must be careful here before releasing resources as there might be
	 * some DMA transfers in flight. So wait for those transfers to complete
	 * before doing the cleanup.
	 */
	for (i = 0; i < TSI148_NUM_DMA_CHANNELS; i++) {

		mutex_lock(&channels[i].lock);
		tsi148_dma_abort(&channels[i]);

		while (channels[i].busy) {
			mutex_unlock(&channels[0].lock);
			wait_event_interruptible(channel_wait[i],
						 !channels[i].busy);
			mutex_lock(&channels[i].lock);
		}

		/*
		 * OK, now we know that the channel is not busy and we have
		 * it locked. Mark it busy so no one will use it anymore.
		 */
		channels[i].busy = 1;
		mutex_unlock(&channels[0].lock);
	}

	tsi148_dma_exit();
}

/**
 * vme_dma_init() - Initialize DMA management
 *
 */
int __devinit vme_dma_init(void)
{
	int i;

	for (i = 0; i < TSI148_NUM_DMA_CHANNELS; i++) {
		mutex_init(&channels[i].lock);
		channels[i].num = i;
		init_waitqueue_head(&channels[i].wait);
		init_waitqueue_head(&channel_wait[i]);
		INIT_LIST_HEAD(&channels[i].hw_desc_list);
	}

	return tsi148_dma_init();
}
