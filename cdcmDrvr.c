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
 * @version
 */
#include "cdcmDrvr.h"
#include "cdcmLynxAPI.h"
#include "cdcmMem.h"
#include "cdcmTime.h"

#include "config_data.h"  /* for info tables data types */
#include "general_both.h" /* for handy macroses */
#include "general_ioctl.h"
#include "list_extra.h" /* for extra handy list operations */
#include "libinstkernel.h" /* for intall library to get/parse info tables */

MODULE_DESCRIPTION("Common Driver Code Manager (CDCM) Driver");
MODULE_AUTHOR("Yury Georgievskiy, CERN BE/CO");
MODULE_LICENSE("GPL");

/* driver parameter */
static char cdcm_d_nm[64] = { 0 };
module_param_string(dname, cdcm_d_nm, sizeof(cdcm_d_nm), 0);
MODULE_PARM_DESC(dname, "Driver name");

/* to avoid struct file to be treated as struct cdcm_file */
#ifdef file
#undef file
#endif

/* external crap */
extern struct dldd entry_points; /* declared in the user driver part */
extern char* read_info_file(char*, int*);


/* general statics data that holds all information about current groups,
   devices, threads, allocated mem, etc. managed by CDCM */
cdcmStatics_t cdcmStatT = {
  .cdcm_dev_list        = LIST_HEAD_INIT(cdcmStatT.cdcm_dev_list),
  .cdcm_mem_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_mem_list_head),
  .cdcm_ipl             = (IPL_ERROR | IPL_INFO), /* info printout level */
  .cdcm_thr_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_thr_list_head),
  .cdcm_sem_list_head   = LIST_HEAD_INIT(cdcmStatT.cdcm_sem_list_head),
  .cdcm_flags           = 0
};

/* LynxOs system kernel-level errno.
   Should be set _ONLY_ by 'pseterr' function */
int cdcm_err = 0;

spinlock_t lynxos_cpu_lock = SPIN_LOCK_UNLOCKED;

struct file_operations cdcm_fops;


/**
 * @brief Cleanup and release claimed devices. Unregister char device.
 *
 * @param none
 *
 * @return void
 */
