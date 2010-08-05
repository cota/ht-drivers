/*
 * sis33_compat_ioctl.c
 * Translates ioctl arguments coming from 32-bits user-space.
 *
 * Copyright (c) 2009-2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <linux/compat.h>
#include <linux/fs.h>

#include <asm/uaccess.h>

#include "sis33core.h"

#ifdef CONFIG_COMPAT

struct sis33_compat_acq_desc {
	u32			segment;
	u32			nr_events;
	u32			ev_length;
	u32			flags;
	struct compat_timespec	timeout;
	u32			unused[4];
};

struct sis33_compat_acq {
	compat_uptr_t		data;
	u32			size;
	u32			nr_samples;
	compat_u64		prevticks;
	u32			first_samp;
	u32			be;
	u32			unused[4];
};

struct sis33_compat_acq_list {
	u32			segment;
	u32			channel;
	u32			n_acqs;
	compat_uptr_t		acqs;
	u32			flags;
	struct compat_timespec	timeout;
	struct compat_timeval	endtime;
	u32			unused[4];
};

struct sis33_combined_acq_list {
	struct sis33_acq_list	list;
	struct sis33_acq	acqs[1];
};

#define SIS33_IOC32_FETCH	_IOW ('3', 0, struct sis33_compat_acq_list)
#define SIS33_IOC32_ACQUIRE	_IOW ('3', 1, struct sis33_compat_acq_desc)

static int sis33_translated_ioctl(struct file *file, unsigned int cm, unsigned long arg)
{
	if (file->f_op && file->f_op->unlocked_ioctl)
		return file->f_op->unlocked_ioctl(file, cm, arg);
	return -ENOTTY;
}

static int
sis33_get_compat_acq_desc(struct sis33_acq_desc __user *desc, struct sis33_compat_acq_desc __user *compat_desc)
{
	struct timespec ts;
	u32 uint;
	int err;

	if (!access_ok(VERIFY_READ, compat_desc, sizeof(*compat_desc)) ||
	    !access_ok(VERIFY_WRITE, desc, sizeof(*desc)))
		return -EFAULT;
	err = 0;
	err |= __get_user(uint, &compat_desc->segment);
	err |= __put_user(uint, &desc->segment);
	err |= __get_user(uint, &compat_desc->nr_events);
	err |= __put_user(uint, &desc->nr_events);
	err |= __get_user(uint, &compat_desc->ev_length);
	err |= __put_user(uint, &desc->ev_length);
	err |= __get_user(uint, &compat_desc->flags);
	err |= __put_user(uint, &desc->flags);
	err |= get_compat_timespec(&ts, &compat_desc->timeout);
	err |= copy_to_user(&desc->timeout, &ts, sizeof(ts));
	if (err)
		return -EFAULT;
	return 0;
}

static int sis33_compat_acquire_ioctl(struct file *file, unsigned long arg)
{
	struct sis33_compat_acq_desc __user *compat_desc;
	struct sis33_acq_desc __user *desc;
	int ret;

	compat_desc = compat_ptr(arg);
	desc = compat_alloc_user_space(sizeof(*desc));

	ret = sis33_get_compat_acq_desc(desc, compat_desc);
	if (ret)
		return ret;

	return sis33_translated_ioctl(file, SIS33_IOC_FETCH, (unsigned long)desc);
}

static int sis33_get_compat_acq(struct sis33_acq __user *acq, struct sis33_compat_acq __user *compat_acq)
{
	int err;
	compat_uptr_t uptr;
	u64 llint;
	u32 uint;

	if (!access_ok(VERIFY_READ, compat_acq, sizeof(*compat_acq)) ||
	    !access_ok(VERIFY_WRITE, acq, sizeof(*acq)))
		return -EFAULT;
	err = 0;
	err |= __get_user(uptr, &compat_acq->data);
	err |= __put_user(compat_ptr(uptr), &acq->data);
	err |= __get_user(uint, &compat_acq->size);
	err |= __put_user(uint, &acq->size);
	err |= __get_user(uint, &compat_acq->nr_samples);
	err |= __put_user(uint, &acq->nr_samples);
	err |= __get_user(llint, &compat_acq->prevticks);
	err |= __put_user(llint, &acq->prevticks);
	err |= __get_user(uint, &compat_acq->first_samp);
	err |= __put_user(uint, &acq->first_samp);
	err |= __get_user(uint, &compat_acq->be);
	err |= __put_user(uint, &acq->be);
	return err ? -EFAULT : 0;
}

static int sis33_put_compat_acq(struct sis33_compat_acq __user *acq32, struct sis33_acq __user *acq)
{
	u64 llint;
	u32 uint;
	int err;

	if (!access_ok(VERIFY_READ, acq, sizeof(*acq)) ||
	    !access_ok(VERIFY_WRITE, acq32, sizeof(*acq32)))
		return -EFAULT;
	err = 0;
	/* leave acq->data untouched */
	err |= __get_user(uint, &acq->size);
	err |= __put_user(uint, &acq32->size);
	err |= __get_user(uint, &acq->nr_samples);
	err |= __put_user(uint, &acq32->nr_samples);
	err |= __get_user(llint, &acq->prevticks);
	err |= __put_user(llint, &acq32->prevticks);
	err |= __get_user(uint, &acq->first_samp);
	err |= __put_user(uint, &acq32->first_samp);
	err |= __get_user(uint, &acq->be);
	err |= __put_user(uint, &acq32->be);
	return err ? -EFAULT : 0;
}

