/* channels per card */
#define	VMOD16A2_CHANNELS	2

/* VMOD 16A2 registers offsets */
#define VMOD16A2_DAC0IN 	0x00	/* channel 0 input */
#define VMOD16A2_DAC1IN 	0x02	/* channel 1 input */
#define VMOD16A2_LDAC0 		0x04	/* channel 0 load (start conversion) */
#define VMOD16A2_LDAC1 		0x06	/* channel 1 load (start conversion) */
#define VMOD16A2_LDAC01 	0x08	/* load both channels */
#define VMOD16A2_CDAC0	 	0x10	/* channel 0 clear */
#define VMOD16A2_CDAC1 		0x12	/* channel 1 clear */
#define VMOD16A2_WDCTRL 	0x16	/* watchdog timer ctrl register */
#define VMOD16A2_WDCLR 		0x18	/* clear watchdog timer */

/** @ brief vmod16a2 register layout */
struct vmod16a2_registers {
	unsigned short	dac0in;		/** DAC0 input */
	unsigned short	dac1in;		/** DAC1 input */
	unsigned short	ldac0;		/** load DAC0 */
	unsigned short	ldac1;		/** load DAC1 */
	unsigned short	ldac01;		/** load both DACs */
	unsigned short	cdac0;		/** clear DAC0 */
	unsigned short	cdac1;		/** clear DAC1 */
	unsigned short	wdctrl;		/** watchdog set/enable */
	unsigned short	wdclr;		/** watchdog clear */
};

/** @brief entry in vmod16a2 config table */
struct vmod16a2_dev {
	int		lun;		/** logical unit number */
	char		*cname;		/** carrier driver name */
	int		carrier;	/** supporting carrier */
	int		slot;		/** slot we're plugged in */
	unsigned long	address;	/** address space */
	int		is_big_endian;	/** MODULBUS endianness */
};

/** @brief stores a selected lun, channel per open file */
struct vmod16a2_state {
	int	lun;		/** logical unit number */
	int	channel;	/** channel */
	int	selected;	/** already selected flag */
};

/** IOCSELECT ioctl arg */
struct vmod16a2_select {
	int	lun;		
	int	channel;
};

/* driver internals, device table size and ioctl commands */

#define	VMOD16A2_MAX_MODULES	64

#define	VMOD16A2_IOC_MAGIC	'J'
#define VMOD16A2_IOCSELECT 	_IOW(VMOD16A2_IOC_MAGIC, 1, struct vmod16a2_select)
#define	VMOD16A2_IOCPUT		_IOW(VMOD16A2_IOC_MAGIC, 2, int)

#define VMOD16A2_NO_LUN_CHANGE	(-1)

