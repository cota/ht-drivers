/**
 * @file cdcmIoctl.h
 *
 * @brief CDCM ioctl numbers and their descriptions
 *
 * @author Yury GEORGIEVSKIY CERN, AB/CO/HT
 *
 * @date Created on 07/01/2009
 *
 * <long description>
 *
 * @version $Id: cdcmIoctl.h,v 1.1 2009/01/07 19:49:23 ygeorgie Exp $
 */
#ifndef _CDCM_IOCTL_H_INCLUDE_
#define _CDCM_IOCTL_H_INCLUDE_

#ifndef __KERNEL__
/* Get necessary definitions from system and kernel headers */
#include <sys/types.h>
#include <sys/ioctl.h>
#endif

#include <cdcm/cdcmUsrKern.h>

/* Private IOCTL codes:
 *
 * CDCM_CDV_INSTALL    -> register new device logical group in the
 *			  driver. Called each time cdv_install() do.
 * CDCM_CDV_UNINSTALL  -> unloading major device (one of the logical
 *			  groups). Called each time cdv_uninstall() do.
 * CDCM_GET_GRP_MOD_AM -> get device amount of the specified group.
 */

#define CDCM_IOCTL_MAGIC 'c'

#define CDCM_IO(nr)      _IO(CDCM_IOCTL_MAGIC, nr)
#define CDCM_IOR(nr,sz)  _IOR(CDCM_IOCTL_MAGIC, nr, sz)
#define CDCM_IOW(nr,sz)  _IOW(CDCM_IOCTL_MAGIC, nr, sz)
#define CDCM_IOWR(nr,sz) _IOWR(CDCM_IOCTL_MAGIC, nr, sz)

#define CDCM_CDV_INSTALL    CDCM_IOW(197, char[PATH_MAX])
#define CDCM_CDV_UNINSTALL  CDCM_IOW(198, int)
#define CDCM_GET_GRP_MOD_AM CDCM_IOW(199, int)

#endif /* _CDCM_IOCTL_H_INCLUDE_ */
