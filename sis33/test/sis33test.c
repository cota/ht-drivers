/*
 * sis33test.c
 * sis33 interactive test-program
 *
 * Copyright (c) 2009-2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <general_usr.h>  /* for handy definitions (mperr etc..) */
#include <extest.h>
#include <sis33.h>

#include "sis33acq.h"
#include "sis33dev.h"

#define SIS33T_MAX_NR_SEGMENTS	8

int use_builtin_cmds = 0;
char xmlfile[128] = "cvorg.xml";

static uint32_t		channel_nr = 0;
static uint32_t		nr_events[SIS33T_MAX_NR_SEGMENTS];
static uint32_t		nr_samples[SIS33T_MAX_NR_SEGMENTS];
static struct sis33_acq_list	acq_list[SIS33T_MAX_NR_SEGMENTS];
static int sis33fd = -1;
static int curr_index;
static int curr_segment;
static char gnuplot[4096];

static struct timespec my_us_to_timespec(const unsigned int us)
{
	struct timespec ts;

	if (!us)
		return (struct timespec) {0, 0};
	ts.tv_sec = us / 1000000;
	ts.tv_nsec = (us % 1000000) * 1000;
	return ts;
}

static int round_uint(int index, const char *name, unsigned int *roundme, enum sis33_round round)
{
	unsigned int *array;
	int ret;
	int n;

	n = sis33dev_get_nr_items(index, name);
	if (n <= 0)
		return -1;
	array = calloc(n, sizeof(*array));
	if (array == NULL)
		return -1;

	ret = sis33dev_get_items_uint(index, name, array, n);
	if (ret)
		goto out;
	*roundme = sis33dev_round_uint(array, n, *roundme, round);
 out:
	free(array);
	return ret;
}

#define MY_FILE_LEN	64

static char *write_file(const char *prefix, struct sis33_acq_list *list, int segment, int event)
{
	FILE *filp;
	struct sis33_acq *acq = &list->acqs[event];
	static char filename[MY_FILE_LEN];
	int i;

	snprintf(filename, MY_FILE_LEN - 1, "/tmp/%s.ch%d.seg%d.ev%d.dat",
		prefix, list->channel, segment, event);
	filename[MY_FILE_LEN - 1] = '\0';
	filp = fopen(filename, "w");
	if (filp == NULL) {
		mperr("fopen");
		return NULL;
	}
	fprintf(filp, "# segment %d event %d\n", segment, event);
	for (i = 0; i < acq->nr_samples; i++)
		fprintf(filp, "0x%x\n", acq->data[i]);
	if (fclose(filp) < 0)
		return NULL;
	printf("Data written to %s.\n", filename);
	return filename;
}

static int h_attr(const struct atom *atoms, const char *param)
{
	int val;
	int ret;

	++atoms;
	if (atoms->type == Numeric) {
		ret = sis33dev_set_attr_int(curr_index, param, atoms->val);
		if (ret < 0) {
			mperr("set_attr");
			return -TST_ERR_SYSCALL;
		}
	}
	ret = sis33dev_get_attr_int(curr_index, param, &val);
	if (ret < 0) {
		mperr("get_attr");
		return ret;
	}
	printf("%d\n", val);
	return 1;
}

static int h_uattr(const struct atom *atoms, const char *param)
{
	unsigned int val;
	int ret;

	++atoms;
	if (atoms->type == Numeric) {
		val = atoms->val;
		ret = sis33dev_set_attr_uint(curr_index, param, val);
		if (ret < 0) {
			mperr("set_attr_uint");
			return -TST_ERR_SYSCALL;
		}
	}
	ret = sis33dev_get_attr_uint(curr_index, param, &val);
	if (ret < 0) {
		mperr("get_attr_uint");
		return ret;
	}
	printf("%u\n", val);
	return 1;
}

int h_start_auto(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Get/set Autostart\n"
			"%s bool\n",
			cmdd->name, cmdd->name);
		return 1;
	}
	return h_attr(atoms, "start_auto");
}

int h_trig_ext(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Enable/Disable external trigger\n"
			"%s bool\n",
			cmdd->name, cmdd->name);
		return 1;
	}
	return h_attr(atoms, "trigger_external_enable");
}

int h_start_delay(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Get/Set Start delay\n"
			"%s n\n"
			"\tdelay: start input delay of n clock ticks\n"
			"\tNote: this has no effect on software-driven start\n"
			"\t(valid range for the sis3320: [0-8K]\n",
			cmdd->name, cmdd->name);
		return 1;
	}
	return h_attr(atoms, "start_delay");
}


