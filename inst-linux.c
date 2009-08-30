/**
 * @file inst_linux.c
 *
 * @brief Utils, used by Linux during driver installation/uninstallation.
 *
 * @author Copyright (C) 2007 - 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date June, 2007
 *
 * Helps to unify driver install procedure between two systems. All standart
 * options are parsed here and group description is build-up.
 * Module-specific options are parsed using defined module-specific vectors.
 * Inspired by module-init-tools and rmmod, written by Rusty Russell. Thxs!
 */
#include "install.h"
#include <general_both.h>  /* for general definitions and macroses */
#include <general_ioctl.h> /* for _IO definitions */


/* sys_init_module system call */
extern long init_module(void *, unsigned long, const char *);

static void*       grab_file(const char*, unsigned long*);
static const char* moderror(int);
static int         find_device_nodes(int, char(*)[32], int);

/**
 * @brief Extract Driver name from the .ko filename
 *
 * @param koptr -- .ko filename
 *
 * @b NOTE Returned pointer should be freed by the caller afterwards.
 *
 * @b Examples: /path/to/driver/tric.2.6.29.1-rt7.ko -> tric
 *              /path/to/driver/tric.ko              -> tric
 *              ./tric.2.6.29.1-rt7.ko               -> tric
 *              tric.2.6.29.1-rt7.ko                 -> tric
 *              tricko                               -> WRONG!
 *
 * @return driver name - if OK
 * @return NULL        - failed
 */
static char* extract_driver_name(char *koptr)
{
	char *ptr;
	int am;

	/* get rid of path if any */
	ptr = rindex(koptr, '/');
	if (ptr)
		koptr = ptr+1;

	/* should have suffix */
	ptr = strchr(koptr, '.');
	if (!ptr)
		return NULL;

	/* get rid of suffix and create driver name */
	am = ptr - koptr;
	asprintf(&ptr, "%.*s", am, koptr);

	return ptr;
}

/**
 * @brief Lynx-like driver installation routine.
 *
 * @param drvr_fn -- driver path (.ko object)
 * @param type    -- @ref BLOCKDRIVER or @ref CHARDRIVER\n
 *                   Special case is possible, where user can pass driver option
 *                   address (casted to @e int) in a string representation.
 *                   It will be passed to the init_module() system call as an
 *                   option to the driver installation (3-rd parameter) instead
 *                   of a standart one - which is a driver name.
 *
 * Will install the driver in the system. Supposed to be called @b ONLY once
 * during installation procedure.
 * @b NOTE If this routine will be called more then once during installation -
 *         then it will terminate installation programm
 *
 * @return driver ID (major number) - if successful
 * @return -1                       - if not
 */
int dr_install(char *drvr_fn, int type)
{
	char *drvr_nm = NULL;	/* extracted driver name */
	int rc = 0;		/* return code */
	char *node_nm = NULL;	/* driver node name */

	void *drvrfile = NULL; /* our .ko */
	FILE *procfd   = NULL;
	unsigned long len;
	char *options = NULL; /* driver option */
	long int ret;
	char buff[32], *bufPtr; /* for string parsing */
	int major_num = 0;
	int devn; /* major/minor device number */

	drvr_nm = extract_driver_name(drvr_fn);
	if (!drvr_nm) {
		fprintf(stderr, "Wrong [.ko] object file name\n");
		return -1;
	}

	/* put .ko in the local buffer */
	if (!(drvrfile = grab_file(drvr_fn, &len))) {
		fprintf(stderr, "Can't read '%s': %s\n",
			drvr_fn, strerror(errno));
		rc = -1;
		goto dr_install_exit;
	}

	/* check for special case. Will overwrite standart one! */
	if (!WITHIN_RANGE(CHARDRIVER, type, BLOCKDRIVER))
		/* this IS a special case. Setup user-defined
		   extra options along with driver name */
		asprintf(&options, "dname=%s %s", drvr_nm, (char*)type);
	else {
		asprintf(&options, "dname=%s", drvr_nm);
	}

	/* insert module in the kernel */
	if ( (ret = init_module(drvrfile, len, options)) != 0 ) {
		fprintf(stderr, "Error inserting '%s' module: %li %s\n",
			drvr_fn, ret, moderror(errno));
		rc = -1;
		goto dr_install_exit;
	}

	/* check if he made it */
	if ( (procfd = fopen("/proc/devices", "r")) == NULL ) {
		mperr("Can't open /proc/devices");
		rc = -1;
		goto dr_install_exit;
	}

	/* search for the device driver entry */
	bufPtr = buff;
	while (fgets(buff, sizeof(buff), procfd))
		if (strstr(buff, drvr_nm)) { /* bingo! */
			major_num = atoi(strsep(&bufPtr, " "));
			break;
		}

	/* check if we cool */
	if (!major_num) {
		fprintf(stderr, "'%s' device NOT found in procfs.\n", drvr_nm);
		rc = -1;
		goto dr_install_exit;
	} else
		printf("\n'%s' device driver installed. Major number %d.\n",
		       drvr_nm, major_num);

	/* create service entry point */
	asprintf(&node_nm, "/dev/%s.0", drvr_nm);
	unlink(node_nm); /* if already exist delete it */
	devn = makedev(major_num, 0);
	if (mknod(node_nm, S_IFCHR | 0666, devn)) {
		mperr("Can't create %s service node", node_nm);
		rc = -1;
		goto dr_install_exit;
	}
	chmod(node_nm, 0666);

	/* driverID (major num) will be used by the cdv_install() */
	rc = major_num;

 dr_install_exit:
	if (drvr_nm)  free(drvr_nm);
	if (options)  free(options);
	if (drvrfile) free(drvrfile);
	if (node_nm)  free(node_nm);
	return rc;
}


