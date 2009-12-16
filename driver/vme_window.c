/*
 * vme_window.c - PCI-VME window management
 *
 * Copyright (c) 2009 Sebastien Dugue
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/*
 *  This file provides the PCI-VME bridge window management support:
 *
 *    - Window creation and deletion
 *    - Mapping creation and removal
 *    - Procfs interface to windows and mappings information
 */

#include <linux/list.h>
#include <linux/pci.h>
#include <asm/uaccess.h>

#include "vmebus.h"
#include "vme_bridge.h"


/**
 * struct window - Hardware window descriptor.
 * @lock: Mutex protecting the descriptor
 * @active: Flag indicating whether the window is in use
 * @rsrc: PCI bus resource of the window
 * @desc: This physical window descriptor
 * @mappings: List of mappings using this window
 * @users: Number of users of this window
 *
 * This structure holds the information concerning hardware
 * windows.
 *
 */
struct window {
	struct mutex		lock;
	unsigned int		active;
	struct resource		rsrc;
	struct vme_mapping	desc;
	struct list_head	mappings;
	int 			users;
};


struct window window_table[TSI148_NUM_OUT_WINDOWS];

/**
 * struct mapping_taskinfo - Store information about a mapping's user
 *
 * @pid: pid number
 * @name: name of the process
 *
 * @note on the name length.
 *       As it's not only the process name that is stored here, but also
 *       module name -- lengths should be MAX allowed for the module name
 */
struct vme_taskinfo {
	pid_t	pid_nr;
	char	name[MODULE_NAME_LEN];
};

/**
 * struct mapping - Logical mapping descriptor
 * @list: List of the mappings
 * @mapping: The mapping descriptor
 * @client: The user of this mapping
 *
 * This structure holds the information concerning logical mappings
 * made on top a hardware windows.
 */
struct mapping {
	struct list_head	list;
	struct vme_mapping	desc;
	struct vme_taskinfo	client;
};

/*
 * Flag controlling whether to create a new window if a mapping cannot
 * be found.
 */
unsigned int vme_create_on_find_fail;
/*
 * Flag controlling whether removing the last mapping on a window should
 * also destroys the window.
 */
unsigned int vme_destroy_on_remove;


#ifdef CONFIG_PROC_FS

/* VME address modifiers names */
static char *amod[] = {
	"A64MBLT", "A64", "Invalid 0x02", "A64BLT",
	"A64LCK", "A32LCK", "Invalid 0x06", "Invalid 0x07",
	"A32MBLT USER", "A32 USER DATA", "A32 USER PROG", "A32BLT USER",
	"A32MBLT SUP", "A32 SUP DATA", "A32 SUP PROG", "A32BLT SUP",
	"Invalid 0x10", "Invalid 0x11", "Invalid 0x12", "Invalid 0x13",
	"Invalid 0x14", "Invalid 0x15", "Invalid 0x16", "Invalid 0x17",
	"Invalid 0x18", "Invalid 0x19", "Invalid 0x1a", "Invalid 0x1b",
	"Invalid 0x1c", "Invalid 0x1d", "Invalid 0x1e", "Invalid 0x1f",
	"2e6U", "2e3U", "Invalid 0x22", "Invalid 0x23",
	"Invalid 0x24", "Invalid 0x25", "Invalid 0x26", "Invalid 0x27",
	"Invalid 0x28", "A16 USER", "Invalid 0x2a", "Invalid 0x2b",
	"A16LCK", "A16 SUP", "Invalid 0x2e", "CR/CSR",
	"Invalid 0x30", "Invalid 0x31", "Invalid 0x32", "Invalid 0x33",
	"A40", "A40LCK", "Invalid 0x36", "A40BLT",
	"A24MBLT USER", "A24 USER DATA", "A24 USER PROG", "A24BLT USER",
	"A24MBLT SUP", "A24 SUP DATA", "A24 SUP PROG", "A24BLT SUP"
};

static int vme_window_proc_show_mapping(char *page, int num,
					struct mapping *mapping)
{
	char *p = page;
	char *pfs;
	struct vme_mapping *desc = &mapping->desc;

/*
           va       PCI     VME      Size     Address Modifier   Data   Prefetch
                                                 Description     Width   Size
    dd: xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx - ssssssssssssssss /  sss    sssss
    client: ddddd ssssssssssssssss
*/
	p += sprintf(p, "    %2d: %p %.8x %.8x %.8x - ",
		     num,
		     desc->kernel_va, desc->pci_addrl,
		     desc->vme_addrl, desc->sizel);

