/**
 * @file cdcmTime.c
 *
 * @brief All timing-related staff
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 07/07/2006
 *
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 * All timing-related staff for CDCM is located here, i.e. Lynx stub functions
 * and supplementary functions.
 *
 * @version 
 */
#include <linux/time.h>
#include <linux/delay.h>
#include "list_extra.h" /* for extra handy list operations */
#include "cdcmTime.h"
#include "cdcmThread.h"
#include "cdcmLynxDefs.h"

extern cdcmStatics_t cdcmStatT;	/* declared in the cdcmDrvr.c module */
extern int cdcm_dbg_cntr; /* TODO. REMOVE. For CDCM debugging only. 
			     Defined in cdcm.c module */
int cdcm_dbg_irq = 0; /* TODO. REMOVE. For iterrupts debugging only. */


extern cdcmthr_t* cdcm_get_thread_handle(int);

/**
 * @brief busy loop for at least the requested number of microseconds
 *
 * @param usecs - microseconds
 */
void usec_sleep(unsigned long usecs)
{
       udelay(usecs);
}

/**
 * @brief Generic timer callback.
 *
 * @param arg - Index of the timer data. Assume to be a correct one.
 *
 * @return void
 */
static void cdcm_timer_callback(unsigned long arg)
{
  cdcmt_t *my_data = &cdcmStatT.cdcm_timer[arg];

  del_timer(&my_data->ct_timer);
  my_data->ct_on = 0;
  //PRNT_DBG(cdcmStatT.cdcm_ipl, "is called. Index[%d]", (int)arg);
  my_data->ct_uf(my_data->ct_arg); /* call user payload */
}


/**
 * @brief LynxOs service call wrapper.
 *
 * @param sec - current time in seconds is placed here
 *
 * @return fraction of a second in nanoseconds.
 *         @e sec will hold current time in seconds.
 */
int nanotime(unsigned long *sec)
{
  struct timespec curtime;

  curtime = current_kernel_time();

  *sec = (unsigned long)curtime.tv_sec;
  
  return((int)curtime.tv_nsec);
}


/**
 * @brief LynxOs service call wrapper.
 *
 * @param func     - function to call
 * @param arg      - argument for function handler
 * @param interval - timeout interval (10 millisecond granularity)
 *
 * @return non-negative timeout id - if there is a timer available.
 * @return SYSERR (i.e. -1)        - otherwise.
 */
int timeout(tpp_t func, void *arg, int interval)
{
  int cntr;
  //cdcmt_t *cdcmtptr; /* timer handle */

  for (cntr = 0; cntr < MAX_CDCM_TIMERS; cntr++) {
    if (!cdcmStatT.cdcm_timer[cntr].ct_on) { /* not used */
      /* initialize for usage */
      cdcmStatT.cdcm_timer[cntr] = (cdcmt_t) {
	.ct_on      = current->pid,
	/* TIMER_INITIALIZER(payload, when to trigger, payload parameter) */
	.ct_timer = TIMER_INITIALIZER(cdcm_timer_callback, jiffies + msecs_to_jiffies(10*interval), cntr),
	.ct_uf  = func,
	.ct_arg = arg
      };

      init_timer(&cdcmStatT.cdcm_timer[cntr].ct_timer);
      add_timer(&cdcmStatT.cdcm_timer[cntr].ct_timer);

      //PRNT_DBG(cdcmStatT.cdcm_ipl, "<%d> for tid[%d]. TimerID[%d] with interval %d ADDED", cdcm_dbg_cntr++, current->pid, cntr+1, interval);
      return(cntr + 1); /* timeout ID */
    }
  }
  
  PRNT_ERR(cdcmStatT.cdcm_ipl, "No more timers (MAX %d)", MAX_CDCM_TIMERS);
  return(SYSERR);
}


/**
 * @brief LynxOs service call wrapper.
 *
 * @param arg - timeout id
 *
 * @return 
 */
