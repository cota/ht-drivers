#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include "modulbus_register.h"
#include "mod-pci.h"

#define	MAX_DEVICES	16
#define	DRIVER_NAME	"mod-pci"
#define PFX		DRIVER_NAME ": "

/*
 * this module is invoked as
 *     $ insmod mod-pci lun=0,1,2,4 bus_number=1,1,1,2 slot_number=2,3,5,1
 * to describe the bus number and slot number associated to devices with
 * prescribed luns.
 */

static int lun[MAX_DEVICES];
static unsigned int nlun;
module_param_array(lun, int, &nlun, S_IRUGO);

static int bus_number[MAX_DEVICES];
static unsigned int nbus_number;
module_param_array(bus_number, int, &nbus_number, S_IRUGO);

static int slot_number[MAX_DEVICES];
static unsigned int nslot_number;
module_param_array(slot_number, int, &nslot_number, S_IRUGO);

/** static device table */
static struct mod_pci device_table[MAX_DEVICES];
static int devices;

static struct pci_device_id mod_pci_ids[] = {
	{ PCI_VENDOR_ID_JANZ, PCI_DEVICE_ID_JANZ_MOD_PCI_1,
	  PCI_SUBSYSTEM_VENDOR_ID_JANZ, PCI_SUBSYSTEM_ID_MOD_PCI_1_0, },
	{ PCI_VENDOR_ID_JANZ, PCI_DEVICE_ID_JANZ_MOD_PCI_1,
	  PCI_SUBSYSTEM_VENDOR_ID_JANZ, PCI_SUBSYSTEM_ID_MOD_PCI_2_0, },
	{ PCI_VENDOR_ID_JANZ, PCI_DEVICE_ID_JANZ_MOD_PCI_1,
	  PCI_SUBSYSTEM_VENDOR_ID_JANZ, PCI_SUBSYSTEM_ID_MOD_PCI_2_1, },
	{ PCI_VENDOR_ID_JANZ, PCI_DEVICE_ID_JANZ_MOD_PCI_1,
	  PCI_SUBSYSTEM_VENDOR_ID_JANZ, PCI_SUBSYSTEM_ID_MOD_PCI_3_0, },
};

static struct mod_pci *find_device_by_lun(int lun)
{
	int i;

	for (i = 0; i < devices; i++)
		if (device_table[i].lun == lun)
			return &device_table[i];
	return NULL;
}

static unsigned int modpci_offsets[] = {
	MOD_PCI_SLOT0_OFFSET,
	MOD_PCI_SLOT1_OFFSET,
};

static int get_address_space(
	struct carrier_as *as,
	int board_number,
	int board_position,
	int address_space_number)
{
	struct mod_pci *dev = find_device_by_lun(board_number);

	if (dev == NULL || (board_position & (~0x1)) != 0)
		return -1;
	as->address = (unsigned long)dev->vaddr + modpci_offsets[board_position];
	as->width   = MOD_PCI_WIDTH;
	as->size    = MOD_PCI_WINDOW_SIZE/4;
	as->is_big_endian = 1;

	return 0;
}

static struct mod_pci *find_device_config(struct pci_dev *dev)
{
	int bus  = dev->bus->number;
	int slot = PCI_SLOT(dev->devfn);
	int i;

	/* find the device in config table */
	for (i = 0; i < devices; i++) {
		struct mod_pci *entry = &device_table[i];
		if (entry->bus_number  == bus &&
		    entry->slot_number == slot)
		    return entry;
	}
	return NULL;
}

static void __iomem
*map_bar(struct pci_dev *dev, unsigned int bar, unsigned int bar_size)
{
	void __iomem *ret;

	if (!(pci_resource_flags(dev, bar) & IORESOURCE_MEM)) {
		printk(KERN_ERR PFX "BAR#%d not a MMIO, device not present?\n", bar);
		goto failed_request;
	}
	if (pci_resource_len(dev, bar) < bar_size) {
		printk(KERN_ERR PFX "wrong BAR#%d size\n", bar);
		goto failed_request;
	}
	if (pci_request_region(dev, bar, DRIVER_NAME) != 0) {
		printk(KERN_ERR PFX "could not request BAR#%d\n", bar);
		goto failed_request;
	}
	ret = ioremap(pci_resource_start(dev, bar), bar_size);
	if (ret == NULL) {
		printk(KERN_ERR PFX "could not map BAR#%d\n", bar);
		goto failed_map;
	}
	return ret;

failed_map:
	pci_release_regions(dev);
failed_request:
	return NULL;
}

