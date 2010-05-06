/**
 * \file vmebus.h
 * \brief PCI-VME public API
 * \author Sebastien Dugue
 * \date 04/02/2009
 *
 *  This API presents in fact 2 APIs with some common definitions. One for
 * drivers and one for user applications. User applications cannot use the
 * driver specific parts enclosed in \#ifdef __KERNEL__ sections.
 *
 * Copyright (c) 2009 \em Sebastien \em Dugue
 *
 * \par License:
 *      This program is free software; you can redistribute  it and/or
 *      modify it under  the terms of  the GNU General  Public License as
 *      published by the Free Software Foundation;  either version 2 of the
 *      License, or (at your option) any later version.
 *
 */

#ifndef _VME_H
#define _VME_H

#ifdef __KERNEL__
#include <linux/device.h>
#include <linux/kernel.h>
#endif /* __KERNEL__ */

#include <linux/types.h>

/*
 * VME window attributes
 */

/**
 * \brief Read prefetch size
 */
enum vme_read_prefetch_size {
	VME_PREFETCH_2	= 0,
	VME_PREFETCH_4,
	VME_PREFETCH_8,
	VME_PREFETCH_16
};

/**
 * \brief Data width
 */
enum vme_data_width {
	VME_D8	= 8,
	VME_D16	= 16,
	VME_D32	= 32,
	VME_D64	= 64
};

/**
 * \brief 2eSST data transfer speed
 */
enum vme_2esst_mode {
	VME_SST160	= 0,
	VME_SST267,
	VME_SST320
};

/**
 * \brief Address modifiers
 */
enum vme_address_modifier {
	VME_A64_MBLT		= 0,	/* 0x00 */
	VME_A64_SCT,			/* 0x01 */
	VME_A64_BLT		= 3,	/* 0x03 */
	VME_A64_LCK,			/* 0x04 */
	VME_A32_LCK,			/* 0x05 */
	VME_A32_USER_MBLT	= 8,	/* 0x08 */
	VME_A32_USER_DATA_SCT,		/* 0x09 */
	VME_A32_USER_PRG_SCT,		/* 0x0a */
	VME_A32_USER_BLT,		/* 0x0b */
	VME_A32_SUP_MBLT,		/* 0x0c */
	VME_A32_SUP_DATA_SCT,		/* 0x0d */
	VME_A32_SUP_PRG_SCT,		/* 0x0e */
	VME_A32_SUP_BLT,		/* 0x0f */
	VME_2e6U		= 0x20,	/* 0x20 */
	VME_2e3U,			/* 0x21 */
	VME_A16_USER		= 0x29,	/* 0x29 */
	VME_A16_LCK		= 0x2c,	/* 0x2c */
	VME_A16_SUP		= 0x2d,	/* 0x2d */
	VME_CR_CSR		= 0x2f,	/* 0x2f */
	VME_A40_SCT		= 0x34, /* 0x34 */
	VME_A40_LCK, 			/* 0x35 */
	VME_A40_BLT		= 0x37, /* 0x37 */
	VME_A24_USER_MBLT,		/* 0x38 */
	VME_A24_USER_DATA_SCT,		/* 0x39 */
	VME_A24_USER_PRG_SCT,		/* 0x3a */
	VME_A24_USER_BLT,		/* 0x3b */
	VME_A24_SUP_MBLT,		/* 0x3c */
	VME_A24_SUP_DATA_SCT,		/* 0x3d */
	VME_A24_SUP_PRG_SCT,		/* 0x3e */
	VME_A24_SUP_BLT,		/* 0x3f */
};



/**
 * \brief PCI-VME mapping descriptor
 * \param window_num Hardware window number
 * \param kernel_va Kernel virtual address of the mapping for use by drivers
 * \param user_va User virtual address of the mapping for use by applications
 * \param fd User file descriptor for this mapping
 * \param window_enabled State of the hardware window
 * \param data_width VME data width
 * \param am VME address modifier
 * \param read_prefetch_enabled PCI read prefetch enabled state
 * \param read_prefetch_size PCI read prefetch size (in cache lines)
 * \param v2esst_mode VME 2eSST transfer speed
 * \param bcast_select VME 2eSST broadcast select
 * \param pci_addru PCI bus start address upper 32 bits
 * \param pci_addrl PCI bus start address lower 32 bits
 * \param sizeu Window size upper 32 bits
 * \param sizel Window size lower 32 bits
 * \param vme_addru VME bus start address upper 32 bits
 * \param vme_addrl VME bus start address lower 32 bits
 *
 * This data structure is used for describing both a hardware window
 * and a logical mapping on top of a hardware window. Therefore some of
 * the fields are only relevant to one of those two entities.
 */
struct vme_mapping {
	int	window_num;

	/* Reserved for kernel use */
	void	*kernel_va;

	/* Reserved for userspace */
	void	*user_va;
	int	fd;