/**
 * @brief Lynx call wrapper. Will uninstall the driver in the system. !TODO!
 *
 * @param id - driver ID to uninstall
 *
 * Supposed to be called @b ONLY once during uninstallation.
 *
 * @return
 */
int dr_uninstall(int id)
{
	return -1;
}


/**
 * @brief Lynx call wrapper for character device installation.
 *
 * @param path      -- info file
 * @param driver_id -- driver ID (major number) returned by @e dr_install()
 * @param extra     -- extra parameter for the driver
 *
 * Normally this routine can be called several times from the driver
 * installation procedure, namely as many times, as modules declared.
 *
 * In our case it can be called <B>only once</B> per driver (dr_install), so
 * that only one statics table per driver is possible.
 *
 * In LynxOS, each time @e cdv_install() or @e bdv_install() are called by the
 * user-space installation programm, driver installation vector from @b dldd
 * structure is called. It returns statics table, that in turn is used in every
 * entry point of @b dldd vector table.
 *
 * If @ref extra parameter is @b NOT zero - it will be used in
 * @ref _GIOCTL_CDV_INSTALL ioctl call instead of @ref path parameter.
 * I.e. they _CAN'T_ be used simultaneoulsy!
 *
 * @return major device ID - if successful.
 * @return -1              - if not.
 */
int cdv_install(char *path, int driver_id, int extra)
{
	static int hist[32] = { 0 }; /* for whom i was already called */
	int dfd  = -1;	 /* driver file descriptor */
	int maj_num = -1;
	int cntr = 0;
	char nn[32]; /* device node name */

	/* check, if called more then once for this driver */
	while (hist[cntr]) {
		if (hist[cntr] == driver_id) {
			fprintf(stderr, "%s() is not allowed to be called more"
				"than one time for the driver\n", __func__);
			return -1;
		}
		++cntr;
	}

	hist[cntr] = driver_id;	/* save calling history */

	if (!find_device_nodes(driver_id, &nn, 1)) {
		fprintf(stderr, "Can't find driver node (maj %d)\n",
			driver_id);
		return -1;
	}

	if ((dfd = open(nn, O_RDWR)) == -1) {
		mperr("Can't open %s driver node", nn);
		return -1;
	}

	if ( (maj_num = ioctl(dfd, _GIOCTL_CDV_INSTALL,
			      (extra)?(void*)extra:path)) < 0) {
		mperr("_GIOCTL_CDV_INSTALL ioctl fails. Can't get major"
		      "number");
		close(dfd);
		return -1;
	}

	close(dfd);
	return maj_num;
}


/**
 * @brief Driver uninstallation procedure. TODO.
 *
 * @param did - deviceID
 *
 * @return
 */
int cdv_uninstall(int did)
{
	return -1;
}


/**
 * @brief Read .ko and place it in the local buffer
 *
 * @param filename -- .ko file name
 * @param size     -- its size will be placed here
 *
 * @return void
 */
static void* grab_file(const char *filename, unsigned long *size)
{
	unsigned int max = 16384;
	int ret, fd;
	void *buffer = malloc(max);
	if (!buffer)
		return NULL;

	if (streq(filename, "-"))
		fd = dup(STDIN_FILENO);
	else
		fd = open(filename, O_RDONLY, 0);

	if (fd < 0)
		return NULL;

	*size = 0;
	while ((ret = read(fd, buffer + *size, max - *size)) > 0) {
		*size += ret;
		if (*size == max)
			buffer = realloc(buffer, max *= 2);
	}
	if (ret < 0) {
		free(buffer);
		buffer = NULL;
	}
	close(fd);
	return buffer;
}


/**
 * @brief We use error numbers in a loose translation...
 *
 * @param err
 *
 * @return
 */
static const char* moderror(int err)
{
	switch (err) {
	case ENOEXEC:
		return "Invalid module format";
	case ENOENT:
		return "Unknown symbol in module";
	case ESRCH:
		return "Module has wrong symbol version";
	case EINVAL:
		return "Invalid parameters";
	default:
		return strerror(err);
	}
}


/**
 * @brief Search for device nodes by major in /dev directory.
 *
 * @param maj - major device ID to find nodes for
 * @param nn  - massive of the strings to store found device node names.
 *              Each string in this massive @b should be 32 bytes long.
 * @param sz  - massive size, i.e. how many strings of 32 bytes long are in
 *              this massive.
 *
 * @return Number of found device nodes.
 */
static int find_device_nodes(int maj, char (*nn)[32], int sz)
{
	DIR *dir = opendir("/dev");
	struct direct *direntry = NULL;
	int nodeCntr = 0;
	char fname[64];
	struct stat fstat;

	while ( (direntry = readdir(dir)) ) {
		snprintf(fname, sizeof(fname), "/dev/%s", direntry->d_name);
		stat(fname, &fstat);
		/* st_rdev holds [Major | Minor] numbers */
		if (major(fstat.st_rdev) == maj) { /* bingo */
			if (nn && (nodeCntr < sz))
				strncpy(nn[nodeCntr], fname,
					sizeof(nn[nodeCntr]));
			++nodeCntr;
		}
	}

	closedir(dir);
	return nodeCntr;
}
