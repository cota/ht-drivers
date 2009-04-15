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