	if (desc->read_prefetch_enabled)
		switch (desc->read_prefetch_size) {
		case VME_PREFETCH_2:
			pfs = "PFS2";
			break;
		case VME_PREFETCH_4:
			pfs = "PFS4";
			break;
		case VME_PREFETCH_8:
			pfs = "PFS8";
			break;
		case VME_PREFETCH_16:
			pfs = "PFS16";
			break;
		default:
			pfs = "?";
			break;
		}
	else
		pfs = "NOPF";

	p += sprintf(p, "(0x%02x)%s / D%2d %5s\n",
		     desc->am, amod[desc->am], desc->data_width, pfs);
	p += sprintf(p, "        client: %d %s\n",
		(int)mapping->client.pid_nr, mapping->client.name);

	return p - page;
}

static int vme_window_proc_show_window(char *page, int window_num)
{
	char *p = page;
	char *pfs;
	struct window *window = &window_table[window_num];
	struct mapping *mapping;
	struct vme_mapping *desc;
	int count = 0;


	p += sprintf(p, "Window %d:  ", window_num);

	if (!window->active)
		p += sprintf(p, "Not Active\n");
	else {
		p += sprintf(p, "Active - ");

		if (window->users == 0)
			p += sprintf(p, "No users\n");
		else
			p += sprintf(p, "%2d user%c\n", window->users,
				     (window->users > 1)?'s':' ');
	}

	if (!window->active) {
		p += sprintf(p, "\n");
		return p - page;
	}

	desc = &window->desc;

	p += sprintf(p, "        %p %.8x %.8x %.8x - ",
		     desc->kernel_va, desc->pci_addrl,
		     desc->vme_addrl, desc->sizel);

		if (desc->read_prefetch_enabled)
			switch (desc->read_prefetch_size) {
			case VME_PREFETCH_2:
				pfs = "PFS2";
				break;
			case VME_PREFETCH_4:
				pfs = "PFS4";
				break;
			case VME_PREFETCH_8:
				pfs = "PFS8";
				break;
			case VME_PREFETCH_16:
				pfs = "PFS16";
				break;
			default:
				pfs = "?";
				break;
			}
		else
			pfs = "NOPF";

		p += sprintf(p, "(0x%02x)%s / D%2d %5s\n",
			     desc->am, amod[desc->am], desc->data_width, pfs);

	if (list_empty(&window->mappings)) {
		p += sprintf(p, "\n");
		return p - page;
	}

	p += sprintf(p, "\n  Mappings:\n");

	list_for_each_entry(mapping, &window->mappings, list) {
		p += vme_window_proc_show_mapping(p, count, mapping);
		count++;
	}

	p += sprintf(p, "\n");

    return p - page;
}

int vme_window_proc_show(char *page, char **start, off_t off, int count,
			 int *eof, void *data)
{
	char *p = page;
	int i;

	p += sprintf(p, "\nPCI-VME Windows\n");
	p += sprintf(p, "===============\n\n");
	p += sprintf(p, "        va       PCI      VME      Size      Address Modifier Data  Prefetch\n");
	p += sprintf(p, "                                             and Description  Width Size\n");
	p += sprintf(p, "----------------------------------------------------------------------------\n\n");

	for (i = 0; i < TSI148_NUM_OUT_WINDOWS; i++)
		p += vme_window_proc_show_window(p, i);

	*eof = 1;
	return p - page;
}

#endif /* CONFIG_PROC_FS */

/**
 * vme_window_release() - release file method for the VME window device
 * @inode: Device inode
 * @file: Device file descriptor
 *
 *  The release method is in charge of releasing all the mappings made by
 * the process closing the device file.
 */
