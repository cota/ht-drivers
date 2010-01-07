/*
 * VMEBus Monitor
 *
 * This module is intended to be a multi-purpose VME diagnostic tool.
 *
 * Currently it supports:
 * - VME Bus Error notification
 *
 * Copyright (c) 2010 Emilio G. Cota <cota@braap.org>
 */
#include <linux/module.h>
#include <linux/err.h>

#include "../include/vmebus.h"

#define PFX "VMEMon: "

static int am = VME_A32_USER_DATA_SCT;
static unsigned int size = 1024;
static unsigned int offset = 0;

module_param(am, int, 0644);
MODULE_PARM_DESC(am, "Address Modifier");
module_param(size, int, 0644);
MODULE_PARM_DESC(size, "Size of the region of interest");
module_param(offset, int, 0644);
MODULE_PARM_DESC(offset, "Offset of the region of interest");

static struct vme_berr_handler *handler;

static void berr_handler(struct vme_bus_error *error)
{
	printk(KERN_INFO PFX "Bus Error @ 0x%llx am=0x%x\n",
		(unsigned long long)error->address, error->am);
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

	printk(KERN_INFO PFX "Berr handler registered successfully\n");
	return 0;
}

static void __exit vmemon_exit(void)
{
	printk(KERN_INFO PFX "Unregistering berr handler\n");
	vme_unregister_berr_handler(handler);
}

module_init(vmemon_init);
module_exit(vmemon_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emilio G. Cota");
MODULE_DESCRIPTION("VME Monitor");
