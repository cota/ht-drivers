/**
 * @file vmod-ttl.c
 *
 * @brief VMOD-TTL Digital I/O card driver.
 *
 * <long description>
 *
 * @author Copyright (C) 2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 20/01/2010
 *
 * @section license_sec License
 *          Released under the GPL
 */

#if 0
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* copy_*_user */

#include "vmod16a2.h"
#include "carrier.h"
#endif

#define	VMOD16A2_PARAMS_PER_LUN  4

static char		*luns[VMOD16A2_PARAMS_PER_LUN*VMOD16A2_MAX_MODULES];
static int 		num;
module_param_array(luns, charp, &num, S_IRUGO);

/* The One And Only Device (OAOD) */
struct cdev cdev;

/* module config tables */
static int 			configured_modules = 0;
static struct vmod16a2_dev 	modules[VMOD16A2_MAX_MODULES];
static int 			lun_to_dev[VMOD16A2_MAX_MODULES];

static dev_t devno;

/* carrier entry points for address space and interrupt hook */
static gas_t			get_address_space;
/* not needed
static risr_t			register_isr;
 */

/** @brief sanity check (lun, channel) pair */
static int good(int lun, int channel)
{
	return 	(0 < lun) &&
		(lun <= configured_modules) &&
		(channel == 1 || channel == 0);
}

/**
 * @brief allocate a struct vmod16a2_state  on a
 * per-open-file basis, which stores the last lun,channel pair selected
 */
static int open(struct inode *ino, struct file *filp)
{
	struct vmod16a2_state *vsp;

	vsp = kmalloc(sizeof(*vsp), GFP_KERNEL);
	if (vsp == NULL) {
		printk(KERN_INFO "could not open, bad kmalloc\n");
		return -ENOMEM;
	} else {
		vsp->selected = 0; /* invalid until IOCSELECT */
		filp->private_data = vsp;
		return 0;
	}
}

/**
 *  @brief release the struct vmod16a2_select struct */
static int release(struct inode *ino, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}


static int ioctl(struct inode *inode,
			struct file *fp,
			unsigned op,
			unsigned long arg)
{
	struct vmod16a2_select 		sel, *selp;
	struct vmod16a2_state 		*statep = fp->private_data;
	struct vmod16a2_registers	*regp;
	struct vmod16a2_dev		*modp;
	u16				value, tmp;

	switch (op) {

	case VMOD16A2_IOCSELECT:

		/* sanity check user params */
		selp = (void*)arg;
		if (!access_ok(VERIFY_READ, selp, sizeof(sel)))
			return -EINVAL;
		if (copy_from_user(&sel, selp, sizeof(sel)) != 0)
			return -EINVAL;

		/* ioctl arg ok, extract lun+channel and check sanity */
		statep->lun	= (sel.lun != VMOD16A2_NO_LUN_CHANGE) ?
						sel.lun :
						statep->lun;
		statep->channel = sel.channel;
		if (!good(statep->lun, statep->channel))
			return -EINVAL;
		statep->selected = 1;

		return 0;
		break;

	case VMOD16A2_IOCPUT:

		/* sanity check user params */
		if (!access_ok(VERIFY_READ, arg, sizeof(int)))
			return -EINVAL;
		if (copy_from_user(&value, (int*)arg, sizeof(int)) != 0)
			return -EINVAL;
		if (!statep->selected)
			return -EINVAL;

		/* just do it */
		modp = &(modules[lun_to_dev[statep->lun]]);
		regp = (void*)modp->address;
		if (modp->is_big_endian)
			tmp = cpu_to_be16(value);
		else
			tmp = value;
		if (statep->channel == 0) {
#ifdef DEBUG
			printk(KERN_INFO "writing %x = %d to addr 0x%p\n",
				value, value, &(regp->dac0in));
#endif /* DEBUG */
			iowrite16(tmp, &(regp->dac0in));
			iowrite16(tmp, &(regp->ldac0));
		} else if (statep->channel == 1) {
#ifdef DEBUG
			printk(KERN_INFO "writing %x = %d to addr 0x%p\n",
				value, value, &(regp->dac1in));
#endif /* DEBUG */
			iowrite16(tmp, &(regp->dac1in));
			iowrite16(tmp, &(regp->ldac1));
		}
		return 0;
		break;

	default:
		return -ENOTTY;
	}
	return -ENOTTY;
}