int h_stop_delay(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Get/Set Stop delay\n"
			"%s n\n"
			"\tdelay: stop input delay of n clock ticks\n"
			"\tNote: this has no effect on software-driven stop\n"
			"\t(valid range for the sis3320: [0-8K]\n",
			cmdd->name, cmdd->name);
		return 1;
	}
	return h_attr(atoms, "stop_delay");
}

static int show_chan_offset_all(void)
{
	char attr[SIS33_PATH_MAX];
	int n_channels;
	unsigned int val;
	int ret;
	int i;

	n_channels = sis33dev_get_nr_subattrs(curr_index, "channels");
	if (n_channels < 0)
		return n_channels;

	for (i = 0; i < n_channels; i++) {
		snprintf(attr, sizeof(attr) - 1, "channels/channel%d/offset", i);
		attr[sizeof(attr) - 1] = '\0';
		ret = sis33dev_get_attr_uint(curr_index, attr, &val);
		if (ret < 0)
			continue;
		printf("%d\t0x%-8x (%d)\n", i, val, val);
	}
	return 1;
}

static int set_offset(int offset, int channel)
{
	char attr[SIS33_PATH_MAX];
	int n_channels;
	int init, end;
	int ret;
	int i;

	n_channels = sis33dev_get_nr_subattrs(curr_index, "channels");
	if (n_channels < 0)
		return n_channels;
	if (channel == -1) {
		init = 0;
		end = n_channels;
	} else {
		init = channel;
		end = channel + 1;
	}
	for (i = init; i < end; i++) {
		snprintf(attr, sizeof(attr) - 1, "channels/channel%d/offset", i);
		attr[sizeof(attr) - 1] = '\0';
		ret = sis33dev_set_attr_int(curr_index, attr, offset);
		if (ret < 0) {
			mperr("set_offset");
			return -TST_ERR_SYSCALL;
		}
	}
	return 0;
}

int h_offset(struct cmd_desc *cmdd, struct atom *atoms)
{
	int n_channels;
	int channel;
	int ret;

	n_channels = sis33dev_get_nr_subattrs(curr_index, "channels");
	if (n_channels < 0)
		return n_channels;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Configure Input Offset\n"
			"%s channel_nr value\n"
			"\tchannel_nr: [0-%d]. -1 to select all channels\n"
			"\value: 16 bits (max. 0xffff.)\n",
			cmdd->name, cmdd->name, n_channels - 1);
		return 1;
	}

	++atoms;
	if (atoms->type == Terminator)
		return show_chan_offset_all();

	if (atoms->type != Numeric)
		return -TST_ERR_WRONG_ARG;
	if (!WITHIN_RANGE(-1, atoms->val, n_channels - 1))
		return -TST_ERR_WRONG_ARG;
	channel = atoms->val;

	++atoms;
	if (atoms->type != Numeric)
		return -TST_ERR_WRONG_ARG;
	ret = set_offset(atoms->val, channel);
	if (ret)
		return ret;

	printf("Updated:\n");
	return show_chan_offset_all();
}

int h_clksrc(struct cmd_desc *cmdd, struct atom *atoms)
{
	int clksrc;
	int ret;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Configure Clock Source\n"
			"%s [source]\n"
			"\tsource: 0 (internal), 1 (external)\n",
			cmdd->name, cmdd->name);
		return 1;
	}

	++atoms;
	if (atoms->type == Numeric) {
		ret = sis33dev_set_attr_int(curr_index, "clock_source", atoms->val);
                if (ret < 0) {
			mperr("set_attr clock_source");
			return -TST_ERR_SYSCALL;
                }
	}
	ret = sis33dev_get_attr_int(curr_index, "clock_source", &clksrc);
	if (ret < 0) {
		mperr("get_attr clock_source");
		return ret;
	}
	if (clksrc == SIS33_CLKSRC_INTERNAL)
		printf("internal");
	else if (clksrc == SIS33_CLKSRC_EXTERNAL)
		printf("external");
	else
		printf("unknown");
	printf("\n");
	return 1;
}

