/*
 * vme_irq.c - PCI-VME bridge interrupt management
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
 *  This file provides the PCI-VME bridge interrupt management code.
 */

#include <linux/interrupt.h>

#include "tsi148.h"
#include "vme_bridge.h"


unsigned int vme_interrupts_enabled;

struct vme_irq {
	int	(*handler)(void *arg);
	void	*arg;
#ifdef CONFIG_PROC_FS
	int	count;
	char	*name;
#endif
};

#define VME_NUM_VECTORS		256

/* Mutex to prevent concurrent access to the IRQ table */
static DEFINE_MUTEX(vme_irq_table_lock);
static struct vme_irq vme_irq_table[VME_NUM_VECTORS];

/* Interrupt counters */
enum interrupt_idx {
	INT_DMA0 = 0,
	INT_DMA1,
	INT_MB0,
	INT_MB1,
	INT_MB2,
	INT_MB3,
	INT_LM0,
	INT_LM1,
	INT_LM2,
	INT_LM3,
	INT_IRQ1,
	INT_IRQ2,
	INT_IRQ3,
	INT_IRQ4,
	INT_IRQ5,
	INT_IRQ6,
	INT_IRQ7,
	INT_PERR,
	INT_VERR,
	INT_SPURIOUS
};

struct interrupt_stats  {
	unsigned int 	count;
	char		*name;
} int_stats[] = {
	{.name = "DMA0"}, {.name = "DMA1"},
	{.name = "MB0"}, {.name = "MB1"}, {.name = "MB2"}, {.name = "MB3"},
	{.name = "LM0"}, {.name = "LM1"}, {.name = "LM2"}, {.name = "LM3"},
	{.name = "IRQ1"}, {.name = "IRQ2"}, {.name = "IRQ3"}, {.name = "IRQ4"},
	{.name = "IRQ5"}, {.name = "IRQ6"}, {.name = "IRQ7"},
	{.name = "PERR"}, {.name = "VERR"}, {.name = "SPURIOUS"}
};


#ifdef CONFIG_PROC_FS

int vme_interrupts_proc_show(char *page, char **start, off_t off,
			     int count, int *eof, void *data)
{
	char *p = page;
	int i;

	p += sprintf(p, " Source       Count\n");
	p += sprintf(p, "--------------------------\n\n");

	for (i = 0; i < ARRAY_SIZE(int_stats); i++)
		p += sprintf(p, "%-8s      %d\n", int_stats[i].name,
			     int_stats[i].count);

	*eof = 1;
	return p - page;
}

int vme_irq_proc_show(char *page, char **start, off_t off, int count,
		      int *eof, void *data)
{
	char *p = page;
	int i;
	struct vme_irq *virq;

	p += sprintf(p, "Vector     Count      Client\n");
	p += sprintf(p, "------------------------------------------------\n\n");

	for (i = 0; i < VME_NUM_VECTORS; i++) {
		virq = &vme_irq_table[i];

		if (virq->handler)
			p += sprintf(p, " %3d     %10u   %s\n",
				     i, virq->count, virq->name);
	}

	*eof = 1;
	return p - page;
}

#endif /* CONFIG_PROC_FS */

void account_dma_interrupt(int channel_mask)
{
	if (channel_mask & 1)
		int_stats[INT_DMA0].count++;

	if (channel_mask & 2)
		int_stats[INT_DMA1].count++;
}

static void handle_pci_error(void)
{
	tsi148_handle_pci_error();

	int_stats[INT_PERR].count++;
}

static void handle_vme_error(void)
{
	tsi148_handle_vme_error();

	int_stats[INT_VERR].count++;
}

static void handle_mbox_interrupt(int mb_mask)
{
	if (mb_mask & 1)
		int_stats[INT_MB0].count++;

	if (mb_mask & 2)
		int_stats[INT_MB1].count++;

	if (mb_mask & 4)
		int_stats[INT_MB2].count++;

	if (mb_mask & 8)
		int_stats[INT_MB3].count++;
}

