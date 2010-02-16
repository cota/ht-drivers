
/* ===================================================== */
/*                                                       */
/* Acknowledgment:                                       */
/*                                                       */
/* Much of the ideas and some bits of this source        */
/* have been inspired by or copied from the book ...     */
/* Linux Device Drivers (Second Edition)                 */
/* by Alessandro Rubini and Jonathan Corbet              */
/* published by O'Reilly & Associates.                   */
/*                                                       */
/* ===================================================== */

/* ===================================================== */
/* Translate Linux driver calls into LynxOs driver calls */
/* Julian Lewis May 2004                                 */
/* ===================================================== */


#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/mutex.h>

#include <EmulateLynxOs.h>
#include <DrvrSpec.h>

extern void *LynxOsInfoTable;
extern int LynxOsInitInfoTable();
extern int LynxOsMemoryGetSize(int cmd, int *dirp);

extern LynxOsIsr lynxos_isrs[LynxOsMAX_DEVICE_COUNT];

int debug              = 0;
int recover            = 0;
int modules            = 1;
unsigned long infoaddr = 0;
static char dname[64]  = { 0 };

static int   LynxOs_major      = LynxOsMAJOR;
static char *LynxOs_major_name = LynxOsMAJOR_NAME;

MODULE_AUTHOR(LynxOsAUTHOR);
MODULE_LICENSE(LynxOsLICENSE);
MODULE_DESCRIPTION(LynxOsDESCRIPTION);
MODULE_SUPPORTED_DEVICE(LynxOsSUPPORTED_DEVICE);

/* To get instalation full debug info:  insmod <module> debug=7 */
/* Then look to: /var/log/messeage for the install debug output */

module_param(debug,     int,   0644);
module_param(recover,   int,   0644);
module_param(modules,   int,   0644);
module_param(infoaddr,  ulong, 0644);

module_param(LynxOs_major,int,0644);
MODULE_PARM_DESC(LynxOs_major,"Major device number");

module_param(LynxOs_major_name,charp,0);
MODULE_PARM_DESC(LynxOs_major_name,"Name of major device");

module_param_string(dname, dname, sizeof(dname), 0);
MODULE_PARM_DESC(dname, "Driver name");

