/**
 * @file lib/sis33.c
 *
 * @brief SIS33 library
 *
 * Copyright (c) 2010 CERN
 * @author Emilio G. Cota <cota@braap.org>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <libsis33.h>
#include <sis33acq.h>
#include <sis33dev.h>
#include "libinternal.h"

#define LIBSIS33_VERSION	"1.0"

/**
 * @brief libsis33 version string
 */
const char *libsis33_version = LIBSIS33_VERSION;

int __sis33_init;

static void __sis33_initialize(void)
{
	char *value;

	__sis33_init = 1;
	value = getenv("LIBSIS33_LOGLEVEL");
	if (value) {
		__sis33_loglevel = strtol(value, NULL, 0);
		LIBSIS33_DEBUG(3, "Setting loglevel to %d\n", __sis33_loglevel);
	}
}

/**
 * @brief Open an SIS33 device handle
 * @param index	- index number of the device to open
 *
 * @return Pointer to the opened file handle on success, NULL on failure
 */
sis33_t *sis33_open(int index)
{
	sis33_t *dev;
	char filename[SIS33_PATH_MAX];
	char devname[32];

	if (!__sis33_init)
		__sis33_initialize();
	LIBSIS33_DEBUG(4, "opening device %d\n", index);

	if (!sis33dev_device_exists(index)) {
		__sis33_libc_error(__func__);
		__sis33_lib_error(LIBSIS33_ENODEV);
		return NULL;
	}

	dev = malloc(sizeof(*dev));
	if (dev == NULL) {
		__sis33_libc_error(__func__);
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));

	dev->index = index;
	if (sis33dev_get_devname(index, devname, sizeof(devname)) < 0)
		goto out_free;
	snprintf(filename, sizeof(filename), "/dev/%s", devname);
	filename[sizeof(filename) - 1] = '\0';

	if (sis33dev_get_attr_int(dev->index, "n_bits", &dev->n_bits) < 0)
		goto out_free;

	if (sis33dev_get_attr_int(dev->index, "event_timestamping_support", &dev->ev_tstamp_supported) < 0)
		goto out_free;

	dev->n_clkfreqs = sis33dev_get_nr_items(dev->index, "available_clock_frequencies");
	if (dev->n_clkfreqs <= 0)
		goto out_free;

	dev->clkfreqs = calloc(dev->n_clkfreqs, sizeof(*dev->clkfreqs));
	if (dev->clkfreqs == NULL)
		goto out_free;

	if (sis33dev_get_items_uint(dev->index, "available_clock_frequencies",
					dev->clkfreqs, dev->n_clkfreqs))
		goto out_free;

	dev->n_ev_lengths = sis33dev_get_nr_items(dev->index, "available_event_lengths");
	if (dev->n_ev_lengths <= 0)
		goto out_free;

	dev->ev_lengths = calloc(dev->n_ev_lengths, sizeof(*dev->ev_lengths));
	if (dev->ev_lengths == NULL)
		goto out_free;

	if (sis33dev_get_items_uint(dev->index, "available_event_lengths",
					dev->ev_lengths, dev->n_ev_lengths))
		goto out_free;

	dev->fd = open(filename, O_RDWR, 0);
	if (dev->fd < 0)
		goto out_free;
	LIBSIS33_DEBUG(4, "opened device %d on handle %p, fd %d\n", index, dev, dev->fd);
	return dev;

out_free:
	if (dev->ev_lengths)
		free(dev->ev_lengths);
	if (dev->clkfreqs)
		free(dev->clkfreqs);
	if (dev)
		free(dev);
	__sis33_libc_error(__func__);
	return NULL;
}

/**
 * @brief Close an SIS33 handle
 * @param device	- SIS33 device handle
 *
 * @return 0 on success, -1 on failure
 */
int sis33_close(sis33_t *device)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = close(device->fd);
	if (ret < 0)
		__sis33_libc_error(__func__);
	free(device->ev_lengths);
	free(device->clkfreqs);
	free(device);

	return ret;
}

