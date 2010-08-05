/*
 * two_segments.c
 *
 * Sample from two sis33 segments
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libsis33.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

#define PROGNAME	"two_segments"

static int		module_nr = MODULE_NR;
static unsigned int	ev_length = 64*1024;
extern char *optarg;

static const char usage_string[] =
	"Sample from two sis33 segments\n"
	" " PROGNAME " [-h] [-l<LENGTH>] [-m<LUN>]";

static const char commands_string[] =
	"options:\n"
	" -h = show this help text\n"
	" -l = event length (number of samples per event)\n"
	" -m = Module number (default: " my_stringify(MODULE_NR) ")";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "hl:m:");
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'l':
			ev_length = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			module_nr = strtol(optarg, NULL, 0);
			break;
		}
	}
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

static void print_acq_tv(const struct timeval *times, int i)
{
	static char datetime[26]; /* length imposed by ctime_r */
	time_t time;

	time = times[i].tv_sec;
	ctime_r(&time, datetime);
	datetime[sizeof(datetime) - 1] = '\0';
	remove_trailing_chars(datetime, '\n');
	printf("acq%d: %s +%06luus\n", i, datetime, times[i].tv_usec);
}

int main(int argc, char *argv[])
{
	struct sis33_acq *acqs[2];
	int acquired_events[2];
	struct timeval endtimes[2];
	sis33_t *dev;

	parse_args(argc, argv);

	/* verbose debugging */
	sis33_loglevel(4);

	dev = sis33_open(module_nr);
	if (dev == NULL)
		exit(EXIT_FAILURE);

	/* get a valid event length */
	ev_length = sis33_round_event_length(dev, ev_length, SIS33_ROUND_NEAREST);

	acqs[0] = sis33_acqs_zalloc(1, ev_length);
	if (acqs[0] == NULL)
		exit(EXIT_FAILURE);
	acqs[1] = sis33_acqs_zalloc(1, ev_length);
	if (acqs[1] == NULL)
		exit(EXIT_FAILURE);

	/*
	 * Acquire from segment 0, waiting for it to finish. Then start acquiring
	 * on segment 1, and at the same time, fetch from segment 0--without
	 * '_wait' because we know it finished already. Then fetch (waiting,
	 * because we don't know if the acquisition is still ongoing) from
	 * segment 1.
	 *
	 * Note the following:
	 * - The device can acquire only from one segment at a time
	 * - We cannot fetch samples from a segment that is sampling
	 */
	if (sis33_set_nr_segments(dev, 2) < 0)
		exit(EXIT_FAILURE);

	if (sis33_acq_wait(dev, 0, 1, ev_length) < 0)
		exit(EXIT_FAILURE);
	if (sis33_acq(dev, 1, 1, ev_length) < 0)
		exit(EXIT_FAILURE);

	acquired_events[0] = sis33_fetch(dev, 0, 0, acqs[0], 1, &endtimes[0]);
	if (acquired_events[0] < 0)
		exit(EXIT_FAILURE);
	acquired_events[1] = sis33_fetch_wait(dev, 1, 0, acqs[1], 1, &endtimes[1]);
	if (acquired_events[1] < 0)
		exit(EXIT_FAILURE);

	/*
	 * in this example we don't to anything with the acquired data, we just
	 * print the acquisition timestamps
	 */
	print_acq_tv(endtimes, 0);
	print_acq_tv(endtimes, 1);

	sis33_acqs_free(acqs[0], 1);
	sis33_acqs_free(acqs[1], 1);
	if (sis33_close(dev))
		exit(EXIT_FAILURE);

	return 0;
}
