/* $Id: cdcmSyscalls.c,v 1.4 2008/05/13 16:51:07 ygeorgie Exp $ */
/**
 * @file cdcmSyscalls.c
 *
 * @brief All functions, using _syscalls are placed here.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Feb, 2006
 *
 * CDCM is doing system calls from the kernel mode.
 * Inspired by  <tda1004x.c> on how to do sys_open && co.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version 5.0  ygeorgie  13/05/2008  Adapted for kernels > 2.6.20 as there is
 *                                     no more _syscall. They are deprecated.
 * @version 4.0  ygeorgie  01/08/2007  Full Lynx-like installation behaviour.
 * @version 3.0  ygeorgie  14/03/2007  Production release, CVS controlled.
 * @version 2.0  ygeorgie  27/07/2006  First working release.
 * @version 1.0  ygeorgie  02/06/2006  Initial version.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
#include <linux/fs.h>
#include <linux/sched.h>
#else
#define __KERNEL_SYSCALLS__
#include <asm/unistd.h>	/* for _syscall */

#define CDCM_SEEK_SET 0
#define CDCM_SEEK_CUR 1
#define CDCM_SEEK_END 2

/* for _syscall's. Othervise we've got undefined symbol */
static int errno;

/* we'll need this system calls to read info file */
/* Starting form 2.6.20 - _syscall are deprecated */
static __inline__ _syscall3(int,open,const char *,file,int,flag,int,mode)
static __inline__ _syscall3(off_t,lseek,int,fd,off_t,offset,int,count)
static __inline__ _syscall3(int,read,int,fd,char *,buf,off_t,count)
static __inline__ _syscall1(int,close,int,fd)

#endif /* 2.6.20 */

#include "cdcmDrvr.h"
#include "cdcmInfoT.h"
#include "cdcmLynxAPI.h"

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern int cdcm_err;		/* global error */
extern cdcmStatics_t cdcmStatT; /* CDCM statics table */

/**
 * 
 * @brief Get info tables.
 *
 * @param ifn   - info file path to open
 * @param itszp - if not NULL, then table size will be placed here
 *
 * Open info files and deliver its data into the driver space.
 *
 * @note Returned pointer should be freed afterwards!
 *
 * @return NULL               - in case of failure.
 * @return info table pointer - in case of success.
 */
char* cdcm_get_info_table(char *ifn, int *itszp)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
  struct file *filp;
  long fsz; /* file size */
  loff_t pos = 0;
  char *infoTab = NULL;
  
  /* open info file */
  filp = filp_open(ifn, 0, 0);
  if (IS_ERR(filp)) {
    PRNT_ABS_ERR("Unable to load info file '%s'", ifn);
    return(NULL);
  }

  /* check if empty and set file size */
  fsz = filp->f_path.dentry->d_inode->i_size;
  if (fsz <= 0) {
    PRNT_ABS_ERR("Empty info file '%s' detected!", ifn);
    goto info_out;
  }
  
  /* allocate info table */
  if ( (infoTab = sysbrk(fsz)) == NULL ) {
    PRNT_ABS_ERR("Out of memory allocating info table");
    goto info_out;
  }
  
  /* put info table in local buffer */
  if (vfs_read(filp, infoTab, fsz, &pos) != fsz) {
    PRNT_ABS_ERR("Failed to read info file '%s'", ifn);
    sysfree(infoTab, fsz);
    goto info_out;
  }
  
  if (itszp)
    *itszp = fsz; /* give back the info file size */
  
 info_out:
  filp_close(filp, current->files);
  return(infoTab);

#else  /* older, then 2.6.20 */

  int ifd;
  int filesize;
  char *infoTab = NULL;
  mm_segment_t fs = get_fs(); /* save current address limit */
  
  /* This allows to bypass the security checks so we can invoke
     system call service routines within the kernel mode */
  set_fs(get_ds()); 

  /* open info file */
  if ( (ifd = open(ifn, 0, 0)) < 0) {
    PRNT_ABS_ERR("Unable to open info file %s", ifn);
    return(NULL);
  }
  
  /* check if empty and set file size */
  if ( (filesize = lseek(ifd, 0L, CDCM_SEEK_END)) <= 0) {
    PRNT_ABS_ERR("Info file is empty.");
    goto cdcm_git_exit;
  }
  
  /* move to the beginning of the file */
  lseek(ifd, 0L, CDCM_SEEK_SET);

  /* allocate info table */
  if ( (infoTab = sysbrk(filesize)) == NULL ) {
    PRNT_ABS_ERR("Out of memory allocating info table");
    goto cdcm_git_exit;
  }
  
  /* put info table in local buffer */
  if (read(ifd, infoTab, filesize) != filesize) {
    PRNT_ABS_ERR("Failed to read module info file");
    sysfree(infoTab, filesize);
    infoTab = NULL;
    goto cdcm_git_exit;
  }

  if (itszp)
    *itszp = filesize; /* give back the info file size */

  cdcm_git_exit:
  close(ifd);
  set_fs(fs);	/* restore original limitation */
  return(infoTab);
#endif /* LINUX_VERSION_CODE */
}
