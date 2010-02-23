/**
 * \file libvmebus.c
 * \brief VME Bus access user library
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include <vmebus.h>

/** \brief VME address space mapping device */
#define VME_MWINDOW_DEV "/dev/vme_mwindow"
/** \brief VME DMA device */
#define VME_DMA_DEV "/dev/vme_dma"

/**
 * \brief Check for VME a bus error
 * \param desc VME mapping descriptor
 *
 * \return 0 if no bus error occured, 1 if bus error occured or -1 on
 *         any other error (with errno set appropriately).
 *
 * Note: This function is DEPRECATED. Use vme_bus_error_check_clear instead.
 */
int vme_bus_error_check(struct vme_mapping *desc)
{
	int bus_err;

	if (ioctl(desc->fd, VME_IOCTL_GET_BUS_ERROR, &bus_err) < 0) {
#ifdef DEBUG
		printf("libvmebus: Failed to get bus error status: %s\n",
		       strerror(errno));
#endif
		return -1;
	}

	return bus_err;
}

/**
 * \brief Check and clear a VME bus error from a given address and mapping
 * \param desc VME mapping descriptor
 * \param address VME address to be checked
 *
 * \return 0 if no bus error occured, 1 if bus error occured or -1 on
 *         any other error (with errno set appropriately).
 *
 * Note that the VME bus error is cleared _only_ if it matches the
 * given address/am pair.
 */
int vme_bus_error_check_clear(struct vme_mapping *vme_desc, __u64 address)
{
	struct vme_bus_error_desc desc;

	desc.valid = 0;
	desc.error.address	= address;
	desc.error.am		= vme_desc->am;

	if (ioctl(vme_desc->fd, VME_IOCTL_CHECK_CLEAR_BUS_ERROR, &desc) < 0) {
#ifdef DEBUG
		printf("libvmebus: Failed to check bus error status: %s\n",
		       strerror(errno));
#endif
		return -1;
	}
	return desc.valid;
}

static off_t __page_addr(off_t address)
{
	long pagemask = sysconf(_SC_PAGESIZE) - 1;

	return address & ~pagemask;
}

static size_t __page_aligned_size(struct vme_mapping *desc)
{
	return desc->pci_addrl + desc->sizel - __page_addr(desc->pci_addrl);
}

static size_t __page_aligned_size_incr(struct vme_mapping *desc)
{
	return desc->pci_addrl - __page_addr(desc->pci_addrl);
}

/*
 * Note that the caller might want to map a non-page-aligned region.
 * In order to comply with the alignment requirements of mmap, we map a larger,
 * page-aligned region and then store the mapped address of the requested
 * non-page-aligned region.
 */
static int vme_mmap(struct vme_mapping *desc)
{
	void *addr;
	size_t size;
	off_t page_addr;

	page_addr = __page_addr(desc->pci_addrl);
	size = __page_aligned_size(desc);

	addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, desc->fd, page_addr);

	if (addr == MAP_FAILED)
		return -1;

	desc->user_va = addr + __page_aligned_size_incr(desc);
	return 0;
}

static int vme_munmap(struct vme_mapping *desc)
{
	void *addr = desc->user_va - __page_aligned_size_incr(desc);

	return munmap(addr, __page_aligned_size(desc));
}

/**
 * \brief Map a VME address space into the user address space
 * \param vmeaddr VME physical start address of the mapping
 * \param len Window size (must be a multiple of 64k)
 * \param am VME address modifier
 * \param offset Offset in the mapping for read access test (Not used)
 * \param size Data width
 * \param param VME mapping parameters
 *
 * \return the virtual address of the mapping on succes, or 0 if an error
 *         occurs (in that case errno is set appropriately).
 *
 * \deprecated This function is an emulation of the CES functionality on LynxOS,
 *             new applications should use the vme_map() function instead.
 *
 * The CES function interface does not give all the needed VME parameters, so
 * the following choices were made and may have to be tweaked:
 *
 *  \li if read prefetch is enabled then the prefetch size is set to 2 cache
 *      lines
 *  \li the VME address and size are automatically aligned on 64k if needed
 *  \li the VME address is limited to 32 bits
 *
 * \note The descriptor allocated by this function is stored into the
 *       pdparam_master->sgmin field and the file descriptor in
 *       pdparam_master->dum[1].
 *
 * \note We voluntarily do not close the file descriptor here, because doing
 *       so would automatically remove the mapping made using it. The
 *       mapping will be removed at return_controller() time.
 */