static void handle_lm_interrupt(int lm_mask)
{
	if (lm_mask & 1)
		int_stats[INT_LM0].count++;

	if (lm_mask & 2)
		int_stats[INT_LM1].count++;

	if (lm_mask & 4)
		int_stats[INT_LM2].count++;

	if (lm_mask & 8)
		int_stats[INT_LM3].count++;
}

/**
 * handle_vme_interrupt() - VME IRQ handler
 * @irq_mask: Mask of the raised IRQs
 *
 *  Get the IRQ vector through an IACK cycle and call the handler for
 * that vector if installed.
 */
static void handle_vme_interrupt(int irq_mask)
{
	int i;
	int vec;
	struct vme_irq *virq;

	for (i = 7; i > 0; i--) {

		if (irq_mask & (1 << i)) {
			/* Generate an 8-bit IACK cycle and get the vector */
			vec = tsi148_iack8(vme_bridge->regs, i);

			virq = &vme_irq_table[vec];

			if (virq->handler) {
#ifdef CONFIG_PROC_FS
				virq->count++;
#endif
				virq->handler(virq->arg);
			}

			int_stats[INT_IRQ1 + i - 1].count++;
		}
	}
}


/**
 * vme_bridge_interrupt() - VME bridge main interrupt handler
 *
 */
irqreturn_t vme_bridge_interrupt(int irq, void *ptr)
{
	unsigned int raised;
	unsigned int mask;

	/*
	 * We need to read the interrupt status from the VME bus to make
	 * sure the internal FIFO has been flushed of pending writes.
	 */
	while ((raised = tsi148_get_int_status(crg_base)) != 0) {

		/*
		 * Clearing of the interrupts must be done by writing to the
		 * INTS register through the VME bus.
		 */
		tsi148_clear_int(crg_base, raised);

		mask = raised & vme_interrupts_enabled;

		/* Only handle enabled interrupts */
		if (!mask) {
			int_stats[INT_SPURIOUS].count++;
			return IRQ_NONE;
		}

		if (mask & TSI148_LCSR_INT_DMA_M) {
			handle_dma_interrupt((mask & TSI148_LCSR_INT_DMA_M) >>
					     TSI148_LCSR_INT_DMA_SHIFT);
			mask &= ~TSI148_LCSR_INT_DMA_M;
		}

		if (mask & TSI148_LCSR_INT_PERR) {
			handle_pci_error();
			mask &= ~TSI148_LCSR_INT_PERR;
		}

		if (mask & TSI148_LCSR_INT_VERR) {
			handle_vme_error();
			mask &= ~TSI148_LCSR_INT_VERR;
		}

		if (mask & TSI148_LCSR_INT_MB_M) {
			handle_mbox_interrupt((mask & TSI148_LCSR_INT_MB_M) >>
					      TSI148_LCSR_INT_MB_SHIFT);
			mask &= ~TSI148_LCSR_INT_MB_M;
		}

		if (mask & TSI148_LCSR_INT_LM_M) {
			handle_lm_interrupt((mask & TSI148_LCSR_INT_LM_M) >>
					    TSI148_LCSR_INT_LM_SHIFT);
			mask &= ~TSI148_LCSR_INT_LM_M;
		}

		if (mask & TSI148_LCSR_INT_IRQM) {
			handle_vme_interrupt(mask & TSI148_LCSR_INT_IRQM);
			mask &= ~TSI148_LCSR_INT_IRQM;
		}

		/* Check that we handled everything */
		if (mask)
			printk(KERN_WARNING PFX
			       "Unhandled interrupt %08x (enabled %08x)\n",
			       mask, vme_interrupts_enabled);
	}

	return IRQ_HANDLED;
}

/**
 * vme_enable_interrupts() - Enable VME bridge interrupts
 * @mask: Interrupts to enable
 *
 */
