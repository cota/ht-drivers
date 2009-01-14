/**
 * @file cdcmDrvr.c
 *
 * @brief CDCM Linux driver
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 02/06/2006
 *
 * Encapsulates user-written LynxOS driver. Consider it like a wrapper of the
 * user Lynx driver.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version $Id: cdcmDrvr.c,v 1.7 2009/01/09 10:26:03 ygeorgie Exp $
 */
#include "cdcmDrvr.h"
#include "cdcmLynxAPI.h"
#include "cdcmMem.h"
#include "cdcmTime.h"
#include "cdcmUsrKern.h"
#include "general_both.h" /* for WITHIN_RANGE */

MODULE_DESCRIPTION("Common Driver Code Manager (CDCM) Driver");
MODULE_AUTHOR("Yury Georgievskiy, CERN AB/CO");
MODULE_LICENSE("GPL");

/* driver parameter. Info file path. */
static char cdcmInfoF[PATH_MAX];
module_param_string(ipath, cdcmInfoF, sizeof(cdcmInfoF), 0);
MODULE_PARM_DESC(ipath, "Absolute path name of the CDCM info file.");

/* external crap */
extern char* cdcm_get_info_table(char*, int*);
extern void  cdcm_pci_cleanup(void);
#if (CPU != L864 && CPU != L865)		/* TODO Quick fixup for xmem driver */
extern void cdcm_vme_cleanup(void);
#endif

extern struct dldd entry_points; /* declared in the user driver part */

/* general statics data that holds all information about current groups,
   devices, threads, allocated mem, etc. managed by CDCM */
cdcmStatics_t cdcmStatT = {
  .cdcm_grp_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_grp_list_head),
  .cdcm_drvr_type       = 0,	/* 'P'(for PCI) or 'V' (for VME) */
  .cdcm_mem_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_mem_list_head),
  .cdcm_ipl             = (IPL_ERROR | IPL_INFO), /* info printout level */
  .cdcm_thr_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_thr_list_head),
  .cdcm_sem_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_sem_list_head),
  .cdcm_flags           = 0
};

struct list_head *glob_cur_grp_ptr = NULL; /* current working group pointer */

/* LynxOs system kernel-level errno. is set _ONLY_ by 'pseterr' function */
int cdcm_err = 0;

spinlock_t lynxos_cpu_lock = SPIN_LOCK_UNLOCKED;

struct file_operations cdcm_fops;

/* get group info table from the given major. NULL - if not found */
#define from_maj_to_grp(_maj)						\
({									\
  __label__ done;							\
  struct cdcm_grp_info_t *_grp;						\
  struct cdcm_grp_info_t *_res = NULL;					\
									\
  list_for_each_entry(_grp, &cdcmStatT.cdcm_grp_list_head, grp_list)	\
    if (_grp->grp_maj == _maj) { /* bingo */				\
      _res = _grp;							\
      goto done;							\
    }									\
									\
  done:									\
  (_res);								\
})


/**
 * @brief Get statics table
 *
 * @param cur_maj - major number
 *
 * Will be passed as a parameter to the driver entry points.
 *
 * @return user-defined statics table - if OK
 * @return NULL                       - if FAILS (i.e. no group with
 *						  such major number)
 */
static char* cdcm_get_grp_stat_tbl(int cur_maj)
{
  struct cdcm_grp_info_t *grp_it = from_maj_to_grp(cur_maj);
  return(grp_it ? grp_it->grp_stat_tbl : NULL);
}


/**
 * @brief Cleanup and release claimed groups.
 *
 * @param doomed - if to kill only one, then not NULL.
 *
 * @return void
 */
static void cdcm_cleanup_groups(struct cdcm_grp_info_t *doomed)
{
  struct cdcm_grp_info_t *grpp, *tmpp;
  
  if (doomed) {		/* only one */
    unregister_chrdev(doomed->grp_maj, doomed->grp_name);
    list_del(&doomed->grp_list);
    
    kfree(doomed);
  } else {		/* all that still left */
    list_for_each_entry_safe(grpp, tmpp, &cdcmStatT.cdcm_grp_list_head, grp_list) {
      unregister_chrdev(grpp->grp_maj, grpp->grp_name);
      list_del(&grpp->grp_list);
      kfree(grpp);
    }
  }
}