unsigned long find_controller(unsigned long vmeaddr, unsigned long len,
			      unsigned long am, unsigned long offset,
			      unsigned long size, struct pdparam_master *param)
{
	struct vme_mapping *desc;
	int org_force_create;
	int force_create = 1;
	int fd;

	/* Allocate our mapping descriptor */
	if ((desc = calloc(1, sizeof(struct vme_mapping))) == NULL)
		return 0;

	/* Now fill it with the parameters we got */
	desc->window_num = 0;
	desc->am = am;

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
		printf("libvmebus: %s - "
		       "Unsupported data width %ld\n", __func__, size);
		goto out_free;
		break;
	}

	desc->bcast_select = 0;

	if (len & 0xffff) {
		printf("libvmebus: %s - "
		       "Mapping size %lx is not 64k aligned, "
		       "aligning it to %lx.\n",
		       __func__, len, (len + 0x10000) & ~0xffff);
		len &= ~0xffff;
	}

	desc->sizel = len;
	desc->sizeu = 0;

	if (vmeaddr & 0xffff) {
		printf("libvmebus: %s - "
		       "VME address %lx is not 64k aligned, "
		       "aligning it to %lx.\n",
		       __func__, vmeaddr, vmeaddr & ~0xffff);
		vmeaddr &= ~0xffff;
	}

	desc->vme_addrl = vmeaddr;
	desc->vme_addru = 0;

	/*
	 * Now we're all set up, let's start the real stuff
	 */
	if ((fd = open(VME_MWINDOW_DEV, O_RDWR)) < 0)
		goto out_free;

	desc->fd = fd;

	/* Save the force create flag */
	if (ioctl(fd, VME_IOCTL_GET_CREATE_ON_FIND_FAIL, &org_force_create) < 0)
		goto out_free;

	/* Set the force create flag */
	if (ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL, &force_create) < 0)
		goto out_free;

	/* Create a new mapping for the area */
	if (ioctl(fd, VME_IOCTL_FIND_MAPPING, desc) < 0)
		goto out_restore_flag;

	/* Now mmap the area */
	if (vme_mmap(desc))
		goto out_restore_flag;

	/*
	 * Save the descriptor address into the struct pdparam_master sgmin
	 * field, it will be freed with the call to return_controller().
	 */
	param->sgmin = (unsigned long)desc;

	/* Restore the force create flag */
	ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL, &org_force_create);

#ifdef DEBUG
	printf("Created mapping\n\tVME addr: 0x%08x size: 0x%08x "
	       "AM: 0x%02x data width: %d\n\n",
	       desc->vme_addrl, desc->sizel, desc->am, desc->data_width);
#endif

	return (unsigned long)desc->user_va;


out_restore_flag:
	ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL, &org_force_create);

out_free:
	free(desc);

	return 0;
}

/**
 * \brief Release a VME mapping
 * \param desc VME mapping descriptor
 *
 * \return 0 on success, or -1 if an error occurs (in that case errno is set
 *           appropriately).
 *
 * \deprecated This function is an emulation of the CES functionality on LynxOS,
 *             new applications should use the vme_unmap() function instead.
 *
 * \warning The function interface has been changed to a single parameter,
 *          a struct vme_mapping descriptor as the CES original parameters
 *          are not enough to match a VME mapping.
 */
unsigned long return_controller(struct vme_mapping *desc)
{
	int ret = 0;
	int org_force_destroy;
	int force_destroy = 1;

	if (!desc) {
		errno = -EFAULT;
		return -1;
	}

	if (vme_munmap(desc)) {
		printf("libvmebus: %s - failed to munmap\n", __func__);
		ret = -1;
	}

	/* Save the force destroy flag */
	if (ioctl(desc->fd, VME_IOCTL_GET_DESTROY_ON_REMOVE,
		  &org_force_destroy) < 0) {
		printf("libvmebus: %s - "
		       "Failed to get the force destroy flag\n", __func__);
	}

	/* Set the force destroy flag */
	if (ioctl(desc->fd, VME_IOCTL_SET_DESTROY_ON_REMOVE,
		  &force_destroy) < 0) {
		printf("libvmebus: %s - "
		       "Failed to set the force destroy flag\n", __func__);
	}

	if (ioctl(desc->fd, VME_IOCTL_RELEASE_MAPPING, desc) < 0) {
		printf("libvmebus: %s - "
		       "failed to release mapping\n", __func__);
		ret = -1;
	}

	/* Restore the force destroy flag */
	if (ioctl(desc->fd, VME_IOCTL_SET_DESTROY_ON_REMOVE,
		  &org_force_destroy) < 0)
		printf("libvmebus: %s - "
		       "Failed to restore force destroy flag\n", __func__);

	close(desc->fd);

	free(desc);

	return ret;
}