/**
 * @brief Set the clock source of the device
 * @param device	- SIS33 device handle
 * @param clksrc	- desired clock source
 *
 * @return 0 on success, -1 on failure
 */
int sis33_set_clock_source(sis33_t *device, enum sis33_clksrc clksrc)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p clksrc %d\n", device, clksrc);
	ret = sis33dev_set_attr_int(device->index, "clock_source", clksrc);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Get the clock source of the device
 * @param device	- SIS33 device handle
 * @param clksrc	- retrieved clock source
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_clock_source(sis33_t *device, enum sis33_clksrc *clksrc)
{
	int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_int(device->index, "clock_source", &val);
	if (ret) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*clksrc = val;
	return 0;
}

/**
 * @brief Get number of clock frequencies
 * @param device	- SIS33 device handle
 *
 * @return number of clock frequencies of the device
 */
int sis33_get_nr_clock_frequencies(sis33_t *device)
{
	LIBSIS33_DEBUG(4, "handle %p\n", device);
	return device->n_clkfreqs;
}

/**
 * @brief Get the available clock frequencies
 * @param device	- SIS33 device handle
 * @param freqs		- array of clock frequencies
 * @param n		- max. number of items in the array
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_available_clock_frequencies(sis33_t *device, unsigned int *freqs, int n)
{
	LIBSIS33_DEBUG(4, "handle %p\n", device);
	if (n != device->n_clkfreqs) {
		__sis33_param_error(LIBSIS33_EINVAL);
		return -1;
	}
	memcpy(freqs, device->clkfreqs, n * sizeof(*device->clkfreqs));
	return 0;
}

/**
 * @brief Round a clock frequency to a valid value
 * @param device	- SIS33 device handle
 * @param clkfreq	- clock frequency to be rounded
 * @param round		- rounding direction
 *
 * @return rounded frequency value
 */
unsigned int
sis33_round_clock_frequency(sis33_t *device, unsigned int clkfreq, enum sis33_round round)
{
	LIBSIS33_DEBUG(4, "handle %p clkfreq %u round %d\n", device, clkfreq, round);
	return sis33dev_round_uint(device->clkfreqs, device->n_clkfreqs, clkfreq, round);
}

/**
 * @brief Set the clock frequency of the device
 * @param device	- SIS33 device handle
 * @param hz		- desired clock frequency, in Hz
 *
 * @return 0 on success, -1 on failure
 *
 * \see sis33_round_clock_frequency
 */
