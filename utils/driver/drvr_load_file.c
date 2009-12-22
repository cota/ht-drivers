/**
 * @file drvr_load_file.c
 *
 * @brief Read info file
 *
 * @author Yury Georgievskiy, CERN.
 *
 * @date May, 2008
 *
 * Inspired by [sound_firmware.c@2.6.23] on howto read the file from the
 * kernel space.
 *
 * @version 1.0  ygeorgie  13/05/2008  Creation date.
 */
#include <linux/fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>	/* for get_fs() && set_fs() */
#include "drvr_dbg_prnt.h"	/* for PRNT_ABS_ERR etc. */
#include "general_drvr.h"
// /acc/src/dsc/drivers/coht/include/

/**
 * @brief Get driver info table.
 *
 * @param ifn   -- info file path to open
 * @param itszp -- if not NULL, then table size in bytes will be placed here
 *
 * Open info files and deliver its data into the driver space.
 *
 * @b NOTE Returned pointer should be freed afterwards by the caller
 *
 * @return NULL               - in case of failure.
 * @return info table pointer - in case of success.
 */
char* read_info_file(char *ifn, int *itszp)
{
	struct file *filp;
	long fsz; /* file size */
	loff_t pos = 0;
	char *infoTab = NULL;
	mm_segment_t fs = get_fs(); /* save current address limit */

	/* This allows to bypass the security checks so we can invoke
	   system call service routines within the kernel mode */
	set_fs(get_ds());

	/* open info file */
	filp = filp_open(ifn, 0, 0);
	if (IS_ERR(filp)) {
		PRNT_ABS_ERR("Unable to load info file '%s'", ifn);
		set_fs(fs);	/* restore original limitation */
		return NULL;
	}

	/* check if empty and set file size
	   To be compatible with old kernels use f_dentry
	   instead of f_path.dentry.
	   it's [ #define f_dentry f_path.dentry ] in newer ones */
	fsz = filp->f_dentry->d_inode->i_size;
	if (fsz <= 0) {
		PRNT_ABS_ERR("Empty info file '%s' detected!", ifn);
		goto info_out;
	}

	/* allocate info table */
	if ((infoTab = sysbrk(fsz)) == NULL) {
		PRNT_ABS_ERR("Out of memory allocating info table");
		goto info_out;
	}

	/* put info table in the local buffer */
	if (vfs_read(filp, infoTab, fsz, &pos) != fsz) {
		PRNT_ABS_ERR("Failed to read info file '%s'", ifn);
		sysfree(infoTab, fsz);
		goto info_out;
	}

	if (itszp)
		*itszp = fsz; /* give back the info file size */

 info_out:
	filp_close(filp, current->files);
	set_fs(fs);	/* restore original limitation */
	return infoTab;
}