int h_clkfreq(struct cmd_desc *cmdd, struct atom *atoms)
{
	int show_available = 0;
	unsigned int val;
	int ret;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Get/Set Clock Frequency\n"
			"%s [Hz]\n"
			"\tHz: clock frequency, in Hz.\n",
			cmdd->name, cmdd->name);
		return 1;
	}
	++atoms;
	if (atoms->type == Terminator)
		show_available++;
	else if (atoms->type == Numeric) {
		val = atoms->val;
		if (round_uint(curr_index, "available_clock_frequencies", &val, SIS33_ROUND_NEAREST) < 0)
			return -TST_ERR_SYSCALL;
		ret = sis33dev_set_attr_int(curr_index, "clock_frequency", val);
		if (ret < 0) {
			mperr("set_attr");
			return -TST_ERR_SYSCALL;
		}
	}
	ret = sis33dev_get_attr_uint(curr_index, "clock_frequency", &val);
	if (ret < 0) {
		mperr("get_attr");
		return ret;
	}
	printf("%u\n", val);
	if (show_available) {
		char clkfreqs[256];

		ret = sis33dev_get_attr(curr_index, "available_clock_frequencies", clkfreqs, sizeof(clkfreqs));
		if (ret < 0) {
			mperr("get_attr available_clock_frequencies");
			return ret;
		}
		printf("Available clock frequencies: %s\n", clkfreqs);
	}
	return 1;
}

int h_stop_auto(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Get/Set Autostop\n"
			"%s bool\n"
			"Stop sampling for each event after the "
			"specified number of samples have been collected.\n",
			cmdd->name, cmdd->name);
		return 1;
	}
	return h_attr(atoms, "stop_auto");
}

int h_ch(struct cmd_desc *cmdd, struct atom *atoms)
{
	int n_channels;

	n_channels = sis33dev_get_nr_subattrs(curr_index, "channels");
	if (n_channels < 0)
		return n_channels;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Get/Set Channel\n"
			"%s [channel_nr]\n"
			"\tchannel_nr: 0-%d\n",
			cmdd->name, cmdd->name, n_channels - 1);
		return 1;
	}

	++atoms;
	if (atoms->type == Terminator) {
		printf("%d\n", channel_nr);
		goto out;
	}

	if (atoms->type != Numeric || !WITHIN_RANGE(0, atoms->val, n_channels - 1))
		return -TST_ERR_WRONG_ARG;

	channel_nr = atoms->val;
 out:
	return 1;
}

int h_ch_next(struct cmd_desc *cmdd, struct atom *atoms)
{
	int n_channels;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Select Next Channel\n", cmdd->name);
		return 1;
	}
	n_channels = sis33dev_get_nr_subattrs(curr_index, "channels");
	if (n_channels < 0)
		return n_channels;
	if (channel_nr + 1 < n_channels)
		channel_nr++;

	return 1;
}

static void remove_trailing_chars(char *str, char c)
{
	size_t len;

	if (str == NULL)
		return;
	len = strlen(str);
	while (len > 0 && str[len - 1] == c)
		str[--len] = '\0';
}

static char *mydatetime(const struct timeval *tv)
{
	static char datetime[26]; /* length imposed by ctime_r */
	time_t time;

	time = tv->tv_sec;
	ctime_r(&time, datetime);
	datetime[sizeof(datetime) - 1] = '\0';
	remove_trailing_chars(datetime, '\n');
	return datetime;
}

static void print_prevticks(const struct sis33_acq *acqs, int n)
{
	int supported;
	int log2;
	int i;

	if (sis33dev_get_attr_int(curr_index, "event_timestamping_support", &supported) < 0) {
		mperr("get_attr event_timestamping_support");
		return;
	}
	if (!supported) {
		printf("This device doesn't support event timestamping\n");
		return;
	}

	for (i = 0; i < n; i++)
		printf("%4u 0x%8llx\n", i, (unsigned long long)acqs[i].prevticks);
	if (sis33dev_get_attr_int(curr_index, "event_timestamping_max_ticks_log2", &log2) < 0) {
		mperr("get_attr event_timestamping_max_ticks_log2");
		return;
	}
	printf("Note that this counter overflows every 2**%d (0x%llx) ticks\n", log2, (unsigned long long)1 << log2);
}

