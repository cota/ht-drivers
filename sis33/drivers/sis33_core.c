/*
 * sis33_core.c
 * Core kernel code for sis33 drivers.
 *
 * Copyright (c) 2009-2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/log2.h>
#include <linux/fs.h>

#include <asm/uaccess.h>

#include "sis33core.h"
#include "sis33core_internal.h"

#define DRV_NAME	"sis33"
#define PFX		DRV_NAME ": "
#define DRV_MODULE_VERSION	"1.0"

/*
 * we create our own wq because contention is high when there are
 * many concurrent acquisitions -- they are all serialised
 * through the same bus and using the common 'events' wq would
 * be anti-social.
 */
struct workqueue_struct *sis33_workqueue;
EXPORT_SYMBOL_GPL(sis33_workqueue);

static struct class *sis33_class;
static dev_t sis33_devno;
static DEFINE_MUTEX(sis33_lock);
static u32 sis33_cards_lock;

int sis33_is_acquiring(struct sis33_card *card)
{
	int i;

	for (i = 0; i < card->n_segments; i++) {
		if (atomic_read(&card->segments[i].acquiring))
			return 1;
	}
	return 0;
}

int sis33_is_transferring(struct sis33_card *card)
{
	int i;

	for (i = 0; i < card->n_segments; i++) {
		if (atomic_read(&card->segments[i].transferring))
			return 1;
	}
	return 0;
}

/*
 * This function assumes that the first entry in card->ev_lengths is the
 * maximum length of a segment.
 */
static int
sis33_segment_max_nr_samples(struct sis33_card *card)
{
	return card->ev_lengths[0] / (card->n_segments / card->n_segments_min);
}

int sis33_value_is_in_array(const unsigned int *array, int n, unsigned int value)
{
	int i;

	for (i = 0; i < n; i++) {
		if (value == array[i])
			return 1;
	}
	return 0;
}

static int sis33_check_acq_buffers(const struct sis33_card *card, int segment_nr, const struct sis33_acq *acqs, int n)
{
	const struct sis33_segment *segment = &card->segments[segment_nr];
	ssize_t acq_minsize = segment->nr_samp_per_ev * sizeof(u16);
	int i;

	for (i = 0; i < n; i++) {
		if (acqs[i].size == 0 || acqs[i].size != acq_minsize) {
			dev_info(card->dev, "Invalid size %u of acquisition[%d]. Valid acq size: %u\n",
				acqs[i].size, i, acq_minsize);
			return -EINVAL;
		}
	}
	return 0;
}

static int sis33_fetch_ioctl(struct sis33_card *card, void __user *arg)
{
	struct sis33_acq_list list;
	struct sis33_acq *acqs;
	struct sis33_segment *segment;
	unsigned int flags;
	ssize_t size;
	int nr_events;
	int ret;

	if (copy_from_user(&list, arg, sizeof(list)))
		return -EFAULT;

	flags = list.flags & (SIS33_ACQF_DONTWAIT | SIS33_ACQF_TIMEOUT);
	if (flags & SIS33_ACQF_DONTWAIT && flags & SIS33_ACQF_TIMEOUT) {
		dev_info(card->dev, "Invalid flags: DONTWAIT and TIMEOUT can't coexist.\n");
		return -EINVAL;
	}

	if (list.n_acqs == 0)
		return -EINVAL;
	size = list.n_acqs * sizeof(struct sis33_acq);
	acqs = kzalloc(size, GFP_KERNEL);
	if (acqs == NULL)
		return -ENOMEM;

	if (copy_from_user(acqs, list.acqs, size)) {
		ret = -EFAULT;
		goto out;
	}

	if (mutex_lock_interruptible(&card->lock)) {
		ret = -EINTR;
		goto out;
	}

	if (list.segment >= card->n_segments ||
	    list.channel >= card->n_channels) {
		ret = -EINVAL;
		goto out_unlock;
	}

	segment = &card->segments[list.segment];

	if (atomic_read(&segment->acquiring)) {
		mutex_unlock(&card->lock);
		/* wait for the acquisition to finish */
		if (!card->ops->acq_wait) {
			ret = -EINVAL;
			goto out;
		}
		if (flags & SIS33_ACQF_TIMEOUT)
			ret = card->ops->acq_wait(card, &list.timeout);
		else if (flags & SIS33_ACQF_DONTWAIT)
			ret = -EBUSY;
		else
			ret = card->ops->acq_wait(card, NULL);
		if (ret)
			goto out;
		if (mutex_lock_interruptible(&card->lock)) {
			ret = -EINTR;
			goto out;
		}
	}
	/*
	 * We need to check segment->acquiring again because we had to
	 * re-acquire the card's lock.
	 */
	if (atomic_read(&segment->acquiring) ||
	    atomic_read(&segment->transferring)) {
		ret = -EBUSY;
		goto out_unlock;
	}
	/* fill in as many events as we can */
	nr_events = min(segment->nr_events, list.n_acqs);
	if (nr_events == 0) {
		ret = -ENODATA;
		goto out_unlock;
	}
	ret = sis33_check_acq_buffers(card, list.segment, acqs, nr_events);
	if (ret)
		goto out_unlock;
	list.endtime = segment->endtime;
	atomic_set(&segment->transferring, 1);
	mutex_unlock(&card->lock);
	/* .fetch always does blocking I/O */
	if (card->ops->fetch)
		ret = card->ops->fetch(card, list.segment, list.channel, acqs, nr_events);
	else
		ret = -EINVAL;
	atomic_set(&segment->transferring, 0);

	if (ret < 0)
		goto out;

	if (copy_to_user(arg, &list, sizeof(list))) {
		ret = -EFAULT;
		goto out;
	}
	if (copy_to_user(list.acqs, acqs, size))
		ret = -EFAULT;
	goto out;

 out_unlock:
	mutex_unlock(&card->lock);
 out:
	kfree(acqs);
	return ret;
}

