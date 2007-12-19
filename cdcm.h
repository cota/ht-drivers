/* $Id: cdcm.h,v 1.3 2007/12/19 09:02:05 ygeorgie Exp $ */
/**
 * @file cdcm.h
 *
 * @brief CDCM driver header file module.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Feb, 2006
 *
 * Exported for the user. Definitions, that should be visible both in user and
 * kernel space are located here.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version 4.0  ygeorgie  01/08/2007  Full Lynx-like installation behaviour.
 * @version 3.0  ygeorgie  14/03/2007  Production release, CVS controlled.
 * @version 2.0  ygeorgie  27/07/2006  First working release.
 * @version 1.0  ygeorgie  02/06/2006  Initial version.
 */
#ifndef _CDCM_H_INCLUDE_
#define _CDCM_H_INCLUDE_

#ifndef __KERNEL__
/* Get necessary definitions from system and kernel headers. */
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

#endif /* _CDCM_H_INCLUDE_ */
