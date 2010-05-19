/* ========================================================= */
/* Clone the driver tree for drivers running in kernel space */
/* Julian Lewis Mon 2nd Feb 2009                             */
/* ========================================================= */
#ifndef LIBINST_KERNEL
#define LIBINST_KERNEL

#include <config_data.h>

#if defined (__linux__)
#include <linux/vmalloc.h>
#elif defined (__LYNXOS) /* for Lynx kernel only */
#include <kernel.h>
#define printk kkprintf
#endif

void (*InsLibSetCopyRoutine(void (*)(void*, const void*, int)))
(void*, const void*, int);

InsLibCarAddressSpace *InsLibCloneCarSpace(InsLibCarAddressSpace *);
InsLibVmeAddressSpace *InsLibCloneVmeSpace(InsLibVmeAddressSpace *);
InsLibPciAddressSpace *InsLibClonePciSpace(InsLibPciAddressSpace *);
InsLibCarModuleAddress *InsLibCloneCar(InsLibCarModuleAddress *);
InsLibVmeModuleAddress *InsLibCloneVme(InsLibVmeModuleAddress *);
InsLibPciModuleAddress *InsLibClonePci(InsLibPciModuleAddress *);
InsLibModlDesc *InsLibCloneModule(InsLibModlDesc *);
InsLibDrvrDesc *InsLibCloneDriver(InsLibDrvrDesc *);
InsLibDrvrDesc *InsLibCloneOneDriver(InsLibDrvrDesc *);
InsLibHostDesc *InsLibCloneHost(InsLibHostDesc *);

void InsLibFreeCarSpace(InsLibCarAddressSpace *);
void InsLibFreeVmeSpace(InsLibVmeAddressSpace *);
void InsLibFreePciSpace(InsLibPciAddressSpace *);
void InsLibFreeCar(InsLibCarModuleAddress *);
void InsLibFreeVme(InsLibVmeModuleAddress *);
void InsLibFreePci(InsLibPciModuleAddress *);
void InsLibFreeModule(InsLibModlDesc *);
void InsLibFreeDriver(InsLibDrvrDesc *);
void InsLibFreeHost(InsLibHostDesc *);

InsLibDrvrDesc *InsLibGetDriver(InsLibHostDesc *, char *);
InsLibModlDesc *InsLibGetModule(InsLibDrvrDesc *, int);
InsLibAnyAddressSpace *InsLibGetAddressSpace(InsLibModlDesc *, int);
InsLibAnyAddressSpace *InsLibGetAddressSpaceWidth(InsLibModlDesc *, int, int);

void InsLibPrintModule(InsLibModlDesc *);
void InsLibPrintDriver(InsLibDrvrDesc *);

#endif	/* LIBINST_KERNEL */
