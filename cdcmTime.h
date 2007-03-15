/* $Id: cdcmTime.h,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmTime.h
; Module Descr:	 CDCM timing-related header file.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: June, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmTime.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   07/07/06   Initial version.
*/

#ifndef _CDCM_TIME_H_INCLUDE_
#define _CDCM_TIME_H_INCLUDE_

#include "cdcmDrvr.h"

#define __wait_event_exclusive(wq, condition)				\
do {									\
  DEFINE_WAIT(__wait);							\
  									\
  for (;;) {								\
    prepare_to_wait_exclusive(&wq, &__wait, TASK_UNINTERRUPTIBLE);	\
    if (condition)							\
      break;								\
    schedule();								\
  }									\
  finish_wait(&wq, &__wait);						\
} while (0)

#define wait_event_exclusive(wq, condition)	\
do {						\
  if (condition)				\
    break;					\
  __wait_event_exclusive(wq, condition);	\
} while (0)



#define __wait_event_interruptible_exclusive(wq, condition, ret)	\
do {									\
  DEFINE_WAIT(__wait);							\
									\
  for (;;) {								\
    prepare_to_wait_exclusive(&wq, &__wait, TASK_INTERRUPTIBLE);	\
    if (condition)							\
      break;								\
    if (!signal_pending(current)) {					\
      schedule();							\
      continue;								\
    }									\
    ret = -ERESTARTSYS;							\
    break;								\
  }									\
  finish_wait(&wq, &__wait);						\
} while (0)


#define wait_event_interruptible_exclusive(wq, condition)	\
({								\
  int __ret = 0;						\
  if (!(condition))						\
    __wait_event_interruptible_exclusive(wq, condition, __ret);	\
  __ret;							\
})

#endif /* _CDCM_TIME_H_INCLUDE_ */