/**
 * @brief Lynx read stub.
 *
 * @param filp - file struct pointer
 * @param buf  - user space
 * @param size - how many bytes to read
 * @param off  - offset
 *
 * @return how many bytes was read - if success.
 * @return negative value          - if error.
 */
static ssize_t cdcm_fop_read(struct file *filp, char __user *buf, size_t size, loff_t *off)
{
  char *iobuf  = NULL;
  char *grp_statt = NULL; /* group statics table */
  struct cdcm_file lynx_file = {
    .dev = filp->f_dentry->d_inode->i_rdev, /* set Device Number */ 
    .access_mode = (filp->f_flags & O_ACCMODE),
    .position = *off
  };

  PRNT_DBG("Read device with minor = %d", MINOR(lynx_file.dev));
  cdcm_err = 0;	/* reset */

  if ( !(grp_statt = cdcm_get_grp_stat_tbl(imajor(filp->f_dentry->d_inode))) )
    /* can't find group with given major number */
    return(-ENXIO);
  
  if (!access_ok(VERIFY_WRITE, buf, size)) {
    PRNT_ABS_ERR("Can't verify user buffer for writing.");
    return(-EFAULT);
  }

  if ( (iobuf = cdcm_mem_alloc(size, _IOC_READ|CDCM_M_FLG_READ)) == ERR_PTR(-ENOMEM))
    return(-ENOMEM);
  
  
  cdcm_err = entry_points.dldd_read(grp_statt, &lynx_file, iobuf, size);
  if (cdcm_err == SYSERR)
    cdcm_err = -EAGAIN;
  else {
    if (__copy_to_user(buf,  iobuf, size))
      return(-EFAULT);
    *off = lynx_file.position;
  }

  cdcm_mem_free(iobuf);

  return(cdcm_err);
}


/**
 * @brief Lynx write stub.
 *
 * @param filp - file struct pointer
 * @param buf  - user space
 * @param size - how many bytes to write
 * @param off  - offset
 *
 * @return how many bytes was written - if success.
 * @return negative value             - if error.
 */
static ssize_t cdcm_fop_write(struct file *filp, const char __user *buf, size_t size, loff_t *off)
{
  char *iobuf = NULL;
  char *grp_statt = NULL; /* group statics table */
  struct cdcm_file lynx_file = {
    .dev = filp->f_dentry->d_inode->i_rdev, /* set Device Number */  
    .access_mode = (filp->f_flags & O_ACCMODE),
    .position = *off
  };
  
  PRNT_DBG("Write device with minor = %d", MINOR(lynx_file.dev));
  cdcm_err = 0; /* reset */

  if ( !(grp_statt = cdcm_get_grp_stat_tbl(imajor(filp->f_dentry->d_inode))) )
    /* can't find group with given major number */
    return(-ENXIO);

  if (!access_ok(VERIFY_READ, buf, size)) {
    PRNT_ABS_ERR("Can't verify user buffer for reading.");
    return(-EFAULT);
  }

  if ( (iobuf = cdcm_mem_alloc(size, _IOC_WRITE|CDCM_M_FLG_WRITE)) == ERR_PTR(-ENOMEM))
    return(-ENOMEM);
  
  __copy_from_user(iobuf, buf, size);

  cdcm_err = entry_points.dldd_write(grp_statt, &lynx_file, iobuf, size);
  if (cdcm_err SYSERR)
    cdcm_err = -EAGAIN;
  else
    *off = lynx_file.position;

  cdcm_mem_free(iobuf);

  return(cdcm_err);
}


