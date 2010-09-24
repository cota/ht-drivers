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

struct vmod12a2_output {
	int	channel;
	int	value;
};
#endif /* _VMOD12A2_H_ */
