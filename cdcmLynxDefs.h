/* $Id: cdcmLynxDefs.h,v 1.1 2007/03/15 07:40:54 ygeorgie Exp $ */
/*
; Module Name:	 cdcmLynxDefs.h
; Module Descr:	 Everything is comming from the LynxOS header files. 
;		 Taken purely from the LynxOS.
;		 Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
; Creation Date: May, 2006
; Author:	 Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmLynxDefs.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 3.0   ygeorgie   14/03/07   Production release, CVS controlled.
; 2.0   ygeorgie   27/07/06   First working release.
; 1.0	ygeorgie   01/06/06   Initial version.
*/

#ifndef _CDCM_LYNX_DEFINITIONS_H_INCLUDE_
#define _CDCM_LYNX_DEFINITIONS_H_INCLUDE_

/*------------------------ from LynxOS pci_resource.h -----------------------*/

/*
*  PCI bus layer ID
*/

#define PCI_BUSLAYER		1

/*
* PCI resource ID's
*/

/* Resource ID to perform IO on specific pci register */
 
#define PCI_RESID_REGS		1

/* Resource ID to get pci bus_no, device_no or function number */

#define PCI_RESID_BUSNO		2
#define PCI_RESID_DEVNO		3
#define PCI_RESID_FUNCNO	4

/* Resource id to perform IO on some pci configuration space fields */

#define PCI_RESID_DEVID		5
#define PCI_RESID_VENDORID	6
#define PCI_RESID_CMDREG	7
#define PCI_RESID_REVID		8
#define PCI_RESID_STAT		9
#define PCI_RESID_CLASSCODE	16
#define PCI_RESID_SUBSYSID	17
#define PCI_RESID_SUBSYSVID	18

/* Resource ID's for PCI Base address registers (BAR) */

#define PCI_RESID_BAR0		10
#define PCI_RESID_BAR1		11
#define PCI_RESID_BAR2		12
#define PCI_RESID_BAR3		13
#define PCI_RESID_BAR4		14
#define PCI_RESID_BAR5		15

/* Geographic Property of a PCI devices */
struct pci_geoprop {
	int	device_no;		/* Device number */
	int	func_no;		/* Function Number */
};
/*------------------------ LynxOS pci_resource.h ends -----------------------*/

/*------------------------ from LynxOS drm_errno.h --------------------------*/

/* This is a status which indicates no error */

#define DRM_OK			0
#define DRM_ERRBASE		1000

/* All the errno definitions for DRM */

#define DRM_ENOMEM		DRM_ERRBASE+1
#define DRM_ESYSCTLADD		DRM_ERRBASE+2
#define DRM_ENOLAYERINIT	DRM_ERRBASE+3
#define DRM_ENODETYPE		DRM_ERRBASE+4
#define DRM_EFAULT		DRM_ERRBASE+5
#define DRM_ENODEV		DRM_ERRBASE+6
#define DRM_EINVALID		DRM_ERRBASE+7
#define DRM_EBADSTATE		DRM_ERRBASE+8
#define DRM_EBADTYPE		DRM_ERRBASE+9
#define DRM_ENOTSUP		DRM_ERRBASE+10
#define DRM_EBUSY		DRM_ERRBASE+11
#define DRM_EEXIST		DRM_ERRBASE+12
#define DRM_EMEMMAP		DRM_ERRBASE+13
#define DRM_EMEMUNMAP		DRM_ERRBASE+14
#define DRM_EIO			DRM_ERRBASE+15
#define DRM_EMAXPCILEVEL	DRM_ERRBASE+16
#define DRM_ECONFIG_PCIBUSNO	DRM_ERRBASE+17
#define DRM_EALLOC_PCIBUSNO	DRM_ERRBASE+18
#define DRM_EALLOC_PCIIO	DRM_ERRBASE+19
#define DRM_EALLOC_PCIMEM	DRM_ERRBASE+20
#define DRM_EALLOC_PCIMEM64	DRM_ERRBASE+21
#define DRM_EALLOC_PCIMEMPF	DRM_ERRBASE+22
#define DRM_EBUSNO		DRM_ERRBASE+23
/*------------------------ LynxOS drm_errno.h ends --------------------------*/

/*------------------------ form LynxOS kernel.h -----------------------------*/
/* System semaphore constants */
#define SEM_SIGIGNORE	-1
#define SEM_SIGRETRY	0
#define SEM_SIGABORT	1
#define SEM_SIGFADA 	17 /* Non-Posix constant for fast_ada support */
/*------------------------ LynxOS kernel.h ends -----------------------------*/
#endif /*  _CDCM_LYNX_DEFINITIONS_H_INCLUDE_ */