/**
 * \brief Map a VME address space into the user address space
 *
 * \param desc  -- a struct vme_mapping descriptor describing the mapping
 * \param force -- flag indicating whether a new window should be created if
 *                 needed.
 *
 * \return a userspace virtual address for the mapping or NULL on error
 *         (in that case errno is set appropriately).
 */
void *vme_map(struct vme_mapping *desc, int force)
{
	int fd;
	int org_force_create;

	if ((fd = open(VME_MWINDOW_DEV, O_RDWR)) < 0)
		return NULL;

	desc->fd = fd;

	/* Save the force create flag */
	if (ioctl(fd, VME_IOCTL_GET_CREATE_ON_FIND_FAIL, &org_force_create) < 0)
		goto out_close;

	/* Set the force create flag */
	if (ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL, &force) < 0)
		goto out_close;

	/* Create a new mapping for the area */
	if (ioctl(fd, VME_IOCTL_FIND_MAPPING, desc) < 0)
		goto out_restore_flag;

	/* Now mmap the area */
	if (vme_mmap(desc))
		goto out_restore_flag;

	/* Restore the force create flag */
	ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL, &org_force_create);

#ifdef DEBUG
	printf("Created mapping\n\tVME addr: 0x%08x size: 0x%08x "
	       "AM: 0x%02x data width: %d\n\n",
	       desc->vme_addrl, desc->sizel, desc->am, desc->data_width);
#endif

	return desc->user_va;


out_restore_flag:
	ioctl(fd, VME_IOCTL_SET_CREATE_ON_FIND_FAIL, &org_force_create);

out_close:
	close(fd);

	return NULL;
}

/**
 * \brief Unmap a VME address space
 * \param desc a struct vme_mapping descriptor describing the mapping
 * \param force flag indicating whether the window should be destroyed if
 *              needed
 *
 * \return 0 on success or -1 on error (in that case errno is set
 *         appropriately).
 *
 * This function unmaps a VME address space mapped using vme_map().
 *
 */
int vme_unmap(struct vme_mapping *desc, int force)
{
	int ret = 0;
	int org_force_destroy;

	if (!desc) {
		errno = -EFAULT;
		return -1;
	}

	if (vme_munmap(desc)) {
		printf("libvmebus: %s - failed to munmap\n", __func__);
		ret = -1;
	}

	if (force) {
		/* Save the force destroy flag */
		if (ioctl(desc->fd, VME_IOCTL_GET_DESTROY_ON_REMOVE,
			  &org_force_destroy) < 0) {
			printf("libvmebus: %s - "
			       "Failed to get the force destroy flag\n",
			       __func__);
		}

		/* Set the force destroy flag */
		if (ioctl(desc->fd, VME_IOCTL_SET_DESTROY_ON_REMOVE,
			  &force) < 0) {
			printf("libvmebus: %s - "
			       "Failed to set the force destroy flag\n",
			       __func__);
		}
	}

	if (ioctl(desc->fd, VME_IOCTL_RELEASE_MAPPING, desc) < 0) {
		printf("libvmebus: %s - "
		       "failed to release mapping\n",
		       __func__);
		ret = -1;
	}

	if (force) {
		/* Restore the force destroy flag */
		if (ioctl(desc->fd, VME_IOCTL_SET_DESTROY_ON_REMOVE,
			  &org_force_destroy) < 0)
			printf("libvmebus: %s - "
			       "Failed to restore force destroy flag\n",
			       __func__);
	}

	close(desc->fd);

	return ret;
}


/**
 * \brief Perform a DMA read on the VME bus
 * \param desc DMA transfer descriptor
 *
 * \return 0 on success or -1 on error (in that case errno is set
 *         appropriately).
 *
 *  This function performs a DMA read with the parameters specified in the
 * struct vme_dma descriptor.
 */
int vme_dma_read(struct vme_dma *desc)
{
	int rc = 0;
	int fd;

	if ((fd = open(VME_DMA_DEV, O_RDONLY)) < 0)
		return -1;

	if (ioctl(fd, VME_IOCTL_START_DMA, desc) < 0)
		rc = -1;

	close(fd);

	return rc;
}

/**
 * \brief Perform a DMA write on the VME bus
 * \param desc DMA transfer descriptor
 *
 * \return 0 on success or -1 on error (in that case errno is set
 *         appropriately).
 *
 *  This function performs a DMA write with the parameters specified in the
 * struct vme_dma descriptor.
 *
 */
int vme_dma_write(struct vme_dma *desc)
{
	int rc = 0;
	int fd;

	if ((fd = open(VME_DMA_DEV, O_WRONLY)) < 0)
		return -1;

	if (ioctl(fd, VME_IOCTL_START_DMA, desc) < 0)
		rc = -1;

	close(fd);

	return rc;
}