int h_fetch(struct cmd_desc *cmdd, struct atom *atoms)
{
	struct timespec timestart, timeswap, timeend;
	struct timespec ts;
	struct sis33_acq_list *list;
	unsigned int segment;
	unsigned int flags;
	double delta;
	int acq_events;
	int show_tstamps = 0;
	int ret = 1;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Fetch Acquired Data\n"
			"%s [segment] [timeout_us] [show_tstamps]\n"
			"\tsegment: segment number. -1 for current (default).\n"
			"\ttimeout_us: 0 - blocking wait until the acquisition finishes\n"
			"\t\tn > 0 - blocking wait with timeout equal to n us\n"
			"\tshow_tstamps: 1 to show all the events' timestamps. Default: 0\n"
			"Note: If no timeout is provided, no blocking wait is done\n",
			cmdd->name, cmdd->name);
		return 1;
	}

	++atoms;
	segment = curr_segment;
	flags = SIS33_ACQF_DONTWAIT;
	if (atoms->type == Terminator)
		goto fetch;
	if (atoms->type == Numeric && atoms->val != -1) {
		segment = atoms->val;
		if (segment < 0 || segment >= SIS33T_MAX_NR_SEGMENTS)
			return -TST_ERR_WRONG_ARG;
	}
	++atoms;
	if (atoms->type == Terminator)
		goto fetch;
	if (atoms->type == Numeric) {
		if (atoms->val > 0) {
			ts = my_us_to_timespec(atoms->val);
			flags = SIS33_ACQF_TIMEOUT;
		} else if (atoms->val == 0) {
			flags = 0;
		}
	}
	++atoms;
	if (atoms->type == Terminator)
		goto fetch;
	if (atoms->type == Numeric && atoms->val)
		show_tstamps = 1;
 fetch:
	if (!nr_samples[segment] || !nr_events[segment]) {
		fprintf(stderr, "segment %d: No acquisitions done yet\n", segment);
		return -TST_ERR_WRONG_ARG;
	}

	list = &acq_list[segment];
	/* free the complete list in case there's some data already */
	if (list->acqs)
		sis33acq_free(list->acqs, list->n_acqs);

	memset(list, 0, sizeof(*list));

	list->acqs = sis33acq_zalloc(nr_events[segment], nr_samples[segment]);
	if (list->acqs == NULL) {
		ret = TST_ERR_SYSCALL;
		goto out;
	}
	list->n_acqs	= nr_events[segment];
	list->segment	= segment;
	list->channel	= channel_nr;
	list->flags	= flags;
	list->timeout	= ts;
	list->endtime.tv_sec	= 0;
	list->endtime.tv_usec	= 0;

	/* fetch the samples */
	if (clock_gettime(CLOCK_REALTIME, &timestart)) {
		mperr("clock_gettime() failed for timestart");
		ret = -TST_ERR_SYSCALL;
		goto out;
	}
	acq_events = ioctl(sis33fd, SIS33_IOC_FETCH, list);
	if (acq_events < 0) {
		mperr("Fetch");
		ret = -TST_ERR_IOCTL;
		goto out;
	}
	if (acq_events < nr_events[segment]) {
		fprintf(stderr, "Acquired only %d out of %d event%s\n",
			acq_events, nr_events[segment], nr_events[segment] > 1 ? "s" : "");
	}
	if (clock_gettime(CLOCK_REALTIME, &timeswap)) {
		mperr("clock_gettime() failed for timestart");
		ret = -TST_ERR_SYSCALL;
		goto out;
	}

	if (sis33acq_list_normalize(list, acq_events)) {
		mperr("sis33acq_list_normalize failed");
		ret = -TST_ERR_SYSCALL;
		goto out;
	}

	if (clock_gettime(CLOCK_REALTIME, &timeend)) {
		mperr("clock_gettime() failed for timestart");
		ret = -TST_ERR_SYSCALL;
		goto out;
	}

	printf("Acquisition datetime: %s +%06luus\n",
		mydatetime(&list->endtime), list->endtime.tv_usec);
	delta = (double)(timeswap.tv_sec - timestart.tv_sec) * 1000000 +
		(double)(timeswap.tv_nsec - timestart.tv_nsec) / 1000.;
	printf("Acquisition CPU time: %g us\n", delta);
	delta = (double)(timeend.tv_sec - timeswap.tv_sec) * 1000000 +
		(double)(timeend.tv_nsec - timeswap.tv_nsec) / 1000.;
	printf("swap CPU time: %g us\n", delta);
	if (show_tstamps)
		print_prevticks(list->acqs, acq_events);

 out:
	return ret;
}

int exec_trigger(uint32_t trigger)
{
	int ret;

	ret = sis33dev_set_attr_int(curr_index, "trigger", trigger);
	if (ret < 0) {
		mperr("set_attr trigger");
		return -TST_ERR_SYSCALL;
	}
	return 1;
}

