#ifndef _SIS33CORE_INTERNAL_H_
#define _SIS33CORE_INTERNAL_H_

#include <linux/fs.h>

#include "sis33core.h"

ssize_t sis33_create_sysfs_files(struct sis33_card *card);
void sis33_remove_sysfs_files(struct sis33_card *card);
int sis33_is_acquiring(struct sis33_card *card);
int sis33_is_transferring(struct sis33_card *card);
int sis33_value_is_in_array(const unsigned int *array, int n, unsigned int value);

#ifdef CONFIG_COMPAT
long sis33_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#else
#define sis33_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

#endif /* _SIS33CORE_INTERNAL_H_ */
