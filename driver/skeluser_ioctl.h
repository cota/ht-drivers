/**
 * @file skeluser_ioctl.h
 *
 * @brief Specific IOCTL's for the MTT
 *
 * Copyright (c) 2009 CERN
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _MTT_IOCTL_H_
#define _MTT_IOCTL_H_

/*
 * Note. You can change any macro _name_ in this file *except* the following:
 * - SKELUSER_IOCTL_MAGIC
 * - SkelDrvrSPECIFIC_IOCTL_CALLS
 * - SkelUserIoctlFIRST
 * - SkelUserIoctlLAST
 */

#define SKELUSER_IOCTL_MAGIC '%'

#define MTT_IO(nr)		_IO(SKELUSER_IOCTL_MAGIC, nr)
#define MTT_IOR(nr, sz)		_IOR(SKELUSER_IOCTL_MAGIC, nr, sz)
#define MTT_IOW(nr, sz)		_IOW(SKELUSER_IOCTL_MAGIC, nr, sz)
#define MTT_IOWR(nr, sz)	_IOWR(SKELUSER_IOCTL_MAGIC, nr, sz)

/*
 * S (Set)-> set through a pointer
 * T (Tell)-> Tell directly with the argument value
 * G (Get)-> reply by setting through a pointer
 * Q (Query)-> response is on the return value
 * X (eXchange)-> switch G and S atomically
 * H (sHift)-> switch T and Q atomically
 */
#define SkelUserIoctlFIRST	MTT_IO(0)
#define MTT_IOCSCABLEID		MTT_IOW(1, uint32_t)
#define MTT_IOCGCABLEID		MTT_IOR(2, uint32_t)
#define MTT_IOCSUTC_SENDING	MTT_IOW(3, uint32_t)
#define MTT_IOCSEXT_1KHZ	MTT_IOW(4, uint32_t)
#define MTT_IOCSOUT_DELAY	MTT_IOW(5, uint32_t)
#define MTT_IOCGOUT_DELAY	MTT_IOR(6, uint32_t)
#define MTT_IOCSOUT_ENABLE	MTT_IOR(7, uint32_t)
#define MTT_IOCSSYNC_PERIOD	MTT_IOW(8, uint32_t)
#define MTT_IOCGSYNC_PERIOD	MTT_IOR(9, uint32_t)
#define MTT_IOCSUTC		MTT_IOW(10, MttDrvrTime)
#define MTT_IOCGUTC		MTT_IOR(11, MttDrvrTime)
#define MTT_IOCSSEND_EVENT	MTT_IOW(12, uint32_t)
#define MTT_IOCSTASKS_START	MTT_IOW(13, uint32_t)
#define MTT_IOCSTASKS_STOP	MTT_IOW(14, uint32_t)
#define MTT_IOCSTASKS_CONT	MTT_IOW(15, uint32_t)
#define MTT_IOCGTASKS_CURR	MTT_IOR(16, uint32_t)
#define MTT_IOCSTCB		MTT_IOW(17,  MttDrvrTaskBuf)
#define MTT_IOCGTCB		MTT_IOWR(18, MttDrvrTaskBuf)
#define MTT_IOCSGRVAL		MTT_IOW(19,  MttDrvrGlobalRegBuf)
#define MTT_IOCGGRVAL		MTT_IOWR(20, MttDrvrGlobalRegBuf)
#define MTT_IOCSTRVAL		MTT_IOW(21,  MttDrvrTaskRegBuf)
#define MTT_IOCGTRVAL		MTT_IOWR(22, MttDrvrTaskRegBuf)
#define MTT_IOCSPROGRAM		MTT_IOW(23,  MttDrvrProgramBuf)
#define MTT_IOCGPROGRAM		MTT_IOWR(24, MttDrvrProgramBuf)
#define MTT_IOCGSTATUS		MTT_IOR(25, uint32_t)
#define MTT_IOCRAW_WRITE	MTT_IOW(26, MttDrvrRawIoBlock)
#define MTT_IOCRAW_READ		MTT_IOWR(27, MttDrvrRawIoBlock)
#define MTT_IOCGVERSION		MTT_IOR(28, MttDrvrVersion)
#define SkelUserIoctlLAST	MTT_IO(29)

#define SkelDrvrSPECIFIC_IOCTL_CALLS (_IOC_NR(SkelUserIoctlLAST) - \
					_IOC_NR(SkelUserIoctlFIRST) + 1)

#endif /* _MTT_IOCTL_H_ */
