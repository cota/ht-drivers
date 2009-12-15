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
#define SKELUSER_IOCTL_MAGIC 'I'

#define ICV_IO(nr)       _IO(SKELUSER_IOCTL_MAGIC, nr)
#define ICV_IOR(nr, sz)	 _IOR(SKELUSER_IOCTL_MAGIC, nr, sz)
#define ICV_IOW(nr, sz)	 _IOW(SKELUSER_IOCTL_MAGIC, nr, sz)
#define ICV_IOWR(nr, sz) _IOWR(SKELUSER_IOCTL_MAGIC, nr, sz)


#define SkelUserIoctlFIRST   ICV_IO(0)
#define ICVVME_getmoduleinfo ICV_IOR(1, struct icv196T_ModuleInfo)
#define ICVVME_connect       ICV_IOW(2, struct icv196T_connect)
#define ICVVME_disconnect    ICV_IOW(3, struct icv196T_connect)
#define ICVVME_reset         ICV_IO(4)
#define ICVVME_setDbgFlag    ICV_IOWR(5, int)
#define ICVVME_nowait        ICV_IO(6)
#define ICVVME_wait          ICV_IO(7)
#define ICVVME_setupTO       ICV_IOWR(8, int)
#define ICVVME_intcount      ICV_IOWR(9, struct icv196T_Service)
#define ICVVME_setreenable   ICV_IOW(10, struct icv196T_UserLine)
#define ICVVME_clearreenable ICV_IOW(11, struct icv196T_UserLine)
#define ICVVME_enable        ICV_IOW(12, struct icv196T_UserLine)
#define ICVVME_disable       ICV_IOW(13, struct icv196T_UserLine)
#define ICVVME_iosem         ICV_IOWR(14, struct icv196T_Service)
#define ICVVME_readio        ICV_IOWR(15, struct icv196T_Service)
#define ICVVME_setio         ICV_IOW(16, struct icv196T_Service)
#define ICVVME_intenmask     ICV_IOWR(17, struct icv196T_Service)
#define ICVVME_reenflags     ICV_IOWR(18, struct icv196T_Service)
#define ICVVME_gethandleinfo ICV_IOR(19, struct icv196T_HandleInfo)
#define SkelUserIoctlLAST    ICV_IO(20)

#define SkelDrvrSPECIFIC_IOCTL_CALLS (_IOC_NR(SkelUserIoctlLAST) - \
					_IOC_NR(SkelUserIoctlFIRST) + 1)

#endif /* _SKEL_USER_IOCTL_H_ */
