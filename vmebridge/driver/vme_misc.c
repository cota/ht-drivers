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
 * vme_register_berr_handler() - register a VME Bus Error handler
 * @error:	Bus Error descriptor: Initial Address + Address Modifier
 * @size:	Size of the address range of interest. The Initial Address
 * 		is the address provided in @error.
 * @func:	Handler function for the bus errors in the range above.
 *
 * NOTE: the handler function will be called in interrupt context.
 * Return the address of the registered handler on success, ERR_PTR otherwise.
 */
struct vme_berr_handler *
vme_register_berr_handler(struct vme_bus_error *error, size_t size,
			vme_berr_handler_t func)
{
	spinlock_t *lock = &vme_bridge->verr.lock;
	struct vme_berr_handler *handler;
	unsigned long flags;

	if (!size) {
		printk(KERN_WARNING PFX "%s: size cannot be 0.\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	handler = kzalloc(sizeof(struct vme_berr_handler), GFP_KERNEL);
	if (handler == NULL) {
		printk(KERN_ERR PFX "Unable to allocate Bus Error Handler\n");
		return ERR_PTR(-ENOMEM);
	}

	spin_lock_irqsave(lock, flags);

	memcpy(&handler->error, error, sizeof(struct vme_bus_error));
	handler->size = size;
	handler->func = func;
	list_add_tail(&handler->h_list, &vme_bridge->verr.h_list);

	spin_unlock_irqrestore(lock, flags);

	return handler;
}
EXPORT_SYMBOL_GPL(vme_register_berr_handler);

static void __vme_unregister_berr_handler(struct vme_berr_handler *handler)
{
	struct list_head *h_pos, *temp;
	struct vme_berr_handler *entry;

	list_for_each_safe(h_pos, temp, &vme_bridge->verr.h_list) {
		entry = list_entry(h_pos, struct vme_berr_handler, h_list);

		if (entry == handler) {
			list_del(h_pos);
			kfree(entry);
			return;
		}
	}
	printk(KERN_WARNING PFX "%s: handler not found\n", __func__);
}

/**
 * vme_unregister_berr_handler - Unregister a VME Bus Error Handler
 * @handler:	Address of the registered handler to be removed.
 */
void vme_unregister_berr_handler(struct vme_berr_handler *handler)
{
	spinlock_t *lock = &vme_bridge->verr.lock;
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	__vme_unregister_berr_handler(handler);
	spin_unlock_irqrestore(lock, flags);
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

