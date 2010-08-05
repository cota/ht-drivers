#ifndef _SIS33_H_
#define _SIS33_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/time.h>
#else
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdint.h>
#include <time.h>
#ifndef __user
#define __user
#endif
#endif /* __KERNEL__ */

#define SIS33_MAX_DEVICES	32

/**
 * @brief sis33 rounding options
 */
enum sis33_round {
	SIS33_ROUND_NEAREST,
	SIS33_ROUND_DOWN,
	SIS33_ROUND_UP,
};

/**
 * @brief sis33 triggers
 */
enum sis33_trigger {
	SIS33_TRIGGER_STOP,
	SIS33_TRIGGER_START,
};

/**
 * @brief clock sources
 */
enum sis33_clksrc {
	SIS33_CLKSRC_INTERNAL,
	SIS33_CLKSRC_EXTERNAL
};

/** @cond INTERNAL */
/* Acquisition flags */
#define SIS33_ACQF_DONTWAIT	1	/**< Do not sleep-wait */
#define SIS33_ACQF_TIMEOUT	2	/**< Sleep with timeout */
/** @endcond */

/** @cond INTERNAL */
/**
 * @brief acquisition descriptor
 */
struct sis33_acq_desc {
	uint32_t		segment;	/**< memory segment. 0 to n-1 */
	uint32_t		nr_events;	/**< number of events */
	uint32_t		ev_length;	/**< event length, i.e. number of samples per event */
	uint32_t		flags;		/**< acquisition flags ACQF_* */
	struct timespec		timeout;	/**< timeout */
	uint32_t		unused[4];
};
/** @endcond */

/**
 * @brief acquisition buffer struct
 */
struct sis33_acq {
	uint16_t __user		*data;		/**< buffer to store the requested data */
	uint32_t		size;		/**< size of the data buffer */
	uint32_t		nr_samples;	/**< number of samples acquired */
	uint64_t		prevticks;	/**< in multievent, number of clock ticks from the first stop trigger */
/** @cond INTERNAL */
/* nr_samples, first_samp and be are filled in by SIS33_IOC_FETCH */
	uint32_t		first_samp;	/**< index of the first valid sample */
	uint32_t		be;		/**< 1 if the data is in big endian format; 0 otherwise */
	uint32_t		unused[4];
/** @endcond */
};

/** @cond INTERNAL */
/**
 * @brief acquisition list struct
 */
struct sis33_acq_list {
	uint32_t		segment;	/**< memory segment. 0 to n-1 */
	uint32_t		channel;	/**< channel to read from. 0 to n-1 */
	struct sis33_acq __user	*acqs;		/**< pointer to the array of acquisitions */
	uint32_t		n_acqs;		/**< number of acquisitions */
	uint32_t		flags;		/**< acquisition flags ACQF_*  */
	struct timespec		timeout;	/**< acquisition timeout */
	struct timeval		endtime;	/**< local time when the acquisition finished */
	uint32_t		unused[4];
};
/** @endcond */

/** @cond INTERNAL */
#define SIS33_IOC_FETCH		_IOWR('3', 0, struct sis33_acq_list)
#define SIS33_IOC_ACQUIRE	_IOW ('3', 1, struct sis33_acq_desc)
/** @endcond */

#endif /* _SIS33_H_ */
