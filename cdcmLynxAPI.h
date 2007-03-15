/* $Id: cdcmLynxAPI.h,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmLynxAPI.h
; Module Descr:	 All LynxOS wrapper function definitions are located here.
;		 It's included in the driver in case of Linux compilation.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: July, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmLynxAPI.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   07/07/06   Initial version.
*/

#ifndef _CDCM_LYNX_API_H_INCLUDE_
#define _CDCM_LYNX_API_H_INCLUDE_

#define tickspersec HZ	/* number of ticks per second */

/* minor/major handling */
#define minor    MINOR
#define major    MAJOR
#define minordev MINOR
#define majordev MAJOR

/*  */
#define bcopy(s1,s2,n)    memcpy(s2,s1,n)
#define bzero(s,n)        memset(s,0,n)
#define bcmp(s1,s2,n)     memcmp(s1,s2,n)
#define kkprintf          printk
#define cprintf           printk
#define _kill(pid,sig)    kill_proc(pid,sig,0)
#define _killpg(pgrp,sig) kill_pg(pgrp,sig,0)
#define getpid()          current->pid

//#define disable(x)        spin_lock_irqsave(&lynxos_cpu_lock,x)
//#define restore(x)        spin_unlock_irqrestore(&lynxos_cpu_lock,x)
/* TODO. Should they look like this??? */
#define disable(x)        local_irq_save(x)
#define restore(x)        local_irq_restore(x)


/* Trying to combine 'struct file' from Linux and Lynx. For now only one 
   field is used. It's one that contains major/minor dev numbers. Note
   that we stick to the Lynx interface.
   See (sys/file.h) for Lynx and (linux/fs.h) for Linux */
struct cdcm_file {
  dev_t dev;            /* major/minor dev numbers */
  /* these are file access modes */
#define FREAD   O_RDONLY
#define FWRITE  O_WRONLY
//#define FUPDATE - Not implemented (even on Lynx)
  int access_mode;    /* access modes FREAD, FWRITE, FUPDATE */

  long long position; /* current logical file position */
};

/* select implementation */
#define SREAD		0
//#define SWRITE 1 /* already defined in Linux (fs.h as 3) So let it be. */
#define SEXCEPT		2
struct cdcm_sel {
  int *iosem;
  int **sel_sem;
  long mask;
  long *pmask;
};

/*-----------------------------------------------------------------------------
 * The Unix driver will get hold of the entry points by using the LynxOs dldd
 * entry points structure. The instance must be called "entry_points".
 *-----------------------------------------------------------------------------
 */
#define kaddr_t daddr_t
struct dldd {
  int     (*dldd_open)(void*, int, struct cdcm_file*);
  int     (*dldd_close)(void*, struct cdcm_file*);
  int     (*dldd_read)(void*, struct cdcm_file*, char*, int);
  int     (*dldd_write)(void*, struct cdcm_file*, char*, int);
  int     (*dldd_select)(void*, struct cdcm_file*, int, struct cdcm_sel*);
  int     (*dldd_ioctl)(void*, struct cdcm_file*, int, char*);
  char*   (*dldd_install)(void*);
  int     (*dldd_uninstall)(void*);
  kaddr_t (*dldd_mmap)(void);
};


/* LynxOS system API */
int   recoset(void);
void  noreco(void);
int   pseterr(int);
char* sysbrk(unsigned long);
void  sysfree(char *, unsigned long);
int   ksprintf(char *, char *, ...);
long  rbounds(unsigned long);
long  wbounds(unsigned long);
int   timeout(int(*)(void*), char*, int);
int   cancel_timeout(int);
int   swait(int*, int);
int   ssignal(int*);
void  sreset(int*);
int   ststart(int(*)(void*), int, int, char *, int, ...);
void  stremove(int);

/* LynxOS drm API */
int drm_get_handle(int, int, int, struct drm_node_s **);
int drm_free_handle(struct drm_node_s *);
int drm_device_read(struct drm_node_s *, int, unsigned int, unsigned int, void *);
int drm_device_write(struct drm_node_s *, int, unsigned int, unsigned int, void *);

/* CES LynxOS API */
/*-----------------------------------------------------------------------------
 * Lynx maintains a stack of interrupt handlers for each vector.
 * We emulate the same machanism for VME user interrupts using
 * a structure allocated by the user driver to save the previous
 * state of corresponding vector table entry. (see prototypes
 * for vme_intset() and vme_intclr()
 *
 * structure used for VME vector jump table 
 *-----------------------------------------------------------------------------
 */
typedef struct vme_intr_entry {
  int (*rout)(void*);
  char *arg;
} vme_vec_t;


/*  */
struct pdparam_master {
  unsigned long iack;	/* vme iack 0: IACK pages, 1: not IACK pages */
  unsigned long rdpref;	/* VME read prefetch option 0: disable, 1: enable */
  unsigned long wrpost;	/* VME write posting option 0: disable, 1: enable */
  unsigned long swap;	/* VME swap option. One of
			   SINGLE_NO_SWAP, SINGLE_AUTO_SWAP,
			   SINGLE_WORD_SWAP or SINGLE_BYTEWORD_SWAP */
  unsigned long sgmin;	/* reserved, must be 0  */
  unsigned long dum[3];	/* dum[0] shared/private,
			   dum[1] XPC ADP-type on RIO3, 
			   dum[2] reserved, must be 0 */
};

int vme_intset(int, int(*)(void*), char*, vme_vec_t*);
int vme_intclr(int, vme_vec_t*);
unsigned int find_controller(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, struct pdparam_master*);
unsigned int return_controller(unsigned int, unsigned int);

#endif /* _CDCM_LYNX_API_H_INCLUDE_ */
