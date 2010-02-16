#ifndef __drm_h
#define __drm_h
/************************************************************
(C) Copyright 1998
Lynx Real-Time Systems, Inc., San Jose, CA
All rights reserved.

 File: drm.h
$Date: 2009/04/29 08:32:03 $
$Revision: 1.1 $
************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h> 	/* for struct timeval */


/* 
   This is a mogic number kept in the drm node to validate the same. This
   is in hex for the characters "lynx"
*/
 
#define DRM_MAGIC	0x6c796e78

/* Buslayer_id's used in drm_conf.c to statically configure the root node, 
   the PCI HostBridge, ISA hostBridge  and some dummy isa nodes.
*/

/* buslayer call index */

#define DRM_SYS_BUSLAYER	0
#define DRM_PCI_BUSLAYER	1
#define DRM_ISA_BUSLAYER	2
#define DRM_VME_BUSLAYER	3

/* This is a vendor_id,device_id for the root node. And the pci host bridge */

#define DRM_SYS_VENDID	1
#define DRM_SYS_DEVID	1
#define DRM_PCIHB_DEVID	2

/* This is a filler used to initialize a don't care field */

#define DRM_DONT_CARE	-1

/* DRM node states */

#define DRM_VIRGIN	1
#define DRM_IDLE	2
#define DRM_SELECTED	3
#define DRM_READY	4
#define DRM_ACTIVE	5

/* DRM interrupt service routine states */

#define DRM_ISR_NOT_NEEDED 0
#define DRM_ISR_NEEDED 	1
#define DRM_ISR_SET 	2

/* DRM node types and creation type; Only following combinations are
  legal: DRM_STATIC | DRM_DEVICE, DRM_STATIC | DRM_BUS,
         DRM_STATIC | DRM_BUS | DRM_HB
         DRM_AUTO | DRM_DEVICE, DRM_AUTO | DRM_BUS  
         DRM_AUTO | DRM_DEVICE | DRM_HB */

/* indicates this is is a build time configured node */

#define DRM_STATIC	0x1

/* indicates this is a run time configured node */

#define DRM_AUTO	0x2

/* Indicates this node can have children */

#define DRM_BUS		0x4

/* Indicates this is a leaf node */

#define DRM_DEVICE	0x8

/* Indicates this is a host bridge */

#define DRM_HB		0x10

#define DRM_ERR_BAD_NODE_TYPE	1

typedef int drm_id_t;

/* The drm_node_s defines the DRM node structure. */

struct drm_node_s {

	int	magic;		/* Sanity check */

/* Node handler */

	struct drm_bushandle_s *bushandle;	/* primary bus handler */

/* Node connectivity */

	struct drm_node_s *parent;		/* parent of this node */
	struct drm_node_s *child;		/* child of this node */
	struct drm_node_s *sibling;	/* next sibling of this node */

/* Node identification */

	drm_id_t	pbuslayer_id;	/* primary bus layer id */
	drm_id_t	sbuslayer_id;	/* secondary bus layer id */
	drm_id_t	device_id;	/* device id */
	drm_id_t	vendor_id;	/* vendor id */
	int		drm_state;	/* device state (IDLE,SELECTED,READY.ACTIVE) */
	int		node_type;	/* Auto, Static, Bus, device */
	struct timespec	drm_tstamp;	/* time stamp */ 

/* Node IRQ properties */

	int		intr_cntlr;	/* Interrupt controller index */
	int		intr_irq;	/* Interrupt request line */
	int		intr_flg;	/* Flag indicating Polled/Intr device */

	void		*prop;		/* Bus specific properties */
};

typedef struct drm_node_s  drm_node_t;
typedef struct drm_node_s  *drm_node_handle;

/*
	This structure defines the interface between the DRM and the 
	bus-layers. drm_conf.c registers the bus-layers statically 
	during kernel build. Each is a pointer to a function, and provides
	the following facilities to DRM:

		bus-layer init; locate node, init bus and device nodes
		insert nodes, delete nodes, allocate and free resources
		for bus and device nodes
*/

