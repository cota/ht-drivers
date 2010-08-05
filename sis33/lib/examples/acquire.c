/*
 * acquire.c
 *
 * Simple sis33 acquisition
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <libsis33.h>
#include "my_stringify.h"

#define MODULE_NR	0
#define SEGMENT_NR	0
#define CHANNEL_NR	0
#define EV_LENGTH	64
#define NR_EVENTS	1

#define PROGNAME	"acquire"

static int		module_nr = MODULE_NR;
static unsigned int	segment_nr = SEGMENT_NR;
static unsigned int	channel_nr = CHANNEL_NR;
static unsigned int	ev_length = EV_LENGTH;
static unsigned int	nr_events = NR_EVENTS;
static int		fetch = 1;
static int		acquire = 1;
static int		no_wait;
static int		show_tstamps;
extern char *optarg;

static const char usage_string[] =
	"Sample from two sis33 segments\n"
	" " PROGNAME "[-a] [-c<CHANNEL>] [-e<EVENTS>] [-f] [-h] [-l<LENGTH>] [-m<LUN>] [-n] [-s<SEGMENT>] [-t]";

static const char commands_string[] =
	"options:\n"
	" -a = Acquire samples only. No fetching done\n"
	" -c = channel. Default: " my_stringify(CHANNEL_NR) "\n"
	" -e = number of events. Default: " my_stringify(NR_EVENTS) "\n"
	" -f = Fetch samples only. No acquisition done\n"
	" -h = show this help text\n"
	" -l = event length (number of samples per event). Default: "
	my_stringify(EV_LENGTH) "\n"
	" -m = Module. Default: " my_stringify(MODULE_NR) "\n"
	" -n = No wait\n"
	" -s = segment. Default: " my_stringify(SEGMENT_NR) "\n"
	" -t = Fetch and only print timestamps (if supported)";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "ac:e:fhl:m:ns:t");
		if (c < 0)
			break;
		switch (c) {
		case 'a':
			acquire = 1;
			fetch = 0;
			break;
		case 'c':
			channel_nr = strtoul(optarg, NULL, 0);
			break;
		case 'e':
			nr_events = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			acquire = 0;
			fetch = 1;
			break;
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'l':
			ev_length = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			module_nr = strtol(optarg, NULL, 0);
			break;
		case 'n':
			no_wait = 1;
			break;
		case 's':
			segment_nr = strtoul(optarg, NULL, 0);
			break;
		case 't':
			acquire = 0;
			fetch = 1;
			show_tstamps = 1;
			break;
		}
	}
}

static inline void print_acq(const struct sis33_acq *acq)
{
	int i;

	for (i = 0; i < acq->nr_samples; i++) {
		printf("0x%x\n", acq->data[i]);
	}
}

static void print_acqs(const struct sis33_acq *acqs, int n_acqs)
{
	int i;

	for (i = 0; i < n_acqs; i++) {
		print_acq(&acqs[i]);
		printf("\n");
	}
}

static void print_timestamps(const struct sis33_acq *acqs, int n_acqs)
{
	int i;

	for (i = 0; i < n_acqs; i++) {
		printf("%4i 0x%llx\n", i, acqs[i].prevticks);
	}
}

int main(int argc, char *argv[])
{
	struct sis33_acq *acqs;
	int acq_events;
	sis33_t *dev;

	parse_args(argc, argv);

	/* log errors */
	sis33_loglevel(3);

	dev = sis33_open(module_nr);
	if (dev == NULL)
		exit(EXIT_FAILURE);

	/* get a valid event length */
	ev_length = sis33_round_event_length(dev, ev_length, SIS33_ROUND_NEAREST);

	if (acquire) {
		if (no_wait) {
			if (sis33_acq(dev, segment_nr, nr_events, ev_length) < 0)
				exit(EXIT_FAILURE);
		} else {
			if (sis33_acq_wait(dev, segment_nr, nr_events, ev_length) < 0)
				exit(EXIT_FAILURE);
		}
	}

	if (fetch) {
		acqs = sis33_acqs_zalloc(nr_events, ev_length);
		if (acqs == NULL)
			exit(EXIT_FAILURE);
		acq_events = sis33_fetch(dev, segment_nr, channel_nr, acqs, nr_events, NULL);
		if (acq_events < 0)
			exit(EXIT_FAILURE);
		if (show_tstamps) {
			if (sis33_event_timestamping_is_supported(dev))
				print_timestamps(acqs, acq_events);
			else
				fprintf(stderr, "This device does not support event timestamping\n");
		} else {
			print_acqs(acqs, acq_events);
		}
		sis33_acqs_free(acqs, nr_events);
	}

	if (sis33_close(dev))
		exit(EXIT_FAILURE);
	return 0;
}
