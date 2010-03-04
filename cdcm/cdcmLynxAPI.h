/**
 * @file cdcmLynxAPI.h
 *
 * @brief All LynxOS wrapper function decl and definitions are located here.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 07/07/2006
 *
 * It's included in the driver in case of Linux compilation.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version
 */
#ifndef _CDCM_LYNX_API_H_INCLUDE_
#define _CDCM_LYNX_API_H_INCLUDE_

/* ioctl control codes from Lynx sys/ioctl.h */
#define FIOPRIO 20  /* priority of a process has changed */

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
#define st_getstid()      current->pid /* TODO */


/* To mimic disable() in an SMP machine, we have to use spin_lock_irqsave.
 * This disables interrupts and pre-emption on the local CPU, but not on
 * the others. Also, if any of the other CPUs tries to access the region
 * protected by lynxos_cpu_lock, it'll busy-wait.
 * It is obvious that having a global 'lock' (lynxos_cpu_lock) hidden inside
 * CDCM is not such a good idea, since when held, _any_ other concurrent
 * attempt to access any other protected region (i.e. from any other CPU)
 * will busy-wait.
 * To avoid this problem we'd need to improve the CDCM API
 * to include a way of defining a lock -- it would be defined for every
 * particular region to be protected against concurrent access.
 */
extern spinlock_t lynxos_cpu_lock;
#define disable(x) spin_lock_irqsave(&lynxos_cpu_lock, x)
#define restore(x) spin_unlock_irqrestore(&lynxos_cpu_lock, x)

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

  // I need to store a user private pointer
  // In lynxOs this is called "buffer"

  char *buffer; // Like the private_data pointer

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
int   nanotime(unsigned long*);
int   timeout(int(*)(void*), void *, int);
int   cancel_timeout(int);
void  usec_sleep(unsigned long);
int   swait(int*, int);
int   tswait(int *, int, int);
int   ssignal(int*);
int   ssignaln(int*, int);
int   ststart(int(*)(void*), int, int, char *, int, ...);
void  stremove(int);
void  sreset(int*);
int   scount(int*);



/* LynxOS drm API */
int drm_get_handle(int, int, int, struct drm_node_s **);
int drm_free_handle(struct drm_node_s *);
int drm_device_read(struct drm_node_s *, int, unsigned int, unsigned int, void *);
int drm_device_write(struct drm_node_s *, int, unsigned int, unsigned int, void *);
int drm_register_isr(struct drm_node_s *, void *, void *);
int drm_unregister_isr(struct drm_node_s *);
int drm_map_resource(struct drm_node_s *, int, unsigned int *);
int drm_unmap_resource(struct drm_node_s *, int);


/* CES LynxOS API */
/*-----------------------------------------------------------------------------
 * Lynx maintains a stack of interrupt handlers for each vector.
 * We emulate the same mechanism for VME user interrupts using
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

/* page qualifiers */
#define VME_PG_SHARED  0x00
#define VME_PG_PRIVATE 0x02

/* LynxOs to Linux memory constants */
#define PHYSBASE PAGE_OFFSET
#define PAGESIZE PAGE_SIZE

char *get_phys(long);
int mem_lock(int, char *, unsigned long);
int mem_unlock(int, char *, long, int);
char *get1page(void);
void free1page(void *addr);


#endif /* _CDCM_LYNX_API_H_INCLUDE_ */