int vme_window_release(struct inode *inode, struct file *file)
{
	int i;
	struct task_struct *p = current;
	struct window *window;
	struct mapping *mapping;
	struct mapping *tmp;

	for (i = 0; i < TSI148_NUM_OUT_WINDOWS; i++) {
		window = &window_table[i];

		if (mutex_lock_interruptible(&window->lock))
			return -ERESTARTSYS;

		if ((!window->active) || (list_empty(&window->mappings)))
			goto try_next;

		list_for_each_entry_safe(mapping, tmp,
					 &window->mappings, list) {

			if (task_pid_nr(p) == mapping->client.pid_nr) {
				/*
				 * OK, that mapping is held by the process
				 * release it.
				 */
				list_del(&mapping->list);
				kfree(mapping);
				window->users--;
			}
		}

try_next:
		mutex_unlock(&window->lock);
	}

	return 0;
}

/**
 * add_mapping() - Helper function to add a mapping to a window
 * @window: Window to add the mapping to
 * @desc: Mapping descriptor
 *
 * It is assumed that the window mutex is held on entry to this function.
 */
static int add_mapping(struct window *window, struct vme_mapping *desc)
{
	struct mapping *mapping;

		/* Create a logical mapping for this hardware window */
	if ((mapping = kmalloc(sizeof(struct mapping), GFP_KERNEL)) == NULL) {
		printk(KERN_ERR PFX "%s - "
		       "Failed to allocate mapping\n", __func__);
		return -ENOMEM;
	}

	/* Save the window descriptor for this window. */
	memcpy(&mapping->desc, desc, sizeof(struct vme_mapping));

	/*
	 * Set the client of this mapping.
	 * This may end up being wrong if this is called from other
	 * drivers at module load time.
	 */
	mapping->client.pid_nr = task_pid_nr(current);
	strcpy(mapping->client.name, current->comm);

	/* Insert mapping at end of window mappings list */
	list_add_tail(&mapping->list, &window->mappings);

	/* Increment user count */
	window->users++;

	return 0;
}

/**
 * remove_mapping() - Helper function to remove a mapping from a window
 * @window: Window to remove the mapping from
 * @desc: Mapping descriptor
 *
 * The specified descriptor is searched into the window mapping list by
 * only matching the VME address, the size and the virtual address.
 *
 * It is assumed that the window mutex is held on entry to this function.
 */
static int remove_mapping(struct window *window, struct vme_mapping *desc)
{
	struct mapping *mapping;
	struct mapping *tmp;

	list_for_each_entry_safe(mapping, tmp, &window->mappings, list) {

		if ((mapping->desc.vme_addru == desc->vme_addru) &&
		    (mapping->desc.vme_addrl == desc->vme_addrl) &&
		    (mapping->desc.sizeu == desc->sizeu) &&
		    (mapping->desc.sizel == desc->sizel) &&
		    (mapping->desc.kernel_va == desc->kernel_va)) {
			/* Found the matching mapping */
			list_del(&mapping->list);
			kfree(mapping);
			window->users--;

			return 0;
		}
	}

	return -EINVAL;
}

/**
 * find_vme_mapping_from_addr - Find corresponding vme_mapping structure from
 *                              logical address, returned by find_controller()
 *
 * @param logaddr - address to search for.
 *
 *
 * @return vme_mapping pointer - if found.
 * @return NULL                - not found.
 */
struct vme_mapping* find_vme_mapping_from_addr(unsigned logaddr)
{
	int cntr;
	struct window *window;
	struct mapping *mapping;

	for (cntr = 0; cntr < TSI148_NUM_OUT_WINDOWS; cntr++) {
		window = &window_table[cntr];
		if (mutex_lock_interruptible(&window->lock))
			return NULL;
		list_for_each_entry(mapping, &window->mappings, list) {
			if ((unsigned)mapping->desc.kernel_va == logaddr) {
				mutex_unlock(&window->lock);
				return &mapping->desc; /* bingo */
			}
		}
		mutex_unlock(&window->lock);
	}

	return NULL; /* not found */
}
EXPORT_SYMBOL_GPL(find_vme_mapping_from_addr);


/**
 * vme_get_window_attr() - Get a PCI-VME hardware window attributes
 * @desc: Contains the window number we're interested in
 *
 * Get the specified window attributes.
 *
 * This function is used by the VME_IOCTL_GET_WINDOW_ATTR ioctl from
 * user applications but can also be used by drivers stacked on top
 * of this one.
 *
 * Return 0 on success, or %EINVAL if the window number is out of bounds.
 */