int sis33_set_clock_frequency(sis33_t *device, unsigned int hz)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p hz %u\n", device, hz);
	ret = sis33dev_set_attr_uint(device->index, "clock_frequency", hz);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Get the clock frequency of the device
 * @param device	- SIS33 device handle
 * @param hz		- retrieved clock frequency, in Hz
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_clock_frequency(sis33_t *device, unsigned int *hz)
{
	unsigned int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_uint(device->index, "clock_frequency", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*hz = val;
	return 0;
}

/**
 * @brief Get number of event lengths
 * @param device	- SIS33 device handle
 *
 * @return number of event lengths of the device
 */
int sis33_get_nr_event_lengths(sis33_t *device)
{
	LIBSIS33_DEBUG(4, "handle %p\n", device);
	return device->n_ev_lengths;
}

/**
 * @brief Get the available event lengths
 * @param device	- SIS33 device handle
 * @param lengths	- array of event lengths
 * @param n		- max. number of items in the array
 *
 * @return 0 on success, -1 on failure
 *
 * The values in the array are ordered by value in descending order. The first
 * value is the maximum event length when the number of segments is minimum.
 */
int sis33_get_available_event_lengths(sis33_t *device, unsigned int *lengths, int n)
{
	LIBSIS33_DEBUG(4, "handle %p\n", device);
	if (n != device->n_ev_lengths) {
		__sis33_param_error(LIBSIS33_EINVAL);
		return -1;
	}
	memcpy(lengths, device->ev_lengths, n * sizeof(*device->ev_lengths));
	return 0;
}

/**
 * @brief Round an event length to a valid value
 * @param device	- SIS33 device handle
 * @param ev_length	- event length to be rounded
 * @param round		- rounding direction
 *
 * @return rounded event length value
 */
unsigned int
sis33_round_event_length(sis33_t *device, unsigned int ev_length, enum sis33_round round)
{
	LIBSIS33_DEBUG(4, "handle %p ev_length %u round %d\n", device, ev_length, round);
	return sis33dev_round_uint(device->ev_lengths, device->n_ev_lengths, ev_length, round);
}

/**
 * @brief Set autostart mode
 * @param device	- SIS33 device handle
 * @param value		- 1 == enable, 0 == disable
 *
 * @return 0 on success, -1 on failure
 *
 * With autostart mode enabled, the module starts sampling immediately
 * after being armed. Otherwise, a start trigger should arrive in
 * order to start sampling. When acquiring multiple events, in autostart
 * mode the acquisition of an event starts immediately after the
 * acquisition of the previous event has finished. If autostart mode is not
 * enabled, a start trigger must be sent to the device to initiate the
 * acquisition of the event.
 */
int sis33_set_start_auto(sis33_t *device, int value)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p value %d\n", device, value);
	ret = sis33dev_set_attr_int(device->index, "start_auto", value);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Get autostart mode
 * @param device	- SIS33 device handle
 * @param value		- 1 == enabled, 0 == disabled
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_start_auto(sis33_t *device, int *value)
{
	int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_int(device->index, "start_auto", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*value = val;
	return 0;
}

/**
 * @brief Enable/Disable autostop mode
 * @param device	- SIS33 device handle
 * @param value		- 1 == enable, 0 == disable
 *
 * @return 0 on success, -1 on failure
 *
 * In autostop mode, the device samples an event until it has acquired
 * the number of samples required for that event.
 * With autostop mode disabled, an event's acquisition does not finish
 * unless a stop trigger is sent to the device.
 */
int sis33_set_stop_auto(sis33_t *device, int value)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p value %d\n", device, value);
	ret = sis33dev_set_attr_int(device->index, "stop_auto", value);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Get the Enable/Disable status of the autostop mode
 * @param device	- SIS33 device handle
 * @param value		- 1 == enabled, 0 == disabled
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_stop_auto(sis33_t *device, int *value)
{
	int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_int(device->index, "stop_auto", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*value = val;
	return 0;
}

/**
 * @brief Enable/Disable external trigger
 * @param device	- SIS33 device handle
 * @param value		- 1 == enable, 0 == disable
 *
 * @return 0 on success, -1 on failure
 */
int sis33_set_enable_external_trigger(sis33_t *device, int value)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p value %d\n", device, value);
	ret = sis33dev_set_attr_int(device->index, "trigger_external_enable", value);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Get the Enable/Disable status of the external trigger
 * @param device	- SIS33 device handle
 * @param value		- 1 == enabled, 0 == disabled
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_enable_external_trigger(sis33_t *device, int *value)
{
	int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_int(device->index, "trigger_external_enable", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*value = val;
	return 0;
}

/**
 * @brief Set the start delay
 * @param device	- SIS33 device handle
 * @param delay		- delay value (in sampling clock ticks)
 *
 * @return 0 on success, -1 on failure
 */
int sis33_set_start_delay(sis33_t *device, int delay)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p delay %d\n", device, delay);
	ret = sis33dev_set_attr_int(device->index, "start_delay", delay);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Get the start delay
 * @param device	- SIS33 device handle
 * @param delay		- Retrieved delay (in sampling clock ticks)
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_start_delay(sis33_t *device, int *delay)
{
	int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_int(device->index, "start_delay", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*delay = val;
	return 0;
}

/**
 * @brief Set the stop delay
 * @param device	- SIS33 device handle
 * @param delay		- delay value (in sampling clock ticks)
 *
 * @return 0 on success, -1 on failure
 */
int sis33_set_stop_delay(sis33_t *device, int delay)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p delay %d\n", device, delay);
	ret = sis33dev_set_attr_int(device->index, "stop_delay", delay);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Get the stop delay
 * @param device	- SIS33 device handle
 * @param delay		- Retrieved delay (in sampling clock ticks)
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_stop_delay(sis33_t *device, int *delay)
{
	int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_int(device->index, "stop_delay", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*delay = val;
	return 0;
}

/**
 * @brief Get the number of channels on the device
 * @param device	- SIS33 device handle
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_nr_channels(sis33_t *device)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_nr_subattrs(device->index, "channels");
	if (ret < 0)
		__sis33_libc_error(__func__);
	return ret;
}

static int __set_offset(sis33_t *device, int channel_nr, unsigned int offset)
{
	char attr[SIS33_PATH_MAX];
	int ret;

	snprintf(attr, sizeof(attr) - 1, "channels/channel%d/offset", channel_nr);
	attr[sizeof(attr) - 1] = '\0';
	ret = sis33dev_set_attr_uint(device->index, attr, offset);
	if (ret < 0)
		return -1;
	return 0;
}

/**
 * @brief Set the input offset of a channel
 * @param device	- SIS33 device handle
 * @param channel_nr	- Channel nr (0 to n-1)
 * @param offset	- Desired offset
 *
 * @return 0 on success, -1 on failure
 */
int sis33_channel_set_offset(sis33_t *device, int channel_nr, unsigned int offset)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p channel %d offset 0x%x\n", device, channel_nr, offset);
	ret = __set_offset(device, channel_nr, offset);
	if (ret < 0) {
		if (errno == ENOENT)
			__sis33_lib_error(LIBSIS33_ENOTSUPP);
		else
			__sis33_libc_error(__func__);
	}
	return ret;
}

/**
 * @brief Set the input offset of all channels
 * @param device	- SIS33 device handle
 * @param offset	- Desired offset
 *
 * @return 0 on success, -1 on failure
 */
int sis33_channel_set_offset_all(sis33_t *device, unsigned int offset)
{
	int n_channels;
	int ret;
	int i;

	LIBSIS33_DEBUG(4, "handle %p offset 0x%x\n", device, offset);
	n_channels = sis33dev_get_nr_subattrs(device->index, "channels");
	if (n_channels < 0) {
		__sis33_libc_error(__func__);
		return n_channels;
	}
	for (i = 0; i < n_channels; i++) {
		ret = __set_offset(device, i, offset);
		if (ret < 0) {
			if (errno == ENOENT)
				__sis33_lib_error(LIBSIS33_ENOTSUPP);
			else
				__sis33_libc_error(__func__);
			return ret;
		}
	}
	return 0;
}

/**
 * @brief Get the current input offset of a channel
 * @param device	- SIS33 device handle
 * @param channel_nr	- Channel nr (0 to n-1)
 * @param offset	- Retrieved offset
 *
 * @return 0 on success, -1 on failure
 */
int sis33_channel_get_offset(sis33_t *device, int channel_nr, unsigned int *offset)
{
	char attr[SIS33_PATH_MAX];
	unsigned int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	snprintf(attr, sizeof(attr) - 1, "channels/channel%d/offset", channel_nr);
	attr[sizeof(attr) - 1] = '\0';
	ret = sis33dev_get_attr_uint(device->index, attr, &val);
	if (ret < 0) {
		if (errno == ENOENT)
			__sis33_lib_error(LIBSIS33_ENOTSUPP);
		else
			__sis33_libc_error(__func__);
		return -1;
	}
	*offset = val;
	return 0;
}

/**
 * @brief Send trigger to the device
 * @param device	- SIS33 device handle
 * @param trigger	- trigger to be sent
 *
 * @return 0 on success, -1 on failure
 */
int sis33_trigger(sis33_t *device, enum sis33_trigger trigger)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p trigger %d\n", device, trigger);
	ret = sis33dev_set_attr_int(device->index, "trigger", trigger);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Set the number of segments
 * @param device	- SIS33 device handle
 * @param segments	- desired number of segments (may be rounded)
 *
 * @return 0 on success, -1 on failure
 */
int sis33_set_nr_segments(sis33_t *device, unsigned int segments)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p segments %d\n", device, segments);
	ret = sis33dev_set_attr_uint(device->index, "n_segments", segments);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Get the current number of segments
 * @param device	- SIS33 device handle
 * @param segments	- Retrieved number of segments
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_nr_segments(sis33_t *device, unsigned int *segments)
{
	unsigned int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_uint(device->index, "n_segments", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*segments = val;
	return 0;
}

/**
 * @brief Get the maximum number of segments
 * @param device	- SIS33 device handle
 * @param segments	- Retrieved number of maximum segments
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_max_nr_segments(sis33_t *device, unsigned int *segments)
{
	unsigned int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_uint(device->index, "n_segments_max", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*segments = val;
	return 0;
}

/**
 * @brief Get the minimum number of segments
 * @param device	- SIS33 device handle
 * @param segments	- Retrieved number of minimum segments
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_min_nr_segments(sis33_t *device, unsigned int *segments)
{
	unsigned int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_uint(device->index, "n_segments_min", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*segments = val;
	return 0;
}

/**
 * @brief Get the maximum number of events
 * @param device	- SIS33 device handle
 * @param events	- Retrieved number of max. events
 *
 * @return 0 on success, -1 on failure
 */
int sis33_get_max_nr_events(sis33_t *device, unsigned int *events)
{
	unsigned int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_uint(device->index, "n_events_max", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*events = val;
	return 0;
}

static int __acq(sis33_t *device, unsigned int segment, unsigned int nr_events,
		 unsigned int ev_length, int flags, const struct timespec *timeout)
{
	struct sis33_acq_desc desc;
	int ret = 0;

	memset(&desc, 0, sizeof(desc));
	desc.segment	= segment;
	desc.nr_events	= nr_events;
	desc.ev_length	= ev_length;
	desc.flags	= flags;
	if (timeout)
		memcpy(&desc.timeout, timeout, sizeof(*timeout));

	if (ioctl(device->fd, SIS33_IOC_ACQUIRE, &desc) < 0) {
		__sis33_libc_error(__func__);
		ret = -1;
	}
	return ret;
}

/**
 * @brief Start an acquisition without waiting for it to finish
 * @param device	- sis33 device
 * @param segment	- segment number (0 to n-1) to acquire on
 * @param nr_events	- number of events
 * @param ev_length	- number of samples per event
 *
 * @return 0 on success, -1 on failure
 *
 * Note that an acquisition cannot start if there is an ongoing acquisition
 * on this or any other segment. Furthermore, an acquisition cannot start
 * on a given segment if there is a client fetching data from the same segment.
 *
 * \see sis33_round_event_length
 */
int sis33_acq(sis33_t *device, unsigned int segment, unsigned int nr_events, unsigned int ev_length)
{
	LIBSIS33_DEBUG(4, "handle %p segment %u nr_events %u ev_length %u\n",
		device, segment, nr_events, ev_length);
	return __acq(device, segment, nr_events, ev_length, SIS33_ACQF_DONTWAIT, NULL);
}

/**
 * @brief Start an acquisition, sleeping until it finishes
 * @param device	- sis33 device
 * @param segment	- segment number (0 to n-1) to acquire on
 * @param nr_events	- number of events
 * @param ev_length	- number of samples per event
 *
 * @return 0 on success, -1 on failure
 *
 * A signal delivered to this process interrupts the wait.
 * \see sis33_acq
 */
int sis33_acq_wait(sis33_t *device, unsigned int segment, unsigned int nr_events, unsigned int ev_length)
{
	LIBSIS33_DEBUG(4, "handle %p segment %u nr_events %u ev_length %u\n",
		device, segment, nr_events, ev_length);
	return __acq(device, segment, nr_events, ev_length, 0, NULL);
}

/**
 * @brief Start an acquisition, sleeping until it finishes or a timeout expires
 * @param device	- sis33 device
 * @param segment	- segment number (0 to n-1) to acquire on
 * @param nr_events	- number of events
 * @param ev_length	- number of samples per event
 * @param timeout	- desired timeout
 *
 * @return 0 on success, -1 on failure
 *
 * A signal delivered to this process interrupts the wait.
 * The eventual timeout may exceed the amount of time specified in 'timeout'.
 * Note that the implementation of the timeout is platform-dependent and
 * therefore its granularity may not be down to the nanosecond range.
 * \see sis33_acq
 */
int sis33_acq_timeout(sis33_t *device, unsigned int segment, unsigned int nr_events,
		      unsigned int ev_length, const struct timespec *timeout)
{
	LIBSIS33_DEBUG(4, "handle %p segment %u nr_events %u ev_length %u sec %u nsec %u\n",
		device, segment, nr_events, ev_length, (unsigned int)timeout->tv_sec,
		(unsigned int)timeout->tv_nsec);
	return __acq(device, segment, nr_events, ev_length, SIS33_ACQF_TIMEOUT, timeout);
}

/**
 * @brief Cancel an ongoing acquisition
 *
 * @return 0 on success, -1 on failure
 *
 * \see sis33_acq
 */
int sis33_acq_cancel(sis33_t *device)
{
	LIBSIS33_DEBUG(4, "handle %p\n", device);
	if (sis33dev_set_attr_int(device->index, "acq_cancel", 1) < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Allocate (and zero) an array of acquisition buffers
 * @param nr_events	- Number of events
 * @param ev_length	- Event length
 *
 * @return pointer to the array on success, NULL on failure
 *
 * Note: Although recommended, it is not mandatory to allocate arrays
 * of acquisition buffers through this function.
 * \see sis33_fetch
 */
struct sis33_acq *sis33_acqs_zalloc(unsigned int nr_events, unsigned int ev_length)
{
	struct sis33_acq *acqs;

	LIBSIS33_DEBUG(4, "nr_events %u ev_length %u\n", nr_events, ev_length);
	acqs = sis33acq_zalloc(nr_events, ev_length);
	if (acqs == NULL)
		__sis33_libc_error(__func__);
	return acqs;
}

/**
 * @brief Free an array of acquisition buffers previously allocated with sis33_acqs_zalloc.
 * @param acqs		- Pointer to the array to be freed
 * @param n_acqs	- Number of elements in the array
 */
void sis33_acqs_free(struct sis33_acq *acqs, unsigned int n_acqs)
{
	LIBSIS33_DEBUG(4, "acqs %p n_acqs %u\n", acqs, n_acqs);
	return sis33acq_free(acqs, n_acqs);
}

static int __fetch(sis33_t *device, unsigned int segment, unsigned int channel,
		struct sis33_acq *acqs, unsigned int n_acqs, int flags,
		struct timeval *endtime, struct timespec *ts)
{
	struct sis33_acq_list list;
	int acq_events;

	if (acqs == NULL || n_acqs == 0) {
		__sis33_param_error(LIBSIS33_EINVAL);
		return -1;
	}
	memset(&list, 0, sizeof(list));
	list.segment	= segment;
	list.channel	= channel;
	list.acqs	= acqs;
	list.n_acqs	= n_acqs;
	list.flags	= flags;
	if (ts)
		memcpy(&list.timeout, ts, sizeof(*ts));
	list.endtime.tv_sec	= 0;
	list.endtime.tv_usec	= 0;

	acq_events = ioctl(device->fd, SIS33_IOC_FETCH, &list);
	if (acq_events < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	if (sis33acq_list_normalize(&list, acq_events)) {
		__sis33_libc_error(__func__);
		return -1;
	}
	if (endtime)
		memcpy(endtime, &list.endtime, sizeof(*endtime));
	return acq_events;
}

/**
 * @brief Fetch, normalize and store samples from an sis33 device
 * @param device	- sis33 device
 * @param segment	- segment to get the samples from
 * @param channel	- channel on the device to get the samples from
 * @param acqs		- array of acquisition buffers
 * @param n_acqs	- number of acquisition buffers in the array
 * @param endtime	- Local time when the acquisition finished. Can be NULL
 *
 * @return Number of acquired events on success, -1 on failure.
 *
 * If there is an ongoing acquisition on the specified segment, the function
 * fails returning immediately, setting sis33_errno appropriately.
 *
 * Note that the acquired number of events may be smaller than n_acqs.
 *
 * After calling this function, the acquisition buffers are said to be
 * normalized; this means that it is safe to assume that the first acquired
 * sample is always the first sample in the buffer and that the endianness of
 * the sampled data is that of the host machine.
 * The valid range for the samples goes from 0 to 2^n_bits.
 * \see sis33_get_nr_bits
 * \see sis33_acqs_zalloc
 * \see sis33_fetch_wait
 * \see sis33_fetch_timeout
 */
int sis33_fetch(sis33_t *device, unsigned int segment, unsigned int channel,
		struct sis33_acq *acqs, unsigned int n_acqs, struct timeval *endtime)
{
	LIBSIS33_DEBUG(4, "handle %p segment %u channel %u acqs %p n_acqs %u endtime %p\n",
		device, segment, channel, acqs, n_acqs, endtime);
	return __fetch(device, segment, channel, acqs, n_acqs, SIS33_ACQF_DONTWAIT, endtime, NULL);
}

/**
 * @brief Fetch, normalize and store samples from an sis33 device with sleeping wait
 * @param device	- sis33 device
 * @param segment	- segment to get the samples from
 * @param channel	- channel on the device to get the samples from
 * @param acqs		- array of acquisition buffers
 * @param n_acqs	- number of acquisition buffers in the array
 * @param endtime	- Local time when the acquisition finished. Can be NULL
 *
 * @return Number of acquired events on success, -1 on failure.
 *
 * The process is put to sleep while there is an ongoing acquisition on the
 * given segment. When the acquisition finishes it then fetches the data.
 * \see sis33_acq_wait
 * \see sis33_fetch
 */
int sis33_fetch_wait(sis33_t *device, unsigned int segment, unsigned int channel,
		struct sis33_acq *acqs, unsigned int n_acqs, struct timeval *endtime)
{
	LIBSIS33_DEBUG(4, "handle %p segment %u channel %u acqs %p n_acqs %u endtime %p\n",
		device, segment, channel, acqs, n_acqs, endtime);
	return __fetch(device, segment, channel, acqs, n_acqs, 0, endtime, NULL);
}

/**
 * @brief Fetch, normalize and store samples from an sis33 device with timeout
 * @param device	- sis33 device
 * @param segment	- segment to get the samples from
 * @param channel	- channel on the device to get the samples from
 * @param acqs		- array of acquisition buffers
 * @param n_acqs	- number of acquisition buffers in the array
 * @param endtime	- Local time when the acquisition finished. Can be NULL
 * @param timeout	- acquisition timeout
 *
 * @return Number of acquired events on success, -1 on failure.
 *
 * If there is an ongoing acquisition on the given segment, the process is put
 * to sleep until it either finishes or the timeout expires. Note that this
 * timeout only affects the acquisition; the fetch operation takes as long as
 * it needs.
 * \see sis33_acq_timeout
 * \see sis33_fetch
 */
int sis33_fetch_timeout(sis33_t *device, unsigned int segment, unsigned int channel,
			struct sis33_acq *acqs, unsigned int n_acqs,
			struct timeval *endtime, struct timespec *timeout)
{
	LIBSIS33_DEBUG(4, "handle %p segment %u channel %u acqs %p n_acqs %u sec %lu nsec %lu endtime %p\n",
		device, segment, channel, acqs, n_acqs, timeout->tv_sec, timeout->tv_nsec, endtime);
	return __fetch(device, segment, channel, acqs, n_acqs, SIS33_ACQF_TIMEOUT, endtime, timeout);
}

/**
 * @brief get the number of bits of a device
 * @param device	- sis33 device
 *
 * @return number of bits of the device
 */
int sis33_get_nr_bits(sis33_t *device)
{
	return device->n_bits;
}

/**
 * @brief return whether or not a device implements event hardware timestamping
 * @param device	- sis33 device
 *
 * @return 1 (supported) or 0 (not supported)
 */
int sis33_event_timestamping_is_supported(sis33_t *device)
{
	return device->ev_tstamp_supported;
}

/**
 * @brief Get the value of the maximum event timestamping divider available on a device
 * @param device	- SIS33 device handle
 * @param max		- Retrieved maximum value
 *
 * @return 0 on success, -1 on failure
 *
 * Some devices allow a certain range of values for the timestamping device.
 * This allows to cover longer time spans at the expense of a loss in precision.
 *
 * \see sis33_set_event_timestamping_divider
 */
int sis33_get_max_event_timestamping_divider(sis33_t *device, unsigned int *max)
{
	unsigned int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_uint(device->index, "event_timestamping_divider_max", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*max = val;
	return 0;
}

/**
 * @brief Get the value of the current event timestamping divider on a device
 * @param device	- SIS33 device handle
 * @param divider	- Retrieved timestamping divider
 *
 * @return 0 on success, -1 on failure
 *
 * This divider is used to generate the event counting clock from the sampling
 * clock.
 *
 * \see sis33_get_max_event_timestamping_divider
 */
int sis33_get_event_timestamping_divider(sis33_t *device, unsigned int *divider)
{
	unsigned int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_uint(device->index, "event_timestamping_divider", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*divider = val;
	return 0;
}

/**
 * @brief Set the current event timestamping divider on a device
 * @param device	- SIS33 device handle
 * @param divider	- Desired value for the divider
 *
 * @return 0 on success, -1 on failure
 *
 * \see sis33_get_event_timestamping_divider
 * \see sis33_get_max_event_timestamping_divider
 * \see sis33_event_timestamping_is_supported
 */
int sis33_set_event_timestamping_divider(sis33_t *device, unsigned int divider)
{
	int ret;

	LIBSIS33_DEBUG(4, "handle %p divider %u\n", device, divider);
	ret = sis33dev_set_attr_uint(device->index, "event_timestamping_divider", divider);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	return 0;
}

/**
 * @brief Get maximum number of clock ticks of the event timestamping unit.
 * @param device	- SIS33 device handle
 * @param maxticks_log2	- Retrieved maximum value, in log to base 2 format
 *
 * @return 0 on success, -1 on failure
 *
 * This function is used to query the number of the sampling clock ticks at
 * which the event timestamping counter overflows. In most implementations
 * this can be understood as the counter's number of bits, thus its representation
 * as a logarithm to base 2.
 *
 * \see sis33_set_event_timestamping_divider
 */
int sis33_get_max_event_timestamping_clock_ticks(sis33_t *device, unsigned int *maxticks_log2)
{
	unsigned int val;
	int ret;

	LIBSIS33_DEBUG(4, "handle %p\n", device);
	ret = sis33dev_get_attr_uint(device->index, "event_timestamping_max_ticks_log2", &val);
	if (ret < 0) {
		__sis33_libc_error(__func__);
		return -1;
	}
	*maxticks_log2 = val;
	return 0;
}