/* @brief file operations for this driver */
struct file_operations vmod16a2_fops = {
	.owner =    THIS_MODULE,
	.ioctl =    ioctl,
	.open =     open,
	.release =  release,
};


/* @brief fill in the vmod16a2 tables with configuration data

   Config data is passed through a vector luns[] of length num*4,
   containing triple of lun,board_number,slot_number values
   describing
   	- device LUN
	- carrier LUN
	- carrier slot
   This routine scans this vector and stores its information in the
   modules vector, storing the number of modules to be
   handled in configured_modules.

   It also constructcs a reverse lookup table lun_to_dev useful to
   index modules by lun.

   @return 0 on success
   @return -1 on failure (incorrect number of parameters)
*/

static int check_luns_args(char *luns[], int num)
{
	int i, idx;
	const int ppl = VMOD16A2_PARAMS_PER_LUN;

	if (num < 0 || num >= ppl * VMOD16A2_MAX_MODULES || num % ppl != 0) {
		printk(KERN_ERR "Incorrect number %d of lun parameters\n", num);
		return -1;
	}

	printk(KERN_INFO "Configuring %d vmod16a2 devices\n", num/ppl);
	for (i = 0, idx=0; idx < num; i++, idx += ppl) {
		int   lun     = simple_strtol(luns[idx], NULL, 10);
		char *cname   = luns[idx+1];
		int   carrier = simple_strtol(luns[idx+2], NULL, 10);
		int   slot    = simple_strtol(luns[idx+3], NULL, 10);
		struct carrier_as as, *asp = &as;

		get_address_space = carrier_as_entry(cname);
		if (get_address_space == NULL) {
			printk(KERN_ERR "no carrier %s registered\n", cname);
			return -1;
		}
		if (get_address_space(asp, carrier, slot, 1)) {
			printk(KERN_ERR "no address space for module<%d>: "
				"carrier %s lun = %d position %d\n",
				lun, cname, carrier, slot);
			continue;
		}

		modules[i].lun		= lun;
		modules[i].cname	= cname;
		modules[i].carrier	= carrier;
		modules[i].slot		= slot;
		modules[i].address 	= asp->address;
		modules[i].is_big_endian = asp->is_big_endian;
		lun_to_dev[lun] = i;

		printk(KERN_INFO "    module<%d>: lun = %d "
				"carrier = %d slot = %d addr = 0x%lx,"
				" %s endian\n",
				i, lun, carrier, slot, asp->address,
				asp->is_big_endian? "big" : "little" );
	}
	configured_modules = i;
	return 0;
}

/* module initialization and cleanup */
static int __init init(void)
{
	int err;

	printk(KERN_INFO "Initializing vmod16a2 driver with %d luns\n",
				num/VMOD16A2_PARAMS_PER_LUN);
	err = check_luns_args(luns, num);
	if (err != 0)
		return -1;

	err = alloc_chrdev_region(&devno, 0, VMOD16A2_MAX_MODULES, "vmod16a2");
	if (err != 0) goto fail_chrdev;
	printk("Allocated devno %0x\n",devno);

	cdev_init(&cdev, &vmod16a2_fops);
	cdev.owner = THIS_MODULE;
	err = cdev_add(&cdev, devno, 1);
	if (err) goto fail_cdev;
	printk(KERN_INFO "Added cdev with err = %d\n", err);

	return 0;

fail_cdev:	unregister_chrdev_region(devno, VMOD16A2_MAX_MODULES);
fail_chrdev:	return -1;
}

static void __exit exit(void)
{
	cdev_del(&cdev);
	unregister_chrdev_region(devno, VMOD16A2_MAX_MODULES);
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
