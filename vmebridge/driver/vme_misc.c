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

static struct vme_berr_handler *
__vme_register_berr_handler(struct vme_bus_error *error, size_t count,
			vme_berr_handler_t func)
{
	struct vme_berr_handler *handler;

	handler = kzalloc(sizeof(struct vme_berr_handler), GFP_KERNEL);
	if (handler == NULL) {
		printk("shit shit shit");
		return NULL;
	}

	memcpy(&handler->error, error, sizeof(struct vme_bus_error));
	handler->count = count;
	handler->func = func;
	list_add_tail(&handler->list, &vme_bridge->verr.handlers_list);

	return handler;
}

struct vme_berr_handler *
vme_register_berr_handler(struct vme_bus_error *error, size_t count,
			vme_berr_handler_t func)
{
	struct mutex *lock = &vme_bridge->verr.handlers_lock;
	struct vme_berr_handler *ret;

	mutex_lock(lock);
	ret = __vme_register_berr_handler(error, count, func);
	mutex_unlock(lock);

	return ret;
}
EXPORT_SYMBOL_GPL(vme_register_berr_handler);

static void __vme_unregister_berr_handler(struct vme_berr_handler *handler)
{
	struct list_head *handler_pos, *temp;
	struct vme_berr_handler *entry;

	list_for_each_safe(handler_pos, temp, &vme_bridge->verr.handlers_list) {
		entry = list_entry(handler_pos, struct vme_berr_handler, list);

		if (entry == handler) {
			list_del(handler_pos);
			kfree(entry);
			return;
		}
	}
	printk(KERN_WARNING PFX "%s: handler not found\n", __func__);
}

void vme_unregister_berr_handler(struct vme_berr_handler *handle)
{
	struct mutex *lock = &vme_bridge->verr.handlers_lock;

	mutex_lock(lock);
	__vme_unregister_berr_handler(handle);
	mutex_unlock(lock);
}
EXPORT_SYMBOL_GPL(vme_unregister_berr_handler);

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

