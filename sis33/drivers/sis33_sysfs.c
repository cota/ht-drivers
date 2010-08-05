/*
 * sis33_sysfs.c
 * sysfs management for sis33
 *
 * Copyright (c) 2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <linux/device.h>
#include <linux/ctype.h>

#include "sis33core.h"
#include "sis33core_internal.h"

#ifdef CONFIG_SYSFS

/*
 * Note that the read-only parameters' handlers do not take the card's lock.
 * This is because the information they export is not supposed to change
 * once the device has been loaded.
 */

static ssize_t
sis33_show_description(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	if (strlen(card->description))
		return snprintf(buf, PAGE_SIZE, "%s\n", card->description);
	else
		return snprintf(buf, PAGE_SIZE, "none\n");
}

static ssize_t
sis33_show_n_bits(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", card->n_bits);
}

static ssize_t
sis33_show_n_segments_max(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", card->n_segments_max);
}

static ssize_t
sis33_show_n_segments_min(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", card->n_segments_min);
}

static ssize_t
sis33_show_n_events_max(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", card->max_nr_events);
}

static ssize_t
sis33_show_available_clkfreqs(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	for (i = 0; i < card->n_freqs; i++) {
		count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
				"%u ", card->freqs[i]);
	}
	count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\n");
	return count;
}

static ssize_t
sis33_show_available_ev_lengths(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	for (i = 0; i < card->n_ev_lengths; i++) {
		count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
				"%u ", card->ev_lengths[i]);
	}
	count += snprintf(buf + count, max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\n");
	return count;
}

static ssize_t
sis33_show_ev_tstamp_support(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", card->ev_tstamp_support);
}

static ssize_t
sis33_show_ev_tstamp_max_ticks_log2(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", card->ev_tstamp_max_ticks_log2);
}

static ssize_t
sis33_show_ev_tstamp_divider_max(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", card->ev_tstamp_divider_max);
}

static DEVICE_ATTR(description, S_IRUGO, sis33_show_description, NULL);
static DEVICE_ATTR(n_bits, S_IRUGO, sis33_show_n_bits, NULL);
static DEVICE_ATTR(n_segments_max, S_IRUGO, sis33_show_n_segments_max, NULL);
static DEVICE_ATTR(n_segments_min, S_IRUGO, sis33_show_n_segments_min, NULL);
static DEVICE_ATTR(n_events_max, S_IRUGO, sis33_show_n_events_max, NULL);
static DEVICE_ATTR(available_clock_frequencies, S_IRUGO, sis33_show_available_clkfreqs, NULL);
static DEVICE_ATTR(available_event_lengths, S_IRUGO, sis33_show_available_ev_lengths, NULL);
static DEVICE_ATTR(event_timestamping_support, S_IRUGO, sis33_show_ev_tstamp_support, NULL);
static DEVICE_ATTR(event_timestamping_max_ticks_log2, S_IRUGO, sis33_show_ev_tstamp_max_ticks_log2, NULL);
static DEVICE_ATTR(event_timestamping_divider_max, S_IRUGO, sis33_show_ev_tstamp_divider_max, NULL);

static int sis33_getval(const char *buf, size_t count, int *val)
{
	long value;
	char *endp;

	if (!isdigit(buf[0]))
		return -EINVAL;
	value = simple_strtol(buf, &endp, 0);
	/* add one to endp to allow a trailing '\n' */
	if (endp + 1 < buf + count)
		return -EINVAL;
	*val = value;
	return 0;
}

static int sis33_getuval(const char *buf, size_t count, unsigned int *val)
{
	unsigned long value;
	char *endp;

	if (!isdigit(buf[0]))
		return -EINVAL;
	value = simple_strtoul(buf, &endp, 0);
	/* add one to endp to allow a trailing '\n' */
	if (endp + 1 < buf + count)
		return -EINVAL;
	*val = value;
	return 0;
}

static ssize_t
sis33_show_int(struct sis33_card *card, struct device_attribute *attr, char *buf, int *valp)
{
	ssize_t ret;

	if (mutex_lock_interruptible(&card->lock))
		return -EINTR;
	ret = snprintf(buf, PAGE_SIZE, "%d\n", *valp);
	mutex_unlock(&card->lock);
	return ret;
}