int h_acq(struct cmd_desc *cmdd, struct atom *atoms)
{
	struct sis33_acq_desc desc;
	struct timespec timestart, timeend;
	double delta;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Start Acquisition\n"
			"%s segment nr_events ev_length [timeout_us]\n"
			"\tsegment: segment number [0..%d]\n"
			"\tnr_events: number of events\n"
			"\tev_length: number of samples per event\n"
			"\ttimeout_us: 0 - blocking wait until the acquisition finishes\n"
			"\t\tn > 0 - blocking wait with timeout equal to n us\n"
			"Note: If no timeout is provided, no blocking wait is done\n",
			cmdd->name, cmdd->name, SIS33T_MAX_NR_SEGMENTS - 1);
		return 1;
	}
	memset(&desc, 0, sizeof(desc));
	++atoms;
	if (atoms->type != Numeric)
		return -TST_ERR_WRONG_ARG;
	if (atoms->val < 0 || atoms->val >= SIS33T_MAX_NR_SEGMENTS)
		return -TST_ERR_WRONG_ARG;
	desc.segment = atoms->val;
	++atoms;
	if (atoms->type != Numeric)
		return -TST_ERR_WRONG_ARG;
	desc.nr_events = atoms->val;
	++atoms;
	if (atoms->type != Numeric)
		return -TST_ERR_WRONG_ARG;
	desc.ev_length = atoms->val;
	if (round_uint(curr_index, "available_event_lengths",
			&desc.ev_length, SIS33_ROUND_NEAREST) < 0)
		return -TST_ERR_SYSCALL;
	++atoms;
	if (atoms->type == Numeric) {
		if (atoms->val > 0) {
			desc.timeout = my_us_to_timespec(atoms->val);
			desc.flags = SIS33_ACQF_TIMEOUT;
		} else if (atoms->val == 0)
			desc.flags = 0;
		else
			desc.flags = SIS33_ACQF_DONTWAIT;
	} else {
		desc.flags = SIS33_ACQF_DONTWAIT;
	}
	if (clock_gettime(CLOCK_REALTIME, &timestart)) {
		mperr("clock_gettime() failed for timestart");
		return -TST_ERR_SYSCALL;
	}

	if (ioctl(sis33fd, SIS33_IOC_ACQUIRE, &desc) < 0) {
		mperr("acquire");
		return -TST_ERR_IOCTL;
	}

	if (clock_gettime(CLOCK_REALTIME, &timeend)) {
		mperr("clock_gettime() failed for timeend");
		return -TST_ERR_SYSCALL;
	}

	curr_segment			= desc.segment;
	nr_events[curr_segment]		= desc.nr_events;
	nr_samples[curr_segment]	= desc.ev_length;

	delta = (double)(timeend.tv_sec - timestart.tv_sec) * 1000000 +
		(double)(timeend.tv_nsec - timestart.tv_nsec) / 1000.;

	printf("events: %d\nev_length: %d\n", nr_events[curr_segment], nr_samples[curr_segment]);
	printf("acquisition: time elapsed: %g us\n", delta);

	return 1;
}

int h_cancel(struct cmd_desc *cmdd, struct atom *atoms)
{
	int ret;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Cancel Acquisition\n", cmdd->name);
		return 1;
	}

	ret = sis33dev_set_attr_int(curr_index, "acq_cancel", 1);
	if (ret < 0) {
		mperr("set_attr acq_cancel");
		return -TST_ERR_SYSCALL;
	}
	return 1;
}

int h_start(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Start Sampling\n", cmdd->name);
		return 1;
	}
	return exec_trigger(SIS33_TRIGGER_START);
}

int h_stop(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Stop Sampling\n", cmdd->name);
		return 1;
	}
	return exec_trigger(SIS33_TRIGGER_STOP);
}

static void gnuplot_append(const char *string)
{
	strncat(gnuplot, string, sizeof(gnuplot) - strlen(gnuplot));
	gnuplot[sizeof(gnuplot) - 1] = '\0';
}

static void gnuplot_init(int sis33_index)
{
	char value[4];

	gnuplot[0] = '\0';
	if (sis33dev_get_attr(sis33_index, "n_bits", value, sizeof(value))) {
		fprintf(stderr, "Warning: Cannot get n_bits of device %u\n", sis33_index);
	} else {
		int n_bits = strtol(value, NULL, 0);
		char str[32];

		snprintf(str, sizeof(str), "set yrange [0:%d]; ", 1 << n_bits);
		str[sizeof(str) - 1] = '\0';
		gnuplot_append(str);
	}
}