static void cdcm_cleanup_dev(void)
{
	struct cdcm_dev_info *dip, *tmpp;

	list_for_each_entry_safe(dip, tmpp, &cdcmStatT.cdcm_dev_list,
				 di_list) {
		/* TODO. Perform correct pci/vme cleanup */
		list_del(&dip->di_list); /* exclude from linked list */
		if (dip->di_vme) kfree(dip->di_vme);	/* vme cleanup */
		kfree(dip);
	}

	unregister_chrdev(cdcmStatT.cdcm_major, cdcmStatT.cdcm_mn);
	kfree(cdcmStatT.cdcm_mn);
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
static ssize_t cdcm_fop_read(struct file *filp, char __user *buf,
			     size_t size, loff_t *off)
{
	char *iobuf  = NULL;
	struct cdcm_file lynx_file = {
		/* set Device Number */
		.dev = filp->f_dentry->d_inode->i_rdev,
		.access_mode = (filp->f_flags & O_ACCMODE),
		.position = *off
	};

	PRNT_DBG(cdcmStatT.cdcm_ipl, "Read device with minor = %d",
		 MINOR(lynx_file.dev));
	cdcm_err = 0;	/* reset */

	if (!access_ok(VERIFY_WRITE, buf, size)) {
		PRNT_ABS_ERR("Can't verify user buffer for writing.");
		return -EFAULT;
	}

	if ( (iobuf = cdcm_mem_alloc(size, _IOC_READ|CDCM_M_FLG_READ))
	     == ERR_PTR(-ENOMEM))
		return -ENOMEM;


	/* uza */
	cdcm_err = entry_points.dldd_read(cdcmStatT.cdcm_st, &lynx_file,
					  iobuf, size);
	if (cdcm_err == SYSERR)
		cdcm_err = -EAGAIN;
	else {
		if (__copy_to_user(buf,  iobuf, size))
			return(-EFAULT);
		*off = lynx_file.position;
	}

	cdcm_mem_free(iobuf);

	return cdcm_err;
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
static ssize_t cdcm_fop_write(struct file *filp, const char __user *buf,
			      size_t size, loff_t *off)
{
	char *iobuf = NULL;
	struct cdcm_file lynx_file = {
		/* set Device Number */
		.dev = filp->f_dentry->d_inode->i_rdev,
		.access_mode = (filp->f_flags & O_ACCMODE),
		.position = *off
	};

	PRNT_DBG(cdcmStatT.cdcm_ipl, "Write device with minor = %d",
		 MINOR(lynx_file.dev));
	cdcm_err = 0; /* reset */

	if (!access_ok(VERIFY_READ, buf, size)) {
		PRNT_ABS_ERR("Can't verify user buffer for reading.");
		return -EFAULT;
	}

	if ( (iobuf = cdcm_mem_alloc(size, _IOC_WRITE|CDCM_M_FLG_WRITE))
	     == ERR_PTR(-ENOMEM))
		return -ENOMEM;

	__copy_from_user(iobuf, buf, size);

	/* uza */
	cdcm_err = entry_points.dldd_write(cdcmStatT.cdcm_st, &lynx_file,
					   iobuf, size);
	if (cdcm_err == SYSERR)
		cdcm_err = -EAGAIN;
	else
		*off = lynx_file.position;

	cdcm_mem_free(iobuf);

	return cdcm_err;
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

	/* set Device Number */
	lynx_file.dev = filp->f_dentry->d_inode->i_rdev;

	if (entry_points.dldd_select(cdcmStatT.cdcm_st, &lynx_file,
				     SREAD|SWRITE|SEXCEPT, &sel) == OK)
		mask = POLLERR; /* for now - we are NEVER cool */

	return mask;
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
 */
static int process_cdcm_srv_ioctl(struct inode *inode, struct file *file,
				  unsigned int cmd, unsigned long arg)
{
	int ret = 0; /* return code. OK by default */

	switch (cmd) {
	case _GIOCTL_DR_INSTALL: /* cdv_install() */
	{
		char itp[128] = { 0 }; /* info table path */
		ulong *addr = NULL; /* user space info table address */
		void (*prev)(void*, const void*, int) = NULL;
		InsLibDrvrDesc *dd = NULL;

		/* get info file path */
		if (strncpy_from_user(itp, (char*)arg, 128) < 0) {
                        ret = -EFAULT;
                        goto exit_dr_install;
                }

		/* get user-space address to copy info table from */
		if (!(addr = (ulong*)read_info_file(itp, NULL))) {
			ret = -EFAULT;
                        goto exit_dr_install;
		}

		/* set copy method */
		prev = InsLibSetCopyRoutine((void(*)(void*, const void*, int n))
					    copy_from_user);

		/* get info table from the user space */
		if (!(dd = InsLibCloneOneDriver((InsLibDrvrDesc*)*addr))) {
			ret = -ENOMEM;
			goto exit_dr_install;
		}

		/* restore previous copy method */
		InsLibSetCopyRoutine(prev);

		/* hook the uza, call install entry point,
		   save user statics table */
		if ( (cdcmStatT.cdcm_st =
		      entry_points.dldd_install((void*)dd)) == (char*)SYSERR) {
			PRNT_ABS_ERR("Install vector failed.");
			ret = -EAGAIN;
			goto exit_dr_install;
		}

	exit_dr_install:
		if (addr) sysfree((char*)addr, 0/*not used*/);
		if (dd) InsLibFreeDriver(dd);
		return cdcmStatT.cdcm_major; /* return major number */
	}
	case _GIOCTL_DR_UNINSTALL: /* cdv_uninstall() */
	{
		int cid = 0;	/* device ID */
		int rc;

		if (list_empty(&cdcmStatT.cdcm_dev_list))
			/* no device installed */
			return -ENXIO;

		if (copy_from_user(&cid, (void __user*)arg, sizeof(cid)))
			return -EFAULT;

		/* hook the uza */
		if ( (rc = entry_points.dldd_uninstall(cdcmStatT.cdcm_st))
		     != OK) {
			PRNT_ABS_ERR("Uninstall vector failed.");
			return -EAGAIN;
		}

		/* cleanup all devices */
		cdcm_cleanup_dev();

		return rc;
	}
	case _GIOCTL_GET_MOD_AM: /* how many modules */
		return list_capacity(&cdcmStatT.cdcm_dev_list);
	default:
		return -ENOIOCTLCMD;
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
static int process_mod_spec_ioctl(struct inode *inode, struct file *file,
				  int cmd, long arg)
{
	struct cdcm_file lynx_file;
	int rc = 0;	/* ret code */
	char *iobuf  = NULL;
	int iodir = _IOC_DIR(cmd);
	int iosz  = _IOC_SIZE(cmd);

	if (iodir != _IOC_NONE) { /* we should move user <-> driver data */
		if ( (iobuf = cdcm_mem_alloc(iosz, iodir|CDCM_M_FLG_IOCTL))
		     == ERR_PTR(-ENOMEM))
			return -ENOMEM;
	}

	if (iodir & _IOC_WRITE) {
		if (!access_ok(VERIFY_READ, (void __user*)arg, iosz)) {
			PRNT_ABS_ERR("Can't verify user buffer for reading.");
			cdcm_mem_free(iobuf);
			return -EFAULT;
		}
		/* take data from user */
		__copy_from_user(iobuf, (char*)arg, iosz);
	}

	/* Carefull with the type correspondance.
	   int in Lynx <-> unsigned int in Linux */
	lynx_file.dev = (int)inode->i_rdev; /* set Device Number */

	/* hook the user ioctl */
	rc = entry_points.dldd_ioctl(cdcmStatT.cdcm_st, &lynx_file, cmd,
				     (iodir == _IOC_NONE) ? NULL : iobuf);
	if (rc == SYSERR) {
		cdcm_mem_free(iobuf);
		return -(cdcm_err); /* error is set by the user */
	}

	if (iodir & _IOC_READ) {
		if (!access_ok(VERIFY_WRITE, (void __user*)arg, iosz)) {
			PRNT_ABS_ERR("Can't verify user buffer for writing.");
			cdcm_mem_free(iobuf);
			return -EFAULT;
		}

		/* give data to the user */
		if (__copy_to_user((char*)arg, iobuf, iosz))
			return -EFAULT;
	}

	cdcm_mem_free(iobuf);
	return rc; /* we cool, return user-set return code */
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
static int cdcm_fop_ioctl(struct inode *inode, struct file *file,
			  unsigned int cmd, unsigned long arg)
{
	cdcm_err = 0;
	return( !(iminor(inode)) ?
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
	return 0; /* success */
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

	cdcm_err = 0;	/* reset */

	if (!iminor(inode)) /* service entry point */
		return 0;


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

	if (entry_points.dldd_open(cdcmStatT.cdcm_st, dnum, &lynx_file) == OK) {
		//filp->private_data = (void*)dnum; /* set for us */
		/* you can use private_data for your own purposes */
		return 0;	/* success */
	}

	cdcm_err = -ENODEV; /* TODO. What val to return??? */
	return cdcm_err; /* failure */
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

  cdcm_err = 0;	/* reset */

  /* fill in Lynxos file */
  lynx_file.dev = (int)inode->i_rdev;
  lynx_file.access_mode = (filp->f_flags & O_ACCMODE);

  if (entry_points.dldd_close(cdcmStatT.cdcm_st, &lynx_file) == OK)
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
 * __drvrnm - Driver Debug printout mechanism (drvr_dgb_prnt.h) needs this
 *            function to get driver name.
 *
 * @param none
 *
 * <long-description>
 *
 * @return driver name
 */
char* __drvrnm(void)
{
	return cdcmStatT.cdcm_mn;
}


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

	cdcm_sema_cleanup_all(); /* cleanup semaphores */
	cdcm_cleanup_dev();
}


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
	if (!(cdcmStatT.cdcm_mn = kasprintf(GFP_KERNEL, "%s", cdcm_d_nm)))
		return -ENOMEM;

	/* register character device */
	if ((cdcmStatT.cdcm_major =
	     register_chrdev(0, cdcm_d_nm, &cdcm_fops)) < 0) {
		PRNT_ABS_ERR("Can't register character device");
		return cdcmStatT.cdcm_major;
	}

	cdcm_mem_init(); /* init memory manager */
	return 0;
}

module_init(cdcm_driver_init);
module_exit(cdcm_driver_cleanup);