static ssize_t
sis33_show_uint(struct sis33_card *card, struct device_attribute *attr, char *buf, unsigned int *valp)
{
	ssize_t ret;

	if (mutex_lock_interruptible(&card->lock))
		return -EINTR;
	ret = snprintf(buf, PAGE_SIZE, "%u\n", *valp);
	mutex_unlock(&card->lock);
	return ret;
}

static ssize_t
__sis33_conf_store_int(struct sis33_card *card, struct device_attribute *attr, const char *buf,
		       size_t count, struct sis33_cfg *cfg, int *valp, int bool)
{
	int val;
	int ret;

	ret = sis33_getval(buf, count, &val);
	if (ret)
		return ret;
	if (mutex_lock_interruptible(&card->lock))
		return -EINTR;
	if (sis33_is_acquiring(card)) {
		ret = -EBUSY;
		goto out_unlock;
	}
	memcpy(cfg, &card->cfg, sizeof(*cfg));
	if (bool)
		*valp = !!val;
	else
		*valp = val;
	if (!card->ops->conf_acq) {
		ret = -EINVAL;
		goto out_unlock;
	}
	ret = card->ops->conf_acq(card, cfg);
	if (ret)
		goto out_unlock;
	memcpy(&card->cfg, cfg, sizeof(*cfg));
	ret = count;
 out_unlock:
	mutex_unlock(&card->lock);
	return ret;
}

static ssize_t
sis33_conf_store_bool(struct sis33_card *card, struct device_attribute *attr, const char *buf,
		      size_t count, struct sis33_cfg *cfg, int *valp)
{
	return __sis33_conf_store_int(card, attr, buf, count, cfg, valp, 1);
}

static ssize_t
sis33_conf_store_int(struct sis33_card *card, struct device_attribute *attr, const char *buf,
		     size_t count, struct sis33_cfg *cfg, int *valp)
{
	return __sis33_conf_store_int(card, attr, buf, count, cfg, valp, 0);
}

static ssize_t
sis33_show_start_auto(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return sis33_show_int(card, attr, buf, &card->cfg.start_auto);
}

static ssize_t
sis33_store_start_auto(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	struct sis33_cfg cfg;

	return sis33_conf_store_bool(card, attr, buf, count, &cfg, &cfg.start_auto);
}

static ssize_t
sis33_show_stop_auto(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return sis33_show_int(card, attr, buf, &card->cfg.stop_auto);
}

static ssize_t
sis33_store_stop_auto(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	struct sis33_cfg cfg;

	return sis33_conf_store_bool(card, attr, buf, count, &cfg, &cfg.stop_auto);
}

static ssize_t
sis33_show_trigger_ext(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return sis33_show_int(card, attr, buf, &card->cfg.trigger_ext_en);
}

static ssize_t
sis33_store_trigger_ext(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	struct sis33_cfg cfg;

	return sis33_conf_store_bool(card, attr, buf, count, &cfg, &cfg.trigger_ext_en);
}

/*
 * Purposedly we don't take the device's lock here, to make this as close to an
 * external trigger as possible.
 */
static ssize_t
sis33_store_trigger(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	int trigval;
	int ret;

	ret = sis33_getval(buf, count, &trigval);
	if (ret)
		return ret;
	if (!card->ops->trigger)
		return -EINVAL;
	ret = card->ops->trigger(card, trigval);
	if (ret)
		return ret;
	return count;
}
/*
 * This may be used when the module misbehaves: for this reason we don't take
 * the device's lock.
 */
static ssize_t
sis33_store_acq_cancel(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	int ret;

	if (!card->ops->acq_cancel)
		return -EINVAL;
	ret = card->ops->acq_cancel(card);
	if (ret)
		return ret;
	return count;
}

static ssize_t
sis33_show_start_delay(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return sis33_show_int(card, attr, buf, &card->cfg.start_delay);
}

static ssize_t
sis33_store_start_delay(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	struct sis33_cfg cfg;
	int val;
	int ret;

	ret = sis33_getval(buf, count, &val);
	if (ret)
		return ret;
	if (val < 0 || val > card->max_delay)
		return -EINVAL;
	return sis33_conf_store_int(card, attr, buf, count, &cfg, &cfg.start_delay);
}

