/*
 * vme_bridge.c - PCI-VME bridge driver
 *
 * Copyright (c) 2009 Sébastien Dugué
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

/*
 *  This file provides the PCI-VME bridge support. It is highly coupled to
 * the Tundra TSI148 bridge chip capabilities.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/pci.h>

#include "vme_bridge.h"

static char version[] __devinitdata =
	"PCI-VME bridge: V" DRV_MODULE_VERSION " (" DRV_MODULE_RELDATE ")";

/* Module parameters */
static int vme_irq;
static int vme_slotnum = -1;

struct vme_bridge_device *vme_bridge;

/* CRG mapping on the VME bus - Maps to the top of the A24 address space */
#define A24D32_WINDOW	7
#define A24D32_AM	VME_A24_USER_DATA_SCT
#define A24D32_DBW	VME_D32
#define A24D32_VME_SIZE	0x1000000

struct vme_mapping a24d32_desc;

#define CRG_WINDOW	7
#define CRG_AM		VME_A24_USER_DATA_SCT
#define CRG_DBW		VME_D32
#define CRG_VME_BASE	0x00fff000
#define CRG_VME_SIZE	0x1000
struct vme_mapping crg_desc;
void *crg_base;

static unsigned int dma_ok;


static struct class *vme_class;

static __devinitdata struct pci_device_id tsi148_ids[] = {
    { PCI_DEVICE(PCI_VENDOR_ID_TUNDRA, PCI_DEVICE_ID_TUNDRA_TSI148) },
    { 0, },
};

static int __devinit vme_bridge_init(struct pci_dev *pdev,
				     const struct pci_device_id *id);
static void __devexit vme_bridge_remove(struct pci_dev *pdev);

static struct pci_driver vme_bridge_driver = {
	.name = "vme_bridge",
	.id_table = tsi148_ids,
	.probe = vme_bridge_init,
	.remove = __devexit_p(vme_bridge_remove),
};

#ifdef CONFIG_PROC_FS
struct proc_dir_entry *vme_root;

/*
 * Procfs stuff
 */
static int vme_info_proc_show(char *page, char **start, off_t off, int count,
			int *eof, void *data)
{
    char *p = page;

    p += sprintf(p, "PCI-VME driver v%s (%s)\n\n",
		 DRV_MODULE_VERSION, DRV_MODULE_RELDATE);

    p += sprintf(p, "chip revision:     %08X\n", vme_bridge->rev);
    p += sprintf(p, "chip IRQ:          %d\n", vme_bridge->irq);
    p += sprintf(p, "VME slot:          %d\n", vme_bridge->slot);
    p += sprintf(p, "VME SysCon:        %s\n",
		 vme_bridge->syscon ? "Yes" : "No");
    p += sprintf(p, "chip registers at: 0x%p\n", vme_bridge->regs);

    p += sprintf(p, "\n");

    *eof = 1;
    return p - page;
}



void __devinit vme_procfs_register(void)
{
	struct proc_dir_entry *entry;

	/* Create /proc/vme directory */
	vme_root = proc_mkdir("vme", NULL);

	/* Create /proc/vme/info file */
	entry = create_proc_entry("info", S_IFREG | S_IRUGO, vme_root);

	if (!entry)
		printk(KERN_WARNING PFX "Failed to create proc info node\n");

	entry->read_proc = vme_info_proc_show;

	/* Create /proc/vme/windows file */
	entry = create_proc_entry("windows", S_IFREG | S_IRUGO, vme_root);

	if (!entry)
		printk(KERN_WARNING PFX "Failed to create proc windows node\n");

	entry->read_proc = vme_window_proc_show;

	/* Create /proc/vme/interrupts file */
	entry = create_proc_entry("interrupts", S_IFREG | S_IRUGO, vme_root);

	if (!entry)
		printk(KERN_WARNING PFX
		       "Failed to create proc interrupts node\n");

	entry->read_proc = vme_interrupts_proc_show;

	/* Create /proc/vme/irq file */
	entry = create_proc_entry("irq", S_IFREG | S_IRUGO, vme_root);

	if (!entry)
		printk(KERN_WARNING PFX "Failed to create proc irq node\n");

	entry->read_proc = vme_irq_proc_show;

	/* Create specific TSI148 proc entries */
	tsi148_procfs_register(vme_root);
}

