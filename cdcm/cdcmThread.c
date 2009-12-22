/**
 * @file cdcmThread.c
 *
 * @brief LynxOS thread implementation.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 02/06/2006
 *
 * All, that concerning threads is located here, i.e. all stub functions and
 * supplementary functions.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 */
#include "cdcmDrvr.h"
#include "cdcmThread.h"

/* global CDCM statics table */
extern cdcmStatics_t cdcmStatT;


/* if you know the better way to pass arbitrary number of parameters to
   the function - let me know pls... (IMHO __builtin_apply_args() and co.
   is not fit here) */
#define ARGS0 tparam.tp_payload()
#define ARGS1 tparam.tp_payload(tparam.tp_args[0])
#define ARGS2 tparam.tp_payload(tparam.tp_args[0], tparam.tp_args[1])
#define ARGS3 tparam.tp_payload(tparam.tp_args[0], tparam.tp_args[1], tparam.tp_args[2])
#define ARGS4 tparam.tp_payload(tparam.tp_args[0], tparam.tp_args[1], tparam.tp_args[2], tparam.tp_args[3])
#define ARGS5 tparam.tp_payload(tparam.tp_args[0], tparam.tp_args[1], tparam.tp_args[2], tparam.tp_args[3], tparam.tp_args[4])
#define ARGS6 tparam.tp_payload(tparam.tp_args[0], tparam.tp_args[1], tparam.tp_args[2], tparam.tp_args[3], tparam.tp_args[4], tparam.tp_args[5])
/**
 * @brief
 *
 * @param data -
 *
 * @return The return value should be zero or a negative error number.
 *         It will be passed to @e kthread_stop().
 */
static int cdcm_local_thread(void *data)
{
  cdcmtpar_t tparam = *(cdcmtpar_t *)data; /* put param in local space */

  /* set my process descriptor now, because we need it soon */
  tparam.tp_handle->thr_pd = current;
  complete(&tparam.tp_handle->thr_c); /* notify, that we start */

  switch (tparam.tp_argn) {
  case 0:
    ARGS0;
    break;
  case 1:
    ARGS1;
    break;
  case 2:
    ARGS2;
    break;
  case 3:
    ARGS3;
    break;
  case 4:
    ARGS4;
    break;
  case 5:
    ARGS5;
    break;
  case 6:
    ARGS6;
    break;
  default:
	  PRNT_ABS_ERR("Too much args (%d) for stream task payload!",
		       tparam.tp_argn);
    return EINVAL;
    break;
  }

  /* if here - thread receive termination order. die... */

  printk("\n[CDCM] [%d] (aka %s) Receive termination order!\n", tparam.tp_handle->thr_pd->pid, tparam.tp_handle->thr_nm);

  if (!tparam.tp_handle->thr_pd) {
    printk("\n--------------------->FUCKEN SHIT!! WE SHOULDN'T BE HERE!!!!\n");
    /* checks if parameter is already initialized. It should be assigned
       with the 'kthread_run()' return value. If it's still not done - wait
       for it. */
    //INIT_COMPLETION(tparam.tp_handle->thr_c);
    /* in 2.6.9 still no _interruptible completion */
    //wait_for_completion_interruptible(&tparam.tp_handle->thr_c);
    //wait_for_completion(&tparam.tp_handle->thr_c);
  }

  PRNT_DBG(cdcmStatT.cdcm_ipl, "[%d] (aka %s) exits...\n",
	   tparam.tp_handle->thr_pd->pid, tparam.tp_handle->thr_nm);

  return(0); /* all OK */
}


/**
 * @brief Obtains thread handle structure based on the thread id.
 *
 * @param stid -
 *
 * @return thread handle pointer - if SUCCESS.
 * @return NULL                  - otherwise.
 */
cdcmthr_t* cdcm_get_thread_handle(int stid)
{
  cdcmthr_t *stptr; /* stream task pointer */

  list_for_each_entry(stptr, &cdcmStatT.cdcm_thr_list_head, thr_list) {
    if (stptr->thr_pd && stptr->thr_pd->pid == stid)
      return(stptr);
  }

  return(NULL);	/* not found */

}


/**
 * @brief Lynx stub.
 *
 * @param procaddr  - payload
 * @param stacksize - thread stack size
 * @param prio      - priority
 * @param name      - desired thread name
 * @param nargs     - number of arguments
 * @param ...       - args to pass to the stream task
 *
 * @return stid (stream task ID) - if success.
 * @return SYSERR (-1)           - if fails.
 */