static int sis33_acq_start_ioctl(struct sis33_card *card, void __user *arg)
{
	struct sis33_acq_desc desc;
	struct sis33_segment *segment;
	unsigned int flags;
	int ret;

	if (copy_from_user(&desc, arg, sizeof(desc)))
		return -EFAULT;

	flags = desc.flags & (SIS33_ACQF_DONTWAIT | SIS33_ACQF_TIMEOUT);
	if (flags & SIS33_ACQF_DONTWAIT && flags & SIS33_ACQF_TIMEOUT) {
		dev_info(card->dev, "Invalid flags: DONTWAIT and TIMEOUT can't coexist.\n");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&card->lock))
		return -EINTR;

	if (desc.nr_events > card->max_nr_events) {
		dev_info(card->dev, "Invalid number of events %d. valid: 1-%d.\n",
			desc.nr_events, card->max_nr_events);
		ret = -EINVAL;
		goto out_unlock;
	}
	if (desc.segment >= card->n_segments) {
		dev_info(card->dev, "Invalid segment number %d. [0-%d] segments active\n",
			desc.segment, card->n_segments - 1);
		ret = -EINVAL;
		goto out_unlock;
	}
	if (!sis33_value_is_in_array(card->ev_lengths, card->n_ev_lengths, desc.ev_length)) {
		dev_info(card->dev, "Invalid event length %u\n", desc.ev_length);
		ret = -EINVAL;
		goto out_unlock;
	}
	if (desc.nr_events * desc.ev_length > sis33_segment_max_nr_samples(card)) {
		dev_info(card->dev, "Required number of samples per segment %d greater than the current maximum %d\n",
			desc.nr_events * desc.ev_length, sis33_segment_max_nr_samples(card));
		ret = -EINVAL;
		goto out_unlock;
	}
	segment = &card->segments[desc.segment];
	if (atomic_read(&segment->transferring) || sis33_is_acquiring(card)) {
		ret = -EBUSY;
		goto out_unlock;
	}
	if (!card->ops->conf_ev) {
		ret = -EINVAL;
		goto out_unlock;
	}
	ret = card->ops->conf_ev(card, &desc);
	if (ret)
		goto out_unlock;
	atomic_set(&segment->acquiring, 1);
	segment->nr_samp_per_ev = desc.ev_length;
	/* segment->nr_events is set asynchronously by the acq. handler */
	card->curr_segment = desc.segment;
	mutex_unlock(&card->lock);
	if (!card->ops->acq_start)
		return -EINVAL;
	card->ops->acq_start(card);
	if (!card->ops->acq_wait)
		return -EINVAL;
	if (flags & SIS33_ACQF_TIMEOUT)
		ret = card->ops->acq_wait(card, &desc.timeout);
	else if (flags & SIS33_ACQF_DONTWAIT)
		ret = 0;
	else
		ret = card->ops->acq_wait(card, NULL);
	/*
	 * No need to lock again: the driver sets segment->acquiring to 0
	 * when the acquisition finishes--it may be asynchronous.
	 */
	return ret;

 out_unlock:
	mutex_unlock(&card->lock);
	return ret;
}