int vme_get_window_attr(struct vme_mapping *desc)
{
	int window_num = desc->window_num;

	if ((window_num < 0) || (window_num >= TSI148_NUM_OUT_WINDOWS))
		return -EINVAL;

	if (mutex_lock_interruptible(&window_table[window_num].lock))
		return -ERESTARTSYS;

	memcpy(desc, &window_table[window_num].desc,
	       sizeof(struct vme_mapping));

	tsi148_get_window_attr(desc);

	mutex_unlock(&window_table[window_num].lock);

	return 0;
}
EXPORT_SYMBOL_GPL(vme_get_window_attr);

/**
 * vme_create_window() - Create and map a PCI-VME window
 * @desc: Descriptor of the window to create
 *
 * Create and map a PCI-VME window according to the &struct vme_mapping
 * parameter.
 *
 * This function is used by the VME_IOCTL_CREATE_WINDOW ioctl from
 * user applications but can also be used by drivers stacked on top
 * of this one.
 *
 * Return 0 on success, or a standard kernel error code on failure.
 */
int vme_create_window(struct vme_mapping *desc)
{
	int window_num = desc->window_num;
	struct window *window;
	int rc = 0;

	/* A little bit of checking */
	if ((window_num < 0) || (window_num >= TSI148_NUM_OUT_WINDOWS))
		return -EINVAL;

	if (desc->sizel == 0)
		return -EINVAL;

	/* Round down the initial VME address to a 64K boundary */
	if (desc->vme_addrl & 0xffff) {
		unsigned int lowaddr = desc->vme_addrl & ~0xffff;

		printk(KERN_INFO PFX "%s - aligning VME address %08x to 64K "
			"boundary %08x.\n", __func__, desc->vme_addrl, lowaddr);
		desc->vme_addrl = lowaddr;
		desc->sizel += desc->vme_addrl - lowaddr;
	}

	/*
	 * Round up the mapping size to a 64K boundary
	 * Note that vme_addrl is already aligned
	 */
	if (desc->sizel & 0xffff) {
		unsigned int newsize = (desc->sizel + 0x10000) & ~0xffff;

		printk(KERN_INFO PFX "%s - rounding up size %08x to 64K "
			"boundary %08x.\n", __func__, desc->sizel, newsize);
		desc->sizel = newsize;
	}

	/*
	 * OK from now on we don't want someone else mucking with our
	 * window.
	 */
	window = &window_table[window_num];

	if (mutex_lock_interruptible(&window->lock))
		return -ERESTARTSYS;

	if (window->active) {
		rc = -EBUSY;
		goto out_unlock;
	}

	/* Allocate and map a PCI address space for the window */
	window->rsrc.name = kmalloc(32, GFP_KERNEL);

	if (!window->rsrc.name)
		/* Not fatal, we can live with a nameless resource */
		printk(KERN_WARNING PFX "%s - "
		       "failed to allocate resource name\n", __func__);
	else
		sprintf((char *)window->rsrc.name, "VME Window %d", window_num);

	window->rsrc.start = 0;
	window->rsrc.end = desc->sizel;
	window->rsrc.flags = IORESOURCE_MEM;

	/*
	 * Allocate a PCI region for our window. Align the region to a 64K
	 * boundary for the TSI148 chip.
	 */
	rc = pci_bus_alloc_resource(vme_bridge->pdev->bus, &window->rsrc,
				    desc->sizel, 0x10000,
				    PCIBIOS_MIN_MEM, 0, NULL, NULL);

	if (rc) {
		printk(KERN_ERR PFX "%s - "
		       "Failed to allocate bus resource for window %d "
		       "start 0x%lx size 0x%.8x\n",
		       __func__, window_num, (unsigned long)window->rsrc.start,
		       desc->sizel);

		goto out_free;
	}

	desc->kernel_va = ioremap(window->rsrc.start, desc->sizel);

	if (desc->kernel_va == NULL) {
		printk(KERN_ERR PFX "%s - "
		       "failed to map window %d start 0x%lx size 0x%.8x\n",
		       __func__, window_num, (unsigned long)window->rsrc.start,
		       desc->sizel);

		rc = -ENOMEM;
		goto out_release;
	}

	desc->pci_addrl = window->rsrc.start;

	/* Now setup the chip for that window */
	rc = tsi148_create_window(desc);

	if (rc)
		goto out_unmap;

	/* Copy the descriptor */
	memcpy(&window->desc, desc, sizeof(struct vme_mapping));

	/* Mark the window as active now */
	window->active = 1;

	mutex_unlock(&window->lock);

	return 0;

out_unmap:
	iounmap(desc->kernel_va);

out_release:
	release_resource(&window->rsrc);

out_free:
	if (window->rsrc.name)
		kfree(window->rsrc.name);
	memset(&window->rsrc, 0, sizeof(struct resource));

out_unlock:
	mutex_unlock(&window->lock);

	return rc;
}
EXPORT_SYMBOL_GPL(vme_create_window);