/**
 * @brief Lynx select stub. ---------> !NOT IMPLEMENTED! <--------------------
 *
 * @param filp - file struct pointer
 * @param wait - 
 *
 * I see no way to get descriptor set (in, out, except) to pass them as a third
 * parameter (condition to monitor - SREAD, SWRITE, SEXCEPT) to the LynxOS
 * @e select() driver entry point. We are entering @e cdcm_fop_poll() from
 * @e do_select(), but descriptor set stays in <b>fd_set_bits *fds</b> var of
 * @e do_select(). IMHO there is no way to hook it somehow... Fuck!
 *
 * @return 
 */
static unsigned int cdcm_fop_poll(struct file* filp, poll_table* wait)
{
  int mask = POLLERR;
  struct cdcm_sel sel;
  struct cdcm_file lynx_file;
  char *grp_statt = NULL; /* group statics table */

  lynx_file.dev = filp->f_dentry->d_inode->i_rdev; /* set Device Number */

  if ( !(grp_statt = cdcm_get_grp_stat_tbl(imajor(filp->f_dentry->d_inode))) )
    /* can't find group with given major number */
    return(-ENXIO);
  
  if (entry_points.dldd_select(grp_statt, &lynx_file, SREAD|SWRITE|SEXCEPT, &sel) == OK)
    mask = POLLERR;		/* for now - we are NEVER cool */

  return(mask);
}


/**
 * @brief 
 *
 * @param inode - inode struct pointer
 * @param file  - file struct pointer
 * @param cmd   - IOCTL command number
 * @param arg   - user args
 *
 * @return 
 * @return 
 */
static int process_cdcm_srv_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
  void __user *uptr = (void __user *)arg;

  switch (cmd) {
  case CDCM_CDV_INSTALL:	/* cdv_install() stub */
    {
      struct cdcm_grp_info_t *cur_grp; /* current working group info table */
      char *itp = NULL; /* info table path */
      int len;

      if (glob_cur_grp_ptr == &cdcmStatT.cdcm_grp_list_head)
	/* cdv_install() was called more times than groups declared */
	return(-EDQUOT);
      
      /* set current group pointer */
      cur_grp = list_entry(glob_cur_grp_ptr, struct cdcm_grp_info_t, grp_list);
      
      /* get string lengths */
      if ( (len = strnlen_user(uptr, PATH_MAX)) <= 0)
	return(-EFAULT);
      
      /* alloc space */
      if (!(itp = sysbrk(len)))
	return(-ENOMEM);

      /* get the string */
      if (copy_from_user(itp, uptr, len))
	return(-EFAULT);

      /* get module info table */
      if ( !(cur_grp->grp_info_tbl = cdcm_get_info_table(itp, &cur_grp->grp_it_sz)) ) {
	PRNT_ABS_ERR("Can't read info file."); /* fatal. barf */
	sysfree(itp, len);
	return(-ENOENT);
      }
      
      sysfree(itp, len); /* we don't need it anymore */

      /* hook the uza */
      if ( (cur_grp->grp_stat_tbl = entry_points.dldd_install(cur_grp->grp_info_tbl)) == (char*)SYSERR) {
	PRNT_ABS_ERR("Install vector failed.");
	return(-EAGAIN);
      }
      
      /* move current working group pointer */
      glob_cur_grp_ptr = list_advance(glob_cur_grp_ptr);

      return(cur_grp->grp_maj);	/* return major dev id */
    }
  case CDCM_CDV_UNINSTALL:	/* cdv_uninstall() stub */
    {
      int majn, rc;
      char *grp_statt;

      if (list_empty(&cdcmStatT.cdcm_grp_list_head))
	/* we are fucked! no device installed */
	return(-ENXIO);

      if (copy_from_user(&majn, (void __user*)arg, sizeof(majn)))
	return(-EFAULT);
      
      if ( !(grp_statt = cdcm_get_grp_stat_tbl(majn)) )
	/* can't find group with given major number */
	return(-ENXIO);

      /* hook the uza */
      if ( (rc = entry_points.dldd_uninstall(grp_statt)) != OK) {
	PRNT_ABS_ERR("Uninstall vector failed.");
	return(-EAGAIN);
      }

      /* exclude given group from the list */
      cdcm_cleanup_groups(from_maj_to_grp(majn));

      return(rc);
    }
  case CDCM_GET_GRP_MOD_AM: /* how many modules in the group */
    {
      struct cdcm_grp_info_t *gp;
      int grpnum;

      if (copy_from_user(&grpnum, (void __user*)arg, sizeof(grpnum)))
	return(-EFAULT);

      if (!WITHIN_RANGE(1, grpnum, 21))
	return(-EINVAL);
      
      list_for_each_entry(gp, &cdcmStatT.cdcm_grp_list_head, grp_list)
	if (gp->grp_num == grpnum) /* bingo */
	  return(list_capacity(&gp->grp_dev_list));

      return(-1);
    }
  default:
    return(-ENOIOCTLCMD);
  }
}


