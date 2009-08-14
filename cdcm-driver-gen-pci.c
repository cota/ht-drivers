/**
 * @file cdcm-driver-gen-pci.c
 *
 * @brief PCI-specific operations
 *
 * @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 14/08/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#include "list_extra.h" /* for extra handy list operations */
#include "cdcmDrvr.h"
#include "cdcmLynxAPI.h"
#include "vmebus.h"
#define __CDCM__
#include "dg/ModuleHeader.h"	/* for DevInfo_t */

int dg_get_dev_info(unsigned long arg)
{ return -1; }

int dg_get_mod_am(void)
{ return -1; }

int dg_cdv_install(char *name, struct file_operations *fops)
{ return -ENODEV; }

int dg_cdv_uninstall(struct file *filp, unsigned long arg)
{ return -ENODEV; }

ssize_t dg_fop_read(struct file *filp, char __user *buf, size_t size,
		    loff_t *off)
{ return -EINVAL; }

ssize_t dg_fop_write(struct file *filp, const char __user *buf,
		     size_t size, loff_t *off)
{ return -EINVAL; }

unsigned int dg_fop_poll(struct file* filp, poll_table* wait)
{ return POLLERR; }

long dg_fop_ioctl(struct file* filp, poll_table* wait)
{ return -ENOTTY; }

int dg_fop_mmap(struct file *filp, struct vm_area_struct *vma)
{ return -ENODEV; }

int dg_fop_open(struct inode *inode, struct file *filp)
{ return -ENOMEM; }

int dg_fop_release(struct inode *inode, struct file *filp)
{ return 0; }
