#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include "vmoddor.h"
#include "modulbus_register.h"
#include "lunargs.h"

#define DRIVER_NAME	"vmoddor"
#define PFX		DRIVER_NAME ": "

static struct vmod_devices    dt, *dev_table = &dt;

static struct vmoddor_dev vmoddor[VMODDOR_MAX_DEVICES];

static struct cdev	*cdev;
static dev_t		dev;

static inline void vmoddor_write_word(struct vmoddor_dev *pd, int offset, uint16_t value)
{
        unsigned long ioaddr = pd->handle + offset;

	if (pd->is_big_endian)
        	iowrite16be(value, (u16 *)ioaddr);
	else
		iowrite16(value, (u16 *)ioaddr);
}

static void vmoddor_write_byte(struct vmoddor_dev *pd, int offset, uint8_t value)
{
        unsigned long ioaddr = pd->handle + offset;

        iowrite8(value, (u8 *)ioaddr);
}

static void vmoddor_write_4bits(struct vmoddor_dev *pd, int offset, uint8_t value)
{
        vmoddor_write_byte(pd, offset, value & 0xf);
}

int vmoddor_open(struct inode *ino, struct file *filp)
{
       return 0;
}

int vmoddor_release(struct inode *ino, struct file *filp)
{
        return 0;
}

static int vmoddor_ioctl(struct inode *inode, struct file *fp, unsigned op,
                        				unsigned long arg)
{
        unsigned int		minor = iminor(inode);
        struct vmoddor_dev	*pd =  &vmoddor[minor];

	if (pd == NULL)
		return -EINVAL;

	switch(op){
	case VMODDOR_WRITE:
	{
		struct vmoddor_arg val;

		if(copy_from_user((char *)&val, (char *)arg, sizeof(struct vmoddor_arg)))
			return -EIO;

		switch(val.size){
		case 16:
			if(val.offset != 0)
				return -EINVAL;

			vmoddor_write_word(pd, 0, val.data);
			break;
		
		case 8:
			if(val.offset > 1)
				return -EINVAL;
			
			vmoddor_write_byte(pd, val.offset, val.data & 0xff);
			break;

		case 4:
			if((val.offset < 2) || (val.offset > 5))
				return -EINVAL;			
	
			vmoddor_write_4bits(pd, val.offset, val.data & 0xff);
			break;
		default:
			return -EINVAL;
		}
	}
	break;

	default:
		printk(KERN_INFO PFX "Ioctl operation not implemented\n");
		return -EINVAL;

	}

	return 0;
}

struct file_operations vmoddor_fops = {
        .owner =    THIS_MODULE,
        .ioctl =    vmoddor_ioctl,
        .open =     vmoddor_open,
        .release =  vmoddor_release
};


static int __init vmoddor_init(void)
{
	int err;
	int major = 0;
	int minor = 0;
	int i;
	
	printk (KERN_INFO PFX "Init Module\n");

	err = read_params(DRIVER_NAME, dev_table);
        if (err != 0)
                return -1;

        printk(KERN_INFO PFX "initialized driver for %d (max %d) cards\n",
		dev_table->num_modules, VMODDOR_MAX_DEVICES);

	err = alloc_chrdev_region(&dev, 0, VMODDOR_MAX_DEVICES, "vmoddor");	
	if (err){
		printk(KERN_ALERT PFX "Cannot allocate character device\n");
		goto error;
	}

	major = MAJOR(dev);
	minor = MINOR(dev);
	cdev = cdev_alloc();
	if (cdev == NULL)
		goto error;

	cdev_init(cdev, &vmoddor_fops);
	cdev->owner = THIS_MODULE;
	cdev->ops = &vmoddor_fops;

	err = cdev_add(cdev, dev, VMODDOR_MAX_DEVICES);
	if (err){
		printk(KERN_ALERT PFX "Error %d adding device\n", err);
		goto error_cdev_add;
	}

	for(i = 0; i < dev_table->num_modules; i++){ 
		struct vmod_dev	*mod = &dev_table->module[i];

	        vmoddor[i].lun		= mod->lun;
                vmoddor[i].cname    	= mod->carrier_name;
                vmoddor[i].carrier  	= mod->carrier_lun;
                vmoddor[i].slot     	= mod->slot;
                vmoddor[i].handle  	= mod->address;

                vmoddor[i].is_big_endian = mod->is_big_endian;
	}

	return 0;

error_cdev_add: 
	cdev_del(cdev);
	unregister_chrdev_region(dev, VMODDOR_MAX_DEVICES);
error:
	return -1;
}

static void __exit vmoddor_exit(void)
{
	cdev_del(cdev);
	unregister_chrdev_region(dev, VMODDOR_MAX_DEVICES);
	printk(KERN_INFO PFX "Exit module.\n");
}

module_init(vmoddor_init);
module_exit(vmoddor_exit);

MODULE_AUTHOR("Samuel I. Gonsalvez");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VMODDOR device driver");
MODULE_VERSION("1.0");