static ssize_t
sis33_show_stop_delay(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return sis33_show_int(card, attr, buf, &card->cfg.stop_delay);
}

static ssize_t
sis33_store_stop_delay(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	struct sis33_cfg cfg;
	int val;
	int ret;

	ret = sis33_getval(buf, count, &val);
	if (ret)
		return ret;
	if (val < 0 || val > card->max_delay)
		return -EINVAL;
	return sis33_conf_store_int(card, attr, buf, count, &cfg, &cfg.stop_delay);
}

static ssize_t
sis33_show_ts_div(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return sis33_show_int(card, attr, buf, &card->cfg.ev_tstamp_divider);
}

static ssize_t
sis33_store_ts_div(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	struct sis33_cfg cfg;
	int val;
	int ret;

	ret = sis33_getval(buf, count, &val);
	if (ret)
		return ret;
	if (val <= 0 || val > card->ev_tstamp_divider_max)
		return -EINVAL;
	return sis33_conf_store_int(card, attr, buf, count, &cfg, &cfg.ev_tstamp_divider);
}

static ssize_t
sis33_show_n_segments(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return sis33_show_int(card, attr, buf, &card->n_segments);
}

static ssize_t
sis33_store_n_segments(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	unsigned long n;
	int segments;
	int ret;

	ret = sis33_getval(buf, count, &segments);
	if (ret)
		return ret;
	if (segments < card->n_segments_min)
		return -EINVAL;

	if (mutex_lock_interruptible(&card->lock))
		return -EINTR;
	if (sis33_is_acquiring(card) || sis33_is_transferring(card)) {
		ret = -EBUSY;
		goto out_unlock;
	}
	n = __roundup_pow_of_two(segments);
	if (n == 0)
		n = 1;
	else if (n > card->n_segments_max)
		n = card->n_segments_max;
	card->n_segments = n;
	ret = count;
 out_unlock:
	mutex_unlock(&card->lock);
	return ret;
}

static ssize_t
sis33_show_clk_src(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return sis33_show_int(card, attr, buf, &card->cfg.clksrc);
}

static ssize_t
sis33_store_clk_src(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	struct sis33_cfg cfg;
	int clksrc;
	int ret;

	ret = sis33_getval(buf, count, &clksrc);
	if (ret)
		return ret;
	if (clksrc != SIS33_CLKSRC_INTERNAL && clksrc != SIS33_CLKSRC_EXTERNAL) {
		dev_info(card->dev, "Invalid clock source %d\n", clksrc);
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&card->lock))
		return -EINTR;
	if (sis33_is_acquiring(card)) {
		ret = -EBUSY;
		goto out_unlock;
	}
	memcpy(&cfg, &card->cfg, sizeof(cfg));
	cfg.clksrc = clksrc;
	if (!card->ops->conf_acq) {
		ret = -EINVAL;
		goto out_unlock;
	}
	ret = card->ops->conf_acq(card, &cfg);
	if (ret)
		goto out_unlock;
	memcpy(&card->cfg, &cfg, sizeof(cfg));
	ret = count;
 out_unlock:
	mutex_unlock(&card->lock);
	return ret;
}

static ssize_t
sis33_show_clk_freq(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct sis33_card *card = dev_get_drvdata(pdev);

	return sis33_show_uint(card, attr, buf, &card->cfg.clkfreq);
}

static ssize_t
sis33_store_clk_freq(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sis33_card *card = dev_get_drvdata(pdev);
	struct sis33_cfg cfg;
	unsigned int clkfreq;
	int ret;

	ret = sis33_getuval(buf, count, &clkfreq);
	if (ret)
		return ret;

	if (mutex_lock_interruptible(&card->lock))
		return -EINTR;
	if (sis33_is_acquiring(card)) {
		ret = -EBUSY;
		goto out_unlock;
	}
	if (!sis33_value_is_in_array(card->freqs, card->n_freqs, clkfreq)) {
		ret = -EINVAL;
		goto out_unlock;
	}
	memcpy(&cfg, &card->cfg, sizeof(cfg));
	cfg.clkfreq = clkfreq;
	if (!card->ops->conf_acq) {
		ret = -EINVAL;
		goto out_unlock;
	}
	ret = card->ops->conf_acq(card, &cfg);
	if (ret)
		goto out_unlock;
	memcpy(&card->cfg, &cfg, sizeof(cfg));
	ret = count;
 out_unlock:
	mutex_unlock(&card->lock);
	return ret;
}

