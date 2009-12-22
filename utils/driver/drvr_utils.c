/**
 * @file drvr_utils.c
 *
 * @brief Handy driver utils && functions.
 *
 * @author Yury Georgievskiy. CERN AB/CO.
 *
 * @date June, 2008
 *
 * @version 1.0  ygeorgie  03/06/2008  Creation date.
 */
#include <linux/list.h>	/* for list */
#include <linux/slab.h>	/* for kmalloc && co. */

#include <data_tables.h>	/* for 'bar_map_t' */
#include <general_both.h>
#include <general_drvr.h>

#if 0
struct module_descr *create_md_from()
{
}
#endif


/**
 * map_bars - Resource allocation for device I/O Memory and I/O Port.
 *            Maps physical address of PCI buffer to virtual kernel space.
 *
 * @param l_head: List that will hold mapped BARs
 * @param pdev:   Pci device description
 * @param bars:   Bitmask of BARs to be requested
 * @param name:   Desired memory region name suffix(or NULL if none)
 *
 * @note Linked list should be freed afterwards by unmap_bars!
 *
 * @return how many BARs were mapped - in case of success.
 * @return -EBUSY                    - in case of failure.
 */
int map_bars(struct list_head *l_head, struct pci_dev *pdev, int bars, char *name)
{
  char res_name[32] = "BAR";
  bar_map_t *mem = NULL;
  bar_map_t *memP, *tmpP;
  int i, bcntr = 0;
  void __iomem *ioaddr;

  INIT_LIST_HEAD(l_head);

  for (i = 0; i < 6; i++)
    if ( (bars & (1 << i)) && (pci_resource_len(pdev, i)) ) {
      memset(&res_name[3], 0, sizeof(res_name)-3);
      snprintf(&res_name[3], sizeof(res_name)-3, "%d_%s", i, (name)?:'\0');
      if (pci_request_region(pdev, i, res_name))
	goto err_out;
      /* we will treat I/O ports as if they were I/O memory */
      if ( !(ioaddr = pci_iomap(pdev, i, 0)) )
	goto err_out_iomap;
      if ( !(mem = kzalloc((sizeof *mem), GFP_KERNEL)) )
	goto err_out_alloc;
      mem->mem_bar   = i;
      mem->mem_pdev  = pdev;
      mem->mem_remap = ioaddr;
      mem->mem_len   = pci_resource_len(pdev, i);
      list_add_tail(&mem->mem_list/*new*/, l_head/*head*/);
      ++bcntr;
    }
  return(bcntr);

 err_out_alloc:
  pci_iounmap(pdev, ioaddr);
 err_out_iomap:
  pci_release_region(pdev, i);
 err_out:
  /* free allocated resourses */
  list_for_each_entry_safe(memP, tmpP, l_head, mem_list) {
    pci_iounmap(memP->mem_pdev, memP->mem_remap);
    pci_release_region(memP->mem_pdev, memP->mem_bar);
    list_del(&memP->mem_list);
    kfree(memP);
  }
  return(-EBUSY);
}


/**
 * unmap_bars - Free resources, previously mapped by map_bars
 *
 * @param l_head: List that will hold mapped BARs.
 * @param bars:   Bitmask of BARs to be released
 *
 * @return number of BARs freed.
 */
int unmap_bars(struct list_head *l_head, int bars)
{
  bar_map_t *pos, *tmp;
  int i;
  int cc = 0;

  for (i = 0; i < 6; i++)
    if (bars & (1 << i)) {	/* search for this BAR in the list */
      list_for_each_entry_safe(pos, tmp, l_head, mem_list) {
	if (pos->mem_bar == i) { /* bingo */
	  pci_iounmap(pos->mem_pdev, pos->mem_remap);
	  pci_release_region(pos->mem_pdev, pos->mem_bar);
	  list_del(&pos->mem_list); /* exclude from the list */
	  kfree(pos); /* free memory */
	  ++cc;
	}
      }
    }

  return(cc);
}