static struct mz_callback {
	isrcb_t	callback;
	void	*dev;
} modpci_callbacks[MAX_DEVICES][MOD_PCI_SLOTS];

static inline int
within_range(int board, int slot)
{
	return board >= 0 && board < MAX_DEVICES &&
	       slot >= 0  && slot  < MOD_PCI_SLOTS;
}

static void modpci_enable_irq(struct mod_pci *dev)
{
	void __iomem *int_enable = &dev->onboard->int_enable;
	iowrite8(0x3, int_enable);
}

static void modpci_disable_irq(struct mod_pci *dev)
{
	void __iomem *int_disable = &dev->onboard->int_disable;
	iowrite8(0x3, int_disable);
}

static void modpci_reset(struct mod_pci *dev)
{
	void __iomem *reset_assert   = &dev->onboard->reset_assert;
	void __iomem *reset_deassert = &dev->onboard->reset_deassert;
	iowrite8(0x3, reset_assert);
	iowrite8(0x3, reset_deassert);
}


static irqreturn_t modpci_interrupt(int irq, void *device_id)
{
	struct mod_pci *dev = device_id;
	int i;
	u8 int_stat;
	u8 slot[] = { 0, 0 };

	/* determine source */
	int_stat = ioread8(&dev->onboard->int_stat);
	slot[0] = !(int_stat & 1);
	slot[1] = !(int_stat & 2);
	if (!slot[0] && !slot[1])
		return IRQ_NONE;	/* not mine */

	for (i = 0; i < MOD_PCI_SLOTS; i++) {
		isrcb_t callback;
		void *source;

		if (slot[i]) {
			struct mz_callback *cb = &modpci_callbacks[dev->lun][i];
			callback = cb->callback;
			source   = cb->dev;
		} else
			continue;
	        if (callback == NULL || callback(source, NULL) < 0) {
			printk(KERN_ERR PFX "unhandled interrupt! "
			" irq = %d lun = %d, slot = %d\n", irq, dev->lun, i);
			return IRQ_NONE;
		}
	}
	return IRQ_HANDLED;
}

static int
modpci_register_isr(isrcb_t callback,
	void *source,
	int board_number,
	int board_position)
{
	struct mz_callback *cb;

	if (!within_range(board_number, board_position)) {
		printk(KERN_ERR PFX "invalid %s board number %d"
			" and position %d\n", DRIVER_NAME,
			board_number, board_position);
		return -1;
	}
	cb = &modpci_callbacks[board_number][board_position];
	cb->callback = callback;
	cb->dev = source;
	return 0;
}

