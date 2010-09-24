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

/*
 *	Definition of module's parameters
 */

static int lun[VMODDOR_MAX_DEVICES];
static int num_luns;
module_param_array(lun, int, &num_luns, S_IRUGO);

static char *carrier[VMODDOR_SIZE_CARRIER_NAME * VMODDOR_MAX_DEVICES];
static int num_carrier;
module_param_array(carrier, charp, &num_carrier, S_IRUGO);

static int carrier_number[VMODDOR_MAX_DEVICES];
static int num_carrier_number;
module_param_array(carrier_number, int, &num_carrier_number, S_IRUGO);

static int slot[VMODDOR_MAX_DEVICES];
static int num_slot;
module_param_array(slot, int, &num_slot, S_IRUGO);

static struct vmoddor_dev vmoddor[VMODDOR_MAX_DEVICES];
static int		lun_to_dev[VMODDOR_MAX_DEVICES];

static struct cdev	*cdev;
static dev_t		dev;

static uint16_t vmoddor_read_word(struct vmoddor_dev *pd, int offset)
{
	unsigned long ioaddr =  pd->handle + offset;

	if (pd->is_big_endian)
		return ioread16be((u16 *)ioaddr);
	else
		return ioread16((u16 *)ioaddr);
}

static void vmoddor_write_word(struct vmoddor_dev *pd, int offset, uint16_t value)
{
        unsigned long ioaddr = pd->handle + offset;

	if (pd->is_big_endian)
        	iowrite16be(value, (u16 *)ioaddr);
	else
		iowrite16(value, (u16 *)ioaddr);
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

        unsigned int	minor = iminor(inode);
        struct vmoddor_dev	*pd =  &vmoddor[minor];

	switch(op){
	case VMODDOR_WRITE:
	{
		int val;
		uint16_t data;

		if(copy_from_user((char *)&val, (char *)arg, sizeof(int)))
			return -EIO;

		data = val;
		vmoddor_write_word(pd, 0, data);

#if 0
		val = -1;
		val = vmoddor_read_word(pd, 0);

		printk(KERN_INFO "VMODDOR: the written value is %d\n", val);
#endif 
	}
		break;

	case VMODDOR_READ:
	{
		int val;

		val = vmoddor_read_word(pd, 0);
		if(copy_to_user((char *)&val, (char *)arg, sizeof(int)))
				return -EIO;
	}
		break;

	default:
		printk(KERN_INFO "VMODDOR: Ioctl operation not implemented\n");
		return -EINVAL;

	}

	return 0;
		

}

/*
 * check_module args (void)
 *	Check the command-line arguments passed to the module
 *	and save them.
 */
static int check_module_args(void)
{

	int i = 0;

        if((num_luns != num_carrier) || (num_luns != num_carrier_number) || (num_luns != num_slot) ||
                (num_luns > VMODDOR_MAX_DEVICES)) {
                printk(KERN_ERR "VMODDOR: the number of parameters doesn't match or it is invalid\n");
                return -1; 
        }
    
        for(i=0; i < num_luns ; i++){
                int lun_id                 = (int) lun[i];
                char *cname             = carrier[i];
                int carrier_num         = (int)carrier_number[i];
                int slot_board          = (int)slot[i];

                struct carrier_as as, *asp  = &as;
                gas_t get_address_space;
			 
			 if (lun_id < 0 || lun_id > VMODDOR_MAX_DEVICES){
				 printk(KERN_ERR "VMODDOR: Invalid lun: %d\n", lun_id);
				 continue;
			 }

                get_address_space = modulbus_carrier_as_entry(cname);
                if (get_address_space == NULL) {
                        printk(KERN_ERR "No carrier %s\n", cname);
                        return -1; 
                } else {
                        printk(KERN_INFO "Carrier %s a.s. entry point at %p\n",
                                cname, get_address_space);
		}

                if (get_address_space(asp, carrier_num, slot_board, 1) < 0){
				 printk(KERN_ERR "VMODDOR: Invalid carrier number: %d or slot: %d\n",
					   carrier_num, slot_board);
                     continue;
		}

                vmoddor[i].lun		= lun_id;
                vmoddor[i].cname    	= cname;
                vmoddor[i].carrier  	= carrier_num;
                vmoddor[i].slot     	= slot_board;
                vmoddor[i].handle  	= asp->address;

                vmoddor[i].is_big_endian = asp->is_big_endian;
                lun_to_dev[lun_id]       = i;

                printk(KERN_INFO
                        "VMODDOR module<%d>: lun %d with "
                        "carrier %s carrier_number = %d slot = %d\n",
                        i, lun_id, cname, carrier_num, slot_board);
        }

        printk(KERN_INFO "VMODDOR configured modules = %d\n", i);
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

	int tmp = 0;
	int major = 0;
	int minor = 0;
	
	printk (KERN_INFO "VMODDOR: Init Module\n");

	tmp = check_module_args();

	tmp = alloc_chrdev_region(&dev, 0, VMODDOR_MAX_DEVICES, "vmoddor");	

	if (tmp){
		printk(KERN_ALERT "VMODDOR: Cannot allocate character device\n");
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

	tmp = cdev_add(cdev, dev, VMODDOR_MAX_DEVICES);
	if (tmp){
		printk(KERN_ALERT "VMODDOR: Error %d adding device\n",tmp);
		goto error_adding;
	}

	return 0;

	error_adding: 
		cdev_del(cdev);
		unregister_chrdev_region(dev, VMODDOR_MAX_DEVICES);

	error:
		return -1;
}

static void __exit vmoddor_exit(void)
{

	cdev_del(cdev);
	unregister_chrdev_region(dev, VMODDOR_MAX_DEVICES);
	printk(KERN_INFO "VMODDOR: Exit module.\n");

}

module_init(vmoddor_init);
module_exit(vmoddor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samuel I. Gonsalvez");
MODULE_DESCRIPTION("VMODDOR device driver");
MODULE_VERSION("1.0");
