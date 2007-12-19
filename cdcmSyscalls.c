/* $Id: cdcmSyscalls.c,v 1.3 2007/12/19 09:02:05 ygeorgie Exp $ */
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
 * @version 4.0   ygeorgie  01/08/2007  Full Lynx-like installation behaviour.
 * @version 3.0   ygeorgie  14/03/2007  Production release, CVS controlled.
 * @version 2.0   ygeorgie  27/07/2006  First working release.
 * @version 1.0   ygeorgie  02/06/2006  Initial version.
 */
#include <asm/unistd.h>	/* for _syscall */
#include "cdcmDrvr.h"
#include "cdcmInfoT.h"
#include "cdcmLynxAPI.h"

#define CDCM_SEEK_SET 0
#define CDCM_SEEK_CUR 1
#define CDCM_SEEK_END 2

/* for _syscall's. Othervise we've got undefined symbol */
static int errno;

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern int cdcm_err;		/* global error */
extern cdcmStatics_t cdcmStatT; /* CDCM statics table */

/* we'll need this system calls to read info file */
static __inline__ _syscall3(int,open,const char *,file,int,flag,int,mode)
static __inline__ _syscall3(off_t,lseek,int,fd,off_t,offset,int,count)
static __inline__ _syscall3(int,read,int,fd,char *,buf,off_t,count)
static __inline__ _syscall1(int,close,int,fd)

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
char*
cdcm_get_info_table(
		    char *ifn,	/* info file path to open */
		    int *itszp)	/* table size will be placed here */
		    
{
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
}