int         LynxOsOpen(struct inode *inode, struct file *filp);
int         LynxOsClose(struct inode *inode, struct file *filp);
ssize_t     LynxOsRead(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t     LynxOsWrite(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int         LynxOsIoctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
void        LynxOsUninstall(void);
int         LynxOsInstall(void);

static DEFINE_MUTEX(ctr_drvr_mutex);

long LynxIoctl64(struct file *filp, unsigned int cmd, unsigned long arg) {
int res;

   mutex_lock(&ctr_drvr_mutex);
   res = LynxOsIoctl(filp->f_dentry->d_inode, filp, cmd, arg);
   mutex_unlock(&ctr_drvr_mutex);
   return res;
}

int LynxIoctl32(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
int res;

   mutex_lock(&ctr_drvr_mutex);
   res = LynxOsIoctl(inode, filp, cmd, arg);
   mutex_unlock(&ctr_drvr_mutex);
   return res;
}

struct file_operations LynxOs_fops = {
   read:           LynxOsRead,
   write:          LynxOsWrite,
   ioctl:          LynxIoctl32,
   compat_ioctl:   LynxIoctl64,
   open:           LynxOsOpen,
   release:        LynxOsClose
};

#include <EmulateLynxOs.h>
#include <DrvrSpec.h>

extern struct dldd entry_points;

char *LynxOsWorkingArea = NULL;

/* ===================================================== */
/* Open                                                  */
/* ===================================================== */

int LynxOsOpen(struct inode *inode, struct file *filp) {

long num;
struct LynxFile lynx_file;

   num = lynx_file.dev = MINOR(inode->i_rdev);

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: LynxOsOpen: Major: %d Minor: %ld\n",MAJOR(inode->i_rdev),num);

   if (entry_points.dldd_open(LynxOsWorkingArea,num,&lynx_file) == OK) {
      filp->private_data = (void *) num;
      return 0;
   }
   return -lynxos_error_num;
}

/* ===================================================== */
/* Close                                                 */
/* ===================================================== */

int LynxOsClose(struct inode *inode, struct file *filp) {

int num;
struct LynxFile lynx_file;

   num = lynx_file.dev = MINOR(inode->i_rdev);

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: LynxOsClose: Major: %d Minor: %d\n",MAJOR(inode->i_rdev),num);

   if (entry_points.dldd_close(LynxOsWorkingArea,&lynx_file) == OK) {
      return 0;
   }
   return -lynxos_error_num;
}

/* ===================================================== */
/* Read                                                  */
/* ===================================================== */

ssize_t LynxOsRead(struct file *filp, char *buf, size_t count, loff_t *f_pos) {

long num;
struct LynxFile lynx_file;
char *lynxmem;
long cc;
int rc;

   num = lynx_file.dev = (long) filp->private_data;

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: LynxOsRead: Minor: %ld Size: %d\n",num,(int) count);

   if (access_ok(VERIFY_WRITE,buf,count)) {
      if ((lynxmem = kmalloc(count,GFP_KERNEL)) != NULL) {
	 rc = entry_points.dldd_read(LynxOsWorkingArea,&lynx_file,lynxmem,count);
	 if (rc >= 0) {
	    cc = __copy_to_user(buf,lynxmem,count);
	    if (cc) {
	       kfree(lynxmem);
	       printk(KERN_WARNING "LynxOsRead __copy_to_user: Returned: %ld\n",cc);
	       return -EACCES;
	    }
	    kfree(lynxmem);
	    return rc;
	 }
	 kfree(lynxmem);
	 return -lynxos_error_num;
      }
      printk(KERN_WARNING "LynxOsRead: LynxOsMemoryAllocate: No more memory\n");
      return -ENOMEM;
   }
   printk(KERN_WARNING "LynxOsRead: access_ok: Can't WRITE memory\n");
   return -EACCES;
}

/* ===================================================== */
/* Write                                                 */
/* ===================================================== */

ssize_t LynxOsWrite(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {

long num;
struct LynxFile lynx_file;
char *lynxmem;
long cc;
int rc;

   num = lynx_file.dev = (long) filp->private_data;

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: LynxOsWrite: Minor: %ld Size: %d\n",num,(int) count);

   if (access_ok(VERIFY_READ,buf,count)) {
      if ((lynxmem = kmalloc(count,GFP_KERNEL)) != NULL) {
	 cc = __copy_from_user(lynxmem,buf,count);
	 if (cc) {
	    kfree(lynxmem);
	    printk(KERN_WARNING "LynxOsWrite __copy_from_user: Returned: %ld\n",cc);
	    return -EACCES;
	 }
	 rc = entry_points.dldd_write(LynxOsWorkingArea,&lynx_file,lynxmem,count);
	 if (rc >= 0) {
	    kfree(lynxmem);
	    return rc;
	 }
	 kfree(lynxmem);
	 return -lynxos_error_num;
      }
      printk(KERN_WARNING "LynxOsWrite: LynxOsMemoryAllocate: No more memory\n");
      return -ENOMEM;
   }
   printk(KERN_WARNING "LynxOsWrite: access_ok: Can't READ memory\n");
   return -EACCES;
}

/* ===================================================== */
/* Ioctl                                                 */
/* ===================================================== */

#define CMD_SET_DEBUG 0

int LynxOsIoctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {

long num;
struct LynxFile lynx_file;
int count, dir;
long cc;
int rc, cm;
char *iobuf;

   if (cmd >= CtrDrvrLAST_IOCTL) return -25;  /* ENOTTY This is not a typewriter */

   num = lynx_file.dev = (long) filp->private_data;

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: LynxOsIoctl: Minor: %ld Cmd: %d\n",num,(int) cmd);

   cc = count = LynxOsMemoryGetSize(cmd,&dir);
   if (cc < 0) {
      printk(KERN_WARNING "LynxOsIoctl: LynxOsMemoryGetSize: cc: %ld Minor: %ld Cmd: %d\n",cc,num,(int) cmd);
      return cc;
   }

   if ((iobuf = LynxOsMemoryAllocate(count,dir)) < 0) {
      printk(KERN_WARNING "LynxOsIoctl: LynxOsMemoryAllocate: No more memory: Minor: %ld Cmd: %d\n",num,cmd);
      return -ENOMEM;
   }

   if ((arg) && (dir & LynxOsMemoryREAD_FLAG)) {
      if (access_ok(VERIFY_READ,(char *) arg,count)) {
	 cc = __copy_from_user(iobuf,(char *) arg,count);
	 if (cc) {
	    LynxOsMemoryFree(iobuf);
	    printk(KERN_WARNING "LynxOsIoctl __copy_from_user: Returned: %ld\n",cc);
	    return -EACCES;
	 }
	 if (cmd == CMD_SET_DEBUG) {
	    cm = *(int *) iobuf;
	    if (cm == 0) debug = 0;

	    if (cm & LynxOsDEBUG_ERRORS) {
	       debug |= LynxOsDEBUG_ERRORS;
	       printk(KERN_INFO "Debug: lynx_os: Module error printing ON\n");
	    } else debug &= ~LynxOsDEBUG_ERRORS;

	    if (cm & LynxOsDEBUG_MODULE) {
	       debug |= LynxOsDEBUG_MODULE;
	       printk(KERN_INFO "Debug: lynx_os: Module trace printing ON\n");
	    } else debug &= ~LynxOsDEBUG_MODULE;

	    if ((cm & LynxOsDEBUG_DRIVER) == 0) *(int *) iobuf = 0;
	 }
      } else {
	 LynxOsMemoryFree(iobuf);
	 printk(KERN_WARNING "LynxOsIoctl: access_ok: Can't READ memory: Minor: %ld Cmd: %d\n",num,cmd);
	 return -EACCES;
      }
   }

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: LynxOsIoctl: Calling: Wa: 0x%lx Minor: 0x%lx Cmd: %d Buf: 0x%lx\n",
	     (unsigned long) LynxOsWorkingArea, num, cmd, (unsigned long) iobuf);

   rc = entry_points.dldd_ioctl(LynxOsWorkingArea,&lynx_file,cmd,iobuf);
   if (rc >= 0) {
      if ((arg) && (dir & LynxOsMemoryWRIT_FLAG)) {
	 if (access_ok(VERIFY_WRIT,(char *) arg,count)) {
	    cc = __copy_to_user((char *) arg,iobuf,count);
	    if (cc) {
	       LynxOsMemoryFree(iobuf);
	       printk(KERN_WARNING "LynxOsIoctl __copy_to_user: Returned: %ld\n",cc);
	       return -EACCES;
	    }
	 } else {
	    LynxOsMemoryFree(iobuf);
	    printk(KERN_WARNING "LynxOsIoctl: access_ok: Can't WRITE memory: Minor: %ld Cmd: %d\n",num,cmd);
	    return -EACCES;
	 }
      }
      LynxOsMemoryFree(iobuf);
      if (debug & LynxOsDEBUG_MODULE)
	 printk(KERN_INFO "Debug: LynxOsIoctl: dldd_ioctl: %ld Minor: %ld Cmd: %d\n",cc,num,cmd);
      return rc;
   }
   LynxOsMemoryFree(iobuf);
   return -lynxos_error_num;
}

/* ===================================================== */
/* Uninstall the driver                                  */
/* ===================================================== */

void LynxOsUninstall(void) {

int cc;

   if (LynxOsWorkingArea) {
      cc = entry_points.dldd_uninstall(LynxOsWorkingArea);
      if (cc < 0) {
	 printk(KERN_WARNING "LynxOsUninstall: cc: %d Can't uninstall\n",cc);
	 kfree(LynxOsWorkingArea);
      }
   }
   cdcm_free_semaphores();
   unregister_chrdev(LynxOs_major,LynxOs_major_name);
}

/* ===================================================== */
/* Initialize                                            */
/* ===================================================== */

#ifndef SET_MODULE_OWNER
#define SET_MODULE_OWNER(dev) ((dev)->owner = THIS_MODULE)
#endif

int LynxOsInstall(void) {

int cc;

   SET_MODULE_OWNER(&LynxOs_fops);

   cc = register_chrdev(LynxOs_major, LynxOs_major_name, &LynxOs_fops);
   if (cc < 0) {
      printk(KERN_WARNING "LynxOsInstall: cc: %d Can't get major %d\n",cc,LynxOs_major);
      return cc;
   }
   if (LynxOs_major == 0) LynxOs_major = cc; /* dynamic */

   LynxOsIsrsInitialize();
   LynxOsTimersInitialize();
   LynxOsMemoryInitialize();
   cc = LynxOsInitInfoTable(recover);
   if (cc < 0) {
      printk(KERN_WARNING "LynxOsInstall: cc: %d Can't initialize driver info table\n",cc);
      return cc;
   }

   LynxOsWorkingArea = entry_points.dldd_install(LynxOsInfoTable);
   if ((long) LynxOsWorkingArea == SYSERR) {
      LynxOsWorkingArea = NULL;
      return lynxos_error_num;
   }

   if (debug & LynxOsDEBUG_MODULE)
      printk(KERN_INFO "Debug: LynxOsInstall: Working Area: 0x%lx OK\n",(unsigned long) LynxOsWorkingArea);

   return 0;
}

/* ===================================================== */
/* Interrupt handler                                     */
/* ===================================================== */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
irqreturn_t LynxOsIntrHandler(int irq, void *arg, struct pt_regs *regs) {
#else
irqreturn_t LynxOsIntrHandler(int irq, void *arg) {
#endif

int i;

   for (i=0; i<LynxOsMAX_DEVICE_COUNT; i++) {
      if ((irq == lynxos_isrs[i].Irq) && ((long) arg == lynxos_isrs[i].Arg)) {
	 if (debug & LynxOsDEBUG_MODULE) {
	    printk(KERN_INFO "Debug: LynxOsIntrHandler: Indx: %d Agr: 0x%lx Isr: 0x%lx\n",
		   i, (unsigned long) arg, (unsigned long) lynxos_isrs[i].Isr);
	 }
	 return lynxos_isrs[i].Isr(arg);
      }
   }

   printk(KERN_WARNING "lynx_os: LynxOsIntrHandler: Unknown: Agr: 0x%lx Irq: 0x%x Not Handled (IRQ_NONE)\n",
	  (unsigned long) arg, (unsigned int) irq);

   for (i=0; i<LynxOsMAX_DEVICE_COUNT; i++) {
      if (lynxos_isrs[i].Handle) {
	 printk(KERN_INFO "lynx_os: Should be: %d Hnd: 0x%lx Isr: 0x%lx Arg: 0x%lx Irq: 0x%x Slt: %d\n",
		i,
		(unsigned long) lynxos_isrs[i].Handle,
		(unsigned long) lynxos_isrs[i].Isr,
		(unsigned long) lynxos_isrs[i].Arg,
		(unsigned int)  lynxos_isrs[i].Irq,
		(unsigned int)  lynxos_isrs[i].Slot);
      }
   }
   return IRQ_NONE;
}

module_init(LynxOsInstall);
module_exit(LynxOsUninstall);
