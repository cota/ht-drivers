/*
 * VMEBus Monitor
 *
 * This module is intended to be a multi-purpose VME diagnostic tool.
 *
 * Currently it supports:
 * - VME Bus Error notification
 * - Debugging of creation/removal of VME mappings
 *
 * Copyright (c) 2010 Emilio G. Cota <cota@braap.org>
 */
#include <linux/module.h>
#include <linux/err.h>

#include "../include/vmebus.h"

#define PFX "VMEMon: "
#define VMEMON_MAPPINGS_MAX	20

static int am = VME_A32_USER_DATA_SCT;
static unsigned int size = 1024;
static unsigned int offset = 0;

module_param(am, int, 0644);
MODULE_PARM_DESC(am, "Address Modifier");
module_param(size, int, 0644);
MODULE_PARM_DESC(size, "Size of the region of interest");
module_param(offset, int, 0644);
MODULE_PARM_DESC(offset, "Offset of the region of interest");

static int addr_mod[VMEMON_MAPPINGS_MAX];
static int addr_mod_num;
MODULE_PARM_DESC(addr_mod, "Enumeration of address modifiers to map");
module_param_array(addr_mod, int, &addr_mod_num, 0);

static int data_width[VMEMON_MAPPINGS_MAX];
static int data_width_num;
MODULE_PARM_DESC(data_width, "Enumeration of data widths to map");
module_param_array(data_width, int, &data_width_num, 0);

static int base_addr[VMEMON_MAPPINGS_MAX];
static int base_addr_num;
MODULE_PARM_DESC(base_addr, "Enumeration of VME addresses to be mapped");
module_param_array(base_addr, int, &base_addr_num, 0);

static int window_size[VMEMON_MAPPINGS_MAX];
static int window_size_num;
MODULE_PARM_DESC(window_size, "Enumeration of data widths to map");
module_param_array(window_size, int, &window_size_num, 0);

static long kaddr[VMEMON_MAPPINGS_MAX];

static struct vme_berr_handler *handler;

static void berr_handler(struct vme_bus_error *error)
{
	printk(KERN_INFO PFX "Bus Error @ 0x%llx am=0x%x\n",
		(unsigned long long)error->address, error->am);
}

static void __init vmemon_pdparam_init(struct pdparam_master *param)
{
	memset(param, 0, sizeof(*param));
	param->iack   = 1;		/* no iack */
	param->rdpref = 0;		/* no VME read prefetch option */
	param->wrpost = 0;		/* no VME write posting option */
	param->swap   = 1;		/* VME auto swap option */
	param->sgmin  = 0;              /* reserved, _must_ be 0  */
	param->dum[0] = VME_PG_SHARED;	/* window is shareable */
	param->dum[1] = 0;		/* XPC ADP-type */
	param->dum[2] = 0;		/* reserved, _must_ be 0 */
}

static void __init vmemon_map(void)
{
	struct pdparam_master pdp;
	int i;

	if (!addr_mod_num)
		return;

	if (data_width_num	!= addr_mod_num ||
		base_addr_num	!= addr_mod_num ||
		window_size_num	!= addr_mod_num) {

		printk(KERN_INFO PFX "Invalid mapping input, aborting.\n");
		addr_mod_num = 0;
		return;
	}

	vmemon_pdparam_init(&pdp);

	for (i = 0; i < addr_mod_num; i++) {
		unsigned long dw = data_width[i] / 8;

		kaddr[i] = find_controller(base_addr[i], window_size[i],
					addr_mod[i], 0, dw, &pdp);
		if (kaddr[i] == -1) {
			printk(KERN_ERR PFX "Mapping %d failed: base=0x%08x "
				"size=0x%08x am=%d dw=%d\n", i, base_addr[i],
				window_size[i], addr_mod[i], data_width[i]);
			kaddr[i] = 0;
			continue;
		}
	}
}

static int __init vmemon_init(void)
{
	struct vme_bus_error error;

	error.address = offset;
	error.am = am;

	printk(KERN_INFO PFX "Registering berr handler, "
		"offset=0x%x size=0x%d am=0x%x\n", offset, size, am);

	handler = vme_register_berr_handler(&error, size, berr_handler);

	if (IS_ERR(handler)) {
		printk(KERN_ERR PFX "Could not register Berr handler: %ld.\n",
			PTR_ERR(handler));
		return PTR_ERR(handler);
	}

	vmemon_map();

	printk(KERN_INFO PFX "Berr handler registered successfully\n");
	return 0;
}

static void __exit vmemon_unmap(void)
{
	int i;

	if (!addr_mod_num)
		return;

	printk(KERN_INFO PFX "Returning %d mappings", addr_mod_num);

	for (i = 0; i < addr_mod_num; i++) {
		if (return_controller(kaddr[i], window_size[i])) {
			printk(KERN_WARNING PFX "Couldn't unmap %d.", i);
			continue;
		}
		kaddr[i] = 0;
	}
}

static void __exit vmemon_exit(void)
{
	printk(KERN_INFO PFX "Unregistering berr handler\n");
	vme_unregister_berr_handler(handler);
	vmemon_unmap();
}

module_init(vmemon_init);
module_exit(vmemon_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emilio G. Cota");
MODULE_DESCRIPTION("VME Monitor");