static int write_samp_data_to_disk(int segment)
{
	char devname[32];
	const char *filename;
	unsigned long acq_samples;
	int i;

	if (!nr_events[segment] || !nr_samples[segment]) {
		fprintf(stderr, "Error: not acquired any data.\n");
		return -TST_ERR_WRONG_ARG;
	}

	if (sis33dev_get_devname(curr_index, devname, sizeof(devname)) < 0) {
		mperr("sis33dev_get_devname");
		return -TST_ERR_SYSCALL;
	}
	gnuplot_init(curr_index);
	acq_samples = 0;
	for (i = 0; i < acq_list[segment].n_acqs; i++) {
		if (acq_list[segment].acqs[i].nr_samples == 0)
			continue;
		filename = write_file(devname, &acq_list[segment], segment, i);
		/* we don't get the error back. Assume it's SYSCALL */
		if (filename == NULL)
			return -TST_ERR_SYSCALL;
		if (i == 0)
			gnuplot_append("plot ");
		else
			gnuplot_append(", ");
		gnuplot_append("'");
		gnuplot_append(filename);
		gnuplot_append("' with linespoints");
		acq_samples += acq_list[segment].acqs[i].nr_samples;
	}
	if (acq_samples == 0)
		fprintf(stderr, "No samples were acquired on segment %d\n", segment);
	return 1;
}

int h_wf(struct cmd_desc *cmdd, struct atom *atoms)
{
	int segment = curr_segment;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s [segment] - Write acquired data to a file\n"
			"segment: segment number. Leave empty for current.\n"
			"Note. The file will be located at /tmp/<curr_device>.ev<n>.<channel>.dat\n",
			cmdd->name);
		return 1;
	}
	++atoms;
	if (atoms->type == Terminator)
		goto write_to_disk;
	if (atoms->type != Numeric)
		return -TST_ERR_WRONG_ARG;
	if (atoms->val < 0 || atoms->val >= SIS33T_MAX_NR_SEGMENTS)
		return -TST_ERR_WRONG_ARG;
	segment = atoms->val;

 write_to_disk:
	return write_samp_data_to_disk(segment);
}

static void show_devices(void)
{
	char value[4096];
	int n_channels;
	int *indexes;
	int ndevs;
	int i;

	ndevs = sis33dev_get_nr_devices();
	if (!ndevs)
		return;
	indexes = calloc(ndevs, sizeof(*indexes));
	if (indexes == NULL) {
		mperr("calloc");
		return;
	}
	if (sis33dev_get_device_list(indexes, ndevs) < 0) {
		mperr("sis33dev_get_device_list");
		goto out;
	}

	for (i = 0; i < ndevs; i++) {
		char devname[32];
		int index = indexes[i];

		if (index == curr_index)
			printf("current");
		else
			printf("-");

		printf("\t%2d", index);
		if (sis33dev_get_devname(index, devname, sizeof(devname)) < 0) {
			fprintf(stderr, "dev%d: retrieval of 'devname' failed\n", index);
			continue;
		}
		printf("\t%8s", devname);

		if (sis33dev_get_attr(index, "description", value, sizeof(value))) {
			fprintf(stderr, "dev%d: retrieval of 'description' failed\n", index);
			continue;
		}
		printf("\t%s", value);

		n_channels = sis33dev_get_nr_subattrs(index, "channels");
		if (n_channels < 0) {
			fprintf(stderr, "dev%d: retrieval of 'n_channels' failed\n", index);
			continue;
		}
		printf("\t%2d", n_channels);

		if (sis33dev_get_attr(index, "n_bits", value, sizeof(value))) {
			fprintf(stderr, "dev%d: retrieval of 'n_bits' failed\n", index);
			continue;
		}
		printf("\t%2s", value);

		printf("\n");
	}
 out:
	free(indexes);
}

static int __h_device(int index)
{
	char file[SIS33_PATH_MAX];
	char devname[32];

	if (!sis33dev_device_exists(index)) {
		fprintf(stderr, "Device with index %d doesn't exist\n", index);
		return -TST_ERR_WRONG_ARG;
	}

	if (sis33fd > 0) {
		if (close(sis33fd))
			mperr("close");
		sis33fd = -1;
	}

	if (sis33dev_get_devname(index, devname, sizeof(devname)) < 0) {
		mperr("sis33dev_get_devname");
		return -TST_ERR_SYSCALL;
	}
	snprintf(file, sizeof(file), "/dev/%s", devname);
	file[sizeof(file) - 1] = '\0';
	sis33fd = open(file, O_RDWR, 0);
	if (sis33fd < 0) {
		mperr("open");
		return -TST_ERR_SYSCALL;
	} else {
		curr_index = index;
	}

	return 1;
}