static DEVICE_ATTR(start_auto, S_IWUSR | S_IRUGO, sis33_show_start_auto, sis33_store_start_auto);
static DEVICE_ATTR(stop_auto, S_IWUSR | S_IRUGO, sis33_show_stop_auto, sis33_store_stop_auto);
static DEVICE_ATTR(trigger_external_enable, S_IWUSR | S_IRUGO, sis33_show_trigger_ext, sis33_store_trigger_ext);
static DEVICE_ATTR(trigger, S_IWUSR, NULL, sis33_store_trigger);
static DEVICE_ATTR(start_delay, S_IWUSR | S_IRUGO, sis33_show_start_delay, sis33_store_start_delay);
static DEVICE_ATTR(stop_delay, S_IWUSR | S_IRUGO, sis33_show_stop_delay, sis33_store_stop_delay);
static DEVICE_ATTR(event_timestamping_divider, S_IWUSR | S_IRUGO, sis33_show_ts_div, sis33_store_ts_div);
static DEVICE_ATTR(n_segments, S_IWUSR | S_IRUGO, sis33_show_n_segments, sis33_store_n_segments);
static DEVICE_ATTR(acq_cancel, S_IWUSR, NULL, sis33_store_acq_cancel);
static DEVICE_ATTR(clock_source, S_IWUSR | S_IRUGO, sis33_show_clk_src, sis33_store_clk_src);
static DEVICE_ATTR(clock_frequency, S_IWUSR | S_IRUGO, sis33_show_clk_freq, sis33_store_clk_freq);

static struct attribute *sis33_attrs[] = {
	&dev_attr_description.attr,
	&dev_attr_n_bits.attr,
	&dev_attr_n_segments_max.attr,
	&dev_attr_n_segments_min.attr,
	&dev_attr_n_events_max.attr,
	&dev_attr_available_clock_frequencies.attr,
	&dev_attr_available_event_lengths.attr,
	&dev_attr_event_timestamping_support.attr,
	&dev_attr_event_timestamping_max_ticks_log2.attr,
	&dev_attr_event_timestamping_divider_max.attr,
	&dev_attr_start_auto.attr,
	&dev_attr_stop_auto.attr,
	&dev_attr_trigger_external_enable.attr,
	&dev_attr_trigger.attr,
	&dev_attr_start_delay.attr,
	&dev_attr_stop_delay.attr,
	&dev_attr_event_timestamping_divider.attr,
	&dev_attr_n_segments.attr,
	&dev_attr_acq_cancel.attr,
	&dev_attr_clock_source.attr,
	&dev_attr_clock_frequency.attr,
	NULL,
};

static struct attribute_group sis33_attr_group = {
	.attrs = sis33_attrs,
};

static struct attribute attr_offset = {
	.name = "offset",
	.mode = S_IWUSR | S_IRUGO,
};

static struct attribute *chan_attrs[] = {
	&attr_offset,
	NULL
};

#define to_sis33_channel(x)	container_of(x, struct sis33_channel, kobj)
#define to_sis33_card(x)	dev_get_drvdata(container_of((x)->parent->parent, struct device, kobj))

static ssize_t chan_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct sis33_channel *channel = to_sis33_channel(kobj);
	struct sis33_card *card = to_sis33_card(kobj);
	ssize_t ret;

	if (mutex_lock_interruptible(&card->lock))
		return -EINTR;
	if (strncmp(attr->name, "offset", 6) == 0)
		ret = snprintf(buf, PAGE_SIZE, "0x%x\n", channel->offset);
	else
		ret = -EIO;
	mutex_unlock(&card->lock);
	return ret;
}

