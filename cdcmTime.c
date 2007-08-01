/* $Id: cdcmTime.c,v 1.2 2007/08/01 15:07:20 ygeorgie Exp $ */
/*
; Module Name:	 cdcmTime.c
; Module Descr:	 All timing-related staff for CDCM is located here, i.e. Lynx
;		 stub functions and supplementary functions.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: June, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmTime.c: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 4.0   ygeorgie   01/08/07   Full Lynx-like installation behaviour.
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   07/07/06   Initial version.
*/

#include <linux/time.h>

#include "cdcmTime.h"
#include "cdcmThread.h"
#include "cdcmLynxDefs.h"

extern cdcmStatics_t cdcmStatT;	/* declared in the cdcmDrvr.c module */
extern int cdcm_dbg_cntr; /* TODO. REMOVE. For CDCM debugging only. 
			     Defined in cdcm.c module */

extern cdcmthr_t* cdcm_get_thread_handle(int);

/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_timer_callback
 * DESCRIPTION: Generic timer callback.
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
static void
cdcm_timer_callback(
		    unsigned long arg) /* index of the timer data. 
					  assume to be a correct one. */
{
  cdcmt_t *my_data = &cdcmStatT.cdcm_timer[arg];

  del_timer(&my_data->ct_timer);
  my_data->ct_on = 0;
  PRNT_DBG("is called. Index[%d]", (int)arg);
  my_data->ct_uf(my_data->ct_arg); /* call user payload */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    nanotime
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:	fraction of a second in nanoseconds.
 *		'sec' will hold current time in seconds.
 *-----------------------------------------------------------------------------
 */
int
nanotime(
	 unsigned long *sec) /* current time in seconds is placed here */
{
  struct timespec curtime;

  curtime = current_kernel_time();

  *sec = (unsigned long)curtime.tv_sec;
  
  return((int)curtime.tv_nsec);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    timeout
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:	non-negative timeout id - if there is a timer available.
 *		SYSERR (i.e. -1)        - otherwise.
 *-----------------------------------------------------------------------------
 */
int
timeout(
	tpp_t  func,     /* function to call */
	char  *arg,      /* argument for function handler */
	int    interval) /* timeout interval (10 millisecond garnularity) */
{
  int cntr;
  //cdcmt_t *cdcmtptr;		/* timer handle */

  for (cntr = 0; cntr < MAX_CDCM_TIMERS; cntr++) {
    if (!cdcmStatT.cdcm_timer[cntr].ct_on) { /* not used */
      /* initialize for usage */
      cdcmStatT.cdcm_timer[cntr] = (cdcmt_t) {
	.ct_on      = current->pid,
	.ct_timer   = TIMER_INITIALIZER(cdcm_timer_callback/*payload*/, (jiffies + interval)/*when to call*/, cntr/*timer payload parameter - my data table index*/),
	.ct_uf  = func,
	.ct_arg = arg
      };

      init_timer(&cdcmStatT.cdcm_timer[cntr].ct_timer);
      add_timer(&cdcmStatT.cdcm_timer[cntr].ct_timer);

      //PRNT_DBG("<%d> for tid[%d]. TimerID[%d] with interval %d ADDED", cdcm_dbg_cntr++, current->pid, cntr+1, interval);
      return(cntr + 1); /* timeout ID */
    }
  }
  
  PRNT_ERR("No more timers (MAX %d)", MAX_CDCM_TIMERS);
  return(SYSERR);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cancel_timeout
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
int
cancel_timeout(
	       int arg)	/* timeout id */
{
  int idx = arg - 1; /* who should die */
  
  if ( idx >= 0 && idx < MAX_CDCM_TIMERS) {
    if (cdcmStatT.cdcm_timer[idx].ct_on) {
      del_timer_sync(&cdcmStatT.cdcm_timer[idx].ct_timer);
      cdcmStatT.cdcm_timer[idx].ct_on = 0;
      //PRNT_DBG("timeout %d cancelled.", arg);
      return(OK);
    }
    
    PRNT_DBG("Timer %d already expired.", arg);
    return(OK);
  }

  PRNT_ERR("No such timer: %d", arg);
  return(SYSERR);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    cdcm_get_sem
 * DESCRIPTION: Gets wait queue for a given semaphore, or if it's a new one,
 *		try to obtain a free wait queue, if any. 
 * RETURNS:	wait_queue_head_t pointer that corresponds to the semaphore
 *		'sem' (either new one or that is already exist).
 *		NULL - if no more free semaphore handles left.
 *-----------------------------------------------------------------------------
 */
spinlock_t get_sem_lock = SPIN_LOCK_UNLOCKED;
static cdcmsem_t*
cdcm_get_sem(
	     int *sem) 	/* user-defined driver semaphore */
{
  int cntr;
  ulong spin_flags;
  
  /* see if user wants already used sema */
  for (cntr = 0; cntr < MAX_CDCM_SEM; cntr++)
    if (sem == cdcmStatT.cdcm_s[cntr].wq_usr_val) { /* bingo */
      //printk("[CDCM] %s() tid [%d] return ---OLD--- semID[%d]\n", __FUNCTION__, current->pid, cdcmStatT.cdcm_s[cntr].wq_id);
      return(&cdcmStatT.cdcm_s[cntr]);
    }
  
  /* it's a new one. try to book the place */

  /* enter critical region */
  spin_lock_irqsave(&get_sem_lock, spin_flags);

  for (cntr = 0; cntr < MAX_CDCM_SEM; cntr++) {
    if (!cdcmStatT.cdcm_s[cntr].wq_usr_val) { /* find free one */
      cdcmStatT.cdcm_s[cntr] = (cdcmsem_t) { /* init */
	.wq_usr_val  = sem,     /* take it */
	.wq_init_val = *sem,    /* set init user value */
	.wq_masta    = current, /* set my masta */
	.wq_tflag    = 0,	/* thread flag */
	.wq_id       = cntr+1	/* set my id */
      };
      init_waitqueue_head(&cdcmStatT.cdcm_s[cntr].wq_head);
      
      //CDCM_DBG("%s() return ---NEW--- semID[%d]\n", __FUNCTION__, cdcmStatT.cdcm_s[cntr].wq_id);

      /* leave critical region */
      spin_unlock_irqrestore(&get_sem_lock, spin_flags);

      return(&cdcmStatT.cdcm_s[cntr]);
    }
  }
  
  /* leave critical region */
  spin_unlock_irqrestore(&get_sem_lock, spin_flags);

  return(NULL);	/* all Lynx semaphores already used */
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    swait
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */

/* One of the events that we are waiting on.
   Depending on the caller (thread or major process),
   different wait conditions are monitored

   Acronyms description:
   WC  - Wait Condition
   SSI - Sem Sig Ignore flag (SEM_SIGIGNORE)
   SSO - Sem Sig all Other flags (SEM_SIGRETRY, SEM_SIGABORT)
   THR - in case if it was called from the THRead
*/

#define WC_SSI_THR (semptr->wq_tflag > 0 || semptr->wq_tflag == -1 || (stptr->thr_sem == current->pid))
#define WC_SSI (semptr->wq_tflag > 0 || semptr->wq_tflag == -1)

#define WC_SSO_THR (semptr->wq_tflag > 0 || semptr->wq_tflag == -1 || (stptr->thr_sem == current->pid))
#define WC_SSO (semptr->wq_tflag > 0 || semptr->wq_tflag == -1)

static DEFINE_SPINLOCK(swait_sem_lock);

int
swait(
      int *sem,	/* user driver semaphore */
      int flag)	/* SEM_SIGIGNORE, SEM_SIGRETRY, SEM_SIGABORT */
{
  int coco = 0;
  cdcmsem_t *semptr = cdcm_get_sem(sem); /* get new or existing one */
  cdcmthr_t *stptr = NULL;
  ulong swait_spin_flags;
  int wee = 0;

  if (!semptr) {
    PRNT_ABS_ERR("EFAULT: No more free wait queues!");
    return(SYSERR);
  }
  
  /* check who is calling - kernel thread or normal process. If stptr is NULL, 
     then swait is called from the process, but __not__ from the kthread */
  stptr = cdcm_get_thread_handle(current->pid);

  //printk("[CDCMDBG]@%s()=> <%d> tid[%d] %s called on semID[%d] semptr->wq_usr_val is %d     stptr is %p\n", __FUNCTION__, cdcm_dbg_cntr++, current->pid, __FUNCTION__, semptr->wq_id, *semptr->wq_usr_val, stptr);

  if (*semptr->wq_usr_val > 0) { /* ------------------------------------ */
    --(*semptr->wq_usr_val); /* decrease sem value */
    //printk("[CDCMDBG]@%s()=> Sem is free. New val is %d. Just continue...\n", __FUNCTION__, *semptr->wq_usr_val);
    return(OK);
  } else {			/* ----------------------------------------- */
    --(*semptr->wq_usr_val); /* decrease sem value */
    if (flag == SEM_SIGIGNORE) {

      //printk("[CDCMDBG]@%s()=> tid[%d] Go to wait_event_exclusive() on semID[%d] with semptr->wq_usr_val is %d   semptr->wq_tflag is %d", __FUNCTION__, current->pid, semptr->wq_id, *semptr->wq_usr_val, semptr->wq_tflag);
      if (stptr) { /* thread */
	//printk("   stptr->thr_sem is %d\n", stptr->thr_sem);
	wait_event_exclusive(semptr->wq_head, WC_SSI_THR);
      } else {	/* process */
	//printk("\n");
	wait_event_exclusive(semptr->wq_head, WC_SSI);
      }

      wee = 1;
    } else {	/* all the rest - SEM_SIGRETRY, SEM_SIGABORT */

      //printk("[CDCMDBG]@%s=> tid[%d] Go to wait_event_interruptable_exclusive() on semID %d with semptr->wq_usr_val is %d   semptr->wq_tflag is %d", __FUNCTION__, current->pid, semptr->wq_id, *semptr->wq_usr_val, semptr->wq_tflag);
      if (stptr) { /* thread */
	//printk("   stptr->thr_sem is %d\n", stptr->thr_sem);
	coco = wait_event_interruptible_exclusive(semptr->wq_head, WC_SSO_THR);
      } else {
	//printk("\n");
	coco = wait_event_interruptible_exclusive(semptr->wq_head, WC_SSO);
      }

      if (coco) {
	//printk(KERN_WARNING "[CDCMDBG]@%s()=> Wait interrupted! (ret code %d)\n", __FUNCTION__, coco);
	return(SYSERR);
      }
    }
  }
  /* if we reach this point, then we actually _were_ sleep waiting... */

  /* enter critical region */
  spin_lock_irqsave(&swait_sem_lock, swait_spin_flags);
  //local_irq_save(swait_spin_flags); /* old logic */
  
  //printk("[CDCMDBG]@%s()=> tid[%d] Out from %s(). wq_tflag is %d", __FUNCTION__, current->pid, (wee)?"wait_event_exclusive":"wait_event_interruptible_exclusive", semptr->wq_tflag);

  /*
  if (stptr)
    printk(" thr_sem is %d\n", stptr->thr_sem);
  else
    printk("\n");
  */

  ++(*semptr->wq_usr_val);	/* increasing semaphore val */
  
  /* now check 'wq_tflag' to find out, why we wake up */
  if (semptr->wq_tflag == -1) { /* sreset is playing */

    //printk("[CDCMDBG]@%s()=> tid[%d] Wake up by sreset()\n", __FUNCTION__, current->pid);

    /* leave critical region */
    spin_unlock_irqrestore(&swait_sem_lock, swait_spin_flags);
    return(OK);
  } else if (semptr->wq_tflag > 0) { /* ssignal */
    --semptr->wq_tflag; /* acknowledge */

    //printk("[CDCMDBG]@%s()=> tid[%d] Wake up by ssignal()\n", __FUNCTION__, current->pid);

    /* leave critical region */
    spin_unlock_irqrestore(&swait_sem_lock, swait_spin_flags);
    return(OK);
  } else if ( (stptr) ? (stptr->thr_sem == current->pid) : 0 ) {
    /* __ONLY__ thread can enter here!
       we are killing you (stremove was called), so wake up */

    //printk("[CDCMDBG]@%s()=> tid[%d] Wake up by stremove()\n", __FUNCTION__, current->pid);

    /* leave critical region */
    spin_unlock_irqrestore(&swait_sem_lock, swait_spin_flags);
    return(OK);
  } else { /* complete fuckup. shouldn't be! can't determine, why I wake up! */
    
    printk("[CDCMDBG]@%s() => tid[%d] Wake Up by the FUCK UP!!!!! This shit shouldn't happen!!!! wq_tflag is %d", __FUNCTION__, current->pid, semptr->wq_tflag);
    if (stptr)
      printk(" thr_sem is %d\n", stptr->thr_sem);
    else
      printk("\n");

    /* leave critical region */
    spin_unlock_irqrestore(&swait_sem_lock, swait_spin_flags);
    return(SYSERR);
  }
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    ssignal
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
static DEFINE_SPINLOCK(ssignal_sem_lock);
int
ssignal(
	int *sem) /* user driver semaphore */
{
  cdcmsem_t *semptr = cdcm_get_sem(sem); /* get semaphore info table */
  ulong ssignal_spin_flags;

  if (semptr) {

    //printk("[CDCMDBG] <%d> tid[%d] %s(). Signalling semID %d. semptr->wq_usr_val is %d. wq_tflag is %d\n", cdcm_dbg_cntr++, current->pid, __FUNCTION__, semptr->wq_id, *semptr->wq_usr_val, semptr->wq_tflag);
    
    ++semptr->wq_tflag;		/* set thread flag */

    /* enter critical region */
    spin_lock_irqsave(&ssignal_sem_lock, ssignal_spin_flags);
    if (!(*semptr->wq_usr_val)) {
      /* it was used but no one is waiting on it now, so no need to wake up */
      ++(*semptr->wq_usr_val);	/* in this case we should increase
				   the semaphore val */
      /* leave critical region */
      spin_unlock_irqrestore(&ssignal_sem_lock, ssignal_spin_flags);
      return(OK);
    }
    /* leave critical region */
    spin_unlock_irqrestore(&ssignal_sem_lock, ssignal_spin_flags);
    
    wake_up(&semptr->wq_head);	/* wake up one of the waiting streams */
    return(OK);
  }

  printk("[CDCMDBG]@%s()=> Can't get handle for %#x sema!\n", __FUNCTION__, (int)sem);
  return(SYSERR);
}


/*-----------------------------------------------------------------------------
 * FUNCTION:    sreset
 * DESCRIPTION: LynxOs service call wrapper.
 * RETURNS:	
 *-----------------------------------------------------------------------------
 */
void 
sreset(
       int *sem) /* user driver semaphore */
{
  cdcmsem_t *semptr = cdcm_get_sem(sem);

  /* TODO Should i call wake_up or wake_up_all? */

  if (semptr) {
    semptr->wq_tflag = -1;	/* se mua */
    //wake_up(&semptr->wq_head);
    wake_up_all(&semptr->wq_head);
    return; /* OK */
  }

  printk("[CDCMDBG]@%s()=> Can't get handle for %#x sema!\n", __FUNCTION__, (int)sem);
  return; /* SYSERR */
}
