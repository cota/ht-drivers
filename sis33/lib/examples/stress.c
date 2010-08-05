/*
 * stress.c
 *
 * Continuously fetch samples from all channels on an sis33 device
 */

#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <libsis33.h>
#include "my_stringify.h"

#define MODULE_NR	0
#define SEGMENT_NR	0
#define EV_LENGTH	64
#define NR_EVENTS	1

#define PROGNAME	"acquire"

static int		module_nr = MODULE_NR;
static unsigned int	segment_nr = SEGMENT_NR;
static unsigned int	ev_length = EV_LENGTH;
static unsigned int	nr_events = NR_EVENTS;
static float		duration;
static struct timeval	init_time;
static unsigned long long nr_transfers;
static unsigned long long nr_samples;

extern char *optarg;

static const char usage_string[] =
	"Fetch samples until a timeout expires or a signal arrives\n"
	" " PROGNAME " [e<EVENTS>] [-h] [-l<LENGTH>] [-m<LUN>] [-s<SEGMENT>] [-t<TIMEOUT>]";

static const char commands_string[] =
	"options:\n"
	" -e = number of events. Default: " my_stringify(NR_EVENTS) "\n"
	" -h = show this help text\n"
	" -l = event length (number of samples per event). Default: "
	my_stringify(EV_LENGTH) "\n"
	" -m = Module. Default: " my_stringify(MODULE_NR) "\n"
	" -s = segment. Default: " my_stringify(SEGMENT_NR) "\n"
	" -t = timeout (duration). 0 (default) to acquire forever\n";

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

static void parse_args(int argc, char *argv[])
{
	int c;

	for (;;) {
		c = getopt(argc, argv, "e:hl:m:s:t:");
		if (c < 0)
			break;
		switch (c) {
		case 'e':
			nr_events = strtoul(optarg, NULL, 0);
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
		case 's':
			segment_nr = strtoul(optarg, NULL, 0);
			break;
		case 't':
			if (!sscanf(optarg, "%g", &duration)) {
				fprintf(stderr, "Invalid duration\n");
				exit(EXIT_FAILURE);
			}
			break;
		}
	}
}

static inline void print_stats(void)
{
	printf("nr_samples:    %12llu\n"
	       "nr_transfers:  %12llu\n", nr_samples, nr_transfers);
}

static void sighandler(int sig)
{
	print_stats();
	exit(EXIT_SUCCESS);
}

static void sighandler_init(void)
{
	struct sigaction act;

	act.sa_handler = sighandler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, 0);
	sigaction(SIGINT, &act, 0);
}

static unsigned long long tv_subtract(struct timeval *t2, struct timeval *t1)
{
	return (t2->tv_sec - t1->tv_sec) * 1000000 + t2->tv_usec - t1->tv_usec;
}

static void check_duration(void)
{
	struct timeval now;

	if (!duration)
		return;
	gettimeofday(&now, 0);
	if (tv_subtract(&now, &init_time) > 1000000 * duration) {
		print_stats();
		exit(EXIT_SUCCESS);
	}
}

static void acquire_all_channels(sis33_t *dev, struct sis33_acq *acqs)
{
	int n_channels;
	int acq_events;
	int i;

	n_channels = sis33_get_nr_channels(dev);

	for (i = 0; i < n_channels; i++) {
		acq_events = sis33_fetch(dev, segment_nr, i, acqs, nr_events, NULL);
		if (acq_events < 0)
			exit(EXIT_FAILURE);
		nr_transfers++;
		nr_samples += nr_events * ev_length;
		check_duration();
	}
}

int main(int argc, char *argv[])
{
	struct sis33_acq *acqs;
	sis33_t *dev;
	unsigned int orig_ev_length;

	sighandler_init();
	parse_args(argc, argv);

	/* log errors */
	sis33_loglevel(3);

	dev = sis33_open(module_nr);
	if (dev == NULL)
		exit(EXIT_FAILURE);

	/* get a valid event length */
	orig_ev_length = ev_length;
	ev_length = sis33_round_event_length(dev, ev_length, SIS33_ROUND_NEAREST);
	if (ev_length != orig_ev_length)
		fprintf(stderr, "rounded event_length to %u\n", ev_length);
	acqs = sis33_acqs_zalloc(nr_events, ev_length);
	if (acqs == NULL)
		exit(EXIT_FAILURE);

	if (sis33_acq_wait(dev, segment_nr, nr_events, ev_length) < 0)
		exit(EXIT_FAILURE);

	gettimeofday(&init_time, NULL);
	while (1) {
		acquire_all_channels(dev, acqs);
	}

	sis33_acqs_free(acqs, nr_events);
	if (sis33_close(dev))
		exit(EXIT_FAILURE);
	return 0;
}