/**
 * get_bar_descr -
 *
 * @param l_head: list head with BAR description
 * @param bar:    BAR number [0 - 5] to search for
 *
 * @return BAR data pointer - if success.
 * @return NULL             - requested BAR not found.
 */
inline bar_map_t* get_bar_descr(struct list_head *l_head, int bar)
{
  bar_map_t *ptr;

  list_for_each_entry(ptr, l_head, mem_list)
    if (ptr->mem_bar == bar)
      return(ptr);

  return(NULL);
}


//!<
//!
/*!
  @todo SHOULD WE CONVERT FROM PCI LITTLE ENDIAN TO CPU ENDIANITY???\n
        Or keep the data as it's comming from the device???
*/
#define CONVERT_TO_CPU

#ifdef CONVERT_TO_CPU
#define _CTC(x) x
#else
#define _CTC(x)
#endif


/*! @name Generic I/O Memory access functions.
 */
//@{
static inline
u16 genpci_read8(bar_map_t *bm, u8 offset)
{
  return(ioread8(bm->mem_remap + offset));
}

static inline
void genpci_write8(bar_map_t *bm, u8 offset, u8 value)
{
  iowrite8(value, bm->mem_remap + offset);
}

static inline
u16 genpci_read16(bar_map_t *bm, u16 offset)
{
  return(_CTC(be16_to_cpu)(ioread16(bm->mem_remap + offset)));
}

static inline
void genpci_write16(bar_map_t *bm, u16 offset, u16 value)
{
  iowrite16(_CTC(cpu_to_le16)(value), bm->mem_remap + offset);
}

static inline
u32 genpci_read32(bar_map_t *bm, u32 offset)
{
  return(_CTC(be32_to_cpu)(ioread32(bm->mem_remap + offset)));
}

static inline
void genpci_write32(bar_map_t *bm, u32 offset, u32 value)
{
  iowrite32(_CTC(cpu_to_le32)(value), bm->mem_remap + offset);
}
//@}


/*! @name Read register with shifting.
 *
 * Memory-Mapped I/O INSert [Byte, Word or Long SHIFT]
 */
//@{

/**
 * _mmio_insb_shift - Read I/O memory in bytes (8bit) with shifting.
 *
 * @param addr:
 * @param dst:
 * @param count:
 *
 * <long-description>
 *
 */
inline void _mmio_insb_shift(void __iomem *addr, u8 *dst, int count)
{
  while (--count >= 0) {
    u8 data = __raw_readb(addr);
    *dst = data;
    ++dst;
    addr += sizeof(*dst);
  }
}


/**
 * _mmio_insw_shift - Read I/O memory in words (16bit) with shifting.
 *
 * @param addr:
 * @param dst:
 * @param count:
 *
 * <long-description>
 *
 */
inline void _mmio_insw_shift(void __iomem *addr, u16 *dst, int count)
{
  while (--count >= 0) {
    u16 data = __raw_readw(addr);
    *dst = data;
    ++dst;
    addr += sizeof(*dst);
  }
}


/**
 * _mmio_insl_shift -  Read I/O memory in lwords (32bit) with shifting.
 *
 * @param addr:  io memory address
 * @param dst:   data goes here
 * @param count: how many lwords to read
 *
 * Copy data from io space to driver space lword by lword
 *
 */
inline void _mmio_insl_shift(void __iomem *addr, u32 *dst, int count)
{
  //static int constdata = 0;
  //int cntr = 0;
  //u16 *ptr;
  while (--count >= 0) {
    //u32 __data = __raw_readl(addr);
    //*dst = __data;
    *dst = __raw_readl(addr);
    //ptr = (u16*)dst;
    //if (constdata) {
      //printk("data[%02d] -> %#x u16[0] -> %#hx u16[1] -> %#hx\n", cntr, *dst, ptr[0], ptr[1]);
    // cntr++;
    // }
    //constdata++;
    //*dst = ioread32((unsigned long __iomem*)addr);
    ++dst;
    addr += sizeof(*dst);
  }
}
//@}