int cancel_timeout(int arg)
{
  int idx = arg - 1; /* who should die */
  
  if ( idx >= 0 && idx < MAX_CDCM_TIMERS) {
    if (cdcmStatT.cdcm_timer[idx].ct_on) {
      del_timer_sync(&cdcmStatT.cdcm_timer[idx].ct_timer);
      cdcmStatT.cdcm_timer[idx].ct_on = 0;
      //PRNT_DBG(cdcmStatT.cdcm_ipl, "timeout %d cancelled.", arg);
      return(OK);
    }
    
    //PRNT_DBG(cdcmStatT.cdcm_ipl, "Timer %d already expired.", arg);
    return(OK);
  }

  PRNT_ERR(cdcmStatT.cdcm_ipl, "No such timer: %d", arg);
  return(SYSERR);
}


/**
 * @brief Gets wait queue for a given semaphore.
 *
 * @param sem - user-defined driver semaphore
 *
 * Or if it's a new one, try to obtain a new one.
 *
 * @return @e wait_queue_head_t pointer that corresponds to the semaphore
 *	   @e sem (either new one or that is already exist).
 * @return NULL - can't allocate semaphore handle.
 */
spinlock_t get_sem_lock = SPIN_LOCK_UNLOCKED;
static cdcmsem_t* cdcm_get_sem(int *sem)
{
  cdcmsem_t *semptr; /* semaphore pointer */
  ulong spin_flags;
  
  /* see if user wants already used semaphore */
  list_for_each_entry(semptr, &cdcmStatT.cdcm_sem_list_head, wq_sem_list) {
    if (sem == semptr->wq_usr_val) { /* bingo! */
      //PRNT_DBG(cdcmStatT.cdcm_ipl, "tid [%d] return -->OLD<-- semID[%d]\n", current->pid, semptr->wq_id);
      return(semptr);
    }
  }
  
  /* it's a new semaphore. allocate the place */

  /* enter critical region */
  spin_lock_irqsave(&get_sem_lock, spin_flags);

  /* allocate and initialize stream task handler */
  if ( !(semptr = kmalloc(sizeof *semptr, GFP_KERNEL)) ) {
    PRNT_ABS_WARN("Couldn't allocate new thread handle");
    return NULL;
  }

  /* init semaphore handle (also will zero it out) */
  *semptr = (cdcmsem_t) {
    .wq_usr_val  = sem,     /* take it */
    .wq_init_val = *sem,    /* set init user value */
    .wq_masta    = current, /* set my masta */
    .wq_tflag    = ATOMIC_INIT(0),  /* thread flag */
    .wq_id = list_capacity(&cdcmStatT.cdcm_sem_list_head) + 1 /* my id */
  };
  init_waitqueue_head(&semptr->wq_head);

  /* add it to the linked list */
  list_add(&semptr->wq_sem_list, &cdcmStatT.cdcm_sem_list_head);

  //PRNT_DBG(cdcmStatT.cdcm_ipl, "return -->NEW<-- semID[%d]\n", semptr->wq_id);
  
  /* leave critical region */
  spin_unlock_irqrestore(&get_sem_lock, spin_flags);

  return(semptr);
}


/* One of the events that we are waiting on.
   Depending on the caller (thread or major process),
   different wait conditions are monitored

   Acronyms description:
   WC  - Wait Condition
   SSI - Sem Sig Ignore flag (SEM_SIGIGNORE)
   SSO - Sem Sig all Other flags (SEM_SIGRETRY, SEM_SIGABORT)
   THR - in case if it was called from the THRead
*/
/*
#define WC_SSI_THR (semptr->wq_tflag > 0 || semptr->wq_tflag < 0 || (stptr->thr_sem == current->pid))
#define WC_SSI (semptr->wq_tflag > 0 || semptr->wq_tflag < 0)

#define WC_SSO_THR (semptr->wq_tflag > 0 || semptr->wq_tflag < 0 || (stptr->thr_sem == current->pid))
#define WC_SSO (semptr->wq_tflag > 0 || semptr->wq_tflag < 0)
*/

#define WC_SSI_THR (atomic_read(&semptr->wq_tflag) || (stptr->thr_sem == current->pid))
#define WC_SSI (atomic_read(&semptr->wq_tflag))

#define WC_SSO_THR (atomic_read(&semptr->wq_tflag) || (stptr->thr_sem == current->pid))
#define WC_SSO (atomic_read(&semptr->wq_tflag))


/* enter critical region */
#define ENTER_SWAIT_CRITICAL spin_lock_irqsave(&swait_sem_lock, swait_spin_flags)

