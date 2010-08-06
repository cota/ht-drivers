/*
 * sis33dev.c
 * sysfs device management for sis33 devices
 *
 * Copyright (c) 2009-2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <sis33.h>

#include "sis33dev.h"

static void remove_trailing_chars(char *path, char c)
{
	size_t len;

	if (path == NULL)
		return;
	len = strlen(path);
	while (len > 0 && path[len - 1] == c)
		path[--len] = '\0';
}

/* this assumes all devices are called sis33.X */
int sis33dev_get_devname(int index, char *str, size_t len)
{
	snprintf(str, len, "sis33.%d", index);
	str[len - 1] = '\0';
	return 0;
}

static void path_append(char *path, const char *append, size_t len)
{
	snprintf(path + strlen(path), len - strlen(path), "/%s", append);
	path[len - 1] = '\0';
}

/*
 * As stated in linux-2.6/Documentation/sysfs-rules.txt, we first search
 * our devices in /sys/subsystem/sis33. If it doesn't exist, we try with
 * the symlinks in /sys/class/sis33.
 */
static int build_sis33path(char *path, size_t len)
{
	struct stat stats;
	int ret;

	snprintf(path, len, "/sys/subsystem/sis33/devices");
	path[len - 1] = '\0';
	ret = stat(path, &stats);
	if (ret == 0)
		return 0;
	snprintf(path, len, "/sys/class/sis33");
	path[len - 1] = '\0';
	ret = stat(path, &stats);
	if (ret == 0)
		return 0;
	fprintf(stderr, "warning: no sis33 devices found in sysfs\n");
	return -1;
}

static int build_devpath(int index, char *path, size_t len)
{
	char name[32];
	int ret = 0;

	ret = build_sis33path(path, len);
	if (ret < 0)
		return -1;
	ret = sis33dev_get_devname(index, name, sizeof(name));
	if (ret < 0)
		return -1;
	path_append(path, name, len);
	return 0;
}

static int list_devices(const char *path, int *indexes, int elems)
{
	struct dirent *dent;
	DIR *dir;
	int i;

	dir = opendir(path);
	if (dir == NULL)
		return -1;

	i = 0;
	for (dent = readdir(dir); dent != NULL; dent = readdir(dir)) {
		if (dent->d_name[0] == '.')
			continue;
		/*
		 * Fill in the indexes array if provided.
		 * Assume all devices are called 'sis33.X'.
		 */
		if (indexes) {
			if (i >= elems)
				break;
			indexes[i] = strtol(dent->d_name + strlen("sis33."), NULL, 0);
		}
		i++;
	}
	if (closedir(dir) < 0)
		return -1;
	return i;
}

int sis33dev_get_nr_devices(void)
{
	char path[SIS33_PATH_MAX];
	int ret;

	ret = build_sis33path(path, sizeof(path));
	if (ret < 0)
		return ret;
	return list_devices(path, NULL, 0);
}

static int cmp_ints(const void *a, const void *b)
{
	const int *n1 = a;
	const int *n2 = b;

	return *n1 - *n2;
}

int sis33dev_get_device_list(int *indexes, int elems)
{
	char path[SIS33_PATH_MAX];
	int ret;

	ret = build_sis33path(path, sizeof(path));
	if (ret < 0)
		return ret;
	ret = list_devices(path, indexes, elems);
	if (ret < 0)
		return ret;
	qsort(indexes, elems, sizeof(*indexes), cmp_ints);
	return 0;
}

/* Note: this function returns 1 on success */
int sis33dev_device_exists(int index)
{
	struct stat statbuf;
	char path[SIS33_PATH_MAX];

	if (build_devpath(index, path, sizeof(path)) < 0)
		return 0;
	if (stat(path, &statbuf))
		return 0;
	return 1;
}

static int sis33dev_get_attr_dev(char *path, size_t pathlen, const char *attr, char *value, size_t len)
{
	struct stat statbuf;
	size_t size;
	int fd;

	/* make sure it's NUL-terminated even if we fail before filling it in */
	value[len - 1] = '\0';
	path_append(path, attr, pathlen);

	if (lstat(path, &statbuf) != 0) {
		fprintf(stderr, "warning: Attribute %s not found in %s\n", attr, path);
		return -1;
	}

	if (S_ISLNK(statbuf.st_mode))
		return -1;
	if (S_ISDIR(statbuf.st_mode))
		return -1;
	if (!(statbuf.st_mode & S_IRUSR))
		return -1;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "warning: cannot open '%s'\n", path);
		return -1;
	}
	size = read(fd, value, len);
	value[len - 1] = '\0';
	if (close(fd) < 0)
		return -1;
	if (size < 0)
		return -1;
	if (size == len)
		return -1;

	value[size] = '\0';
	remove_trailing_chars(value, '\n');
	return 0;
}

int sis33dev_get_attr(int index, const char *name, char *value, size_t count)
{
	char path[SIS33_PATH_MAX];
	int ret;

	ret = build_devpath(index, path, sizeof(path));
	if (ret < 0)
		return ret;
	return sis33dev_get_attr_dev(path, sizeof(path), name, value, count);
}