void __devexit vme_procfs_unregister(void)
{

	tsi148_procfs_unregister(vme_root);
	remove_proc_entry("irq", vme_root);
	remove_proc_entry("interrupts", vme_root);
	remove_proc_entry("windows", vme_root);
	remove_proc_entry("info", vme_root);
	remove_proc_entry("vme", NULL);
}
#else /* !CONFIG_PROC_FS */
void __devinit vme_procfs_register(void) {}
void __devexit vme_procfs_unregister(void) {}
#endif /* CONFIG_PROC_FS */

/*
 * Driver file operation entry points
 */
static int vme_open(struct inode *, struct file *);
static int vme_release(struct inode *, struct file *);
static ssize_t vme_read(struct file *, char *, size_t, loff_t *);
static ssize_t vme_write(struct file *, const char *, size_t, loff_t *);
static long vme_ioctl(struct file *, unsigned int, unsigned long);
static int vme_mmap(struct file *, struct vm_area_struct *);

static struct file_operations vme_fops = {
	.owner		= THIS_MODULE,
	.open		= vme_open,
	.release	= vme_release,
	.read		= vme_read,
	.write		= vme_write,
	.unlocked_ioctl	= vme_ioctl,
	.mmap		= vme_mmap,
};

static struct file_operations vme_mwindow_fops = {
	.owner		= THIS_MODULE,
	.release	= vme_window_release,
	.unlocked_ioctl	= vme_window_ioctl,
	.mmap		= vme_window_mmap,
};

static struct file_operations vme_dma_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= vme_dma_ioctl,
};

static struct file_operations vme_misc_fops = {
	.owner		= THIS_MODULE,
	.read		= vme_misc_read,
	.write		= vme_misc_write,
	.unlocked_ioctl	= vme_misc_ioctl,
};

const struct dev_entry devlist[] = {
	{VME_MINOR_MWINDOW, "vme_mwindow", 0,      &vme_mwindow_fops},
	{VME_MINOR_DMA,     "vme_dma",     0,      &vme_dma_fops},
	{VME_MINOR_CTL,     "vme_ctl",     0,      &vme_misc_fops},
	{VME_MINOR_REGS,    "vme_regs",    DEV_RW, &vme_misc_fops}
};

static int vme_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct file_operations *f_op = NULL;
	int rc = 0;

	switch (minor) {
	case VME_MINOR_MWINDOW:
		f_op = &vme_mwindow_fops;
		break;

	case VME_MINOR_DMA:
		if (!dma_ok)
			return -ENODEV;
		f_op = &vme_dma_fops;
		break;

	case VME_MINOR_CTL:
	case VME_MINOR_REGS:
		f_op = &vme_misc_fops;
		break;

	default:
		return -ENXIO;
	}

	if (f_op && f_op->open)
		rc = f_op->open(inode, file);

	return rc;
}
	
static int vme_release(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct file_operations *f_op = NULL;

	switch (minor) {
	case VME_MINOR_MWINDOW:
		f_op = &vme_mwindow_fops;
		break;

	case VME_MINOR_DMA:
		f_op = &vme_dma_fops;
		break;

	case VME_MINOR_CTL:
	case VME_MINOR_REGS:
		f_op = &vme_misc_fops;
		break;

	default:
		return -ENXIO;
	}

	if (f_op && f_op->release)
		return f_op->release(inode, file);

	return 0;
}

static ssize_t vme_read(struct file *file, char *buf, size_t count,
			loff_t *ppos)
{
	unsigned int minor = iminor(file->f_dentry->d_inode);
	struct file_operations *f_op = NULL;

	/*
	 * Do some sanity checking before handling processing to the
	 * specific device.
	 */
	if (!(devlist[minor].flags & DEV_RW))
		return -ENODEV;

	switch (minor) {
	case VME_MINOR_REGS:
		f_op = &vme_misc_fops;
		break;

	default:
		return -ENXIO;
	}

	if (f_op && f_op->read)
		return f_op->read(file, buf, count, ppos);

	return 0;
}

