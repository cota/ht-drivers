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

static inline int
__vme_bus_error_check_clear(struct vme_bus_error *err)
{
	struct vme_bus_error_desc *desc = &vme_bridge->verr.desc;
	struct vme_bus_error *vme_err = &desc->error;

	if (desc->valid && vme_err->am == err->am &&
		vme_err->address == err->address) {

		desc->valid = 0;
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

