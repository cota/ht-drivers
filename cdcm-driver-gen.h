/**
 * @file cdcm-driver-gen.h
 *
 * @brief Function declarations
 *
 * @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 12/06/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#ifndef _CDCM_DRIVER_GEN_H_INCLUDE_
#define _CDCM_DRIVER_GEN_H_INCLUDE_

int dg_get_dev_info(unsigned long);
int dg_get_mod_am(void);
int dg_cdv_install(char*, struct file_operations*);
int dg_cdv_uninstall(struct file*, unsigned long);

ssize_t dg_fop_read(struct file*, char __user*, size_t, loff_t*);
ssize_t dg_fop_write(struct file*, const char __user*, size_t, loff_t*);
unsigned int dg_fop_poll(struct file*, poll_table*);
long dg_fop_ioctl(struct file*, unsigned int cmd, unsigned long);
int dg_fop_mmap(struct file*, struct vm_area_struct*);
int dg_fop_open(struct inode*, struct file*);
int dg_fop_release(struct inode*, struct file*);

#endif	/* _CDCM_DRIVER_GEN_H_INCLUDE_ */