/**
 * vme_destroy_window() - Unmap and remove a PCI-VME window
 * @window_num: Window Number of the window to be destroyed
 *
 * Unmap and remove the PCI-VME window specified in the &struct vme_mapping
 * parameter also release all the mappings on top of that window.
 *
 * This function is used by the VME_IOCTL_DESTROY_WINDOW ioctl from
 * user applications but can also be used by drivers stacked on top
 * of this one.
 *
 * NOTE: destroying a window also forcibly remove all the mappings
 *       ont top of that window.
 *
 * Return 0 on success, or a standard kernel error code on failure.
 */
int vme_destroy_window(int window_num)
{
	struct window *window;
	struct mapping *mapping;
	struct mapping *tmp;
	int rc = 0;

	if ((window_num < 0) || (window_num >= TSI148_NUM_OUT_WINDOWS))
		return -EINVAL;

	/*
	 * Prevent somebody else from changing our window from under us
	 */
	window = &window_table[window_num];

	if (mutex_lock_interruptible(&window->lock))
		return -ERESTARTSYS;

	/*
	 * Maybe we should silently ignore trying to destroy an unused
	 * window.
	 */
	if (!window->active) {
		rc = -EINVAL;
		goto out_unlock;
	}

	/* Remove all mappings */
	if (window->users > 0) {
		list_for_each_entry_safe(mapping, tmp,
					 &window->mappings, list) {
			list_del(&mapping->list);
			kfree(mapping);
			window->users--;
		}
	}

	if (window->users)
		printk(KERN_ERR "%s: %d mappings still alive "
		       "on window %d\n",
		       __func__, window->users, window_num);

	/* Mark the window as unused */
	window->active = 0;

	/* Unmap the window */
	iounmap(window->desc.kernel_va);
	window->desc.kernel_va = NULL;

	tsi148_remove_window(&window->desc);

	/* Release the PCI bus resource */
	release_resource(&window->rsrc);

	if (window->rsrc.name)
		kfree(window->rsrc.name);

	memset(&window->rsrc, 0, sizeof(struct resource));

out_unlock:
	mutex_unlock(&window->lock);

	return rc;
}
EXPORT_SYMBOL_GPL(vme_destroy_window);

/**
 * vme_find_mapping() - Find a window matching the specified descriptor
 * @match: Descriptor of the window to look for
 * @force: Force window creation if no match was found.
 *
 *  Try to find a window matching the specified descriptor. If a window is
 * found, then its number is returned in the &struct vme_mapping parameter.
 *
 *  If no window match and force is set, then a new window is created
 * to hold that mapping.
 *
 * This function is used by the VME_IOCTL_FIND_MAPPING ioctl from
 * user applications but can also be used by drivers stacked on top
 * of this one.
 *
 *  Returns 0 on success, or a standard kernel error code.
 */