static int probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct mod_pci *cfg_entry;
	u8 irq;
	int errno;

	/* check static config is present */
	cfg_entry = find_device_config(dev);
	if (!cfg_entry) {
		printk(KERN_INFO PFX
			"no config data for bus = %d, slot %d\n",
			dev->bus->number, PCI_SLOT(dev->devfn));
		goto failed_enable;
	}
	printk(KERN_INFO PFX
		"configuring device at bus = %d, slot %d\n",
		cfg_entry->bus_number, cfg_entry->slot_number);

	if (pci_enable_device(dev) < 0) {
		printk(KERN_ERR PFX "could not enable device\n");
		goto failed_enable;
	}

	/* map MMIO slot addresses and onboard registers */
	cfg_entry->vaddr   = map_bar(dev, MOD_PCI_BIG_BAR, MOD_PCI_BIG_BAR_SIZE);
	cfg_entry->onboard = map_bar(dev, MOD_PCI_ONBOARD_REGS_BAR, MOD_PCI_ONBOARD_REGS_BAR_SIZE);
	if (cfg_entry->vaddr == NULL || cfg_entry->onboard == NULL) {
		printk(KERN_ERR PFX "could not map registers, "
		"mmio = %p, onboard = %p\n",
		cfg_entry->vaddr, cfg_entry->onboard);
		goto failed_map;
	}

	/* success! */
	printk(KERN_INFO PFX "configured device maps, "
		"lun = %d, bus = %d, slot = %d, "
		"vaddr = %p onboard = %p\n",
		cfg_entry->lun,
		cfg_entry->bus_number, cfg_entry->slot_number,
		cfg_entry->vaddr, cfg_entry->onboard);

	/* get interrupt line, enable ints and install handler */
	irq = dev->irq;
	errno = request_irq(irq, modpci_interrupt,
			IRQF_SHARED, DRIVER_NAME, cfg_entry);
	if (errno != 0) {
		printk(KERN_ERR PFX "could not request irq"
			"%d for device %p, err = %d\n",
				irq, cfg_entry, errno);
		goto failed_irq;
	}
	modpci_reset(cfg_entry);
	modpci_enable_irq(cfg_entry);
	printk(KERN_INFO PFX "got irq %d for dev %p\n", irq, cfg_entry);
	return 0;

failed_irq:
	modpci_disable_irq(cfg_entry);
	iounmap(cfg_entry->onboard);
	iounmap(cfg_entry->vaddr);
	pci_release_region(dev, MOD_PCI_ONBOARD_REGS_BAR);
	pci_release_region(dev, MOD_PCI_BIG_BAR);
failed_map:
	pci_disable_device(dev);
failed_enable:
	return -ENODEV;
}

static void remove(struct pci_dev *dev)
{
	struct mod_pci *cfg = find_device_config(dev);

	printk(KERN_INFO PFX "removing device %d\n", cfg->lun);
	modpci_disable_irq(cfg);
	printk(KERN_INFO PFX "freeing irq %d for dev %p\n", dev->irq, cfg);
	free_irq(dev->irq, cfg);
	iounmap(cfg->onboard);
	iounmap(cfg->vaddr);
	pci_release_region(dev, MOD_PCI_ONBOARD_REGS_BAR);
	pci_release_region(dev, MOD_PCI_BIG_BAR);
	pci_disable_device(dev);
}

static struct pci_driver pci_driver = {
	.name     = DRIVER_NAME,
	.id_table = mod_pci_ids,
	.probe    = probe,
	.remove   = remove,
};

static int __init init(void)
{
	int device = 0;

	printk(KERN_INFO PFX "initializing driver\n");
	if (modulbus_carrier_register(
		DRIVER_NAME, get_address_space, modpci_register_isr)) {
		printk(KERN_ERR PFX "could not register with modulbus\n");
		goto failed_init;
	}
	if (nlun < 0 || nlun >= MAX_DEVICES) {
		printk(KERN_ERR PFX
		"invalid number of configured devices (%d)\n", nlun);
		goto failed_init;
	}
	if (nbus_number != nlun || nslot_number != nlun) {
		printk(KERN_ERR PFX "parameter mismatch.\n"
			"Given %d luns, %d bus numbers, %d slot numbers\n",
			nlun, nbus_number, nslot_number);
		goto failed_init;
	}

	for (device = 0; device < nlun; device++) {
		struct mod_pci *dev = &device_table[device];

		dev->lun	= lun[device];
		dev->bus_number	= bus_number[device];
		dev->slot_number= slot_number[device];
		printk(KERN_INFO PFX "shall config lun %d," "bus %d, slot %d\n",
			dev->lun, dev->bus_number, dev->slot_number);
	}
	devices = device;

	printk(KERN_INFO PFX "registering driver\n");
	return pci_register_driver(&pci_driver);

failed_init:
	printk(KERN_ERR PFX "module exiting\n");
	return -1;
}

static void __exit exit(void)
{
	pci_unregister_driver(&pci_driver);
	modulbus_carrier_unregister(DRIVER_NAME);
}

module_init(init);
module_exit(exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan David Gonzalez Cobas <dcobas@cern.ch>");