struct drm_bushandle_s {
	int (*init_buslayer)();		/* Init bus layer */
	int (*busnode_init)();		/* Bus node init */
	int (*alloc_res_bus)();		/* Allocate resource to a bus-node */
	int (*alloc_res_dev)();		/* Allocate resource to a dev-node */
	int (*free_res_bus)();		/* free resource from a bus-node */
	int (*free_res_dev)();		/* free resource from a dev-node */
	int (*find_child)();		/* locate a child-node of a bus-node */ 
	int (*map_resource)();		/* Map a dev resource in kernel virtual address space*/
	int (*unmap_resource)();	/* unmap a resource for kernel virtual address space */
	int (*read_device)();		/* read operation on a device resource */
	int (*write_device)();		/* write operation on a device resource */
	int (*translate_addr)();	/* address translation service */
	int (*del_busnode)();		/* remove a bus node */
	int (*del_devnode)();		/* remove a device node */
	int (*insertnode)();		/* insert a node with a given set of properties */
    int (*ioctl)();             /* buslayer specific ioctl functions */
};

/* This structure is used to determine the buslayer_id for bus-nodes which are
   non standard. drm_conf.c maintains a table of such devices.
*/

struct drm_bustype_s {
	drm_id_t	device_id;	/* device id */
	drm_id_t	vendor_id;	/* vendor id */
	drm_id_t	buslayer_id;	/* primary bus layer id */
};

/* structure used in the translate_addr call. All fields other than
   the vaddr is bus-layer specific. This interface needs to be refined */

struct drm_addr_s {
	unsigned int base;
	unsigned int size;
	unsigned  int type;
	unsigned int vaddr;
};

typedef struct drm_addr_s  drm_addr_t;
typedef struct drm_addr_s *drm_addr_handle;
#ifndef EMULATE_LYNXOS_ON_LINUX
#endif

/* prototype declarations */

extern int drm_init(void);
extern int drm_get_handle(drm_id_t,drm_id_t,drm_id_t,drm_node_handle *);
extern int drm_claim_handle(drm_node_handle);
extern int drm_free_handle(drm_node_handle);
extern int drm_register_isr(drm_node_handle, void (*)(), void *);
extern int drm_unregister_isr(drm_node_handle);
extern int drm_map_resource(drm_node_handle, int, unsigned int *);
extern int drm_unmap_resource(drm_node_handle, int);
extern int drm_device_read(drm_node_handle, int, unsigned int, unsigned int, void *);
extern int drm_device_write(drm_node_handle, int, unsigned int, unsigned int, void *);
extern int drm_translate_addr(drm_node_handle, drm_node_handle, drm_addr_handle,
                       drm_addr_handle, int);

extern int drm_alloc_node(drm_node_handle *);
extern int drm_free_node(drm_node_handle);
extern int drm_find_bus_type(int, int, int *);

extern int drm_getroot(void *);
extern int drm_getchild(drm_node_handle,void *);
extern int drm_getparent(drm_node_handle,void *);
extern int drm_getsibling(drm_node_handle,void *);
extern int drm_getnode(drm_node_handle,drm_node_handle);
extern int drm_setnode(drm_node_handle);

extern int drm_select_node(drm_node_handle);
extern int drm_unselect_node(drm_node_handle);
extern int drm_locate(drm_node_handle);
extern int drm_select_subtree(drm_node_handle);
extern int drm_unselect_subtree(drm_node_handle);
extern int drm_alloc_resource(drm_node_handle);
extern int drm_free_resource(drm_node_handle);
extern int drm_alloc_resource_node(drm_node_handle);
extern int drm_free_resource_node(drm_node_handle);
extern int drm_delete_node(drm_node_handle);
extern int drm_delete_subtree(drm_node_handle);
extern int drm_prune_subtree(drm_node_handle);
extern int drm_insertnode(drm_node_handle, void *, drm_node_handle *);
extern drm_node_handle drm_next_node(drm_node_handle);
extern int drm_ioctl(drm_id_t bus_type, drm_node_handle node_h,
                     int cmd, void *arg);

extern struct drm_bushandle_s *drm_bushandles[];

extern int drm_sem;		/* Lock when allocating/free device handle */

#define LOCK_DRM_TREE	swait(&drm_sem, SEM_SIGRETRY)
#define UNLOCK_DRM_TREE ssignal(&drm_sem)

#if defined(DEBUG)
extern void print_drm_node(drm_node_handle,char *);
extern void print_drm_tree(drm_node_handle,char *);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __drm_h */