/* leave critical region */
#define LEAVE_SWAIT_CRITICAL spin_unlock_irqrestore(&swait_sem_lock, swait_spin_flags)

static DEFINE_SPINLOCK(swait_sem_lock);
/**
 * @brief  LynxOs service call wrapper.
 *
 * @param sem  - user driver semaphore
 * @param flag - one of SEM_SIGIGNORE, SEM_SIGRETRY, SEM_SIGABORT
 *
 * @return 
 */

/*
extern short *ext_reg2;
#define PURGE_CPUPIPELINE asm("eieio")
#define MEAS_Wreg2(X) { *ext_reg2 = (X); PURGE_CPUPIPELINE; }
*/

int swait(int *sem, int flag)
{
  int coco = 0;
  cdcmsem_t *semptr = cdcm_get_sem(sem); /* get new or existing one */
  cdcmthr_t *stptr = NULL;
  ulong swait_spin_flags;
  volatile int wee = 0;

  if (!semptr) {
    PRNT_ABS_ERR("EFAULT: No more free wait queues!");
    return SYSERR;
  }

  /* enter critical region */
  ENTER_SWAIT_CRITICAL;

  /* check who is calling - kernel thread or normal process. If stptr is NULL, 
     then swait is called from the process, but __not__ from the kthread */
  stptr = cdcm_get_thread_handle(current->pid);

  /*
  MEAS_Wreg2(0xdead);
  MEAS_Wreg2(cdcm_dbg_irq);
  MEAS_Wreg2(*semptr->wq_usr_val);
  */

  //printk("[CDCMDBG]@%s()=> (tid[%d] <%d>) Called on semID[%d] semptr->wq_usr_val is %d     stptr is %p\n", __FUNCTION__,  current->pid, cdcm_dbg_cntr++, semptr->wq_id, *semptr->wq_usr_val, stptr);

  if (*semptr->wq_usr_val > 0) { /* ----> NO NEED TO WAIT <---- */
    --(*semptr->wq_usr_val); /* just decrease sem value */

    //printk("[CDCMDBG]@%s()=> (tid[%d] <%d>) Sem is free. New val is %d. Just continue...\n", __FUNCTION__, current->pid, cdcm_dbg_cntr++, *semptr->wq_usr_val);
    LEAVE_SWAIT_CRITICAL;

    /*
    MEAS_Wreg2(0xdea1);
    MEAS_Wreg2(*semptr->wq_usr_val);
    */

    return(OK);
  } else { /* ----> NEED TO WAIT <---- */
    --(*semptr->wq_usr_val); /* decrease sem value */

    /*
    MEAS_Wreg2(0xdea2);
    MEAS_Wreg2(*semptr->wq_usr_val);
    */

    if (flag == SEM_SIGIGNORE) {

      //printk("[CDCMDBG]@%s()=> tid[%d] Go to wait_event_exclusive() on semID[%d] with semptr->wq_usr_val is %d   semptr->wq_tflag is %d", __FUNCTION__, current->pid, semptr->wq_id, *semptr->wq_usr_val, atomic_read(&semptr->wq_tflag));
      if (stptr) { /* thread */
	//printk("   stptr->thr_sem is %d\n", stptr->thr_sem);

	LEAVE_SWAIT_CRITICAL;

	wait_event_exclusive(semptr->wq_head, WC_SSI_THR);
      } else {	/* process */
	//printk("\n");

	/*
	MEAS_Wreg2(0xde21);
	MEAS_Wreg2(atomic_read(&semptr->wq_tflag));
	*/

	LEAVE_SWAIT_CRITICAL;

	/*
	MEAS_Wreg2(0xde22);
	MEAS_Wreg2(atomic_read(&semptr->wq_tflag));
	*/

	wait_event_exclusive(semptr->wq_head, WC_SSI);

	/*
	MEAS_Wreg2(0xde23);
	MEAS_Wreg2(atomic_read(&semptr->wq_tflag));
	*/

      }

      wee = 1;
    } else {	/* all the rest - SEM_SIGRETRY, SEM_SIGABORT */

      //printk("[CDCMDBG]@%s=> tid[%d] Go to wait_event_interruptable_exclusive() on semID %d with semptr->wq_usr_val is %d   semptr->wq_tflag is %d", __FUNCTION__, current->pid, semptr->wq_id, *semptr->wq_usr_val, atomic_read(&semptr->wq_tflag));
      if (stptr) { /* thread */
	//printk("   stptr->thr_sem is %d\n", stptr->thr_sem);
	LEAVE_SWAIT_CRITICAL;
	coco = wait_event_interruptible_exclusive(semptr->wq_head, WC_SSO_THR);
      } else {
	//printk("\n");
	LEAVE_SWAIT_CRITICAL;
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
  ENTER_SWAIT_CRITICAL;
  
  //printk("[CDCMDBG]@%s()=> tid[%d] Out from %s(). wq_tflag is %d", __FUNCTION__, current->pid, (wee)?"wait_event_exclusive":"wait_event_interruptible_exclusive", atomic_read(&semptr->wq_tflag));

#if 0
  if (stptr)
    printk(" thr_sem is %d\n", stptr->thr_sem);
  else
    printk("\n");
#endif

  (*semptr->wq_usr_val)++;	/* increasing semaphore val */

  /*
  MEAS_Wreg2(0xdea3);
  MEAS_Wreg2(*semptr->wq_usr_val);
  */

  /* now check 'wq_tflag' to find out, why we wake up */
  if (atomic_read(&semptr->wq_tflag) < 0) {
    /* ------> wake up by sreset <-------- */
    //printk("[CDCMDBG]@%s()=> tid[%d] Wake up by sreset()\n", __FUNCTION__, current->pid);
    if (atomic_inc_and_test(&semptr->wq_tflag))
      /* last one - set semaphore value to zero */
      *semptr->wq_usr_val = 0;
    
    /*
      ++semptr->wq_tflag;
      if (!semptr->wq_tflag)
      *semptr->wq_usr_val = 0;
      */
    
    /* leave critical region */
    LEAVE_SWAIT_CRITICAL;
    return(OK);
  } else if (atomic_read(&semptr->wq_tflag) > 0) {
    /* ------> wake up by ssignal or ssignaln <-------- */
    atomic_dec(&semptr->wq_tflag); /* acknowledge */

    //printk("[CDCMDBG]@%s()=> tid[%d] Wake up by ssignal()\n", __FUNCTION__, current->pid);

    /* leave critical region */
    LEAVE_SWAIT_CRITICAL;
    return(OK);
  } else if ( (stptr) ? (stptr->thr_sem == current->pid) : 0 ) {
    /* __ONLY__ thread can enter here!
       we are killing you (stremove was called), so just wake up */

    //printk("[CDCMDBG]@%s()=> tid[%d] Wake up by stremove()\n", __FUNCTION__, current->pid);

    /* leave critical region */
    LEAVE_SWAIT_CRITICAL;
    return(OK);
  } else { /* complete fuckup. shouldn't be! can't determine, why I wake up! */
    
    PRNT_ABS_ERR("tid[%d] Wake Up by the FUCK UP!!!! This shouldn't happen!!!"
		 "wq_tflag is %d", current->pid,
		 atomic_read(&semptr->wq_tflag));
#if 0
    if (stptr)
      printk(" thr_sem is %d\n", stptr->thr_sem);
    else
      printk("\n");
#endif

    /* leave critical region */
    LEAVE_SWAIT_CRITICAL;
    return(SYSERR);
  }

}


/**
 * @brief LynxOs service call wrapper.
 *
 * @param sem - user semaphore
 *
 * @return 
 */
static DEFINE_SPINLOCK(ssignal_sem_lock);
int ssignal(int *sem)
{
  cdcmsem_t *semptr = cdcm_get_sem(sem); /* get semaphore info table */
  ulong ssignal_spin_flags;

  if (semptr) {

    //printk("[CDCMDBG]@%s()=> tid[%d] <%d>  Signalling semID[%d]. semptr->wq_usr_val is %d. wq_tflag is %d\n", __FUNCTION__,current->pid,  cdcm_dbg_cntr++,  semptr->wq_id, *semptr->wq_usr_val, atomic_read(&semptr->wq_tflag));

    /* enter critical region */
    spin_lock_irqsave(&ssignal_sem_lock, ssignal_spin_flags);

    atomic_inc(&semptr->wq_tflag); /* set thread flag */

    /*
    MEAS_Wreg2(0xaaaa);
    MEAS_Wreg2(atomic_read(&semptr->wq_tflag));
    MEAS_Wreg2(*semptr->wq_usr_val);
    */
    
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
    
    /* wake up one of the waiting streams */
    wake_up(&semptr->wq_head);
    return(OK);
  }

  PRNT_ERR(cdcmStatT.cdcm_ipl, "Can't get handle for %#x sema!\n", (int)sem);
  return(SYSERR);
}


/**
 * @brief LynxOs service call wrapper. !TODO!
 *
 * @param sem   - user semaphore
 * @param count - how many times to signal semaphore
 *
 * @return 
 */
int ssignaln(int *sem, int count)
{
  cdcmsem_t *semptr = cdcm_get_sem(sem); /* get semaphore info table */

  if (semptr) {
    return(OK);
  }

  PRNT_ERR(cdcmStatT.cdcm_ipl, "Can't get handle for %#x sema!\n", (int)sem);
  return(SYSERR);
}


/**
 * @brief LynxOs service call wrapper.
 *
 * @param sem - user semaphore
 *
 * @return void
 */
static DEFINE_SPINLOCK(sreset_sem_lock);
void sreset(int *sem)
{
  cdcmsem_t *semptr = cdcm_get_sem(sem);
  ulong sreset_spin_flags;
  int ant; /* anticipants */

  if (semptr) {
    
    /*
    MEAS_Wreg2(0xbeef);
    MEAS_Wreg2(*semptr->wq_usr_val);
    */

    /* enter critical region */
    spin_lock_irqsave(&sreset_sem_lock, sreset_spin_flags);

    if ( !(ant = list_capacity(&(semptr->wq_head.task_list))) ) {
      /* no one is waiting on this sema, no need to wake up someone */
      *semptr->wq_usr_val = 0;
      atomic_set(&semptr->wq_tflag, 0);

      /* leave critical region */
      spin_unlock_irqrestore(&sreset_sem_lock, sreset_spin_flags);

      return; /* we done */
    }
    
    /* if here, then there are anticipants on this semaphore. Wake'em up */
    atomic_set(&semptr->wq_tflag, (-1 * ant)); /* set wakeup flag */

    //MEAS_Wreg2(atomic_read(&semptr->wq_tflag));

    /* leave critical region */
    spin_unlock_irqrestore(&sreset_sem_lock, sreset_spin_flags);

    /* wake up everyone who is waiting on this semaphore */
    wake_up_all(&semptr->wq_head);
    return; /* OK */
  }

  /* ERROR! should not get here! */
  PRNT_ABS_ERR("Can't get handle for %#x sema!\n", (int)sem);
  return; /* SYSERR */
}


/**
 * @brief LynxOs service call wrapper
 *
 * @param sem - semaphore in question
 *
 *  Returns the value of the semaphore sem. If the count is negative, it
 * indicates the number of processes that are waiting on the semaphore.
 *  If count is zero, no processes are waiting on sem. If count is greater
 * than zero, it represents the number of times it can be "waited" on
 * without having to wait for the semaphore to be signaled.
 *
 * @return the value of the semaphore sem - if OK.
 * @return SYSERR - if there is no such semaphore.
 */
int scount(int *sem)
{
  cdcmsem_t *semptr = cdcm_get_sem(sem); /* get semaphore info table */

  if (!semptr) {
    //printk("[CDCMDBG]@%s()=> Can't get handle for %#x sema!\n", __FUNCTION__, (int)sem);
    return(SYSERR);
  }
  
  return(*semptr->wq_usr_val);
}

/**
 * @brief Cleanup all allocated semaphores.
 *
 * @param void
 *
 * @return void
 */
void cdcm_sema_cleanup_all(void)
{
  cdcmsem_t *semP, *tmpP;
  
  /* free allocated memory */
  list_for_each_entry_safe(semP, tmpP, &cdcmStatT.cdcm_sem_list_head, wq_sem_list) {
    //PRNT_DBG(cdcmStatT.cdcm_ipl, "Releasing semaphore [ID%d]\n", semP->wq_id);
    list_del(&semP->wq_sem_list);
    kfree(semP);
  }
}