/**
 * @brief Module-specific Ioctl routine.
 *
 * @param inode - inode struct pointer
 * @param file  - file struct pointer
 * @param cmd   - IOCTL command number
 * @param arg   - user args
 *
 * Called in case if major number is not corresponding to the service one.
 * I.e. this device node was created by the user and not by the CDCM.
 *
 * @return
 */
static int process_mod_spec_ioctl(struct inode *inode, struct file *file, int cmd, long arg)
{
  struct cdcm_file lynx_file;
  int rc = 0;	/* ret code */
  char *grp_statt = NULL; /* group statics table */
  char *iobuf  = NULL;
  int iodir = _IOC_DIR(cmd);
  int iosz  = _IOC_SIZE(cmd);

  if ( !(grp_statt = cdcm_get_grp_stat_tbl(imajor(inode))) )
    /* can't find group with given major number */
    return(-ENXIO);

  if (iodir != _IOC_NONE) { /* we should move user <-> driver data */
    if ( (iobuf = cdcm_mem_alloc(iosz, iodir|CDCM_M_FLG_IOCTL)) == ERR_PTR(-ENOMEM))
      return(-ENOMEM);
  }
  
  if (iodir & _IOC_WRITE) {
    if (!access_ok(VERIFY_READ, (void __user*)arg, iosz)) {
      PRNT_ABS_ERR("Can't verify user buffer for reading.");
      cdcm_mem_free(iobuf);
      return(-EFAULT);
    }
    __copy_from_user(iobuf, (char*)arg, iosz); /* take data from user */
  }
  
  /* Carefull with the type correspondance.
     int in Lynx <-> unsigned int in Linux */
  lynx_file.dev = (int)inode->i_rdev; /* set Device Number */

  /* hook the user ioctl */
  rc = entry_points.dldd_ioctl(grp_statt, &lynx_file, cmd, (iodir == _IOC_NONE)?NULL:iobuf);
  if (rc == SYSERR) {
    cdcm_mem_free(iobuf);
    return(-(cdcm_err)); /* error is set by the user */
  }
  
  if (iodir & _IOC_READ) {
    if (!access_ok(VERIFY_WRITE, (void __user*)arg, iosz)) {
      PRNT_ABS_ERR("Can't verify user buffer for writing.");
      cdcm_mem_free(iobuf);
      return(-EFAULT);
    }
    if (__copy_to_user((char*)arg, iobuf, iosz)) /* give data to the user */
      return(-EFAULT);
  }
  
  cdcm_mem_free(iobuf);
  return(rc);	/* we cool, return user-set return code */
}


/**
 * @brief CDCM ioctl routine.
 *
 * @param inode - inode struct pointer
 * @param file  - file struct pointer
 * @param cmd   - IOCTL command number
 * @param arg   - user args
 *
 * Service ioctl (with CDCM_SRVIO_ prefix) normally are called by the
 * installation program to get all needed information. All the rest considered
 * to be user part and is passed downstream.
 *
 * @return 0 or some positve value         - in case of success.
 * @return -EINVAL, -ENOMEM, -EIO, etc...  - if fails.
 */
static int cdcm_fop_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
  cdcm_err = 0; /* reset */
  return( (imajor(inode) == cdcmStatT.cdcm_major) ?
	  process_cdcm_srv_ioctl(inode, file, cmd, arg) :
	  process_mod_spec_ioctl(inode, file, cmd, arg) );
}


