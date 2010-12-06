/**
 * @file skeluser_ioctl.h
 *
 * @brief Specific IOCTL's for the CVORG
 *
 * Copyright (c) 2009 CERN
 * @author Emilio G. Cota <cota@braap.org>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _CVORG_IOCTL_H_
#define _CVORG_IOCTL_H_

#include <cvorg.h>

/*
 * Note. You can change any macro _name_ in this file *except* the following:
 * - SKELUSER_IOCTL_MAGIC
 * - SkelDrvrSPECIFIC_IOCTL_CALLS
 * - SkelUserIoctlFIRST
 * - SkelUserIoctlLAST
 */

#define SKELUSER_IOCTL_MAGIC 'G'

#define CVORG_IO(nr)	   _IO(SKELUSER_IOCTL_MAGIC, nr)
#define CVORG_IOR(nr,sz)   _IOR(SKELUSER_IOCTL_MAGIC, nr, sz)
#define CVORG_IOW(nr,sz)   _IOW(SKELUSER_IOCTL_MAGIC, nr, sz)
#define CVORG_IOWR(nr,sz) _IOWR(SKELUSER_IOCTL_MAGIC, nr, sz)


/*
 * S (Set)-> set through a pointer
 * T (Tell)-> Tell directly with the argument value
 * G (Get)-> reply by setting through a pointer
 * Q (Query)-> response is on the return value
 * X (eXchange)-> switch G and S atomically
 * H (sHift)-> switch T and Q atomically
 *
 * IOCTL numbers:
 * From 1 to 10, IOCTLs that don't require locking/ownership
 * From 11 to 50, IOCTLs that require module locking only
 * From 51 to 90, IOCTLs that require module locking + ownership
 */
#define CVORG_IOCTL_NO_LOCKING	10
#define CVORG_IOCTL_LOCKING	50
#define CVORG_IOCTL_OWNERSHIP	90

#define CVORG_IOCGCHANNEL		CVORG_IOR (11, uint32_t)
#define SkelUserIoctlFIRST		CVORG_IOCGCHANNEL
#define CVORG_IOCSCHANNEL		CVORG_IOW (12, uint32_t)

#define CVORG_IOCGOUTOFFSET		CVORG_IOR (13, uint32_t)
#define CVORG_IOCSOUTOFFSET		CVORG_IOW (53, uint32_t)

#define CVORG_IOCGINPOLARITY		CVORG_IOR (14, uint32_t)
#define CVORG_IOCSINPOLARITY		CVORG_IOW (54, uint32_t)

#define CVORG_IOCGPLL			CVORG_IOR (15, struct ad9516_pll)
#define CVORG_IOCSPLL			CVORG_IOWR(55, struct ad9516_pll)

#define CVORG_IOCGSRAM			CVORG_IOWR(16, struct cvorg_sram_entry)
#define CVORG_IOCSSRAM			CVORG_IOWR(56, struct cvorg_sram_entry)

#define CVORG_IOCSTRIGGER		CVORG_IOW (57, uint32_t)

#define CVORG_IOCSLOADSEQ		CVORG_IOWR(58, struct cvorg_seq)

#define CVORG_IOCGCHANSTAT		CVORG_IOR (19, uint32_t)

#define CVORG_IOCGSAMPFREQ		CVORG_IOR (20, uint32_t)

#define CVORG_IOCTUNLOCK_FORCE		CVORG_IO  (21)
#define CVORG_IOCTUNLOCK		CVORG_IO  (61)
#define CVORG_IOCTLOCK			CVORG_IO  (62)

#define CVORG_IOCGMODE			CVORG_IOR (23, uint32_t)
#define CVORG_IOCSMODE			CVORG_IOW (63, uint32_t)

#define CVORG_IOCGOUTGAIN		CVORG_IOR (24, int32_t)
#define CVORG_IOCSOUTGAIN		CVORG_IOWR(64, int32_t)

#define CVORG_IOCGADC_READ		CVORG_IOWR(65, struct cvorg_adc)

#define CVORG_IOCGDAC_VAL		CVORG_IOR (26, uint32_t)
#define CVORG_IOCSDAC_VAL		CVORG_IOW (66, uint32_t)

#define CVORG_IOCGDAC_GAIN		CVORG_IOR (27, uint16_t)
#define CVORG_IOCSDAC_GAIN		CVORG_IOW (67, uint16_t)

#define CVORG_IOCGDAC_OFFSET		CVORG_IOR (28, int16_t)
#define CVORG_IOCSDAC_OFFSET		CVORG_IOW (68, int16_t)

#define CVORG_IOCGTEMPERATURE		CVORG_IOR (29, int32_t)

#define CVORG_IOCSOUT_ENABLE		CVORG_IOW (70, int32_t)
#define CVORG_IOCGPCB_ID		CVORG_IOR (31, uint64_t)

#define SkelUserIoctlLAST		CVORG_IOCSOUT_ENABLE

#define SkelDrvrSPECIFIC_IOCTL_CALLS (_IOC_NR(SkelUserIoctlLAST) - \
					_IOC_NR(SkelUserIoctlFIRST) + 1)

#endif /* _CVORG_IOCTL_H_ */