int ststart(tpp_t procaddr, int stacksize, int prio, char *name, int nargs, ...)
{
  int cntr;
  cdcmtpar_t tparp; /* multiple items are bundled as a single data structure */
  va_list argptr;
  cdcmthr_t *cdcmthrp;
  struct task_struct *coco;

  /* TODO. Any solution is welcome to bypass this limitation... */
  if (nargs > CDCM_MAX_THR_ARGS)
    return(SYSERR);

  if (strlen(name) > CDCM_TNL)
	  PRNT_ABS_WARN("Desired thread name is too long (%d char > %d char"
			"MAX). Will be truncated!\n", strlen(name), CDCM_TNL);

  /* allocate and initialize stream task handler */
  if ( !(cdcmthrp = kmalloc(sizeof *cdcmthrp, GFP_KERNEL)) ) {
	  PRNT_ABS_WARN("Couldn't allocate new thread handle");
    return SYSERR;
  }

  /* init thread handle (also will zero it out) */
  *cdcmthrp = (cdcmthr_t) {
    .thr_pd    = NULL,
    .thr_c     = COMPLETION_INITIALIZER(cdcmthrp->thr_c)
  };

  /* set stream task name */
  strncpy(cdcmthrp->thr_nm, name, CDCM_TNL - 1);

  /* add it to the linked list */
  list_add(&cdcmthrp->thr_list, &cdcmStatT.cdcm_thr_list_head);

  /* init some params (will be zero out by the compiler) */
  tparp = (cdcmtpar_t) {
    .tp_payload = procaddr,
    .tp_argn    = nargs,
    .tp_handle  = cdcmthrp
  };

  /* get payload arguments and put them into the thread parameter struct */
  va_start(argptr, nargs);
  for (cntr = 0; cntr < nargs; cntr++)
    tparp.tp_args[cntr] = va_arg(argptr, char *);
  va_end(argptr);

  /* Start up our control thread */
  coco = kthread_run(cdcm_local_thread, &tparp, cdcmthrp->thr_nm);

  if (IS_ERR(coco)) {
	  PRNT_ABS_ERR("Unable to start '%s' control thread",
		       cdcmthrp->thr_nm);
    return SYSERR;
  }

  /* wait while crusial info will be set up (cdcmthrp->thr_pd pointer) */
  wait_for_completion(&cdcmthrp->thr_c);

  printk("-----------> kthread %d started\n", cdcmthrp->thr_pd->pid);

  return(cdcmthrp->thr_pd->pid);
}

/**
 * @brief Lynx stub. Remove user-defined kernel thread.
 *
 * @param stid - stream task ID
 *
 * We should take care about all pending signals, semaphores etc...
 *
 * @return void
 */
void stremove(int stid)
{
	struct task_struct *stpd;
	cdcmthr_t *stptr; /* victim data */
	int cntr;
	ulong iflags;

	rcu_read_lock();
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
	stpd = find_task_by_pid_ns(stid, &init_pid_ns);
#else
	stpd = pid_task(find_pid_ns(stid, &init_pid_ns), PIDTYPE_PID);
#endif
	rcu_read_unlock();

	if (!stpd) { /* bugaga! */
		PRNT_DBG(cdcmStatT.cdcm_ipl,
			 "Can't stop thread! No such tid (%d)", stid);
		return;
	}

	PRNT_ABS_INFO("%s() Stopping %d tid.", __func__, stid);

	if ( !(stptr = cdcm_get_thread_handle(stid)) ) {
		PRNT_DBG(cdcmStatT.cdcm_ipl,
			 "Can't stop thread! No thread handle for tid (%d)",
			 stid);
		return;
	}

	/* cleanup timeouts (if any) */
	for (cntr = 0; cntr < MAX_CDCM_TIMERS; cntr++)
		if (cdcmStatT.cdcm_timer[cntr].ct_on == stid) {
			del_timer_sync(&cdcmStatT.cdcm_timer[cntr].ct_timer);
			cdcmStatT.cdcm_timer[cntr].ct_on = 0;
		}

	/* we should protect critical region here */
	local_irq_save(iflags);
	printk("\t[CDCM] %s() tid[%d] Set victim wake up flag\n",
	       __func__, stid);
	stptr->thr_sem = stid;	/* denotes termination order */


	/*
	 * This synchronous operation  will wake the kthread if it is
	 * asleep and will return when thread has terminated.
	 */
	//printk("\t%s() calling kthread_stop()...\n", __func__);
	kthread_stop(stpd);
	list_del(&stptr->thr_list); /* exclude me from the list */
	//printk("\t%s(): kthread_stop() return.\n", __func__);

	local_irq_restore(iflags);
}
