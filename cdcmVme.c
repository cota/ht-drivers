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
 * @version $Id: cdcmVme.c,v 1.5 2009/01/09 10:26:03 ygeorgie Exp $
 */
#include "cdcmDrvr.h"
#include "cdcmLynxAPI.h"
#include "cdcmUsrKern.h"

#include "vmebus.h" /* VME bus driver for TSI148 */

#include <linux/irq.h>

/* CDCM global variables (declared in the cdcmDrvr.c module)  */
extern struct file_operations cdcm_fops; /* char dev file operations */
extern cdcmStatics_t cdcmStatT;	/* CDCM statics table */
extern int cdcm_err;		/* global error */
extern struct list_head *glob_cur_grp_ptr; /* current working group */


/**
 * @brief Device exterminator
 *
 * @param infoT - device to bump off
 *
 * Returns exactly what @e xpc_vme_master_unmap() returns:
 * @return 0       - Success
 */
static int cdcm_exclude_vme_dev(cdcm_dev_info_t *infoT)
{
  kfree(infoT->di_vme);
  list_del_init(&infoT->di_list); /* exclude from the list */
  kfree(infoT); /* free memory */
  return(0);
}

/**
 * @brief Free all system resources related to VME.
 *
 * @param none
 *
 * @return void
 */
void cdcm_vme_cleanup(void)
{
  struct cdcm_grp_info_t *grp_it, *tmp_grp_it; /* group info tables */
  cdcm_dev_info_t *dev_it, *tmp_dev_it;	/* device info tables */

  /* exclude */
  list_for_each_entry_safe(grp_it, tmp_grp_it, &cdcmStatT.cdcm_grp_list_head, grp_list) { /* pass through all the groups */
    list_for_each_entry_safe(dev_it, tmp_dev_it, &grp_it->grp_dev_list, di_list) { /* pass through all the devices in the current group */
      cdcm_exclude_vme_dev(dev_it);
    }
  }
}