int vme_find_mapping(struct vme_mapping *match, int force)
{
	int i;
	int rc = 0;
	struct window *window;
	unsigned int offset;
	struct vme_mapping wnd;

	for (i = 0; i < TSI148_NUM_OUT_WINDOWS; i++) {
		window = &window_table[i];

		if (mutex_lock_interruptible(&window->lock))
			return -ERESTARTSYS;

		/* First check if window is in use */
		if (!window->active)
			goto try_next;

		/*
		 * Check if the window matches what we're looking for.
		 *
		 * Right now we only deal with 32-bit (or lower) address space
		 * windows,
		 */

		/* Check that the window is enabled in the hardware */
		if (!window->desc.window_enabled)
			goto try_next;

		/* Check that the window has a <= 32-bit address space */
		if ((window->desc.vme_addru != 0) || (window->desc.sizeu != 0))
			goto try_next;

		/* Check the address modifier and data width */
		if ((window->desc.am != match->am) ||
		    (window->desc.data_width != match->data_width))
			goto try_next;

		/* Check the boundaries */
		if ((window->desc.vme_addrl > match->vme_addrl) ||
		    ((window->desc.vme_addrl + window->desc.sizel) <
		     (match->vme_addrl + match->sizel)))
			goto try_next;

		/* Check the 2eSST transfer speed if 2eSST is enabled */
		if ((window->desc.am == VME_2e6U) &&
		    (window->desc.v2esst_mode != match->v2esst_mode))
			goto try_next;

		/*
		 * Good, we found one mapping
		 * NOTE: we're exiting the loop with window->lock still held
		 */
		break;

try_next:
		mutex_unlock(&window->lock);
	}

	/* Window found */
	if (i < TSI148_NUM_OUT_WINDOWS) {
		offset = match->vme_addrl - window->desc.vme_addrl;

		/* Now set the virtual address of the mapping */
		match->kernel_va = window->desc.kernel_va + offset;
		match->pci_addrl = window->desc.pci_addrl + offset;

		/* Assign window number */
		match->window_num = i;

		/* Add the new mapping to the window */
		rc = add_mapping(window, match);

		mutex_unlock(&window->lock);

		return rc;
	}

	/*
	 * Bad luck, no matching window found - create a new one if
	 * force is set.
	 */
	if (!force)
		return -EBUSY;

	/* Get the first unused window */
	for (i = 0; i < TSI148_NUM_OUT_WINDOWS; i++) {
		window = &window_table[i];

		if (!window->active)
			break;
	}

	if (i >= TSI148_NUM_OUT_WINDOWS)
		/* No more window available - bail out */
		return -EBUSY;

	/*
	 * Setup the physical window descriptor that can hold the requested
	 * mapping. The VME address and size may be realigned by the low level
	 * code in the descriptor, so make a private copy for window creation.
	 */
	memcpy(&wnd, match, sizeof(struct vme_mapping));

	wnd.window_num = i;

	rc = vme_create_window(&wnd);

	if (rc)
		return rc;

	/* Now set the virtual address of the mapping */
	window = &window_table[wnd.window_num];

	offset = match->vme_addrl - window->desc.vme_addrl;
	match->kernel_va = window->desc.kernel_va + offset;
	match->pci_addrl = window->desc.pci_addrl + offset;
	match->window_num = wnd.window_num;


	/* And add that mapping to it */
	rc = add_mapping(window, match);

	return rc;
}
EXPORT_SYMBOL_GPL(vme_find_mapping);

/**
 * vme_release_mapping() - Release a mapping allocated with vme_find_mapping
 *
 * @desc: Descriptor of the mapping to release
 *
 * Release a VME mapping. If the mapping is the last one on that window and
 * force is set then the window is also destroyed.
 *
 * This function is used by the VME_IOCTL_RELEASE_MAPPING ioctl from
 * user applications but can also be used by drivers stacked on top
 * of this one.
 *
 * @return 0                          - on success.
 * @return standard kernel error code - if failed.
 */
int vme_release_mapping(struct vme_mapping *desc, int force)
{
	int window_num = desc->window_num;
	struct window *window;
	int rc = 0;

	if ((window_num < 0) || (window_num >= TSI148_NUM_OUT_WINDOWS))
		return -EINVAL;

	window = &window_table[window_num];

	if (mutex_lock_interruptible(&window->lock))
		return -ERESTARTSYS;

	if (!window->active) {
		rc = -EINVAL;
		goto out_unlock;
	}

	/* Remove the mapping */
	rc = remove_mapping(window, desc);

	if (rc)
		goto out_unlock;

	/* Check if there are no more users of this window */
	if ((window->users == 0) && force) {
		mutex_unlock(&window->lock);
		return vme_destroy_window(window_num);
	}

out_unlock:
	mutex_unlock(&window->lock);

	return rc;
}
EXPORT_SYMBOL_GPL(vme_release_mapping);

static int vme_destroy_window_ioctl(int __user *argp)
{
	int window_num;

	if (get_user(window_num, argp))
		return -EFAULT;

	return vme_destroy_window(window_num);
}

