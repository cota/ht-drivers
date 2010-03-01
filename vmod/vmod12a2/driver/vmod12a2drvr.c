#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* copy_*_user */
#include "carrier.h"
#include "vmod12a2.h"


#define	PARAMS_PER_LUN	4
static char *luns[PARAMS_PER_LUN*VMOD12A2_MAX_MODULES];
static int num;
module_param_array(luns, charp, &num, S_IRUGO);

/* The One And Only Device (OAOD) */
struct cdev			cdev;

/* module config tables */
static int 			used_modules = 0;
static struct vmod12a2_dev 	modules[VMOD12A2_MAX_MODULES];
static int			lun_to_dev[VMOD12A2_MAX_MODULES];

/* The One And Only Device (OAOD) */
static dev_t devno;

static int vmod12a2_offsets[VMOD_12A2_CHANNELS] = {
	VMOD_12A2_CHANNEL0,
	VMOD_12A2_CHANNEL1,
};

/** @brief sanity check (lun, channel) pair */
static int good(int lun, int channel)
{
	return 	(0 < lun) &&
		(lun <= used_modules) &&
		(channel == 1 || channel == 0);
}

/**
 * @brief allocate a struct vmod12a2_state on a
 * per-open-file basis, which stores the last lun,channel pair selected
 */
int vmod12a2_open(struct inode *ino, struct file *filp)
{
	struct vmod12a2_state *vsp;

	vsp = kmalloc(sizeof(*vsp), GFP_KERNEL);
	if (vsp == NULL) {
		printk(KERN_INFO "could not open, bad kmalloc\n");
		return -ENOMEM;
	} else {
		vsp->selected = 0;
				/* invalid until IOCSELECT */
		filp->private_data = vsp;
		return 0;
	}
}

/**
 *  @brief release the struct vmod12a2_select struct */
int vmod12a2_release(struct inode *ino, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}


/* @brief file operations for this driver */
static int vmod12a2_ioctl(struct inode *inode,
			struct file *fp,
			unsigned op,
			unsigned long arg)
{
	struct vmod12a2_dev	*modp;
	struct vmod12a2_select 	select, *selectp;
	struct vmod12a2_state 	*statep  = fp->private_data;
	unsigned long		addr;
	u16			value;

	switch (op) {

	case VMOD12A2_IOCSELECT:
		selectp = (void*)arg;
		if (!access_ok(VERIFY_READ, selectp, sizeof(*selectp)))
			return -EINVAL;
		if (copy_from_user(&select, selectp, sizeof(*selectp)) != 0)
			return -EINVAL;

		/* ioctl arg ok, extract lun+channel and check sanity */
		statep->lun	 = (select.lun != VMOD12A2_NO_LUN_CHANGE) ?
						select.lun :
						statep->lun;
		statep->channel = select.channel;
		if (!good(statep->lun, statep->channel))
			return -EINVAL;
		statep->selected = 1;
#ifdef DEBUG
		printk(KERN_INFO "selecting lun=%d, channel=%d\n",
			statep->lun, statep->channel);
#endif /* DEBUG */
		return 0;
		break;

	case VMOD12A2_IOCPUT:
		if (!access_ok(VERIFY_READ, arg, sizeof(int)))
			return -EINVAL;
		if (copy_from_user(&value, (int*)arg, sizeof(int)) != 0)
			return -EINVAL;
		if (!statep->selected)
			return -EINVAL;

		modp = &(modules[lun_to_dev[statep->lun]]);
		addr = modp->address + vmod12a2_offsets[statep->channel];
#ifdef DEBUG
		printk(KERN_INFO "writing %x = %d to addr 0x%lx, %s endian\n",
				value, value, addr,
				modp->is_big_endian ? "big" : "little" );
#endif /* DEBUG */
		if (modp->is_big_endian)
			value = cpu_to_be16(value);
		iowrite16(value, (void*)addr);
		return 0;
		break;

	default:
		return -ENOTTY;
	}
	return -ENOTTY;
}

struct file_operations vmod12a2_fops = {
	.owner =    THIS_MODULE,
	.ioctl =    vmod12a2_ioctl,
	.open =     vmod12a2_open,
	.release =  vmod12a2_release,
};


