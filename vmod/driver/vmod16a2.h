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

/** @brief ioctl arg for IOCPUT */
struct vmod16a2_convert {
	unsigned int	channel;	/**< output channel (0 or 1) */
	unsigned int	value;		/**< 16-bit value to output */
};

/* driver internals, device table size and ioctl commands */

#define	VMOD16A2_MAX_MODULES	64

#define	VMOD16A2_IOC_MAGIC	'J'
#define	VMOD16A2_IOCPUT		_IOW(VMOD16A2_IOC_MAGIC, 2, struct vmod16a2_convert)
