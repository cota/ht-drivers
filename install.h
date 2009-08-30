/**
 * @file install.h
 *
 * @brief Generic driver installation/uninstallation utility.
 *
 * @author Copyright (C) 2008 - 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 27/06/2008
 */
#ifndef _INSTALL_H_INCLUDE_
#define _INSTALL_H_INCLUDE_

#define _GNU_SOURCE /* asprintf rocks */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <include/general_usr.h>
#include <list.h>

#if defined(__Lynx__)
#undef __LYNXOS	/* we are not in Lynx kernel space */
#include <io.h>  /* for BLOCK/CHAR DRIVER defs */
#include <dir.h> /* for DIR */
int asprintf(char**, const char*, ...);
int mkstemp(char*);

 /* next three are for getopt() calls */
extern char *optarg;
extern int optind;
extern int opterr;

static inline int makedev(int major, int minor)
{
	int dev_num;
	dev_num = (major << 8) + minor;
	return dev_num;
}

#else  /* Linux */

#include <string.h>
#include <errno.h>
#include <stropts.h>
#include <sys/dir.h>		/* for dirent */
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>	/* for major, minor, makedev */
#include <sys/param.h> /* isset, isclr, clrbit, setbit, MIN, MAX */

/*! @name Possible driver types */
/*@{*/
#define BLOCKDRIVER 1
#define CHARDRIVER  0
/*@}*/

#endif	/* Linux */

/** @brief Command line mentioned drivers && their parameters */
struct drvrd {
	struct list_head list; /**< linked list */
	char *dopt;	       /**< driver option string */
	char  dflag;	       /**< driver flag. Supported options:
				  1. 's' -- Special installation mode.
				            Pass info address table during
					    dr_install().
			                    Valid only for Linux */
	char *slnn;	       /**< symbolic link node name */
	char *dname;	       /**< driver name */
};

/** @brief What should be installed */
enum {
	I_ALL = 1, /**< all drivers from .xml file */
	I_CHOSEN,  /**< only drivers, from the command line */
	I_EXCLUDED /**< all, but command line drivers */
};

int dr_install(char *path, int type);
int dr_uninstall(int id);

int cdv_install(char *path, int driver_id, int extra);
int cdv_uninstall(int id);

/* inst-utils.c */
int   parse_prog_args(int, char *[], int *, char **, struct list_head *);
int   create_driver_nodes(int, char *, char *, int);
char *create_info_file(char *, void *);
char *create_usr_option_string(int, char *[], int);
void  free_drvrd(struct list_head *);
struct drvrd *cmd_line_dd(struct list_head *, char *);
int drvr_pathname(char **, char *);
#endif /* _INSTALL_H_INCLUDE_ */
