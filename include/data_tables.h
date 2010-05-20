/**
 * @file data_tables.h
 *
 * @brief General Data Tables, used by drver and user space.
 *
 * @author Yury GEORGIEVSKIY. CERN BE/CO
 *
 * @date Created on 16/01/2009
 *
 * <long description>
 *
 * @version
 */
#ifndef _DATA_TABLES_H_INCLUDE_
#define _DATA_TABLES_H_INCLUDE_

#include <var_decl.h> /* to be able to declare variables in the header file */
#include <general_both.h>

#if !defined(__KERNEL__)

#include <list.h> /* because user-space doesn't know about struct list_head */
#define BUS_ID_SIZE 20 /* from linux/device.h */

#else

#include <linux/device.h>	/* for 'BUS_ID_SIZE' */
#include <list_extra.h>		/* driver needs it */

/* removed from 2.6.31 */
#ifndef BUS_ID_SIZE
#define BUS_ID_SIZE 20
#endif

#endif

#define MAX_VME_SLOT  21 //!< how many slots in the VME crate

/** @brief Module description table
 *
 * Created for @b each module declared.
 */
struct pci_module_descr {
	struct list_head mod_list;    /**< module linked list */
	int              mod_vid;     /**< Module Vendor ID - @b compulsory */
	int              mod_did;     /**< Module Device ID - @b compulsory */
	int              mod_bus;     /**< bus number module is expected in
					 (8bit long) */
	int              mod_slot;    /**< slot in which module is expected in
					 (8bit long) */
	int              mod_lun;     /**< Logical Unit Number */
	int              mod_ilev;    /**< interrupt level */
	int              mod_ivec;    /**< interrupt vector number */
	void            *mod_usr;     /**< hook for user-specific data
					 (NULL - if none) */
	char mod_bus_id[BUS_ID_SIZE]; /**< position on parent bus
					 (from linux/device.h) */
};


/*! @name For _GIOCTL_BAR_RD && _GIOCTL_BAR_WR ioctl calls.
 *
 * r/w to/from already mapped HW memory space of one of the 6 BAR's
 *
 * @note If you do @b any modification here (inserting/deleting new members
 *       etc...) you @b should recompile @e NODAL program.
 */
//@{
typedef struct {
  int bar_id;    //!< which BAR [0 - 5]
  int bar_offst; //!< register offset (in bytes) from the base address
  int bar_utype; //!< value type (1 | 2 | 4) (char, short, int)
  union {
    int   ival;
    short sval;
    char  cval;
  } bar_data;  //!< data to r/w
} bar_rdwr_t;
//@}


/**
 * @brief Printing out BAR data on the stdout.
 *
 * @param mdp - module description to printout
 */
static inline void prnt_bar_data(bar_rdwr_t *bdp)
{
  if (!bdp) return; /* safety checking */
#ifdef __KERNEL__
  printk
#else
  printf
#endif
("BAR#%d @offset %#x type %d value read is => %#x\n", bdp->bar_id, bdp->bar_offst, bdp->bar_utype, (bdp->bar_utype == 1)?bdp->bar_data.cval:(bdp->bar_utype == 2)?bdp->bar_data.sval:(bdp->bar_utype == 4)?bdp->bar_data.ival:0xdeadface);
}


/*! @name Mapped BARs description. For GET_BAR_INFO ioctl calls.
 *
 * User can get profound BAR information.
 *
 * @note If you do @b any modification here (inserting/deleting new members
 *       etc...) you @b should recompile @e NODAL program.
 */
//@{
#ifndef __KERNEL__
#define __iomem
#endif
typedef struct _tag_bar_map {
  struct list_head  mem_list;  //!< linked list
  int               mem_bar;   //!< which BAR [0 - 5]
  struct pci_dev*   mem_pdev;  //!< pci device description
  void __iomem     *mem_remap; //!< mapped I/O memory address
  int               mem_len;   //!< length
} bar_map_t;
//@}


/**
 * prnt_bar_info - Printout BAR information.
 *
 * @bip - info to print.
 */
static inline void prnt_bar_info(bar_map_t *bip)
{
  if (!bip) return; /* safety checking */
#ifdef __KERNEL__
  printk
#else
  printf
#endif
("+----[ BAR %d ]----\n| Address -> 0x%p\n| Length  -> %#x bytes\n+-----------------\n", bip->mem_bar, (void*)bip->mem_remap, bip->mem_len);
}


/*! @name Register description table.
 *
 * Each register has this description.
 *
 * @note If you do @b any modification here (inserting/deleting new members
 *       etc...) you @b SHOULD recompile @e NODAL program.
 */
//@{
typedef struct  {
  u_int  r_offs;       //!< base address offset in bytes
  u_int  r_am;         //!< elements amount
  u_int  r_sz;         //!< element size in bytes (1, 2, 4, 8)
  u_int  r_dps;        //!< data port size (1, 2, 4, 8)
  int    r_rw;	       //!< access rights (1 - r, 2 - w or 3 - wr). When user passing it as a parameter to register Read/Write - he provides operation type he wants to perform on registers
  char   r_name[32];   //!< name
  char   r_descr[128]; //!< description
  u_int  r_id;         //!< reg ID
  void  *r_data;       //!< data. Used in r/w operations
} reg_desc_t;
//@}


/**
 * prnt_reg_desc - Printout register description.
 *
 * @ptr: Description to print
 */
static inline void prnt_reg_desc(reg_desc_t *ptr)
{
  if (!ptr) return; /* safety checking */
#ifdef __KERNEL__
  printk
#else
  printf
#endif
("[%02d] [%s] %s @%#x %s\n", ptr->r_id, (ptr->r_rw == 1)?"R ":(ptr->r_rw == 2)?" W":(ptr->r_rw == 3)?"RW":"??", ptr->r_name, ptr->r_offs, ptr->r_descr);
}

#endif /* _DATA_TABLES_H_INCLUDE_ */