/**
 * @brief NOT IMPLEMENTED YET.
 *
 * @param file - file struct pointer
 * @param vma  - 
 *
 * @return 
 */
static int cdcm_fop_mmap(struct file *file, struct vm_area_struct *vma)
{
  return(0); /* success */
}


/**
 * @brief Open entry point.
 *
 * @param inode - inode pointer
 * @param filp  - file pointer
 *
 * @return 0   - if success.
 * @return < 0 - if fails.
 */
static int cdcm_fop_open(struct inode *inode, struct file *filp)
{
  int dnum;
  struct cdcm_file lynx_file;
  char *grp_statt = NULL; /* group statics table */

  cdcm_err = 0;	/* reset */

  if (imajor(inode) == cdcmStatT.cdcm_major) /* service call */
    return(0);

  if ( !(grp_statt = cdcm_get_grp_stat_tbl(imajor(inode))) )
    /* can't find group with given major number */
    return(-ENXIO);

  /* enforce read-only access to this chrdev */
  /* 
  if ((filp->f_mode & FMODE_READ) == 0)
    return -EINVAL;
  if (filp->f_mode & FMODE_WRITE)
    return -EINVAL;
  */

  /* fill in Lynxos file */
  dnum = lynx_file.dev = (int)inode->i_rdev;
  lynx_file.access_mode = (filp->f_flags & O_ACCMODE);
  
  if (entry_points.dldd_open(grp_statt, dnum, &lynx_file) == OK) {
    //filp->private_data = (void*)dnum; /* set for us */
    /* you can use private_data for your own purposes */
    return(0);	/* success */
  }

  cdcm_err = -ENODEV; /* TODO. What val to return??? */
  return(cdcm_err); /* failure */
}


/**
 * @brief Close entry point.
 *
 * @param inode - inode pointer
 * @param filp  - file pointer
 *
 * @return 0   - if success.
 * @return < 0 - if fails.
 */
static int cdcm_fop_release(struct inode *inode, struct file *filp)
{
  struct cdcm_file lynx_file;
  char *grp_statt = NULL; /* group statics table */

  cdcm_err = 0;	/* reset */

  if ( !(grp_statt = cdcm_get_grp_stat_tbl(imajor(inode))) )
    /* can't find group with given major number */
    return(-ENXIO);

  /* fill in Lynxos file */
  lynx_file.dev = (int)inode->i_rdev;
  lynx_file.access_mode = (filp->f_flags & O_ACCMODE);

  if (entry_points.dldd_close(grp_statt, &lynx_file) == OK)
    return(0);			/* we cool */

  cdcm_err = -ENODEV; /* TODO. What val to return??? */	
  return(cdcm_err); /* failure */
}


/* CDCM entry points */
struct file_operations cdcm_fops = {
  .owner   = THIS_MODULE,
  .read    = cdcm_fop_read,   /* read   */
  .write   = cdcm_fop_write,  /* write  */
  .poll    = cdcm_fop_poll,   /* select - NOT COMPATIBLE! */
  .ioctl   = cdcm_fop_ioctl,  /* ioctl  */
  .mmap    = cdcm_fop_mmap,   /* mmap   */
  .open    = cdcm_fop_open,   /* open   */
  .release = cdcm_fop_release /* close  */
};


/**
 * @brief Called when module is unloading from the kernel.
 *
 * @param none
 *
 * @return void
 */
static void __exit cdcm_driver_cleanup(void)
{
  int cntr;

  /* cleanup all captured memory */ 
  cdcm_mem_cleanup_all();

  /* stop timers */
  for (cntr = 0; cntr < MAX_CDCM_TIMERS; cntr++)
    if (cdcmStatT.cdcm_timer[cntr].ct_on)
      del_timer_sync(&cdcmStatT.cdcm_timer[cntr].ct_timer);

  if (cdcmStatT.cdcm_drvr_type == 'P')
    cdcm_pci_cleanup();
  else if (cdcmStatT.cdcm_drvr_type == 'V')
#if (CPU != L864  && CPU != L865)
    cdcm_vme_cleanup();
#else
  ;
#endif

  cdcm_sema_cleanup_all();	/* cleanup semaphores */
  cdcm_cleanup_groups(NULL);
}


