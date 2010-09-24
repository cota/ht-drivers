#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* copy_*_user */

#include "vmod16a2.h"
#include "lunargs.h"
#include "modulbus_register.h"

#define DRIVER_NAME	"vmod16a2"
#define PFX		DRIVER_NAME ": "

/* The One And Only Device (OAOD) */
static dev_t devno;
static struct cdev cdev;

/* module config tables */
static struct vmod_devices 	config;

static int open(struct inode *ino, struct file *filp)
{
	unsigned int lun = iminor(ino);
	unsigned int idx = lun_to_index(&config, lun);

	if (idx < 0) {
		printk(KERN_ERR PFX 
			"cannot open, invalid lun %d\n", lun);
		return -EINVAL;
	}
	filp->private_data = &config.module[idx];
	return 0;
}

static int release(struct inode *ino, struct file *filp)
{
	return 0;
}

static int do_output(struct vmod_dev *dev,
		struct vmod16a2_convert *cvrt)
{
	int value = cvrt->value;
	int channel = cvrt->channel;
	struct vmod16a2_registers __iomem *regp = 
		(struct vmod16a2_registers __iomem *)dev->address;

	if (channel == 0) {
		iowrite16be(value, &(regp->dac0in));
		iowrite16be(value, &(regp->ldac0));
	} else if (channel == 1) {
		iowrite16be(value, &(regp->dac1in));
		iowrite16be(value, &(regp->ldac1));
	} else {
		printk(KERN_ERR PFX "invalid channel %d\n", channel);
		return -EINVAL;
	}
	return 0;
}

static int ioctl(struct inode *inode,
		struct file *fp,
		unsigned op,
		unsigned long arg)
{
	struct vmod_dev *devp = fp->private_data;
	struct vmod16a2_convert cvrt, *cvrtp = &cvrt;

	switch (op) {

	case VMOD16A2_IOCPUT:
		if (copy_from_user(cvrtp, (const void __user*)arg, sizeof(cvrt)) != 0)
			return -EINVAL;
		return do_output(devp, cvrtp);
		break;

	default:
		return -ENOTTY;
	}
	return -ENOTTY;
}

/* @brief file operations for this driver */
static struct file_operations vmod16a2_fops = {
	.owner =    THIS_MODULE,
	.ioctl =    ioctl,
	.open =     open,
	.release =  release,
};

/* module initialization and cleanup */
static int __init init(void)
{
	int err;

	printk(KERN_INFO PFX "initializing driver");
	err = read_params(DRIVER_NAME, &config);
	if (err != 0)
		return -1;

	err = alloc_chrdev_region(&devno, 0, VMOD16A2_MAX_MODULES, DRIVER_NAME);
	if (err != 0)
		goto fail_chrdev;
	printk(KERN_INFO PFX "allocated device %0d\n", MAJOR(devno));

	cdev_init(&cdev, &vmod16a2_fops);
	cdev.owner = THIS_MODULE;
	err = cdev_add(&cdev, devno, VMOD16A2_MAX_MODULES);
	if (err) {
		printk(KERN_ERR PFX
			"failed to create chardev %d with err %d\n",
				MAJOR(devno), err);
		goto fail_cdev;
	}
	return 0;

fail_cdev:	unregister_chrdev_region(devno, VMOD16A2_MAX_MODULES);
fail_chrdev:	return -1;
}

static void __exit exit(void)
{
	cdev_del(&cdev);
	unregister_chrdev_region(devno, VMOD16A2_MAX_MODULES);
}

module_init(init);
module_exit(exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan David Gonzalez Cobas <dcobas@cern.ch>");
