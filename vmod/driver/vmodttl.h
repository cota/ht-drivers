#ifndef __VMODTTL_H__
#define __VMODTTL_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DEFINES
 */
#define CHAN_A		0
#define CHAN_B		1
#define CHAN_C		2

#define A_CHAN_IN	0x00
#define A_CHAN_OUT	0x01
#define B_CHAN_IN	0x00
#define B_CHAN_OUT  	0x02
#define C_CHAN_IN	0x00
#define C_CHAN_OUT  	0x04
#define VMODTTL_O   	0x08

#define A_CHAN_OPEN_COLLECTOR	0x01
#define B_CHAN_OPEN_COLLECTOR	0x02
#define C_CHAN_OPEN_COLLECTOR	0x04

#define NR_BITS 8

#define DEFAULT_DELAY 1 /* Used to wait for a while to allow the Z8536 to perform the read/write operations */

struct vmodttlarg {
	int 	dev;
	int	val;
};

struct vmodttl_pattern {
	unsigned char port;	/* Port A (0) or B (1) */
	unsigned char pos; 	/* Position of the bit [0 - 7] */
	unsigned char ptr;	/* Pattern Transition register */
	unsigned char pmr;	/* Pattern mask register */
	unsigned char ppr;	/* Pattern Polarity Register */
};

struct vmodttlconfig {
	int		io_flag;
	int 		us_pulse;
	int		open_collector;
	int		invert;
	int		pattern_mode_a; /* 1: AND 0: OR */
	int		pattern_mode_b; /* 1: AND 0: OR */
	struct vmodttl_pattern bit_pattern_a[NR_BITS];
	struct vmodttl_pattern bit_pattern_b[NR_BITS];
};

/*
 * IOCTL commands
 */
#define VMODTTL_READ_CHAN	_IOR('v', 0, struct vmodttlarg)	
#define VMODTTL_WRITE_CHAN	_IOW('v', 1, struct vmodttlarg)	
#define VMODTTL_CONFIG		_IOW('v', 2, struct vmodttlconfig)
#define VMODTTL_PATTERN		_IOW('v', 3, struct vmodttl_pattern)
#define VMODTTL_SIMPIO		_IOW('v', 4, int)
#define VMODTTL_READ_CONFIG	_IOR('v', 5, struct vmodttlconfig)



#define VMODTTL_MAX_BOARDS 	64	/* max. # of boards supported   */

#define VMODTTL_PORTC		0
#define VMODTTL_PORTB 		2
#define VMODTTL_PORTA 		4
#define VMODTTL_CONTROL  	6

struct vmodttl {
	int             lun;            /** logical unit number */
	char            *cname;         /** carrier name */
	int             carrier;        /** supporting carrier */
	int             slot;           /** slot we're plugged in */
	unsigned long   address;        /** address space */
	int             is_big_endian;  /** endianness */
};

#ifdef __cplusplus
}
#endif

#endif /* __VMODTTL_H__ */

