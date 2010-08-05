#ifndef _SIS33DEV_H_
#define _SIS33DEV_H_

#include <sis33.h>

#define SIS33_PATH_MAX	256

int sis33dev_device_exists(int index);
int sis33dev_get_nr_devices(void);
int sis33dev_get_device_list(int *indexes, int elems);
int sis33dev_get_devname(int index, char *str, size_t len);
int sis33dev_get_attr(int index, const char *name, char *value, size_t count);
int sis33dev_get_attr_int(int index, const char *name, int *valp);
int sis33dev_get_attr_uint(int index, const char *name, unsigned int *valp);
int sis33dev_set_attr(int index, const char *name, const char *value);
int sis33dev_set_attr_int(int index, const char *name, int value);
int sis33dev_set_attr_uint(int index, const char *name, unsigned int value);
int sis33dev_get_nr_subattrs(int index, const char *name);
int sis33dev_get_nr_items(int index, const char *name);
int sis33dev_get_items_uint(int index, const char *name, unsigned int *array, unsigned int n);
unsigned int sis33dev_round_uint(unsigned int *array, unsigned int n, unsigned int value, enum sis33_round round);

#endif /* _SIS33DEV_H_ */
