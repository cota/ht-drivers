#ifndef _VMOD12A2_H_
#define _VMOD12A2_H_



/* VMOD 12A2 channels */
#define VMOD_12A2_CHANNELS 	2
#define VMOD_12A2_CHANNEL0 	0x0
#define VMOD_12A2_CHANNEL1 	0x2

/* VMOD 12A4 channels */
#define VMOD_12A4_CHANNELS 	4
#define VMOD_12A4_CHANNEL0 	VMOD_12A2_CHANNEL0 
#define VMOD_12A4_CHANNEL1 	VMOD_12A2_CHANNEL1 
#define VMOD_12A4_CHANNEL2 	0x4
#define VMOD_12A4_CHANNEL3 	0x6

/* driver internals, device table size and ioctl commands */

#define	VMOD12A2_MAX_MODULES	64

#define	VMOD12A2_IOC_MAGIC	'J'
#define VMOD12A2_IOCSELECT 	_IOW(VMOD12A2_IOC_MAGIC, 1, struct vmod12a2_select)
#define	VMOD12A2_IOCPUT		_IOW(VMOD12A2_IOC_MAGIC, 2, int)

#define VMOD12A2_NO_LUN_CHANGE	(-1)

/**
 * @brief address of a 12A2 channel given by LUN, slot and channel
 * number 
 */
struct vmod12a2_channel {
	unsigned int	lun;		/**< Logical unit number (1..N) */
	unsigned int	slot;		/**< Slot 12A2 lies in (0-1) (unused) */
	unsigned int	channel;	/**< Channel (0-1) */
};

/** @brief modulbus register address information */
struct vmod12a2_registers {
	int	slot;		/**< slot the card lies in (0-1) */
	char	*address;	/**< address of VMOD12A2 module,
		same as base_address+mod_pci_offsets[slot], but only
		known after a SELECT ioctl */
	char	*channel[VMOD_12A2_CHANNELS];
};

/** @brief entry in vmod12a2 config table */
struct vmod12a2_dev {
	int		lun;		/** logical unit number */
	char		*cname;		/** carrier name */
	int		carrier;	/** supporting carrier */
	int		slot;		/** slot we're plugged in */
	unsigned long	address;	/** address space */
	int		is_big_endian;	/** endianness */
};

/** @brief stores a selected lun, channel per open file */
struct vmod12a2_state {
	int	lun;		/** logical unit number */
	int	channel;	/** channel */
	int	selected;	/** already selected flag */
};

/** IOCSELECT ioctl arg */
struct vmod12a2_select {
	int	lun;		
	int	channel;
};

#endif /* _VMOD12A2_H_ */
