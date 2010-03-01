/**
 * @file vmod12e16.c
 *
 * @brief Driver for VMOD12E16 ADC mezzanine on VMOD/MOD-PCI MODULBUS
 * carrier
 *  
 * Copyright (c) 2009 CERN
 * @author Juan David Gonzalez Cobas <dcobas@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* copy_*_user */
#include "carrier.h"
#include "vmod12e16.h"


/* The One And Only Device (OAOD) */
struct cdev	cdev;
dev_t		devno;

/** @brief entry in MODULBUS config table */
struct modulbus_dev {
	int		lun;		/** logical unit number */
	char		*cname;		/** carrier name */
	int		carrier;	/** supporting carrier */
	int		slot;		/** slot we're plugged in */
	unsigned long	address;	/** address space */
	int		is_big_endian;	/** endianness */
};

/** @brief stores a selected lun, channel per open file */
struct mobulbus_state {
	int	lun;		/** logical unit number */
	int	channel;	/** channel */
	int	selected;	/** already selected flag */
};

/** IOCSELECT ioctl arg 
struct vmod12a2_select {
	int	lun;		
	int	channel;
};
 */

/* module name */
static char	*devname = "vmod12e16";

/* module config tables */
#define		MODULBUS_MAX_MODULES	64
#define		INVALID_LUN		(-1)

static int 			used_modules = 0;
static struct modulbus_dev 	modules[MODULBUS_MAX_MODULES];

/* insmod parameters */
#define	PARAMS_PER_LUN	4
static char *luns[PARAMS_PER_LUN*MODULBUS_MAX_MODULES];
static int num;
module_param_array(luns, charp, &num, S_IRUGO);

static int init_module_table(char *luns[], int num) 
{
	int i, idx;
	const int ppl = PARAMS_PER_LUN;	/* for readability */

	for (i = 0; i < MODULBUS_MAX_MODULES; i++) 
		modules[i].lun = INVALID_LUN;

	if (num % ppl != 0) {
		printk(KERN_ERR "bad number of luns=... arguments\n");
		return -1;
	}
	for (i = 0, idx = 0; idx < num; i++, idx += ppl) {
		int   lun	= simple_strtol(luns[idx], NULL, 10);
		char *cname	= luns[idx+1];
		int   carrier	= simple_strtol(luns[idx+2], NULL, 10);
		int   slot	= simple_strtol(luns[idx+3], NULL, 10);

		struct modulbus_dev	*dev;
		gas_t			get_address_space;
		struct carrier_as	as;

		/* sanity-check mezzanine LUN */
		if (! ((lun > 0) && (lun <= MODULBUS_MAX_MODULES))) {
			printk(KERN_ERR "lun %d out of range\n", lun);
			return -1;
		}
		dev = &(modules[lun]);
		if (dev->lun != INVALID_LUN) {
			printk(KERN_ERR "duplicated lun %d\n", lun);
			return -1;
		}

		/* check carrier name and get address spaces */
		get_address_space = carrier_as_entry(cname);
		if (get_address_space == NULL) {
			printk(KERN_ERR "No carrier %s present\n", cname);
			return -1;
		}

		/* MODULBUS needs a single a.s. */
		if (get_address_space(&as, carrier, slot, 1) != 0) {
			printk(KERN_ERR "No valid address space for "
				"carrier %s, lun = %d, slot = %d\n",
				cname, carrier, slot);
			return -1;
		}

		/* Fill modules table entry */
		dev->lun		= lun;
		dev->cname		= cname;
		dev->carrier		= carrier;
		dev->slot		= slot;
		dev->address		= as.address;
		dev->is_big_endian	= as.is_big_endian;

		printk(KERN_INFO 
			"configured %s device:\n"
			"   lun = %d, carrier = %s, lun = %d, slot = %d\n"
			"       at addr = 0x%lx, %s endian\n",
			devname,
			lun, cname, carrier, slot,
			dev->address, 
			(dev->is_big_endian ? "big" : "little"));
	}

	printk(KERN_INFO "Configured %d %s modules\n", i, devname);
	used_modules = i;

	return 0;
}

static int do_open(struct inode *ino,
		   struct file  *filp)
{
	struct vmod12e16_state *statep = kmalloc(sizeof(*statep), GFP_KERNEL);
	if (statep == NULL) {
		printk(KERN_ERR "%s open(): bad kmalloc\n", devname);
		return -1;
	}
	statep->selected = 0;
	filp->private_data = statep;
	return 0;
}

static int do_release(struct inode *ino,
		   struct file  *filp)
{
	struct vmod12e16_state *statep = filp->private_data;
	kfree(statep);
	return 0;
}

static u16 ioread(void *addr, int be)
{
	u16 val = ioread16(addr);
	if (be)
		return be16_to_cpu(val);
	else
		return val;
}

static void iowrite(u16 val, void *addr, int be)
{
	u16 tmp;

	if (be)
		tmp = cpu_to_be16(val);
	else
		tmp = val;
	iowrite16(tmp, addr);
}

static int do_conversion(struct file *filp, 
			struct vmod12e16_conversion *conversion)
{
	struct vmod12e16_state	*statep = filp->private_data;
	int 			lun     = statep->lun;
	int			channel = conversion->channel;
	int			ampli	= conversion->amplification;
	struct vmod12e16_registers 
				*regs   = (void*)modules[lun].address;
	int 			us_elapsed;
	int			be = modules[lun].is_big_endian;

