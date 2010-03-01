/* =========================================================== 	*/
/* Define the IOCTL calls you will implement         		*/
#ifndef _SKEL_USER_IOCTL_H_
#define _SKEL_USER_IOCTL_H_

#include "mod-pci.h"

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

#define MOD_PCI_GETSLOTADDR	SKELU_IOWR(1,struct mod_pci_memory_space)

#define SkelUserIoctlFIRST		MOD_PCI_GETSLOTADDR
#define SkelUserIoctlLAST		MOD_PCI_GETSLOTADDR

#endif /* _SKEL_USER_IOCTL_H_ */