static ssize_t vme_write(struct file *file, const char *buf, size_t count,
			 loff_t *ppos)
{
	unsigned int minor = iminor(file->f_dentry->d_inode);
	struct file_operations *f_op = NULL;

	/*
	 * Do some sanity checking before handling processing to the
	 * specific device.
	 */
	if (!(devlist[minor].flags & DEV_RW))
		return -ENODEV;

	switch (minor) {
	case VME_MINOR_REGS:
		f_op = &vme_misc_fops;
		break;

	default:
		return -ENXIO;
	}

	if (f_op && f_op->write)
		return f_op->write(file, buf, count, ppos);

	return 0;
}

long vme_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int minor = iminor(file->f_dentry->d_inode);
	struct file_operations *f_op = NULL;

	switch(minor) {
	case VME_MINOR_MWINDOW:
		f_op = &vme_mwindow_fops;
		break;

	case VME_MINOR_DMA:
		f_op = &vme_dma_fops;
		break;
	default:
		return -ENXIO;
	}

	if (f_op && f_op->unlocked_ioctl)
		return f_op->unlocked_ioctl(file, cmd, arg);

	return 0;
}

static int vme_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned int minor = iminor(file->f_dentry->d_inode);
	struct file_operations *f_op = NULL;

	switch(minor) {
	case VME_MINOR_MWINDOW:
		f_op = &vme_mwindow_fops;
		break;
	default:
		return -ENXIO;
	}

	if (f_op && f_op->mmap)
		return f_op->mmap(file, vma);

	return 0;
}

/**
 * vme_bridge_create_devices() - Create the device nodes for the bridge
 *
 * Returns 0 on success, or a standard kernel error.
 */
static int __devinit vme_bridge_create_devices(void)
{
	int i;
	int rc;

	vme_class = class_create(THIS_MODULE, "vme");

	if (IS_ERR(vme_class)) {
		rc = PTR_ERR(vme_class);
		printk(KERN_ERR PFX "Failed to create vme class\n");
		return rc;
	}

	if (register_chrdev(VME_MAJOR, "vme", &vme_fops)) {
		printk(KERN_ERR PFX "Failed to register device\n");
		class_destroy(vme_class);
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(devlist); i++) {
		device_create(vme_class, NULL,
			      MKDEV(VME_MAJOR, devlist[i].minor),
			      devlist[i].name);
	}

	return 0;
}

/**
 * vme_bridge_remove_devices() - Remove the device nodes for the bridge
 *
 */
static void __devexit vme_bridge_remove_devices(void)
{
	int i;

	unregister_chrdev(VME_MAJOR, "vme");

	for (i = 0; i < ARRAY_SIZE(devlist); i++)
		device_destroy(vme_class, MKDEV (VME_MAJOR, devlist[i].minor));

	class_destroy(vme_class);
}

/**
 * vme_bridge_map_regs() - Map VME chip registers
 * @pdev: PCI device to map
 *
 * Maps the 4k VME bridge register space.
 *
 * RETURNS: zero on success or -ERRNO value
 */
static int __devinit vme_bridge_map_regs(struct pci_dev *pdev)
{
	unsigned int tmp;
	unsigned int vid, did;
	int rc = 0;

	/* Check nobody is using our MMIO resource (BAR 0) */
	rc = pci_request_region(pdev, 0, "VME Bridge registers");
	if (rc) {
		printk(KERN_ERR PFX
		       "Failed to allocate PCI resource for BAR 0\n");
		return rc;
	}

	/* Map the chip registers - those are on BAR 0 */
	vme_bridge->regs = (struct tsi148_chip *)pci_iomap(pdev, 0, 4096);

	if (!vme_bridge->regs) {
		printk(KERN_ERR PFX "Failed to map bridge register space\n");
		return -ENOMEM;
	}

	/* Check that the mapping worked out OK */
	tmp = ioread32(vme_bridge->regs);
	vid = tmp & 0xffff;
	did = (tmp >> 16) & 0xffff;

	if ((vid != PCI_VENDOR_ID_TUNDRA) &&
	    (did != PCI_DEVICE_ID_TUNDRA_TSI148)) {
		printk(KERN_ERR PFX
		       "Failed reading mapped VME bridge registers\n");
		pci_iounmap(pdev, vme_bridge->regs);
		return -ENODEV;
	}

	return 0;
}

