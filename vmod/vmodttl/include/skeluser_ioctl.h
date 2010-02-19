/* =========================================================== 	*/
/* Define the IOCTL calls you will implement         		*/
#ifndef _SKEL_USER_IOCTL_H_
#define _SKEL_USER_IOCTL_H_

#include <vmodttl.h>

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

#define VMOD_TTL_IO(nr)		  _IO(SKELUSER_IOCTL_MAGIC, nr)
#define VMOD_TTL_IOR(nr,sz)	 _IOR(SKELUSER_IOCTL_MAGIC, nr, sz)
#define VMOD_TTL_IOW(nr,sz)	 _IOW(SKELUSER_IOCTL_MAGIC, nr, sz)
#define VMOD_TTL_IOWR(nr,sz)	_IOWR(SKELUSER_IOCTL_MAGIC, nr, sz)

/* ============================= */
/* Define Standard IOCTL numbers */

#define VMOD_TTL_CONFIG     	VMOD_TTL_IOWR(1, struct vmodttl_register)
#define VMOD_TTL_READ     	VMOD_TTL_IOWR(3, struct vmodttl_register)
#define VMOD_TTL_WRITE     	VMOD_TTL_IOWR(4, struct vmodttl_register)

#define SkelUserIoctlFIRST		VMOD_TTL_CONFIG
#define SkelUserIoctlLAST		VMOD_TTL_WRITE

#endif /* _SKEL_USER_IOCTL_H_ */

