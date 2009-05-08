/**
 * @file cdcm.h
 *
 * @brief General CDCM definitions.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 02/06/2006
 *
 * Should be included by anyone, who wants to use CDCM.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version
 */
#ifndef _CDCM_H_INCLUDE_
#define _CDCM_H_INCLUDE_

/* for both Lynx/Linux */
#include "cdcmBoth.h"
#include "cdcmIo.h"

#ifdef __linux__

#include "general_drvr.h"
#include "vmebus.h" /* find_controller, etc */
#include "cdcmDrvr.h"
#include "cdcmLynxAPI.h"
#include "cdcmLynxDefs.h"

#define find_controller cdcm_find_controller
extern unsigned int cdcm_find_controller(unsigned int, unsigned int,
					 unsigned int, unsigned int,
					 unsigned int,
					 struct pdparam_master*);

#define sel  cdcm_sel  /* see Lynx <sys/file.h> for more details */
#define file cdcm_file /* see Lynx <sys/file.h> for more details */
#define enable restore

#else  /* __Lynx__ */

#include <dldd.h>
#include <errno.h>
#include <sys/types.h>
#include <conf.h>
#include <kernel.h>
#include <sys/file.h>
#include <io.h>
#include <sys/ioctl.h>
#include <time.h>
#include <param.h>
#include <string.h>
#include <ces/absolute.h>
#include <ces/vmelib.h>
#include <drm.h>

extern unsigned long find_controller   _AP((unsigned long vmeaddr, unsigned long len, unsigned long am, unsigned long offset, unsigned long size, struct pdparam_master  *param));
extern unsigned long return_controller _AP((unsigned long physaddr, unsigned long len));
extern int vme_intset                  _AP((int vct, int (*handler)(),char *arg, long *sav));
extern int vme_intclr                  _AP((int vct, long *sav));
extern int nanotime(unsigned long *);

#endif	/* __linux__ */

#endif /* _CDCM_H_INCLUDE_ */