static int vme_bus_error_check_clear_ioctl(struct vme_bus_error __user *argp)
{
	struct vme_bus_error err;

	if (copy_from_user(&err, argp, sizeof(struct vme_bus_error)))
		return -EFAULT;

	err.valid = vme_bus_error_check_clear(&err);

	if (copy_to_user(argp, &err, sizeof(struct vme_bus_error)))
		return -EFAULT;

	return 0;
}

/**
 * vme_window_ioctl() - ioctl file method for the VME window device
 * @file: Device file descriptor
 * @cmd: ioctl number
 * @arg: ioctl argument
 *
 *  Currently the VME window device supports the following ioctls:
 *
 *    VME_IOCTL_GET_WINDOW_ATTR
 *    VME_IOCTL_CREATE_WINDOW
 *    VME_IOCTL_DESTROY_WINDOW
 *    VME_IOCTL_FIND_MAPPING
 *    VME_IOCTL_RELEASE_MAPPING
 *    VME_IOCTL_GET_CREATE_ON_FIND_FAIL
 *    VME_IOCTL_SET_CREATE_ON_FIND_FAIL
 *    VME_IOCTL_GET_DESTROY_ON_REMOVE
 *    VME_IOCTL_SET_DESTROY_ON_REMOVE
 */
long vme_window_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc;
	struct vme_mapping desc;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case VME_IOCTL_GET_WINDOW_ATTR:
		/*
		 * Get a window attributes.
		 *
		 * arg is a pointer to a struct vme_mapping with only
		 * the window number specified.
		 */
		if (copy_from_user(&desc, (void *)argp,
				   sizeof(struct vme_mapping)))
			return -EFAULT;

		rc = vme_get_window_attr(&desc);

		if (rc)
			return rc;

		if (copy_to_user((void *)argp, &desc,
				 sizeof(struct vme_mapping)))
			return -EFAULT;

		break;

	case VME_IOCTL_CREATE_WINDOW:
		/* Create and map a window.
		 *
		 * arg is a pointer to a struct vme_mapping specifying
		 * the window number as well as its attributes.
		 */
		if (copy_from_user(&desc, (void *)argp,
				   sizeof(struct vme_mapping))) {
			return -EFAULT;
		}

		rc = vme_create_window(&desc);

		if (rc)
			return rc;

		if (copy_to_user((void *)argp, &desc,
				 sizeof(struct vme_mapping)))
			return -EFAULT;

		break;

	case VME_IOCTL_DESTROY_WINDOW:
		/* Unmap and destroy a window.
		 *
		 * arg is a pointer to the window number
		 */
		return vme_destroy_window_ioctl((int __user *)argp);

	case VME_IOCTL_FIND_MAPPING:
		/*
		 * Find a window suitable for this mapping.
		 *
		 * arg is a pointer to a struct vme_mapping specifying
		 * the attributes of the window to look for. If a match is
		 * found then the virtual address for the requested mapping
		 * is set in the window descriptor otherwise if
		 * vme_create_on_find_fail is set then create a new window to
		 * hold that mapping.
		 */
		if (copy_from_user(&desc, (void *)argp,
				   sizeof(struct vme_mapping)))
			return -EFAULT;

		rc = vme_find_mapping(&desc, vme_create_on_find_fail);

		if (rc)
			return rc;

		if (copy_to_user((void *)argp, &desc,
				 sizeof(struct vme_mapping)))
			return -EFAULT;

		break;

	case VME_IOCTL_RELEASE_MAPPING:
		/* remove a mapping.
		 *
		 * arg is a pointer to a struct vme_mapping specifying
		 * the attributes of the mapping to be removed.
		 * If the mapping is the last on that window and if
		 * vme_destroy_on_remove is set then the window is also
		 * destroyed.
		 */
		if (copy_from_user(&desc, (void *)argp,
				   sizeof(struct vme_mapping)))
			return -EFAULT;

		rc = vme_release_mapping(&desc, vme_destroy_on_remove);

		break;

	case VME_IOCTL_GET_CREATE_ON_FIND_FAIL:
		rc = put_user(vme_create_on_find_fail,
			      (unsigned int __user *)argp);
		break;

	case VME_IOCTL_SET_CREATE_ON_FIND_FAIL:
		rc = get_user(vme_create_on_find_fail,
			      (unsigned int __user *)argp);
		break;

	case VME_IOCTL_GET_DESTROY_ON_REMOVE:
		rc = put_user(vme_destroy_on_remove,
			      (unsigned int __user *)argp);
		break;

	case VME_IOCTL_SET_DESTROY_ON_REMOVE:
		rc = get_user(vme_destroy_on_remove,
			      (unsigned int __user *)argp);
		break;
	case VME_IOCTL_GET_BUS_ERROR:
		rc = put_user(vme_bus_error_check(1),
			      (unsigned int __user *)argp);
		break;
	case VME_IOCTL_CHECK_CLEAR_BUS_ERROR:
		return vme_bus_error_check_clear_ioctl(argp);

	default:
		rc = -ENOIOCTLCMD;
	}

	return rc;
}