int sis33dev_get_attr_int(int index, const char *name, int *valp)
{
	char value[32];
	int ret;

	ret = sis33dev_get_attr(index, name, value, sizeof(value));
	if (ret)
		return ret;
	*valp = strtol(value, NULL, 0);
	return 0;
}

int sis33dev_get_attr_uint(int index, const char *name, unsigned int *valp)
{
	char value[32];
	int ret;

	ret = sis33dev_get_attr(index, name, value, sizeof(value));
	if (ret)
		return ret;
	*valp = strtoul(value, NULL, 0);
	return 0;
}

static int sis33dev_set_attr_dev(char *path, size_t pathlen, const char *attr, const char *value)
{
	struct stat statbuf;
	size_t size;
	int fd;

	path_append(path, attr, pathlen);

	if (lstat(path, &statbuf) != 0) {
		fprintf(stderr, "warning: Attribute %s not found in %s\n", attr, path);
		return -1;
	}

	if (S_ISLNK(statbuf.st_mode))
		return -1;
	if (S_ISDIR(statbuf.st_mode))
		return -1;
	if (!(statbuf.st_mode & S_IWUSR))
		return -1;

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "warning: cannot open '%s'\n", path);
		return -1;
	}
	size = write(fd, value, strlen(value));
	if (close(fd) < 0)
		return -1;
	return size;
}

int sis33dev_set_attr(int index, const char *name, const char *value)
{
	char path[SIS33_PATH_MAX];
	int ret;

	ret = build_devpath(index, path, sizeof(path));
	if (ret < 0)
		return ret;
	return sis33dev_set_attr_dev(path, sizeof(path), name, value);
}

int sis33dev_set_attr_int(int index, const char *name, int value)
{
	char buf[32];

	snprintf(buf, sizeof(buf) - 1, "%d", value);
	buf[sizeof(buf) - 1] = '\0';
	return sis33dev_set_attr(index, name, buf);
}

int sis33dev_set_attr_uint(int index, const char *name, unsigned int value)
{
	char buf[32];

	snprintf(buf, sizeof(buf) - 1, "%u", value);
	buf[sizeof(buf) - 1] = '\0';
	return sis33dev_set_attr(index, name, buf);
}

static int sis33dev_get_nr_subattrs_dev(char *path, size_t pathlen, const char *name)
{
	path_append(path, name, pathlen);
	return list_devices(path, NULL, 0);
}

int sis33dev_get_nr_subattrs(int index, const char *name)
{
	char path[SIS33_PATH_MAX];
	int ret;

	ret = build_devpath(index, path, sizeof(path));
	if (ret < 0)
		return ret;
	return sis33dev_get_nr_subattrs_dev(path, sizeof(path), name);
}

int sis33dev_get_nr_items(int index, const char *name)
{
	char value[4096];
	int ret;
	int n;
	int i;

	ret = sis33dev_get_attr(index, name, value, sizeof(value));
	if (ret)
		return ret;
	/* this assumes that all items have one and only one trailing ' ' */
	n = 0;
	for (i = 0; i < strlen(value); i++) {
		if (value[i] == ' ')
			n++;
	}
	return n;
}

static int cmp_uints_desc(const void *a, const void *b)
{
	const unsigned int *n1 = a;
	const unsigned int *n2 = b;

	return *n2 - *n1;
}

/* Note: the returned values are sorted in descending order by magnitude */
int sis33dev_get_items_uint(int index, const char *name, unsigned int *array, unsigned int n)
{
	char value[4096];
	char *init;
	char *end;
	int ret;
	int i;

	ret = sis33dev_get_attr(index, name, value, sizeof(value));
	if (ret)
		return ret;
	/* this assumes that all items have one and only one trailing ' ' */
	init = value;
	for (i = 0; i < n; i++) {
		end = strchr(init, ' ');
		if (end == NULL)
			break;
		array[i] = strtoul(init, NULL, 0);
		init = end + 1;
	}
	qsort(array, i, sizeof(*array), cmp_uints_desc);
	return i < n ? -1 : 0;
}

static int __round_ok(unsigned int candidate, unsigned int value, enum sis33_round round)
{
	if (round == SIS33_ROUND_UP) {
		if (candidate >= value)
			return 1;
		return 0;
	} else if (round == SIS33_ROUND_DOWN) {
		if (candidate <= value)
			return 1;
		return 0;
	} else {
		/* ROUND_NEAREST */
		return 1;
	}
}

static unsigned int
__approx(unsigned int *array, unsigned int n, unsigned int value, enum sis33_round round)
{
	unsigned int mindiff;
	unsigned int diff;
	int result = 0;
	int i;

	mindiff = ~0;

	for (i = 0; i < n; i++) {
		diff = abs(array[i] - value);
		if (diff < mindiff && __round_ok(array[i], value, round)) {
			mindiff = diff;
			result = i;
			if (diff == 0)
				break;
		}
	}
	return array[result];
}

/* this assumes that all elements are ordered descending by magnitude */
unsigned int
sis33dev_round_uint(unsigned int *array, unsigned int n, unsigned int value, enum sis33_round round)
{
	if (value >= array[0])
		return array[0];
	if (value <= array[n - 1])
		return array[n - 1];
	return __approx(array, n, value, round);
}