int h_device(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Set device\n"
			"%s - Show available devices\n"
			"Format:\n"
			"curr\tindex\tdev_name\tdetailed_name\tn_channels\tn_bits\n"
			"%s [index]\n"
			"\tindex: set device to that given by the index\n",
			cmdd->name, cmdd->name, cmdd->name);
		return 1;
	}

	++atoms;
	if (atoms->type == Terminator) {
		show_devices();
		return 1;
	}

	if (atoms->type != Numeric)
		return -TST_ERR_WRONG_ARG;

	return __h_device(atoms->val);
}

int h_device_next(struct cmd_desc *cmdd, struct atom *atoms)
{
	int *indexes;
	int ndevs;
	int ret;
	int i;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - Set next available device\n", cmdd->name);
		return 1;
	}

	ndevs = sis33dev_get_nr_devices();
	if (!ndevs)
		return 1;
	indexes = calloc(ndevs, sizeof(*indexes));
	if (indexes == NULL) {
		mperr("calloc");
		return -TST_ERR_SYSCALL;
	}
	if (sis33dev_get_device_list(indexes, ndevs) < 0) {
		mperr("sis33dev_get_device_list");
		ret = -TST_ERR_SYSCALL;
		goto out;
	}

	for (i = 0; i < ndevs; i++) {
		if (indexes[i] == curr_index)
			break;
	}
	if (i == ndevs) {
		fprintf(stderr, "curr_index %d not in the devices' list. Has it been removed?\n", curr_index);
		ret = -TST_ERR_WRONG_ARG;
		goto out;
	}
	if (i == ndevs - 1)
		ret = 1;
	else
		ret = __h_device(indexes[i + 1]);
 out:
	free(indexes);
	return ret;
}

int h_plot(struct cmd_desc *cmdd, struct atom *atoms)
{
	char cmd[sizeof(gnuplot) + 128];
	int segment = curr_segment;
	int ret;
	int term = 0;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s [segment] [term] - Plot acquired data\n"
			"Note: the data must have previously been written to disk\n"
			"segment: segment number (-1 for current, default)\n"
			"term:\n"
			"\t0: Use 'dumb' terminal. Default.\n"
			"\t1: Use 'X11' terminal.\n",
			cmdd->name);
		return 1;
	}

	++atoms;
	if (atoms->type == Terminator)
		goto plot;
	if (atoms->type != Numeric)
		return -TST_ERR_WRONG_ARG;
	if (atoms->val < -1 || atoms->val >= SIS33T_MAX_NR_SEGMENTS)
		return -TST_ERR_WRONG_ARG;
	if (atoms->val >= 0)
		segment = atoms->val;

	++atoms;
	if (atoms->type == Numeric) {
		if (atoms->val != 0 && atoms->val != 1) {
			fprintf(stderr, "Invalid terminal type %d\n", atoms->val);
			return -TST_ERR_WRONG_ARG;
		}
		term = !!atoms->val;
	}
 plot:
	/* we need the data on disk in order to plot it */
	ret = write_samp_data_to_disk(segment);
	if (ret < 0)
		return ret;
	if (gnuplot[0] == '\0') {
		fprintf(stderr, "No data available\n");
		return 1;
	}

	snprintf(cmd, sizeof(cmd), "echo \"%s%s\" | gnuplot -persist",
		term ? "set term x11; " : "set term dumb; ", gnuplot);
	cmd[sizeof(cmd) - 1] = '\0';
	system(cmd);

	return 1;
}

int h_segments(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s [segment_nr] - Get/Set number of segments\n"
			"segment_nr: segment number to be set\n", cmdd->name);
		return 1;
	}
	return h_uattr(atoms, "n_segments");
}

int h_ts_divider(struct cmd_desc *cmdd, struct atom *atoms)
{
	int val;
	int ret;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s [value] - Get/Set event timestamping clock divider\n"
			"value: desired divider value\n", cmdd->name);
		return 1;
	}
	ret = h_attr(atoms, "event_timestamping_divider");
	if (ret < 0)
		return ret;
	ret = sis33dev_get_attr_int(curr_index, "event_timestamping_divider_max", &val);
	if (ret < 0) {
		mperr("get_attr");
		return ret;
	}
	printf("max: %d\n", val);
	return 1;
}

