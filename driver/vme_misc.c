/*
 * vme_misc.c - PCI-VME bridge miscellaneous interfaces
 *
 * Copyright (c) 2009 Sebastien Dugue
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

/*
 *  This file provides some PCI-VME bridge miscellaneous interfaces:
 *
 *    - Access to VME control (requestor, arbitrer) - Not implemented yet
 *    - VME Bus error checking
 */

#include "vme_bridge.h"

int vme_bus_error_check(int clear)
{
	return tsi148_bus_error_chk(vme_bridge->regs, clear);
}
EXPORT_SYMBOL_GPL(vme_bus_error_check);

static inline void __vme_bus_error_update(void)
{
	tsi148_handle_vme_error();
}

static int
__vme_bus_error_check_clear(struct vme_bus_error *err)
{
	struct vme_bus_error *vme_err = &vme_bridge->verr.error;

	__vme_bus_error_update();

	printk("vme_err->valid: %d\n", vme_err->valid);
	printk("vme_err->am: %x\n", vme_err->am);
	printk("vme_err->address: %8llx\n", (unsigned long long)vme_err->address);

	printk("err->valid: %d\n", err->valid);
	printk("err->am: %x\n", err->am);
	printk("err->address: %8llx\n", (unsigned long long)err->address);

	if (vme_err->valid && vme_err->am == err->am &&
		vme_err->address == err->address) {

		vme_err->valid = 0;
		return 1;
	}

	return 0;
}

/**
 * vme_bus_error_check_clear - check and clear VME bus errors
 * @bus_error:	bus error to be checked
 *
 * Note: the bus error is only cleared if it matches the given bus
 * error descriptor.
 */
int vme_bus_error_check_clear(struct vme_bus_error *err)
{
	unsigned long flags;
	spinlock_t *lock;
	int ret;

	lock = &vme_bridge->verr.lock;

	spin_lock_irqsave(lock, flags);
	ret = __vme_bus_error_check_clear(err);
	spin_unlock_irqrestore(lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(vme_bus_error_check_clear);

ssize_t vme_misc_read(struct file *file, char *buf, size_t count,
			loff_t *ppos)
{
	return 0;
}

ssize_t vme_misc_write(struct file *file, const char *buf, size_t count,
		  loff_t *ppos)
{
	return 0;
}

long vme_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOIOCTLCMD;
}

