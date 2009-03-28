#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm-generic/iomap.h>

#include "../include/vmebus.h"
#include "ctrhard.h"

#define PFX "Ctr1KHzIrq: "
#define DRV_MODULE_VERSION	"1.0"

static struct vme_mapping ctr_desc = {
	.data_width		= VME_D32,
	.am			= VME_A24_USER_DATA_SCT,
	.read_prefetch_enabled	= 0,
	.sizeu			= 0,
	.sizel			= 0x10000,
	.vme_addru		= 0,
	.vme_addrl		= 0xc00000
};

static void *vmeaddr = NULL;    /* Point to CTR hardware */
CtrDrvrMemoryMap *mmap;

#define CTR_IRQ_LEVEL	2
#define CTR_IRQ_VECTOR	0xb8


unsigned int ClearInterrupt(void)
{
	unsigned int source;

	source = ioread32be((unsigned int *)&mmap->InterruptSource);

	return source;
}

void EnableCtrInterrupt(void) {

	unsigned int val;

	printk(KERN_DEBUG PFX "Enabling 1KHz interrupt on IRQ%d Vector %d\n",
	       CTR_IRQ_LEVEL, CTR_IRQ_VECTOR);

	/* Setup the IRQ level and vector */
	val = (unsigned long)((CTR_IRQ_LEVEL << 8) | (CTR_IRQ_VECTOR & 0xff));
	iowrite32be(val, &mmap->Setup);

	if (vme_bus_error_check(1))
		printk(KERN_ERR PFX "Bus error writing Setup\n");

	/* Will interrupt once per millisecond */
	iowrite32be(CtrDrvrInterruptMask1KHZ |
		    CtrDrvrInterruptMaskPPS |
		    CtrDrvrInterruptMaskGMT_EVENT_IN,
		    &mmap->InterruptEnable);

	if (vme_bus_error_check(1))
		printk(KERN_ERR PFX "Bus error writing InterruptEnable\n");

	val = ioread32be(&mmap->InterruptEnable);

	if (vme_bus_error_check(1))
		printk(KERN_ERR PFX "Bus error reading InterruptEnable\n");

	if (val != (CtrDrvrInterruptMask1KHZ |
		    CtrDrvrInterruptMaskPPS |
		    CtrDrvrInterruptMaskGMT_EVENT_IN))
		printk(KERN_DEBUG PFX "Failed to enable 1KHz interrupt %08x\n",
			val);
}

void DisableCtrInterrupt(void)
{
	printk(KERN_DEBUG PFX "Disabling 1KHz interrupt\n");
	iowrite32be(0, &mmap->InterruptEnable);

	if (vme_bus_error_check(1))
		printk(KERN_ERR PFX "Bus error writing InterruptEnable\n");
}

void EnableCtrModule(void)
{
	int val = ioread32be(&mmap->Command);

	printk(KERN_DEBUG PFX "Enabling CTR module\n");

	if (vme_bus_error_check(1))
		printk(KERN_ERR PFX "Bus error reading Command\n");

	val |= CtrDrvrCommandENABLE;
	val &= ~CtrDrvrCommandDISABLE;

	iowrite32be(val, &mmap->Command);

	if (vme_bus_error_check(1))
		printk(KERN_ERR PFX "Bus error writing Command\n");
}

void DisableCtrModule(void)
{
	int val = ioread32be(&mmap->Command);

	printk(KERN_DEBUG PFX "Disabling CTR module\n");

	if (vme_bus_error_check(1))
		printk(KERN_ERR PFX "Bus error reading Command\n");

	val &= ~CtrDrvrCommandENABLE;
	val |= CtrDrvrCommandDISABLE;

	iowrite32be(val, &mmap->Command);

	if (vme_bus_error_check(1))
		printk(KERN_ERR PFX "Bus error writing Command\n");
}

void ResetCtr(void)
{
	printk(KERN_DEBUG PFX "Resetting CTR module\n");
	iowrite32be(CtrDrvrCommandRESET, &mmap->Command);

	if (vme_bus_error_check(1))
		printk(KERN_ERR PFX "Bus error Writing Command\n");

	/* Wait at least 10 ms after a "reset", for the module to recover */
	msleep(20);
}

int IntrHandler(void *arg) {
	unsigned int source;

	source = ClearInterrupt();

	if (source & CtrDrvrInterruptMaskPPS)
		printk(KERN_DEBUG PFX "Received PPS IRQ\n");

	return 0;
}

void show_ctr_info(void)
{
	printk(KERN_DEBUG PFX "CableId:         %08x\n",
	       ioread32be(&mmap->CableId));
	printk(KERN_DEBUG PFX "VhdlVersion:     %08x\n",
	       ioread32be(&mmap->VhdlVersion));
	printk(KERN_DEBUG PFX "InterruptEnable: %08x\n",
	       ioread32be(&mmap->InterruptEnable));
	printk(KERN_DEBUG PFX "Status:          %08x\n",
	       ioread32be(&mmap->Status));
	printk(KERN_DEBUG PFX "Command:         %08x\n",
	       ioread32be(&mmap->Command));
	printk(KERN_DEBUG PFX "Setup:           %08x\n",
	       ioread32be(&mmap->Setup));
	printk(KERN_DEBUG PFX "PartityErrs:     %08x\n",
	       ioread32be(&mmap->PartityErrs));
	printk(KERN_DEBUG PFX "SyncErrs:        %08x\n",
	       ioread32be(&mmap->SyncErrs));
	printk(KERN_DEBUG PFX "TotalErrs:       %08x\n",
	       ioread32be(&mmap->TotalErrs));
	printk(KERN_DEBUG PFX "CodeViolErrs:    %08x\n",
	       ioread32be(&mmap->CodeViolErrs));
	printk(KERN_DEBUG PFX "QueueErrs:       %08x\n",
	       ioread32be(&mmap->QueueErrs));
}

int ctrirq_init(void)
{
	int rc;

	if ((rc = vme_find_mapping(&ctr_desc, 1)) != 0) {
		printk(KERN_ERR PFX "Failed to map CTR_n");
		return rc;
	}

	vmeaddr = ctr_desc.kernel_va;
	mmap = (CtrDrvrMemoryMap *) vmeaddr;

	ResetCtr();

	EnableCtrModule();

	if ((rc = vme_request_irq(CTR_IRQ_VECTOR, IntrHandler, &ctr_desc,
				  "ctrirq")) != 0) {
		printk(KERN_ERR PFX "Failed to register interrupt handler\n");
		goto out_unmap;
	}

	EnableCtrInterrupt();

	show_ctr_info();

	return 0;

out_unmap:
	DisableCtrModule();

	if (vme_release_mapping(&ctr_desc, 1) != 0)
		printk(KERN_WARNING PFX "Failed to release mapping on error\n");

	return rc;
}

void ctrirq_exit(void)
{
	DisableCtrInterrupt();

	DisableCtrModule();

	if (vme_free_irq(CTR_IRQ_VECTOR))
		printk(KERN_WARNING PFX "Failed to free irq\n");

	if (vme_release_mapping(&ctr_desc, 1) != 0)
		printk(KERN_WARNING PFX "Failed to release mapping\n");
}


static int __init ctrirq_init_module(void)
{
	return ctrirq_init();
}

static void __exit ctrirq_exit_module(void)
{
	ctrirq_exit();
}

module_init(ctrirq_init_module);
module_exit(ctrirq_exit_module);

MODULE_AUTHOR("Sebastien Dugue");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CTR 1KHz IRQ test module");
MODULE_VERSION(DRV_MODULE_VERSION);