	/* Window settings */
	int				window_enabled;
	enum vme_data_width		data_width;
	enum vme_address_modifier	am;
	int				read_prefetch_enabled;
	enum vme_read_prefetch_size	read_prefetch_size;
	enum vme_2esst_mode		v2esst_mode;
	int				bcast_select;
	unsigned int			pci_addru;
	unsigned int			pci_addrl;
	unsigned int			sizeu;
	unsigned int			sizel;
	unsigned int			vme_addru;
	unsigned int			vme_addrl;
};

/**
 * \brief VME RMW descriptor
 * \param vme_addru VME address for the RMW cycle upper 32 bits
 * \param vme_addrl VME address for the RMW cycle lower 32 bits
 * \param am VME address modifier
 * \param enable_mask Bitmask of the bit
 * \param compare_data
 * \param swap_data
 *
 */
struct vme_rmw {
	unsigned int vme_addru;
	unsigned int vme_addrl;
	enum vme_address_modifier am;
	unsigned int enable_mask;
	unsigned int compare_data;
	unsigned int swap_data;
};

/**
 * \brief DMA endpoint attributes
 * \param data_width VME data width (only used if endpoint is on the VME bus)
 * \param am VME address modifier (ditto)
 * \param v2esst_mode VME 2eSST transfer speed (ditto)
 * \param bcast_select VME 2eSST broadcast select (ditto)
 * \param addru Address upper 32 bits
 * \param addrl Address lower 32 bits
 *
 *  This data structure is used for describing the attributes of a DMA endpoint.
 * All the field excepted for the address are only relevant for an endpoint
 * on the VME bus.
 */
struct vme_dma_attr {
	enum vme_data_width		data_width;
	enum vme_address_modifier	am;
	enum vme_2esst_mode		v2esst_mode;
	unsigned int			bcast_select;
	unsigned int			addru;
	unsigned int			addrl;
};

/** \brief DMA block size on the PCI or VME bus */
enum vme_dma_block_size {
	VME_DMA_BSIZE_32 = 0,
	VME_DMA_BSIZE_64,
	VME_DMA_BSIZE_128,
	VME_DMA_BSIZE_256,
	VME_DMA_BSIZE_512,
	VME_DMA_BSIZE_1024,
	VME_DMA_BSIZE_2048,
	VME_DMA_BSIZE_4096
};

/** \brief DMA backoff time (us) on the PCI or VME bus */
enum vme_dma_backoff {
	VME_DMA_BACKOFF_0 = 0,
	VME_DMA_BACKOFF_1,
	VME_DMA_BACKOFF_2,
	VME_DMA_BACKOFF_4,
	VME_DMA_BACKOFF_8,
	VME_DMA_BACKOFF_16,
	VME_DMA_BACKOFF_32,
	VME_DMA_BACKOFF_64
};

/**
 * \brief DMA control
 * \param vme_block_size VME bus block size when the source is VME
 * \param vme_backoff_time VME bus backoff time when the source is VME
 * \param pci_block_size PCI/X bus block size when the source is PCI
 * \param pci_backoff_time PCI bus backoff time when the source is PCI
 *
 */
struct vme_dma_ctrl {
	enum vme_dma_block_size	vme_block_size;
	enum vme_dma_backoff	vme_backoff_time;
	enum vme_dma_block_size	pci_block_size;
	enum vme_dma_backoff	pci_backoff_time;
};

/** \brief DMA transfer direction */
enum vme_dma_dir {
	VME_DMA_TO_DEVICE = 1,
	VME_DMA_FROM_DEVICE
};

/**
 * \brief VME DMA transfer descriptor
 * \param status Transfer status
 * \param length Transfer size in bytes
 * \param novmeinc Must be set to 1 when accessing a FIFO like device on the VME
 * \param dir Transfer direction
 * \param src Transfer source attributes
 * \param dst Transfer destination attributes
 * \param opt Transfer control
 *
 */
struct vme_dma {
	unsigned int		status;
	unsigned int		length;
	unsigned int		novmeinc;
	enum vme_dma_dir	dir;

	struct vme_dma_attr	src;
	struct vme_dma_attr	dst;

	struct vme_dma_ctrl	ctrl;
};

/**
 * \brief VME Bus Error
 * \param address Address of the bus error
 * \param am Address Modifier of the bus error
 */
struct vme_bus_error {
	__u64				address;
	enum vme_address_modifier	am;
};

/**
 * \brief VME Bus Error descriptor
 * \param error Address/AM of the bus error
 * \param valid Valid Flag: 0 -> no error, 1 -> error
 */
struct vme_bus_error_desc {
	struct vme_bus_error	error;
	int			valid;
};


/*! @name VME single access swapping policy
 *@{
 */
#define SINGLE_NO_SWAP          0
#define SINGLE_AUTO_SWAP        1
#define SINGLE_WORD_SWAP        2
#define SINGLE_BYTEWORD_SWAP    3
/*@}*/


/*! @name page qualifier
 *
 *@{
 */
