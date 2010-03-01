/* =========================================================== 	*/
/* Define the IOCTL calls you will implement         		*/
#ifndef _SKEL_USER_IOCTL_H_
#define _SKEL_USER_IOCTL_H_

#include <vmod12a2.h>

/*
 * Note. You can change any macro _name_ in this file *except* the following:
 * - SKELUSER_IOCTL_MAGIC
 * - SkelDrvrSPECIFIC_IOCTL_CALLS
 * - SkelUserIoctlFIRST
 * - SkelUserIoctlLAST
 */

/* =========================================================== */
/* Your Ioctl calls are defined here                           */

#define SKELUSER_IOCTL_MAGIC 'V'

#define SKELU_IO(nr)		  _IO(SKELUSER_IOCTL_MAGIC, nr)
#define SKELU_IOR(nr,sz)	 _IOR(SKELUSER_IOCTL_MAGIC, nr, sz)
#define SKELU_IOW(nr,sz)	 _IOW(SKELUSER_IOCTL_MAGIC, nr, sz)
#define SKELU_IOWR(nr,sz)	_IOWR(SKELUSER_IOCTL_MAGIC, nr, sz)

/* ============================= */
/* Define Standard IOCTL numbers */

#define VMOD_12A2_SELECT 	SKELU_IOW(1, struct vmod12a2_channel)
#define VMOD_12A2_PUT		SKELU_IOW(2, struct vmod12a2_write)

#define SkelUserIoctlFIRST VMOD_12A2_SELECT
#define SkelUserIoctlLAST  VMOD_12A2_PUT

#endif /* _SKEL_USER_IOCTL_H_ */