/* next two defines are used to handle claimed groups */
/* bit counter */
#define bitcntr(x)				\
({						\
  int __cntr = 0;				\
  while (x) {					\
    if (x&1)					\
      __cntr++;					\
    x >>= 1;					\
  }						\
  __cntr;					\
})

/* which bits are set */
#define getbitidx(x, m)				\
do {						\
  int __cntr = 0, __mcntr = 0;			\
  typeof(x) __x = (x);				\
						\
  while(__x) {					\
    if (__x&1) {				\
      m[__mcntr] = __cntr;			\
      __mcntr++;				\
    }						\
    __x >>= 1;					\
    __cntr++;					\
  }						\
} while (0)


/**
 * @brief Main driver initialization.
 *
 * @param none
 *
 * @return 0        - in case of success.
 * @return non zero - otherwise.
 */
static int __init cdcm_driver_init(void)
{
  struct cdcm_grp_info_t *grp_ptr;
  cdcm_hdr_t *cdcmHdr;
  int cntr = 0;
  int grp_am = 0, grp_n[21] = { 0 }; /* for group handling */

  /* get module info table */
  if ( !(cdcmHdr = (cdcm_hdr_t*)cdcm_get_info_table(cdcmInfoF, NULL)) ) {
    PRNT_ABS_ERR("Can't read info file."); /* fatal. barf */
    return(-ENOENT);
  }

  /* register service character device */
  if ( (cdcmStatT.cdcm_major = register_chrdev(0, __srv_dev_name(cdcmHdr->cih_dnm), &cdcm_fops)) < 0) {
    PRNT_ABS_ERR("Can't register CDCM as character device... Aborting!");
    sysfree((char*)cdcmHdr, sizeof(*cdcmHdr));
    return(cdcmStatT.cdcm_major); /* error */
  }
  
  grp_am = bitcntr(cdcmHdr->cih_grp_bits); /* how many claimed groups */
  getbitidx(cdcmHdr->cih_grp_bits, grp_n); /* their indexes */

  for (cntr = 0; cntr < grp_am; cntr++) { /* allocate groups */
    if (!(grp_ptr = kzalloc(sizeof(*grp_ptr), GFP_KERNEL))) {
      PRNT_ABS_ERR("Can't allocate group descriptor for %s dev.\n", cdcmStatT.cdcm_mod_name);
      sysfree((char*)cdcmHdr, sizeof(*cdcmHdr));
      cdcm_cleanup_groups(NULL);
      return(-ENOMEM);
    }

    INIT_LIST_HEAD(&grp_ptr->grp_list);
    INIT_LIST_HEAD(&grp_ptr->grp_dev_list);

    grp_ptr->grp_num      = grp_n[cntr] + 1;
    snprintf(grp_ptr->grp_name, sizeof(grp_ptr->grp_name), CDCM_GRP_FMT, cdcmHdr->cih_dnm, grp_ptr->grp_num);
    
    if ( (grp_ptr->grp_maj =  register_chrdev(0, grp_ptr->grp_name, &cdcm_fops)) < 0 ) {
      PRNT_ABS_ERR("Can't register character device for group %d... Aborting!", grp_ptr->grp_num);
      sysfree((char*)cdcmHdr, sizeof(*cdcmHdr));
      cdcm_cleanup_groups(NULL);
      return(grp_ptr->grp_maj);	/* error */
    }
    /* group is correctly inited. add it to the claimed groups list */
    list_add(&grp_ptr->grp_list, &cdcmStatT.cdcm_grp_list_head);
  }

  /* init current working group pointer with the first group */
  glob_cur_grp_ptr = list_advance(&cdcmStatT.cdcm_grp_list_head);

  cdcmStatT.cdcm_hdr = cdcmHdr; /* save header */
  cdcm_mem_init(); /* init memory manager */
  
  return(OK);
}

module_init(cdcm_driver_init);
module_exit(cdcm_driver_cleanup);
