#include <linux/kernel.h>
#include <linux/module.h>
#include "modulbus_register.h"
#include "lunargs.h"

#define PFX "VMOD args: "

/* module parameters */
static int lun[VMOD_MAX_BOARDS];
static unsigned int num_lun;
module_param_array(lun, int, &num_lun, S_IRUGO);
MODULE_PARM_DESC(lun, "Logical Unit Number of the VMOD-XXX board");

static char *carrier[VMOD_MAX_BOARDS];
static unsigned int num_carrier;
module_param_array(carrier, charp, &num_carrier, S_IRUGO);
MODULE_PARM_DESC(carrier, "Name of the carrier in which the VMOD-XXX is plugged in.");

static int carrier_number[VMOD_MAX_BOARDS];
static unsigned int num_carrier_number;
module_param_array(carrier_number, int, &num_carrier_number, S_IRUGO);
MODULE_PARM_DESC(carrier_number, "Logical Unit Number of the carrier");

static int slot[VMOD_MAX_BOARDS];
static unsigned int num_slot;
module_param_array(slot, int, &num_slot, S_IRUGO);
MODULE_PARM_DESC(slot, "Slot of the carrier in which the VMOD-XXX board is placed.");

static int set_module_params(struct vmod_dev *module,
	int lun, char *cname, int carrier_lun, int slot)
{
	gas_t			get_address_space;
	struct carrier_as	as, *asp = &as;

	/* Sanity check parameters */
	if (lun < 0) {
		printk(KERN_ERR PFX "invalid lun: %d\n", lun);
		return -1;
	}
	get_address_space = modulbus_carrier_as_entry(cname);
	if (get_address_space == NULL) {
		printk(KERN_ERR PFX "No carrier %s\n", cname);
		return -1;
	} else
		printk(KERN_INFO PFX 
			"Carrier %s get_address_space entry point at %p\n",
			cname, get_address_space);
	if (get_address_space(asp, carrier_lun, slot, 1) < 0){
		printk(KERN_ERR PFX 
			"Invalid carrier number: %d or slot: %d\n",
			carrier_lun, slot);
		return -1;
	}

	/* Parameters OK, go ahead */
	module->lun           = lun;           
	module->carrier_name  = cname;       
	module->carrier_lun   = carrier_lun;
	module->slot          = slot;          
	module->address	      = asp->address;
	module->is_big_endian = asp->is_big_endian;
	return 0;
}

int read_params(char *driver_name, struct vmod_devices *devs)
{
	int i, device;

	/* check that all insmod argument vectors are the same length */
	if (num_lun != num_carrier || num_lun != num_carrier_number || 
		num_lun != num_slot || num_lun >= VMOD_MAX_BOARDS) {
		printk(KERN_ERR PFX "the number of parameters doesn't match or is invalid\n");
		return -1;
	}

	device = 0;
	for (i=0; i < num_lun ; i++) {
		int 			ret;
		struct vmod_dev 	*module = &devs->module[i];

		printk(KERN_INFO PFX 
			"configuring %s lun %d\n", driver_name, lun[i]);
		ret = set_module_params(module, 
			lun[i], carrier[i], carrier_number[i], slot[i]);
		if (ret != 0)
			return -1;

		device++;
		printk(KERN_INFO PFX
			"module<%02d>: lun %d"
			" carrier %s carrier_number = %d slot = %d"
			" address 0x%08lx endian = %d\n",
			i, module->lun, module->carrier_name, 
			module->carrier_lun, module->slot,
			module->address, module->is_big_endian);
	}
	devs->num_modules = device;
        
        return 0;
}

int lun_to_index(struct vmod_devices *devs, int lun)
{
	int i;

	for (i = 0; i < devs->num_modules; i++) 
		if (lun == devs->module[i].lun)
			return i;
	return -1;
}
