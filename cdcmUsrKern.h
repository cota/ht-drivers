/**
 * @file cdcmUsrKern.h
 *
 * @brief General constants and predefined names used both by the user-space
 *        (both for Linux/Lynx) and driver (for Linux only) are located here.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 04/07/2007
 *
 * @version $Id: cdcmUsrKern.h,v 1.4 2009/01/09 10:26:03 ygeorgie Exp $
 */
#ifndef _CDCM_USER_KERNEL_H_INCLUDE_
#define _CDCM_USER_KERNEL_H_INCLUDE_

#ifdef __Lynx__
extern int snprintf _AP((char*, size_t,const char*, ...));
#endif

#if defined (__linux__) && defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/list.h>
#endif

#define CDCM_PREFIX      "cdcm_"         /* CDCM prefix */
#define CDCM_DEV_NM_FMT  CDCM_PREFIX"%s" /* device name format */
#define CDCM_DNL         32              /* device name length */
#define CDCM_GRP_FMT     "%s_grp%02d"	 /* group naming convention */

/*-----------------------------------------------------------------------------
 * FUNCTION:    __srv_dev_name
 * DESCRIPTION: CDCM service device name container. It looks like
 *		'cdcm_<mod_name>' where 'mod_name' is the module-specific name.
 *		Used both by installation programm and CDCM driver.
 *		Driver will use it for /proc/devices and to build-up irq names.
 *		Installation programm will use it to buid-up service entry
 *		file handle in /dev and to find service file handle major
 *		number in /proc/devices.
 * RETURNS:	device name
 *-----------------------------------------------------------------------------
 */
static inline char* __srv_dev_name(char *mod_name)
{
  static char cdcm_dev_nm[CDCM_DNL] = { 0 };
  static char *dead_name = "_NONE_";

  if (cdcm_dev_nm[0])	/* already defined */
    return(cdcm_dev_nm);

  if (mod_name) /* not yet defined and caller wants to set one */
    snprintf(cdcm_dev_nm, sizeof(cdcm_dev_nm), CDCM_DEV_NM_FMT, mod_name);

  if (cdcm_dev_nm[0])
    return(cdcm_dev_nm);
  else
    return(dead_name);
}


/* Handy extra list functions. Defined here but not in the
   'common/include/list.h', to be able to use it inside the kernel also */
/**
 * list_capacity - how many elements in the list
 *
 * @head: the list to test.
 */
static inline int list_capacity(struct list_head *head)
{
  int __cntr = 0;
  struct list_head *at = head;

  while (at->next != head) {
    at = at->next;
    __cntr++;
  }
  
  return(__cntr);
}

/**
 * list_advance - move to the next element in the list
 *
 * @head: current head list element
 */
static inline struct list_head* list_advance(struct list_head *head)
{
  return head->next;
}

/**
 * list_last - get last element in the list
 *
 * @head: the list in question
 */
static inline struct list_head* list_last(struct list_head *head)
{
  struct list_head *at = head;

  while (at->next != head)
    at = at->next;

  return(at);
}

#endif /* _CDCM_USER_KERNEL_H_INCLUDE_ */