/**
 * vme_bridge_map_crg() - Map the bridge CRG address space
 *
 *
 */
static int __devinit vme_bridge_map_crg(void)
{
	int rc;
	unsigned int tmp;
	unsigned int vid, did;

	/* Create and A24/D32 window */
	a24d32_desc.window_num = A24D32_WINDOW;
	a24d32_desc.am = A24D32_AM;
	a24d32_desc.read_prefetch_enabled = 0;
	a24d32_desc.data_width = A24D32_DBW;
	a24d32_desc.sizeu = 0;
	a24d32_desc.sizel = A24D32_VME_SIZE;
	a24d32_desc.vme_addru = 0;
	a24d32_desc.vme_addrl = 0;

	if ((rc = vme_create_window(&a24d32_desc)) != 0) {
		printk(KERN_ERR PFX "Failed to create A24/D32 window\n");
		return rc;
	}


	/* Create a VME mapping for the CRG address space */
	crg_desc.am = CRG_AM;
	crg_desc.read_prefetch_enabled = 0;
	crg_desc.data_width = CRG_DBW;
	crg_desc.sizeu = 0;
	crg_desc.sizel = CRG_VME_SIZE;
	crg_desc.vme_addru = 0;
	crg_desc.vme_addrl = CRG_VME_BASE;

	if ((rc = vme_find_mapping(&crg_desc, 0)) != 0) {
		printk(KERN_ERR PFX "Failed to create CRG mapping\n");
		return rc;
	}

	/* Setup the CRG inbound address and attributes accordingly */
	if ((rc = tsi148_setup_crg(CRG_VME_BASE, CRG_AM)) != 0) {
		printk(KERN_ERR PFX "Failed to setup CRG\n");
		goto out_destroy_window;
	}

	/* Check that the mapping is OK */
	tmp = ioread32(crg_desc.kernel_va);
	vid = tmp & 0xffff;
	did = (tmp >> 16) & 0xffff;

	if ((vid != PCI_VENDOR_ID_TUNDRA) &&
	    (did != PCI_DEVICE_ID_TUNDRA_TSI148)) {
		printk(KERN_ERR PFX
		       "Failed reading mapped VME CRG\n");
		tsi148_disable_crg(vme_bridge->regs);
		rc = -ENODEV;
		goto out_destroy_window;
	}

	crg_base = crg_desc.kernel_va;

	return 0;

out_destroy_window:
	if (vme_destroy_window(&crg_desc) != 0)
		printk(KERN_ERR PFX
		       "vme_bridge_map_crg - Failed to destroy window\n");

	return rc;
}

static int __devexit vme_bridge_unmap_crg(void)
{
	tsi148_disable_crg(vme_bridge->regs);
	return vme_destroy_window(&crg_desc);
}

/**
 * vme_bridge_init_interrupts() - Initialize the bridge interrupts
 *
 */
static int __devinit vme_bridge_init_interrupts(void)
{
	int rc;
	unsigned int intmask;

	/* Register our interrupt handler */
	rc = request_irq(vme_bridge->irq, vme_bridge_interrupt, IRQF_SHARED,
			 "VME Bridge", vme_bridge);

	if (rc) {
		printk(KERN_ERR PFX "Failed to register irq %d handler\n",
		       vme_bridge->irq);
		return rc;
	}

	/* Enable DMA, mailbox, VIRQ (syscon only) & LM Interrupts */
	intmask = (TSI148_LCSR_INT_DMA1 | TSI148_LCSR_INT_DMA0 |
		   TSI148_LCSR_INT_LM3  | TSI148_LCSR_INT_LM2  |
		   TSI148_LCSR_INT_LM1  | TSI148_LCSR_INT_LM0  |
		   TSI148_LCSR_INT_MB3  | TSI148_LCSR_INT_MB2  |
		   TSI148_LCSR_INT_MB1  | TSI148_LCSR_INT_MB0  |
		   TSI148_LCSR_INT_PERR);
		
	if (vme_bridge->syscon)
		intmask |= (TSI148_LCSR_INT_IRQ7 | TSI148_LCSR_INT_IRQ6 |
			    TSI148_LCSR_INT_IRQ5 | TSI148_LCSR_INT_IRQ4 |
			    TSI148_LCSR_INT_IRQ3 | TSI148_LCSR_INT_IRQ2 |
			    TSI148_LCSR_INT_IRQ1);

	printk(KERN_DEBUG PFX "Enabling interrupts %08x\n", intmask);

	if (vme_enable_interrupts(intmask)) {
		printk(KERN_ERR PFX "Failed to enable interrupts");
		free_irq(vme_bridge->irq, vme_bridge);
		return -ENODEV;
	}

	return 0;
}

