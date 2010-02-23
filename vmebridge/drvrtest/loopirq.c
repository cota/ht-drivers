#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "../include/vmebus.h"

#define PFX "LoopIrq: "
#define DRV_MODULE_VERSION	"1.0"

static int irq_level = -1;
static int irq_vector = -1;

#define DEFAULT_IRQ_LEVEL	7
#define DEFAULT_IRQ_VECTOR	0xaa


int handler(void *arg)
{
	printk(KERN_DEBUG PFX "Received IRQ%d Vector %d\n",
	       DEFAULT_IRQ_LEVEL, DEFAULT_IRQ_VECTOR);

	return 0;
}

static int __init testirq_init_module(void)
{
	int rc;
	int i;

	if ((irq_level < 1) || (irq_level > 7))
		irq_level = DEFAULT_IRQ_LEVEL;

	if ((irq_vector < 0) || (irq_vector > 255))
		irq_vector = DEFAULT_IRQ_VECTOR;

	printk(KERN_INFO "VME IRQ loopback test V%s - IRQ%d Vector %d\n",
	       DRV_MODULE_VERSION, irq_level, irq_vector);

	if ((rc = vme_request_irq(irq_vector, handler, NULL,
				  "loopirq")) != 0) {
		printk(KERN_ERR PFX "Failed to register interrupt handler\n");
		return -ENODEV;
	}

	for (i = 0; i < 10; i++) {
		printk(KERN_DEBUG PFX "Generating VME IRQ%d Vector %d\n",
		       irq_level, irq_vector);

		if (vme_generate_interrupt(irq_level, irq_vector, 100)) {
			printk(KERN_ERR PFX "IACK timeout.\n");
			break;
		}

		msleep(1500);
	}

	return 0;
}

static void __exit testirq_exit_module(void)
{
	vme_free_irq(irq_vector);
}

module_init(testirq_init_module);
module_exit(testirq_exit_module);

/* Module parameters */
module_param(irq_level, int, S_IRUGO);
MODULE_PARM_DESC(irq_level, "VME IRQ level in the range 1-7");

module_param(irq_vector, int, S_IRUGO);
MODULE_PARM_DESC(irq_vector, "VME IRQ vector in the range 0-255");

MODULE_AUTHOR("Sebastien Dugue");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VME IRQ loopback test module");
MODULE_VERSION(DRV_MODULE_VERSION);