static int sis33_get_compat_acq_list(struct sis33_acq_list __user *list,
				struct sis33_compat_acq_list __user *compat_list,
				struct sis33_acq *acqs)
{
	struct sis33_compat_acq __user *compat_acqs;
	struct timeval ktime;
	struct timespec ts;
	compat_uptr_t uptr;
	u32 n_acqs;
	u32 uint;
	int err;
	int i;

	if (!access_ok(VERIFY_READ, compat_list, sizeof(*compat_list)) ||
	    !access_ok(VERIFY_WRITE, list, sizeof(*list)))
		return -EFAULT;
	err = 0;
	err |= __get_user(n_acqs, &compat_list->n_acqs);
	err |= __put_user(n_acqs, &list->n_acqs);
	err |= __get_user(uint, &compat_list->segment);
	err |= __put_user(uint, &list->segment);
	err |= __get_user(uint, &compat_list->channel);
	err |= __put_user(uint, &list->channel);
	err |= __get_user(uint, &compat_list->flags);
	err |= __put_user(uint, &list->flags);
	err |= __put_user(acqs, &list->acqs);
	err |= get_compat_timespec(&ts, &compat_list->timeout);
	err |= copy_to_user(&list->timeout, &ts, sizeof(ts));
	err |= __get_user(ktime.tv_sec, &compat_list->endtime.tv_sec);
	err |= __put_user(ktime.tv_sec, &list->endtime.tv_sec);
	err |= __get_user(ktime.tv_usec, &compat_list->endtime.tv_usec);
	err |= __put_user(ktime.tv_usec, &list->endtime.tv_usec);
	err |= __get_user(uptr, &compat_list->acqs);
	compat_acqs = compat_ptr(uptr);
	if (err)
		return -EFAULT;

	for (i = 0; i < n_acqs; i++) {
		err = sis33_get_compat_acq(&list->acqs[i], &compat_acqs[i]);
		if (err)
			return err;
	}
	return 0;
}

static int sis33_put_compat_acq_list(struct sis33_compat_acq_list __user *compat_list,
				struct sis33_acq_list __user *list)
{
	struct sis33_compat_acq __user *compat_acqs;
	struct timeval ktime;
	struct timespec ts;
	compat_uptr_t uptr;
	u32 n_acqs;
	u32 uint;
	int err;
	int i;

	if (!access_ok(VERIFY_READ, list, sizeof(*list)) ||
	    !access_ok(VERIFY_WRITE, compat_list, sizeof(*compat_list)))
		return -EFAULT;

	err = 0;
	err |= __get_user(uint, &list->segment);
	err |= __put_user(uint, &compat_list->segment);
	err |= __get_user(uint, &list->channel);
	err |= __put_user(uint, &compat_list->channel);
	err |= __get_user(uint, &list->n_acqs);
	err |= __put_user(uint, &compat_list->n_acqs);
	n_acqs = uint;
	err |= __get_user(uint, &list->flags);
	err |= __put_user(uint, &compat_list->flags);
	err |= copy_from_user(&ts, &list->timeout, sizeof(ts));
	err |= put_compat_timespec(&ts, &compat_list->timeout);
	err |= __get_user(ktime.tv_sec, &list->endtime.tv_sec);
	err |= __put_user(ktime.tv_sec, &compat_list->endtime.tv_sec);
	err |= __get_user(ktime.tv_usec, &list->endtime.tv_usec);
	err |= __put_user(ktime.tv_usec, &compat_list->endtime.tv_usec);
	err |= __get_user(uptr, &compat_list->acqs);
	compat_acqs = compat_ptr(uptr);
	if (err)
		return -EFAULT;

	for (i = 0; i < n_acqs; i++) {
		err = sis33_put_compat_acq(&compat_acqs[i], &list->acqs[i]);
		if (err)
			return err;
	}
	return 0;
}

static int sis33_compat_fetch_ioctl(struct file *file, unsigned long arg)
{
	struct sis33_combined_acq_list __user *combined;
	struct sis33_compat_acq_list __user *compat_list;
	u32 n_acqs;
	int err;

	compat_list = compat_ptr(arg);
	if (!access_ok(VERIFY_READ, compat_list, sizeof(*compat_list)))
		return -EFAULT;
	if (__get_user(n_acqs, &compat_list->n_acqs))
		return -EFAULT;
	combined = compat_alloc_user_space(offsetof(struct sis33_combined_acq_list, acqs[n_acqs]));
	err = sis33_get_compat_acq_list(&combined->list, compat_list, &combined->acqs[0]);
	if (err)
		return err;

	err = sis33_translated_ioctl(file, SIS33_IOC_FETCH, (unsigned long)&combined->list);
	if (err)
		return err;

	return sis33_put_compat_acq_list(compat_list, &combined->list);
}

long sis33_compat_ioctl(struct file *file, unsigned int cm, unsigned long arg)
{
	switch (cm) {
	case SIS33_IOC32_ACQUIRE:
		return sis33_compat_acquire_ioctl(file, arg);
	case SIS33_IOC32_FETCH:
		return sis33_compat_fetch_ioctl(file, arg);
	default:
		return -ENOTTY;
	}
}

#endif /* CONFIG_COMPAT */
