/* $Id: cdcmBoth.h,v 1.2 2007/12/19 09:02:05 ygeorgie Exp $ */
/**
 * @file cdcmBoth.h
 *
 * @brief Contains definitions that are used both, by LynxOS and Linux drivers.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Jul, 2006
 *
 * It should be included in the user part of the driver.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version 3.0  ygeorgie  14/03/2007  Production release, CVS controlled.
 * @version 2.0  ygeorgie  27/07/2006  First working release.
 * @version 1.0  ygeorgie  21/07/2006  Initial version.
 */
#ifndef _CDCM_BOTH_H_INCLUDE_
#define _CDCM_BOTH_H_INCLUDE_

int cdcm_copy_from_user(char *, char *, int);
int cdcm_copy_to_user(char *, char *, int);

#ifdef __Lynx__

#define CDCM_LOOP_AGAIN

#endif

#endif /* _CDCM_BOTH_H_INCLUDE_ */