/*! @name Write I/O register with shifting.
 *
 */
//@{
inline void _mmio_outsb_shift(void __iomem *addr, const u8 *src, int count)
{
  while (--count >= 0) {
    __raw_writeb(*src, addr);
    src++;
    addr += sizeof(*src);
  }
}

inline void _mmio_outsw_shift(void __iomem *addr, const u16 *src, int count)
{
  while (--count >= 0) {
    __raw_writew(*src, addr);
    src++;
    addr += sizeof(*src);
  }
}

inline void _mmio_outsl_shift(void __iomem *addr, const u32 *src, int count)
{
  while (--count >= 0) {
    __raw_writel(*src, addr);
    src++;
    addr += sizeof(*src);
  }
}
//@}


/**
 * bar_read - read BARs
 *
 * @param bptr:  BAR data
 * @param bdata: user data
 *
 * To read device mapped BAR
 *
 */
inline void bar_read(bar_map_t* bptr, bar_rdwr_t *bdata)
{
  /* as PCI operates in LE - we ?should? convert data to CPU */
  switch (bdata->bar_utype) {
  case 1:
    bdata->bar_data.cval = genpci_read8(bptr, bdata->bar_offst);
    //bdata->bar_data.cval = ioread8(bptr->mem_remap + bdata->bar_offst);
    break;
  case 2:
    bdata->bar_data.sval = genpci_read16(bptr, bdata->bar_offst);
    //bdata->bar_data.sval = ioread16(bptr->mem_remap + bdata->bar_offst);
    break;
  default:
    bdata->bar_data.ival = le32_to_cpu(genpci_read32(bptr, bdata->bar_offst));
    //bdata->bar_data.ival = ioread32(bptr->mem_remap + bdata->bar_offst);
    break;
  }
}


/**
 * bar_write - write BAR
 *
 * @param bptr:  BAR data
 * @param bdata: user data
 *
 * To write device mapped BAR.
 *
 */
inline void bar_write(bar_map_t* bptr, bar_rdwr_t *bdata)
{
  switch (bdata->bar_utype) {
  case 1:
    genpci_write8(bptr, bdata->bar_offst, bdata->bar_data.cval);
    //iowrite8(bdata->bar_data.cval,  bptr->mem_remap + (u8)bdata->bar_offst);
    break;
  case 2:
    genpci_write16(bptr, bdata->bar_offst, bdata->bar_data.sval);
    //iowrite16(bdata->bar_data.sval, bptr->mem_remap + (u16)bdata->bar_offst);
    break;
  default:
    genpci_write32(bptr, bdata->bar_offst, bdata->bar_data.ival);
    //iowrite32(bdata->bar_data.ival, bptr->mem_remap + (u32)bdata->bar_offst);
    break;
  }
}


/**
 * @brief Allocate memory.
 *
 * @param size - memory size to allocate in bytes
 *
 * Depending on the request memory size - different allocation methods
 * are used. If size is less then 128Kb - kmalloc, vmalloc otherwise.
 * 128kb - for code is to be completely portable (ldd3).
 *
 * @return allocated memory pointer - if success.
 * @return NULL                     - if fails.
 */
inline char* sysbrk(unsigned long size)
{
	char *mem;

	if ( !(mem = (B2KB(size) > MEM_BOUND)?
	       vmalloc(size):kmalloc(size, GFP_KERNEL)) )
		return NULL;

	return mem;
}


/**
 * @brief Free allocated memory
 *
 * @param cp   - memory pointer to free
 * @param size - memory size
 *
 * @return void
 */
inline void sysfree(char *cp, unsigned long size)
{
	if (B2KB(size) > MEM_BOUND)
		vfree(cp);
	else
		kfree(cp);
}