static ssize_t
__chan_offset_store(struct sis33_card *card, struct sis33_channel *channel, const char *buf, size_t count)
{
	unsigned int offset_orig;
	unsigned int offset;
	ssize_t ret;

	ret = sis33_getuval(buf, count, &offset);
	if (ret)
		return ret;
	if (sis33_is_acquiring(card))
		return -EBUSY;
	offset_orig = channel->offset;
	channel->offset = offset & 0xffff;
	if (!card->ops->conf_channels)
		return -EINVAL;
	ret = card->ops->conf_channels(card, card->channels);
	if (ret) {
		channel->offset = offset_orig;
		return ret;
	}
	return count;
}

static ssize_t chan_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	struct sis33_channel *channel = to_sis33_channel(kobj);
	struct sis33_card *card = to_sis33_card(kobj);
	ssize_t ret;

	if (mutex_lock_interruptible(&card->lock))
		return -EINTR;
	if (strncmp(attr->name, "offset", 6) == 0)
		ret = __chan_offset_store(card, channel, buf, count);
	else
		ret = -EIO;
	mutex_unlock(&card->lock);
	return ret;
}

/* nothing to free here, the channel is freed elsewhere */
static void chan_attr_release(struct kobject *kobj)
{
}

static struct sysfs_ops chan_attr_ops = {
	.show	= chan_attr_show,
	.store	= chan_attr_store,
};

static struct kobj_type chan_attr_type = {
	.release	= chan_attr_release,
	.sysfs_ops	= &chan_attr_ops,
	.default_attrs	= chan_attrs,
};

static struct kobj_type chan_attr_type_no_attrs = {
	.release	= chan_attr_release,
	.sysfs_ops	= &chan_attr_ops,
};

static void chan_dir_release(struct kobject *kobj)
{
}

static struct kobj_type chan_dir_type = {
	.release	= chan_dir_release,
};

static int sis33_dev_add_attributes(struct sis33_card *card)
{
	struct sis33_channel *channel;
	int ret;
	int i;

	memset(&card->channels_dir, 0, sizeof(struct kobject));
	kobject_set_name(&card->channels_dir, "channels");
	card->channels_dir.parent	= &card->dev->kobj;
	card->channels_dir.ktype	= &chan_dir_type;
	ret = kobject_register(&card->channels_dir);
	if (ret)
		return ret;

	if (card->n_channels == 0 || card->channels == NULL)
		return 0;

	for (i = 0; i < card->n_channels; i++) {
		channel = &card->channels[i];
		memset(&channel->kobj, 0, sizeof(struct kobject));
		kobject_init(&channel->kobj);
		kobject_set_name(&channel->kobj, "channel%d", i);
		channel->kobj.parent	= &card->channels_dir;
		if (channel->show_attrs)
			channel->kobj.ktype = &chan_attr_type;
		else
			channel->kobj.ktype = &chan_attr_type_no_attrs;
		ret = kobject_add(&channel->kobj);
		if (ret)
			goto error;
	}
	return 0;
 error:
	for (i--; i >= 0; i--) {
		channel = &card->channels[i];
		kobject_unregister(&channel->kobj);
	}
	kobject_unregister(&card->channels_dir);
	return ret;
}

static void sis33_dev_del_attributes(struct sis33_card *card)
{
	struct sis33_channel *channel;
	int i;

	for (i = 0; i < card->n_channels; i++) {
		channel = &card->channels[i];
		kobject_unregister(&channel->kobj);
	}
	kobject_unregister(&card->channels_dir);
}

int sis33_create_sysfs_files(struct sis33_card *card)
{
	int error;

	error = sysfs_create_group(&card->dev->kobj, &sis33_attr_group);
	if (error)
		return error;
	error = sis33_dev_add_attributes(card);
	if (error)
		goto add_attributes_failed;
	card->sysfs_files_created = 1;
	return 0;

 add_attributes_failed:
	sysfs_remove_group(&card->dev->kobj, &sis33_attr_group);
	return error;
}

void sis33_remove_sysfs_files(struct sis33_card *card)
{
	if (!card->sysfs_files_created)
		return;

	sis33_dev_del_attributes(card);
	sysfs_remove_group(&card->dev->kobj, &sis33_attr_group);
	card->sysfs_files_created = 0;
}

#else

int sis33_create_sysfs_files(struct sis33_card *card)
{
	return 0;
}

void sis33_remove_sysfs_files(struct sis33_card *card)
{
}

#endif /* CONFIG_SYSFS */