/* @brief fill in the vmod12a2 tables with configuration data

   Config data is passed through a vector luns[] of length num*PARAMS_PER_LUN,
   containing triples of lun,carrier,slot values describing
   	- device LUN
	// ----> Not yet - (Driver) Name of carrier
	- LUN of carrier
	- slot position on carrier
   This routine scans this vector and stores its information in the
   modules vector, storing the number of modules found in
   used_modules.

   It also constructcs a reverse lookup table lun_to_dev to
   index vmod12a2_modules by lun.

   @return 0 on success
   @return -1 on failure (incorrect number of parameters)
*/

static int check_luns_args(char *luns[], int num)
{
	int i, idx;
	int ppl = PARAMS_PER_LUN;

	if (num < 0 || num >= ppl * VMOD12A2_MAX_MODULES || num % ppl != 0) {
		printk(KERN_ERR "Incorrect number %d of lun parameters\n", num);
		return -1;
	}
	printk(KERN_INFO "Configuring %d vmod12a2 devices\n", num/ppl);
	for (i = 0, idx=0; idx < num; i++, idx += ppl) {
		int lun		= simple_strtol(luns[idx], NULL, 10);
		char *cname	= luns[idx+1];
		int carrier     = simple_strtol(luns[idx+2], NULL, 10);
		int slot        = simple_strtol(luns[idx+3], NULL, 10);
		struct vmod12a2_dev *module
				= &(modules[i]);
		struct carrier_as as, *asp = &as;
		gas_t		get_address_space;

		get_address_space = carrier_as_entry(cname);
		if (get_address_space == NULL) {
			printk(KERN_ERR "No carrier %s\n", cname);
			return -1;
		} else
			printk(KERN_INFO "Carrier %s a.s. entry point at %p\n",
				cname, get_address_space);
		if (get_address_space(asp, carrier, slot, 1))
			continue;

		module->lun    	      = lun;
		module->cname  	      = cname;
		module->carrier	      = carrier;
		module->slot   	      = slot;
		module->address	      = asp->address;
		module->is_big_endian = asp->is_big_endian;
		lun_to_dev[lun]       = i;

		printk(KERN_INFO
			"    module<%d>: lun %d with "
			"carrier %s lun = %d slot = %d\n",
			i, lun, cname, carrier, slot);
	}
	used_modules = i;
	if (used_modules != num/ppl) {
		printk(KERN_ERR "Actual used modules %d != %d", used_modules, num/ppl);
		return -1;
	}
	printk(KERN_INFO "vmod12a2 configured modules = %d\n", used_modules);

	return 0;
}

/* module initialization and cleanup */
static int __init vmod12a2_init(void)
{
	int err;

	printk(KERN_INFO "Initializing module with luns=%d/%d\n", num, PARAMS_PER_LUN);

	err = check_luns_args(luns, num);
	if (err != 0)
		return -1;

	err = alloc_chrdev_region(&devno, 0, VMOD12A2_MAX_MODULES, "vmod12a2");
	if (err != 0) goto fail_chrdev;
	printk("Allocated devno %0x\n",devno);

	cdev_init(&cdev, &vmod12a2_fops);
	cdev.owner = THIS_MODULE;
	err = cdev_add(&cdev, devno, 1);
	if (err) goto fail_cdev;
	printk(KERN_INFO "Added cdev with err = %d\n", err);

	/* roll back if something went wrong */
	if (used_modules == 0) {
		cdev_del(&cdev);
		unregister_chrdev_region(devno, VMOD12A2_MAX_MODULES);
		return -ENODEV;
	} else
		return 0;

fail_cdev:	unregister_chrdev_region(devno, VMOD12A2_MAX_MODULES);
fail_chrdev:	return -1;
}

static void __exit vmod12a2_exit(void)
{
	cdev_del(&cdev);
	unregister_chrdev_region(devno, VMOD12A2_MAX_MODULES);
}

MODULE_LICENSE("GPL");
module_init(vmod12a2_init);
module_exit(vmod12a2_exit);