#define VME_PG_SHARED  0x00
#define VME_PG_PRIVATE 0x02
/*@}*/


/**
 * \brief VME mapping attributes
 * \param iack VME IACK 0 -> IACK pages
 * \param rdpref VME read prefetch option 0 -> Disable
 * \param wrpost VME write posting option
 * \param swap VME swap options
 * \param sgmin page descriptor number returned by find_controller
 * \param dum -- dum[0] page qualifier (shared/private), dum[1] XPC ADP-type
 *               dum[2] - reserved, _must_ be 0.
 *
 * This structure is used for the find_controller() and return_controller()
 * LynxOS CES driver emulation.
 */
struct pdparam_master
{
	unsigned long iack;
	unsigned long rdpref;
	unsigned long wrpost;
	unsigned long swap;
	unsigned long sgmin;
	unsigned long dum[3];
};



/**
 * \name Window management ioctl numbers
 * \{
 */
/** Get a physical window attributes */
#define VME_IOCTL_GET_WINDOW_ATTR	_IOWR('V', 0, struct vme_mapping)
/** Create a physical window */
#define VME_IOCTL_CREATE_WINDOW		_IOW( 'V', 1, struct vme_mapping)
/** Destroy a physical window */
#define VME_IOCTL_DESTROY_WINDOW	_IOW( 'V', 2, int)
/** Create a mapping over a physical window */
#define VME_IOCTL_FIND_MAPPING		_IOWR('V', 3, struct vme_mapping)
/** Remove a mapping */
#define VME_IOCTL_RELEASE_MAPPING	_IOW( 'V', 4, struct vme_mapping)
/** Get the create on find failed flag */
#define VME_IOCTL_GET_CREATE_ON_FIND_FAIL	_IOR( 'V', 5, unsigned int)
/** Set the create on find failed flag */
#define VME_IOCTL_SET_CREATE_ON_FIND_FAIL	_IOW( 'V', 6, unsigned int)
/** Get the destroy on remove flag */
#define VME_IOCTL_GET_DESTROY_ON_REMOVE	_IOR( 'V', 7, unsigned int)
/** Set the destroy on remove flag */
#define VME_IOCTL_SET_DESTROY_ON_REMOVE	_IOW( 'V', 8, unsigned int)
/** Get bus error status -- DEPRECATED */
#define VME_IOCTL_GET_BUS_ERROR		_IOR( 'V', 9, unsigned int)
/** Check (and possibly clear) the bus error status */
#define VME_IOCTL_CHECK_CLEAR_BUS_ERROR	_IOWR('V',10, struct vme_bus_error_desc)
/* \}*/

/**
 * DMA ioctls
 * \{
 */
/** Start a DMA transfer */
#define VME_IOCTL_START_DMA		_IOWR('V', 10, struct vme_dma)
/* \}*/


#ifdef __KERNEL__

/*
 * Those definitions are for drivers only and are not visible to userspace.
 */

struct vme_driver {
	int (*match)(struct device *, unsigned int);
	int (*probe)(struct device *, unsigned int);
	int (*remove)(struct device *, unsigned int);
	void (*shutdown)(struct device *, unsigned int);
	int (*suspend)(struct device *, unsigned int, pm_message_t);
	int (*resume)(struct device *, unsigned int);
	struct device_driver driver;
	struct device *devices;
};

#define to_vme_driver(x) container_of((x), struct vme_driver, driver)

typedef void (*vme_berr_handler_t)(struct vme_bus_error *);

/* API for new drivers */
extern int vme_register_driver(struct vme_driver *vme_driver, unsigned int ndev);
extern void vme_unregister_driver(struct vme_driver *vme_driver);
extern int vme_request_irq(unsigned int, int (*)(void *),
			   void *, const char *);
extern int vme_free_irq(unsigned int );
extern int vme_generate_interrupt(int, int, signed long);

extern struct vme_mapping* find_vme_mapping_from_addr(unsigned);
extern int vme_get_window_attr(struct vme_mapping *);
extern int vme_create_window(struct vme_mapping *);
extern int vme_destroy_window(int);
extern int vme_find_mapping(struct vme_mapping *, int);
extern int vme_release_mapping(struct vme_mapping *, int);

extern int vme_do_dma(struct vme_dma *);
extern int vme_do_dma_kernel(struct vme_dma *);


extern int vme_bus_error_check(int);
extern struct vme_berr_handler *
vme_register_berr_handler(struct vme_bus_error *, size_t, vme_berr_handler_t);
extern void vme_unregister_berr_handler(struct vme_berr_handler *);

/* API providing an emulation of the CES VME driver for legacy drivers */
extern unsigned long find_controller(unsigned long, unsigned long,
				     unsigned long, unsigned long,
				     unsigned long, struct pdparam_master *);
extern unsigned long return_controller(unsigned, unsigned);
extern int vme_intset(int, int (*)(void *), void *, void *);
extern int vme_intclr(int, void *);

#endif /* __KERNEL__ */


#endif /* _VME_H */
