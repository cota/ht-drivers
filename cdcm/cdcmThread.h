/**
 * @file cdcmThread.h
 *
 * @brief CDCM Lynx threads header file.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 02/06/2006
 *
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version $Id: cdcmThread.h,v 1.3 2009/01/09 10:26:03 ygeorgie Exp $
 */
#ifndef _CDCM_THREAD_H_INCLUDE_
#define _CDCM_THREAD_H_INCLUDE_

#include <linux/completion.h>

/* user stream task termination check */
#define CDCM_LOOP_AGAIN				\
do {						\
  if (kthread_should_stop())			\
    return(0); /* exit user thread function */	\
} while (0)

/* max argument number that thread payload can take.
   TODO. How to bypass this limitation? */
#define CDCM_MAX_THR_ARGS 6

/* max thread name length */
#define CDCM_TNL 16

/* CDCM thread handle */
typedef struct _cdcmThread {
  struct list_head    thr_list;	        /* linked list */
  struct task_struct *thr_pd;	        /* process descriptor */
  struct completion   thr_c;            /* thread creation syncronization */
  char                thr_nm[CDCM_TNL]; /* desired thread name */
  int thr_sem;	/* one of the condition flag for thread sleeping. If it's
		   equal tid - then sleeping thread should die and therefore
		   will be waken up, otherwise it's zero */
} cdcmthr_t;

/* payload function pointer (tpp_t - Thread Payload Pointer Type)
   The only reason to declare it - was to get rid of compiler warnings, or
   at least limit it to one place only (this one).
   Also used in timeout() function.
   TODO. How to get rid of warning: function declaration isn't a prototype */
typedef int (*tpp_t) ();

/* CDCM thread parameter */
typedef struct _cdcmThrParam {
  tpp_t      tp_payload;                 /* user payload to lunch */
  int        tp_argn;                    /* actual number of user arguments */
  char      *tp_args[CDCM_MAX_THR_ARGS]; /* user arguments */
  cdcmthr_t *tp_handle;		         /* thread handler */
} cdcmtpar_t;

#endif /* _CDCM_THREAD_H_INCLUDE_ */