/**
 * vme_bridge_init() - Initialize the device
 * @dev: PCI device to register
 * @id: Entry in piix_pci_tbl matching with @pdev
 *
 * Called from kernel PCI layer.  Here we initialize the TSI148 VME bridge,
 * do some sanity checks and finally allocate the driver resources.
 *
 * RETURNS:
 * Zero on success, or -ERRNO value.
 */
static int __devinit vme_bridge_init(struct pci_dev *pdev,
				     const struct pci_device_id *id)
{
	int rc;
	int rev;
	struct resource res;
	struct resource *parent_rsrc;

	if ((rc = pci_enable_device (pdev))) {
		printk(KERN_ERR PFX "Could not enable device %s\n",
		       pci_name(pdev));
		return rc;
	}

	/* Enable bus mastering */
	pci_set_master(pdev);

	vme_bridge = kzalloc(sizeof(struct vme_bridge_device), GFP_KERNEL);

	if (vme_bridge == NULL) {
		printk(KERN_ERR PFX "Could not allocate vme_bridge struct\n");
		rc = -ENOMEM;
		goto out_err;
	}

	pci_set_drvdata(pdev, vme_bridge);
	vme_bridge->pdev = pdev;

	dev_info(&pdev->dev, "%s\n", version);

        /* Get chip revision */
	pci_read_config_dword(pdev, PCI_CLASS_REVISION, &rev);
	vme_bridge->rev = rev & 0xff;
	printk(KERN_INFO PFX "found TSI148 VME bridge rev 0x%.4x\n",
	       vme_bridge->rev);

	/*
	 * Get the VME bridge interrupt number unless it has been specified
	 * by the user.
	 */
	if (vme_irq == 0) {
		vme_irq = pdev->irq;
		printk(KERN_INFO PFX "Using IRQ %d\n", vme_irq);

		if (vme_irq == 0)
			vme_irq = 0x50;
	}

	if ((vme_irq == 0) || (vme_irq == 0xff)) {
		printk(KERN_ERR PFX "Bad IRQ number: %d\n", vme_irq);
		rc = -ENODEV;
		goto out_free;
	}

	vme_bridge->irq = vme_irq;

	/* Check there is enough address space opened in the parent bridge */
	memset(&res, 0, sizeof(res));
	res.flags = IORESOURCE_MEM;

	parent_rsrc = pci_find_parent_resource(pdev, &res);

	if (parent_rsrc == 0) {
		printk(KERN_ERR PFX
		       "cannot get VME parent device PCI resource\n");
		rc = -ENODEV;
		goto out_err;
	}

	if ((parent_rsrc->end - parent_rsrc->start) < 0x2000000) {
		printk(KERN_ERR PFX
		       "Not enough PCI memory space available: %lx\n",
		       (unsigned long)(parent_rsrc->end - parent_rsrc->start));
		rc = -ENOMEM;
		goto out_err;
	}

	printk(KERN_DEBUG PFX "Parent bridge window size: 0x%lx\n",
	       (unsigned long)(parent_rsrc->end - parent_rsrc->start + 1));

	/* Map the bridge register space */
	if ((rc = vme_bridge_map_regs(pdev)) != 0)
		goto out_err;

	if ((rc = pci_set_dma_mask(pdev, DMA_32BIT_MASK)) != 0) {
		printk(KERN_ERR PFX "Bridge does not support 32 bit PCI DMA\n");
		/* Indicate to the DMA handling code that DMA is unusable */
		dma_ok = 0;
	} else
		dma_ok = 1;

	/* Find out which slot we're in unless the user specified it */
	if (vme_slotnum == -1)
		vme_slotnum = tsi148_get_slotnum(vme_bridge->regs);

	if ((vme_slotnum < 1) || (vme_slotnum > 21))
		printk(KERN_WARNING PFX "Weird VME slot %d\n", vme_slotnum);

	vme_bridge->slot = vme_slotnum;

	/* Get System controller status */
	vme_bridge->syscon = tsi148_get_syscon(vme_bridge->regs);

	printk(KERN_DEBUG PFX "Register space @ %p - VME slot: %d SysCon: %s\n",
	       vme_bridge->regs, vme_bridge->slot,
	       vme_bridge->syscon ? "yes" : "no");

	/* Init the hardware */
	tsi148_quiesce(vme_bridge->regs);

	/* Initialize window management */
	vme_window_init();

	/* Initialize DMA management */
	if (dma_ok && ((rc = vme_dma_init()) != 0))
		dma_ok = 0;

	tsi148_init(vme_bridge->regs);

	/* Register char devices */
	if ((rc = vme_bridge_create_devices()) != 0)
		goto out_unmap_regs;

	vme_procfs_register();

	/* Map the CRG address space */
	if ((rc = vme_bridge_map_crg()) != 0)
		goto out_remove_devices;

	printk(KERN_DEBUG PFX "Mapped CRG space @ %p (VME %08x)\n",
	       crg_base, crg_desc.vme_addrl);

	/* Initialize and enable interrupts */
	if ((rc = vme_bridge_init_interrupts()) != 0)
		goto out_unmap_crg;

	return 0;

out_unmap_crg:
	vme_bridge_unmap_crg();

out_remove_devices:
	vme_bridge_remove_devices();

out_unmap_regs:
	pci_iounmap(pdev, vme_bridge->regs);
	pci_release_region(pdev, 0);

out_free:
	kfree(vme_bridge);

out_err:
	pci_disable_device(pdev);

	return rc;
}

