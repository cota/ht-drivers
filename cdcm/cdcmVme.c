/**
 * @file cdcmVme.c
 *
 * @brief CDCM VME Linux driver that encapsulates user-written LynxOS driver.
 *
 * @author Yury Georgievskiy, Alain Gagnaire, Nicolas De Metz-Noblat
 *         CERN AB/CO.
 *
 * @date Created on 16/01/2007
 *
 * Consider it like a wrapper of the user driver.
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version
 */
#include "cdcmDrvr.h"
#include "cdcmLynxAPI.h"
#include "vmebus.h"
#include <linux/irq.h>

/* external crap */
extern cdcmStatics_t cdcmStatT;	/* CDCM statics table */
extern int cdcm_err;		/* global error */


/**
 * @brief Maps a specified part of the VME address space into PCI address
 *        space.
 *
 * @param vmeaddr - start address of VME address range to map
 * @param len     - size of address range to map in bytes
 * @param am      - VME address modifier
 * @param offset  - offset from vmeaddr (bytes) at which a read test should
 *                  be made
 * @param size    - size in bytes of read-test zone (0 to disable read test)
 * @param param   - structure holding AM-code and mode flags.
 *                  <b>NOT implemented for now!!!</b>
 *
 * Maps a specified part of the VME address space into PCI address space.
 * This function should be called to check module presence. We suppose that
 * user will call it at the very beginning of the driver installation for
 * every device he wants to manage. Like this all internal CDCM structures
 * will be initialized. Consider it as the VME analogue to the
 * @e drm_get_handle() for the PCI.
 *
 * @return pointer to be used by the caller to access
 *         the specified VME address range - if SUCCESS.
 * @return -1                              - if FAILED.
 */
unsigned int cdcm_find_controller(unsigned int vmeaddr, unsigned int len,
				  unsigned int am, unsigned int offset,
				  unsigned int size,
				  struct pdparam_master *param)
{
	unsigned int cpu_addr = 0;
	struct cdcm_dev_info *dit = (struct cdcm_dev_info*)
		kzalloc(sizeof(*dit), GFP_KERNEL);

	if (!dit) {
		cdcm_err = -ENOMEM;
		return -1;
	}

	/* vme description table allocation */
	if (!(dit->di_vme = (struct vme_dev*)
	      kzalloc(sizeof(*dit->di_vme), GFP_KERNEL)) ) {
		PRNT_ERR(cdcmStatT.cdcm_ipl,
			 "Can't allocate VME device descriptor\n");
		kfree(dit);
		cdcm_err = -ENOMEM;
		return -1;
	}

#undef find_controller
	cpu_addr = find_controller(vmeaddr, len, am, offset,
			       size, param);
	if (IS_ERR_VALUE(cpu_addr)) {
		kfree(dit->di_vme);
		kfree(dit);
		cdcm_err = cpu_addr; /* save error */
		return -1;
	}
#define find_controller cdcm_find_controller

	/* device is present - save VME device data */
	dit->di_vme->vd_virt_addr = cpu_addr;
	dit->di_vme->vd_phys_addr = vmeaddr;
	dit->di_vme->vd_len       = len;
	dit->di_vme->vd_am        = am;
	dit->di_alive = ALIVE_ALLOC | ALIVE_CDEV;
	dit->di_pci   = NULL;

	/* add device to the device linked list */
	list_add_tail(&dit->di_list, &cdcmStatT.cdcm_dev_list);

	return cpu_addr;
}


/**
 * @brief Free all system resources related to CDCM VME tracking.
 *
 * @param none
 *
 * @return void
 */
void cdcm_vme_cleanup(void)
{
	struct cdcm_dev_info *dit, *tmp;    /* device info table ptrs */

	/* exclude VME devices */
	list_for_each_entry_safe(dit, tmp, &cdcmStatT.cdcm_dev_list, di_list) {
		if (dit->di_vme) {
			list_del_init(&dit->di_list);
			kfree(dit->di_vme);
			kfree(dit);
		}

	}
}