static long sis33_do_ioctl(struct file *file, unsigned int cm, void __user *arg)
{
	struct sis33_card *card = file->private_data;

	switch (cm) {
	case SIS33_IOC_FETCH:
		return sis33_fetch_ioctl(card, arg);
	case SIS33_IOC_ACQUIRE:
		return sis33_acq_start_ioctl(card, arg);
	default:
		return -ENOTTY;
	}
}

static long sis33_ioctl(struct file *file, unsigned int cm, unsigned long arg)
{
	int ret;

	ret = sis33_do_ioctl(file, cm, (void __user *)arg);

	return ret;
}

static int sis33_open(struct inode *inode, struct file *file)
{
	struct sis33_card *card;

	card = container_of(inode->i_cdev, struct sis33_card, cdev);

	if (!try_module_get(card->module))
		return -ENODEV;

	file->private_data = card;

	return 0;
}

static int sis33_release(struct inode *inode, struct file *file)
{
	struct sis33_card *card = file->private_data;

	module_put(card->module);
	return 0;
}

static const struct file_operations sis33_fops = {
	.owner		= THIS_MODULE,
	.open		= sis33_open,
	.release	= sis33_release,
	.unlocked_ioctl	= sis33_ioctl,
	.compat_ioctl	= sis33_compat_ioctl,
};

