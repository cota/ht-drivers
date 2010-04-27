/*
 * lock.c
 *
 * Claim ownership of a module. The module is unlocked when the process is
 * killed.
 */

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <libcvorg.h>
#include "my_stringify.h"

/* default module number (LUN) */
#define MODULE_NR	0

#define PROGNAME	"lock"

static int	module_nr = MODULE_NR;
static cvorg_t *device;

extern char *optarg;

static const char usage_string[] =
	"Claim ownership of a CVORG module\n"
	" " PROGNAME " [-h] [-m<LUN>]";

static const char commands_string[] =
	"options:\n"
	" -h = show this help text\n"
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
		c = getopt(argc, argv, "hm:");
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage_complete();
			exit(EXIT_SUCCESS);
		case 'm':
			module_nr = atoi(optarg);
			break;
		}
	}
}

static void sighandler(int sig)
{
	if (!device)
		exit(EXIT_FAILURE);

	/* unlock the device */
	if (cvorg_unlock(device) < 0) {
		cvorg_perror("cvorg_unlock");
		exit(EXIT_FAILURE);
	}

	/* close the handle */
	if (cvorg_close(device)) {
		cvorg_perror("cvorg_close");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

static void sighandler_register(void)
{
	struct sigaction act;

	act.sa_handler = sighandler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, 0);
	sigaction(SIGINT, &act, 0);
}

int main(int argc, char *argv[])
{
	parse_args(argc, argv);

	/* Get a handle. Don't care which channel we choose */
	device = cvorg_open(module_nr, CVORG_CHANNEL_B);
	if (device == NULL) {
		cvorg_perror("cvorg_open");
		exit(EXIT_FAILURE);
	}

	if (cvorg_lock(device) < 0) {
		cvorg_perror("cvorg_lock");
		exit(EXIT_FAILURE);
	}

	sighandler_register();

	while (1)
		sleep(10);

	exit(EXIT_FAILURE);
}