int vme_enable_interrupts(unsigned int mask)
{
	unsigned int enabled;
	unsigned int new;

	enabled = tsi148_get_int_enabled(vme_bridge->regs);
	new = enabled | mask;

	vme_interrupts_enabled = new;
	return tsi148_set_interrupts(vme_bridge->regs, new);
}

/**
 * vme_disable_interrupts() - Disable VME bridge interrupts
 * @mask: Interrupts to disable
 *
 */
int vme_disable_interrupts(unsigned int mask)
{
	unsigned int enabled;
	unsigned int new;

	enabled = tsi148_get_int_enabled(vme_bridge->regs);
	new = enabled & ~mask;

	vme_interrupts_enabled = new;
	return tsi148_set_interrupts(vme_bridge->regs, new);
}

/**
 * vme_request_irq() - Install handler for a given VME IRQ vector
 * @vec: VME IRQ vector
 * @handler: Interrupt handler
 * @arg: Interrupt handler argument
 * @name: Interrupt name (only used for stats in Procfs)
 *
 */
int vme_request_irq(unsigned int vec, int (*handler)(void *),
		    void *arg, const char *name)
{
	struct vme_irq *virq;
	int rc = 0;

	/* Check the vector is within the bound */
	if (vec >= VME_NUM_VECTORS)
		return -EINVAL;

	if ((rc = mutex_lock_interruptible(&vme_irq_table_lock)) != 0)
		return rc;

	virq = &vme_irq_table[vec];

	/* Check if that vector is already used */
	if (virq->handler) {
		rc = -EBUSY;
		goto out_unlock;
	}

	virq->handler = handler;
	virq->arg = arg;

#ifdef CONFIG_PROC_FS
	virq->count = 0;

	if (name)
		virq->name = (char *)name;
	else
		virq->name = "Unknown";
#endif

out_unlock:
	mutex_unlock(&vme_irq_table_lock);

	if (!rc)
		printk(KERN_DEBUG PFX "Registered vector %d for %s\n",
		       vec, virq->name);
	else
		printk(KERN_WARNING PFX "Could not install ISR: vector %d "
		       "already in use by %s", vec, virq->name);
	return rc;
}
EXPORT_SYMBOL_GPL(vme_request_irq);

/**
 * vme_free_irq() - Uninstall handler for a given VME IRQ vector
 * @vec: VME IRQ vector
 *
 */
int vme_free_irq(unsigned int vec)
{
	struct vme_irq *virq;
	int rc = 0;

	/* Check the vector is within the bound */
	if (vec >= VME_NUM_VECTORS)
		return -EINVAL;

	if ((rc = mutex_lock_interruptible(&vme_irq_table_lock)) != 0)
		return rc;

	virq = &vme_irq_table[vec];

	/* Check there really was a handler installed */
	if (!virq->handler) {
		rc = -EINVAL;
		goto out_unlock;
	}

	virq->handler = NULL;
	virq->arg = NULL;

#ifdef CONFIG_PROC_FS
	virq->count = 0;
	virq->name = NULL;
#endif

out_unlock:
	mutex_unlock(&vme_irq_table_lock);

	return rc;
}
EXPORT_SYMBOL_GPL(vme_free_irq);

/**
 * vme_generate_interrupt() - Generate an interrupt on the VME bus
 * @level: IRQ level (1-7)
 * @vector: IRQ vector (0-255)
 * @msecs: Timeout for IACK in milliseconds
 *
 *  This function generates an interrupt on the VME bus and waits for IACK
 * for msecs milliseconds.
 *
 *  Returns 0 on success or -ETIME if the timeout expired.
 *
 */
int vme_generate_interrupt(int level, int vector, signed long msecs)
{
	return tsi148_generate_interrupt(level, vector, msecs);
}
EXPORT_SYMBOL_GPL(vme_generate_interrupt);
