/*
 * vme_bridge.h - PCI-VME bridge driver definitions
 *
 * Copyright (c) 2009 Sebastien Dugue
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

/*
 *  This file provides the data structures and definitions internal to the
 * driver.
 */

#ifndef _VME_BRIDGE_H
#define _VME_BRIDGE_H

#include <linux/device.h>
#include <linux/pci.h>
#include <linux/interrupt.h>

#include "tsi148.h"
#include "vmebus.h"

#define PFX			"VME Bridge: "
#define DRV_MODULE_NAME		"vmebus"
#define DRV_MODULE_VERSION	"1.0"
#define DRV_MODULE_RELDATE	"Jan, 8 2009"

/*
 * We just keep the last VME error caught, protecting it with a spinlock.
 * A new VME bus error overwrites it.
 * This is very simple yet good enough for most (sane) purposes.
 */
struct vme_verr {
	spinlock_t		lock;
	struct vme_bus_error	error;
	struct mutex		handlers_lock;
	struct list_head	handlers_list;
};

struct vme_bridge_device {
	int			rev;		/* chip revision */
	int			irq;		/* chip interrupt */
	int			slot;		/* the VME slot we're in */
	int			syscon;		/* syscon status */
	struct tsi148_chip	*regs;
	struct pci_dev		*pdev;
	struct vme_verr		verr;
};

typedef void (*vme_berr_handler_t)(struct vme_bus_error *);

struct vme_berr_handler {
	struct list_head list;
	struct list_head err_list;
	struct vme_bus_error error;
	size_t count;
	vme_berr_handler_t func;
};

extern struct vme_bridge_device *vme_bridge;
extern struct resource *vmepcimem;
extern void *crg_base;
extern unsigned int vme_report_bus_errors;

/* Use the standard VME Major */
#define VME_MAJOR	221

/*
 * Define our own minor numbers
 * If a device get added, do not forget to update devlist[] in vme_bridge.c
 */
#define VME_MINOR_MWINDOW	0
#define VME_MINOR_DMA		1
#define VME_MINOR_CTL		3
#define VME_MINOR_REGS		4

#define VME_MINOR_NR	VME_MINOR_LM + 1

/* Devices access rules */
#define DEV_RW		1	/* Device implements read/write */

struct dev_entry {
	unsigned int		minor;
	char			*name;
	unsigned int		flags;
	struct file_operations	*fops;
};

extern const struct dev_entry devlist[];


/* Prototypes */

/* vme_irq.c */
extern void account_dma_interrupt(int);
extern irqreturn_t vme_bridge_interrupt(int, void *);
extern int vme_enable_interrupts(unsigned int);
extern int vme_disable_interrupts(unsigned int);

/* vme_window.c */
extern int vme_window_release(struct inode *, struct file *);
extern long vme_window_ioctl(struct file *, unsigned int, unsigned long);
extern int vme_window_mmap(struct file *, struct vm_area_struct *);
extern void __devinit vme_window_init(void);
extern void __devexit vme_window_exit(void);

/* vme_dma.c */
extern void handle_dma_interrupt(int);
extern long vme_dma_ioctl(struct file *, unsigned int, unsigned long);
extern int __devinit vme_dma_init(void);
extern void __devexit vme_dma_exit(void);

/* vme_misc.c */
extern ssize_t vme_misc_read(struct file *, char *, size_t, loff_t *);
extern ssize_t vme_misc_write(struct file *, const char *, size_t, loff_t *);
extern long vme_misc_ioctl(struct file *, unsigned int, unsigned long);
extern int vme_bus_error_check(int clear);
extern int vme_bus_error_check_clear(struct vme_bus_error *);

/* Procfs stuff grouped here for comodity */
#ifdef CONFIG_PROC_FS
extern int vme_interrupts_proc_show(char *page, char **start, off_t off,
				    int count, int *eof, void *data);
extern int vme_irq_proc_show(char *page, char **start, off_t off,
			     int count, int *eof, void *data);
extern int vme_window_proc_show(char *page, char **start, off_t off,
				int count, int *eof, void *data);
#endif /* CONFIG_PROC_FS */


#endif /* _VME_BRIDGE_H */
