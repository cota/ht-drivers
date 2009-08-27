/**
 * @file skeluser_ioctl.h
 *
 * @brief Specific IOCTL's for my Module
 *
 * Copyright (C) 2009 CERN
 * @author Name Surname	<email@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _SKEL_USER_IOCTL_H_
#define _SKEL_USER_IOCTL_H_

/*
 * Note. You can change any macro _name_ in this file *except* the following:
 * - SKELUSER_IOCTL_MAGIC
 * - SkelDrvrSPECIFIC_IOCTL_CALLS
 * - SkelUserIoctlFIRST
 * - SkelUserIoctlLAST
 */

#define SKELUSER_IOCTL_MAGIC 'U' //!< USER magic number (chosen arbitrarily)

#define USER_IO(nr)	   _IO(SKELUSER_IOCTL_MAGIC, nr)
#define USER_IOR(nr,sz)   _IOR(SKELUSER_IOCTL_MAGIC, nr, sz)
#define USER_IOW(nr,sz)   _IOW(SKELUSER_IOCTL_MAGIC, nr, sz)
#define USER_IOWR(nr,sz) _IOWR(SKELUSER_IOCTL_MAGIC, nr, sz)

#define SkelUserIoctlFIRST	USER_IO(0)
#define MY_IOCTL_NAME_1		USER_IO(1)
#define MY_IOCTL_NAME_2		USER_IOR(2, uint32_t)
#define SkelUserIoctlLAST	USER_IO(3)

#define SkelDrvrSPECIFIC_IOCTL_CALLS (_IOC_NR(SkelUserIoctlLAST) - \
					_IOC_NR(SkelUserIoctlFIRST) + 1)

#endif /* _SKEL_USER_IOCTL_H_ */