/**
 * vme_remap_pfn_range() - Small wrapper for io_remap_pfn_range
 * @vma: User vma to map to
 *
 *  This is a small helper function which sets the approriate vma flags
 * and protection before calling io_remap_pfn_range().
 *
 *  The following flags are added to vm_flags:
 *    - VM_IO         This is an iomem region
 *    - VM_RESERVED   Do not swap that mapping
 *    - VM_DONTCOPY   Don't copy that mapping on fork
 *    - VM_DONTEXPAND Prevent resizing with mremap()
 *
 *  The mapping is also set non-cacheable via the vm_page_prot field.
 */
static int vme_remap_pfn_range(struct vm_area_struct *vma)
{

	vma->vm_flags |= VM_IO | VM_RESERVED | VM_DONTCOPY | VM_DONTEXPAND;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return io_remap_pfn_range(vma,
				  vma->vm_start,
				  vma->vm_pgoff,
				  vma->vm_end - vma->vm_start,
				  vma->vm_page_prot);
}

/**
 * vme_window_mmap() - Map to userspace a VME address mapping
 * @file: Device file descriptor
 * @vma: User vma to map to
 *
 * The PCI physical address is in vma->vm_pgoff and is guaranteed to be unique
 * among all mappings. The range size is given by (vma->vm_end - vma->vm_start)
 */

int vme_window_mmap(struct file *file, struct vm_area_struct *vma)
{
	int i;
	int rc = -EINVAL;
	struct task_struct *p = current;
	struct window *window;
	struct mapping *mapping;
	unsigned int addr;
	unsigned int size;

	/* Find the mapping this mmap call refers to */
	for (i = 0; i < TSI148_NUM_OUT_WINDOWS; i++) {
		window = &window_table[i];

		if (mutex_lock_interruptible(&window->lock))
			return -ERESTARTSYS;

		/* Check the window is active and contains mappings */
		if ((!window->active) || (list_empty(&window->mappings)))
			goto try_next;

		list_for_each_entry(mapping, &window->mappings, list) {
			addr = vma->vm_pgoff << PAGE_SHIFT;
			size = vma->vm_end - vma->vm_start;
			/*
			 * The mapping matches if:
			 *   - The start addresses match
			 *   - The size match
			 *   - The pid match
			 */
			if ((task_pid_nr(p) == mapping->client.pid_nr) &&
			    (mapping->desc.pci_addrl == addr) &&
			    (mapping->desc.sizel == size)) {
				rc = vme_remap_pfn_range(vma);
				mutex_unlock(&window->lock);

				return rc;
			}
		}


try_next:
		mutex_unlock(&window->lock);
	}

	return rc;
}

/**
 * vme_window_init() - Initialize VME windows handling
 *
 *  Not much to do here aside from initializing the windows mutexes and
 * mapping lists.
 */
void __devinit vme_window_init(void)
{
	int i;

	for (i = 0; i < TSI148_NUM_OUT_WINDOWS; i++) {
		INIT_LIST_HEAD(&window_table[i].mappings);
		mutex_init(&window_table[i].lock);
		window_table[i].active = 0;
	}
}

/**
 * vme_window_exit() - Cleanup for module unload
 *
 * Unmap all the windows that were mapped.
 */
void __devexit vme_window_exit(void)
{
	int i;

	for (i = 0; i < TSI148_NUM_OUT_WINDOWS; i++) {
		if (window_table[i].active)
			vme_destroy_window(i);
	}
}