	/* disable interrupt mode 
	printk(KERN_INFO "Writing 0x%x to address %p\n", 
			VMOD_12E16_ADC_INTERRUPT_MASK, &regs->interrupt);
	iowrite(VMOD_12E16_ADC_INTERRUPT_MASK, &regs->interrupt, be);
	*/

	/* specify channel and amplification */
	if ((ampli & ~((1<<2)-1)) || channel & ~((1<<4)-1)) 
		return -EINVAL;
#ifdef DEBUG
	printk(KERN_INFO "Writing 0x%x to address %p\n", 
			(ampli<<4) | channel, &regs->control);
#endif	/* DEBUG */
	iowrite((ampli<<4) | channel, &regs->control, be);

	/* wait at most the manufacturer-supplied max time */ 
	us_elapsed = 0;
	while (us_elapsed < VMOD_12E16_MAX_CONVERSION_TIME) {
		udelay(VMOD_12E16_CONVERSION_TIME);
		if ((ioread(&regs->ready, be) & VMOD_12E16_RDY_BIT) == 0) {
			conversion->data = ioread(&regs->data, be) & VMOD_12E16_ADC_DATA_MASK;
#ifdef DEBUG
			printk(KERN_INFO "Read 0x%x from address %p\n", 
					conversion->data, &regs->data);
#endif	/* DEBUG */
			return 0;
		}
		us_elapsed += VMOD_12E16_CONVERSION_TIME;
	}
	/*
	udelay(VMOD_12E16_MAX_CONVERSION_TIME);
	conversion->data = ioread(&regs->data, be) & VMOD_12E16_ADC_DATA_MASK;
	printk(KERN_INFO "Read 0x%x from address %p\n", conversion->data, &regs->data);
	return 0;
	*/

	/* timeout */
	return -ETIME;
}

static int do_ioctl(struct inode *ino, 
		    struct file *filp, 
		    unsigned int cmd, 
		    unsigned long arg)
{
	struct vmod12e16_state   
			state, 
			*statep = &state, 
			*mystate = filp->private_data;
	struct vmod12e16_conversion
			conversion, 
			*conversionp = &conversion;
	int lun, channel, err;

	switch (cmd) {
	case VMOD12E16_IOCSELECT:
		if (!access_ok(VERIFY_READ, (void*)arg, sizeof(state))
		  || copy_from_user(statep, (void*)arg, sizeof(state)))
			return -EINVAL;

		lun = statep->lun;
		if (!((lun > 0)  && (lun <= MODULBUS_MAX_MODULES))
			    || modules[lun].lun == INVALID_LUN) {
			printk(KERN_ERR "invalid LUN %d\n", lun);
			return -EINVAL;
		}

		channel = statep->channel;
		if (!((channel >= 0) && (channel < VMOD_12E16_CHANNELS))) {
			printk(KERN_ERR "invalid channel %d\n", channel);
			return -EINVAL;
		}

		mystate->lun		= statep->lun;
		mystate->channel	= statep->channel;
		mystate->selected	= 1;

		return 0;
		break;


	case VMOD12E16_IOCCONVERT:
		if (!access_ok(VERIFY_READ,  (void*)arg, sizeof(conversion))
		  ||!access_ok(VERIFY_WRITE, (void*)arg, sizeof(conversion))
		  ||copy_from_user(conversionp, (void*)arg, sizeof(conversion)))
			return -EINVAL;

		if (!mystate->selected) {
			printk(KERN_ERR "Invalid ioctl: CONVERT before SELECT\n");
			return -ENOTTY;
		}

		err = do_conversion(filp, conversionp);
		if (err != 0)
			return -EINVAL;

		if (copy_to_user((void*)arg, conversionp, sizeof(conversion)))
			return -EINVAL;

		return 0;
		break;

	default:
		return -ENOTTY;
		break;
	}
	return 0;
}

struct file_operations fops = {
	.owner =    THIS_MODULE,
	.ioctl =    do_ioctl,
	.open =     do_open,
	.release =  do_release,
};

/* module initialization and cleanup */
static int __init init(void)
{
	printk(KERN_INFO "Initializing module  with %d/%d = %d luns\n", 
				num, PARAMS_PER_LUN, num/PARAMS_PER_LUN);

	/* scan and check command-line parameters */
	if (init_module_table(luns, num) != 0)
		goto fail_chrdev;
	if (used_modules == 0) {
		printk(KERN_ERR "no devices configured, exiting\n");
		goto fail_chrdev;
	}
	printk(KERN_INFO "Configuration for %d modules ok\n", used_modules);

	if (alloc_chrdev_region(&devno, 0, MODULBUS_MAX_MODULES, devname) != 0)
		goto fail_chrdev;
	printk(KERN_INFO "Allocated devno 0x%x for device %s\n",devno, devname); 

	/* create character device */
	cdev_init(&cdev, &fops);
	cdev.owner = THIS_MODULE;
	if (cdev_add(&cdev, devno, 1) != 0)
		goto fail_cdev;
	printk(KERN_INFO "Added cdev %s with devno 0x%x\n", devname, devno);

	return 0;

fail_cdev:	
	unregister_chrdev_region(devno, MODULBUS_MAX_MODULES);
fail_chrdev:	
	return -1;
}

static void __exit exit(void)
{
	cdev_del(&cdev);
	unregister_chrdev_region(devno, MODULBUS_MAX_MODULES);
}


MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);