int main(int argc, char *argv[], char *envp[])
{
	char file[SIS33_PATH_MAX];
	char devname[32];
	int *indexes;
	int ndevs;

	/* try to control the first device */
	ndevs = sis33dev_get_nr_devices();
	if (!ndevs)
		goto out;
	indexes = calloc(ndevs, sizeof(*indexes));
	if (indexes == NULL) {
		mperr("calloc");
		goto out;
	}
	if (sis33dev_get_device_list(indexes, ndevs) < 0) {
		mperr("sis33dev_get_device_list");
		goto out_free;
	}
	if (sis33dev_get_devname(indexes[0], devname, sizeof(devname)) < 0) {
		mperr("sis33dev_get_devname");
		goto out_free;
	}
	snprintf(file, sizeof(file), "/dev/%s", devname);
	file[sizeof(file) - 1] = '\0';
	sis33fd = open(file, O_RDWR, 0);
	if (sis33fd < 0)
		mperr("open");
	else
		curr_index = indexes[0];
 out_free:
	free(indexes);
 out:
	return extest_init(argc, argv);
}

enum _tag_cmd_id {
	CmdACQ = CmdUSR,
	CmdCANCEL,
	CmdCH,
	CmdCH_NEXT,
	CmdCLKSRC,
	CmdCLKFREQ,
	CmdDEVICE,
	CmdDEVICE_NEXT,
	CmdFETCH,
	CmdOFFSET,
	CmdPLOT,
	CmdSEGMENTS,
	CmdSTART,
	CmdSTART_AUTO,
	CmdSTART_DELAY,
	CmdSTOP,
	CmdSTOP_AUTO,
	CmdSTOP_DELAY,
	CmdTRIG_EXT,
	CmdTS_DIVIDER,
	CmdWF,
	CmdLAST
};

struct cmd_desc user_cmds[] = {
	{ 1, CmdACQ,		"acq", "Start Acquisition", "segment nr_events ev_length [timeout_us]", 0,
				h_acq },
	{ 1, CmdCANCEL,		"cancel", "Cancel Acquisition", "", 0,
				h_cancel },

	{ 1, CmdCH,		"ch", "Get/Set Channel", "[0,n-1]", 0,
				h_ch },
	{ 1, CmdCH_NEXT,	"ch_next", "Next Channel", "", 0,
				h_ch_next },

	{ 1, CmdCLKSRC,		"clksrc", "Clock Source", "[src]", 0,
				h_clksrc },
	{ 1, CmdCLKFREQ,	"clkfreq", "Clock Frequency", "[Hz]", 0,
				h_clkfreq },

	{ 1, CmdDEVICE,		"device", "Set device", "[index]", 0,
				h_device },
	{ 1, CmdDEVICE_NEXT,	"device_next", "Set next device", "", 0,
				h_device_next },

	{ 1, CmdFETCH,		"fetch", "Fetch Acquired Data", "[segment] [timeout_us] [show_tstamps]", 0,
				h_fetch },

	{ 1, CmdOFFSET,		"offset", "Input Offset", "channel value", 0,
				h_offset },

	{ 1, CmdPLOT,		"plot", "Plot sampled data", "[segment] [term]", 0,
				h_plot },

	{ 1, CmdSEGMENTS,	"segments", "Get/Set number of segments", "[nr_segments]", 0,
				h_segments },

	{ 1, CmdSTART,		"start", "Start Sampling", "", 0,
				h_start },
	{ 1, CmdSTART_AUTO,	"start_auto", "Get/Set Autostart", "bool", 0,
				h_start_auto },
	{ 1, CmdSTART_DELAY,	"start_delay", "Start delay", "val", 0,
				h_start_delay },

	{ 1, CmdSTOP,		"stop", "Stop Sampling", "", 0,
				h_stop },
	{ 1, CmdSTOP_AUTO,	"stop_auto", "Enable Autostop", "bool", 0,
				h_stop_auto },
	{ 1, CmdSTOP_DELAY,	"stop_delay", "Stop delay", "val", 0,
				h_stop_delay },

	{ 1, CmdTRIG_EXT,	"trig_ext",	"En/Disable external trigger", "bool", 0,
				h_trig_ext },

	{ 1, CmdTS_DIVIDER,	"ts_divider",	"Get/Set the timestamp divider", "[value]", 0,
				h_ts_divider },

	{ 1, CmdWF,		"wf", "Write File", "[segment]", 0,
				h_wf },

	{ 0, }
};