static int sis33_create_cdev(struct sis33_card *card, int ndev)
{
	dev_t devno = MKDEV(MAJOR(sis33_devno), ndev);
	int error;

	cdev_init(&card->cdev, &sis33_fops);
	card->cdev.owner = THIS_MODULE;
	error = cdev_add(&card->cdev, devno, 1);
	if (error) {
		dev_err(card->pdev, "Error %d adding cdev %d\n", error, ndev);
		goto out;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	card->dev = device_create(sis33_class, card->pdev, devno, NULL, "sis33.%i", ndev);
#else
	card->dev = device_create(sis33_class, card->pdev, devno, "sis33.%i", ndev);
#endif /* 2.6.28 */

	if (IS_ERR(card->dev)) {
		error = PTR_ERR(card->dev);
		dev_err(card->pdev, "Error %d creating device %d\n", error, ndev);
		card->dev = NULL;
		goto device_create_failed;
	}

	if (!error) {
		dev_set_drvdata(card->dev, card);
		goto out;
	}

 device_create_failed:
	cdev_del(&card->cdev);
 out:
	return error;
}

static void sis33_destroy_cdev(struct sis33_card *card, int ndev)
{
	dev_t devno = MKDEV(MAJOR(sis33_devno), ndev);

	device_destroy(sis33_class, devno);
	cdev_del(&card->cdev);
}

/* this must be called with sis33_lock held */
static int __sis33_card_lock(int *idx)
{
	int new_idx;

	/* try to find a free slot if the provided index is -1 */
	if (*idx == -1) {
		for (new_idx = 0; new_idx < SIS33_MAX_DEVICES; new_idx++) {
			if (~sis33_cards_lock & 1 << new_idx) {
				*idx = new_idx;
				break;
			}
		}
		if (new_idx == SIS33_MAX_DEVICES) {
			printk(KERN_ERR PFX "idx %d: No card slots left\n", *idx);
			return -ENODEV;
		}
	} else {
		/* check if the given index is already in use */
		if (sis33_cards_lock & (1 << *idx)) {
			printk(KERN_ERR PFX "card index %d already in use\n", *idx);
			return -EBUSY;
		}
	}

	sis33_cards_lock |= 1 << *idx;
	return 0;
}

/**
 * sis33_card_create - create and initialize an sis33_card structure
 * @idx:	card index [0 ... SIS33_MAX_DEVICES-1]
 * @card_ret:	pointer to store the created instance
 * @module:	owner module
 * @pdev:	pointer to the card's struct device
 *
 * return 0 on success or negative error code on failure
 */
int sis33_card_create(int idx, struct module *module,
		struct sis33_card **card_ret, struct device *pdev)
{
	struct sis33_card *card;
	int error = 0;

	if (idx >= SIS33_MAX_DEVICES) {
		printk(KERN_ERR PFX "card index %d out of range [0..%d]\n",
			idx, SIS33_MAX_DEVICES - 1);
		return -EINVAL;
	}

	*card_ret = NULL;
	card = kzalloc(sizeof(*card), GFP_KERNEL);
	if (card == NULL)
		return -ENOMEM;

	mutex_lock(&sis33_lock);
	error = __sis33_card_lock(&idx);
	mutex_unlock(&sis33_lock);
	if (error)
		goto out;

	/* initialize the struct */
	card->number = idx;
	mutex_init(&card->lock);
	*card_ret = card;
	card->module = module;
	dev_set_drvdata(pdev, card);

 out:
	if (error)
		kfree(card);
	return error;
}
EXPORT_SYMBOL_GPL(sis33_card_create);

/**
 * sis33_card_register - register an sis33 card
 * @card:	sis33_card to be registered
 * @ops:	sis33_ops to be called for this card
 *
 * return 0 on success, negative error code on failure
 *
 * NOTE:
 * After the card is registered, it is ready to start being used normally.
 * Before user-space is notified of the creation of the device, it is
 * initialised here to the default config values.
 */
int sis33_card_register(struct sis33_card *card, struct sis33_card_ops *ops)
{
	int error;

	BUG_ON(!ops);
	BUG_ON(card->n_segments == 0);
	BUG_ON(card->n_segments_max == 0);
	BUG_ON(card->n_segments_min == 0);
	card->ops = ops;

	if (ops->config_init)
		ops->config_init(card);
	if (ops->conf_acq) {
		error = ops->conf_acq(card, &card->cfg);
		if (error)
			goto out;
	}
	if (ops->conf_channels) {
		error = ops->conf_channels(card, card->channels);
		if (error)
			goto out;
	}

	error = sis33_create_cdev(card, card->number);
	if (error)
		goto out;

	error = sis33_create_sysfs_files(card);
	if (error)
		goto create_sysfs_files_failed;

	dev_info(card->dev, "%s registered.\n", card->description);
	goto out;

 create_sysfs_files_failed:
	sis33_destroy_cdev(card, card->number);
 out:
	return error;
}
EXPORT_SYMBOL_GPL(sis33_card_register);

/* unregister and free a card */
/**
 * sis33_card_free - unregister and free an sis33 card
 * @card:	sis33 to be unregistered
 */
void sis33_card_free(struct sis33_card *card)
{
	sis33_remove_sysfs_files(card);

	/* avoid deleting unregistered devices */
	if (card->cdev.owner == THIS_MODULE)
		sis33_destroy_cdev(card, card->number);

	mutex_lock(&sis33_lock);
	sis33_cards_lock &= ~(1 << card->number);
	mutex_unlock(&sis33_lock);

	kfree(card);
}
EXPORT_SYMBOL_GPL(sis33_card_free);

static int __init sis33_sysfs_device_create(void)
{
	int error = 0;

	sis33_class = class_create(THIS_MODULE, "sis33");

	if (IS_ERR(sis33_class)) {
		printk(KERN_ERR PFX "Failed to create sis33 class\n");
		error = PTR_ERR(sis33_class);
		goto out;
	}

	error = alloc_chrdev_region(&sis33_devno, 0, SIS33_MAX_DEVICES, "sis33");
	if (error) {
		printk(KERN_ERR PFX "Failed to allocate chrdev region\n");
		goto alloc_chrdev_region_failed;
	}

	if (!error)
		goto out;

 alloc_chrdev_region_failed:
	class_destroy(sis33_class);
 out:
	return error;
}

static void sis33_sysfs_device_remove(void)
{
	dev_t devno = MKDEV(MAJOR(sis33_devno), 0);

	class_destroy(sis33_class);
	unregister_chrdev_region(devno, SIS33_MAX_DEVICES);
}

static int __init sis33_init(void)
{
	int error;

	/*
	 * Most (if not all) sis modules are always accessed across DMA,
	 * due to the (braindead) way they're designed (see the comment
	 * at the top of dma.c). Since a DMA access may sleep, we cannot
	 * possibly access the device in interrupt context.
	 * To work around this, drivers can send interrupt handlers to a
	 * slow workqueue, and treat them there in process context where
	 * sleeping is allowed.
	 * This is far from ideal, because it adds unbounded latency and
	 * makes locking more complex, but at least is better than polling.
	 */
	sis33_workqueue = create_singlethread_workqueue(DRV_NAME);
	if (sis33_workqueue == NULL)
		return -ENOMEM;
	pr_info(PFX "kthread created. Set its rt-priority with 'chrt -f -p [prio] [pid]'\n");

	error = sis33_sysfs_device_create();
	if (error)
		destroy_workqueue(sis33_workqueue);
	return error;
}

static void __exit sis33_exit(void)
{
	sis33_sysfs_device_remove();
	destroy_workqueue(sis33_workqueue);
}

module_init(sis33_init)
module_exit(sis33_exit)

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sis33 core");
MODULE_VERSION(DRV_MODULE_VERSION);