/**
 * vme_bridge_remove() - Cleanup for module unloading
 * @dev: PCI device to unregister
 *
 */
static void __devexit vme_bridge_remove(struct pci_dev *pdev)
{
	/* First quiesce the bridge */
	tsi148_quiesce(vme_bridge->regs);

	/* Disable and free interrupts */
	disable_irq(vme_bridge->irq);
	free_irq(vme_bridge->irq, vme_bridge);

	vme_bridge_unmap_crg();
	vme_window_exit();

	if (dma_ok)
		vme_dma_exit();

	pci_iounmap(pdev, vme_bridge->regs);
	pci_release_region(pdev, 0);

	pci_disable_device(pdev);

	vme_procfs_unregister();

	vme_bridge_remove_devices();
	class_destroy(vme_class);

	kfree(vme_bridge);
}


static int __init vme_bridge_init_module(void)
{
	return pci_register_driver(&vme_bridge_driver);
}

static void __exit vme_bridge_exit_module(void)
{
	pci_unregister_driver(&vme_bridge_driver);
}

module_init(vme_bridge_init_module);
module_exit(vme_bridge_exit_module);

/* Module parameters */
module_param(vme_irq, int, S_IRUGO);
MODULE_PARM_DESC(vme_irq, "VME Bridge interrupt in the range 1-255");

module_param(vme_slotnum, int, S_IRUGO);
MODULE_PARM_DESC(vme_slotnum, "Slot Number in the range 1-21");

extern unsigned int vme_create_on_find_fail;
module_param(vme_create_on_find_fail, int, S_IRUGO);
MODULE_PARM_DESC(vme_create_on_find_fail, "When set, allows creating a new "
		 "window if a mapping cannot be found");

extern unsigned int vme_destroy_on_remove;
module_param(vme_destroy_on_remove, int, S_IRUGO);
MODULE_PARM_DESC(vme_destroy_on_remove, "When set, removing the last mapping "
		 "on a window also destroy the window");


MODULE_AUTHOR("Sebastien Dugue");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Tundra TSI148 PCI-VME Bridge driver");
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_DEVICE_TABLE(pci, tsi148_ids);

