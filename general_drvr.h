/**
 * @file general_drvr.h
 *
 * @brief General driver definitions. For kernel space only.
 *
 * @author Yury GEORGIEVSKIY
 *
 * @date June, 2008
 *
 * <long description>
 *
 * @version 1.0  ygeorige  27/06/2008  Creation date.
 */
#ifndef _GENERAL_DRVR_HEADER_H_INCLUDE_
#define _GENERAL_DRVR_HEADER_H_INCLUDE_

/*! @name Handy size operators
 *
 * Tnks to Linux kernel src
 */
//@{
#ifndef KB
# define KB      (1024UL)
#endif

#ifndef MB
#define MB      (1024UL*KB)
#endif

#ifndef GB
#define GB      (1024UL*MB)
#endif

#ifndef B2KB
#define B2KB(x) ((x) / 1024)
#endif

#ifndef B2MB
#define B2MB(x) (B2KB (B2KB (x)))
#endif
//@}

#ifndef memzero
#define memzero(_buf, _len) memset(_buf, 0, _len)
#endif

#if defined (__linux__) && defined(__KERNEL__)
/* for Linux kernel only */

#include <linux/vmalloc.h>
#include <linux/pci.h>	/* for pci interface */
#include <data_tables.h> /* for 'bar_rdwr_t' && 'bar_map_t' */

/*! @name PCI BAR offsets
 */
//@{
//!< they are in linux/pci_regs.h:83
//@}

//!< Get bar number [0 - 5] from the BAR offset
#define PCI_BAR(_bar) ((_bar - PCI_BASE_ADDRESS_0) >> 2)

//!< BAR bitmask (bits [5 4 3 2 1 0])
#define PCI_BAR_BIT(_bar) (1<<PCI_BAR(_bar))

#define MEM_BOUND 128*KB //!< what to use kmalloc or vmalloc

/*! @name drvr_utils.c functions
 */
//@{
int        map_bars(struct list_head*, struct pci_dev*, int, char*);
int        unmap_bars(struct list_head *, int);
bar_map_t* get_bar_descr(struct list_head*, int);
void       bar_read(bar_map_t*, bar_rdwr_t*);
void       bar_write(bar_map_t*, bar_rdwr_t*);

void _mmio_insb_shift(void __iomem*, u8*,  int);
void _mmio_insw_shift(void __iomem*, u16*, int);
void _mmio_insl_shift(void __iomem*, u32*, int);

void _mmio_outsb_shift(void __iomem*, const u8*,  int);
void _mmio_outsw_shift(void __iomem*, const u16*, int);
void _mmio_outsl_shift(void __iomem*, const u32*, int);

char* sysbrk(unsigned long);
void  sysfree(char *, unsigned long);
//@}


/*! @name drvr_load_file.c functions
 */
//@{
char* read_info_file(char*, int*);
//@}

//!< PCI device initialization on a run
/**
  * INIT_PCI_DEVICE - macro used to initialize a specific pci device
  *
  * @param ptr  - <B>struct pci_device_id</B> to init.
  * @param vend - the 16 bit PCI Vendor ID
  * @parma dev  - the 16 bit PCI Device ID
  *
  * This macro is used to initialize 'on a run' a <B>struct pci_device_id</B>
  * that matches a specific device. The subvendor and subdevice fields will be
  * set to @b PCI_ANY_ID.
  */
#define INIT_PCI_DEVICE(ptr, vend, dev)		\
do {						\
  (ptr)->vendor    = (vend);			\
  (ptr)->device    = (dev);			\
  (ptr)->subvendor = PCI_ANY_ID; 		\
  (ptr)->subdevice = PCI_ANY_ID;		\
} while (0)

#endif /* (__linux__) && defined(__KERNEL__) */
#endif /* _GENERAL_DRVR_HEADER_H_INCLUDE_ */
