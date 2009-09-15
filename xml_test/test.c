/**
 * @file test.c
 *
 * @brief Parse XML installer file and build the module descriptors
 *
 * Test the library routine to build the module descriptor
 *
 * @author Copyright (C) 2009 CERN CO/HT Julian Lewis
 *
 * @date Created on 22/01/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libinst.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Some coloring */
#define RED_CLR	   "\033[0;31m"
#define WHITE_CLR  "\033[1;37m"
#define END_CLR	   "\033[m"

static void display_usage(char *name)
{
	printf("Usage: %s <xml_config_file> <module_name>\n\n"
	       "  <xml_config_file> %scompulsory%s\n"
	       "\tConfiguration file to check.\n\n"
	       "  <module_name> %soptional%s\n"
	       "\tModule name to get specific description.\n\n",
	       basename(name), WHITE_CLR, END_CLR, WHITE_CLR, END_CLR);
}


static int assert_xml_config_file(char *name)
{
	int fd = open(name, O_RDONLY, 0);
	if (fd < 0)
		return -1;
	close(fd);
	return 0;
}


int main(int argc, char *argv[], char *envp[])
{
	InsLibHostDesc *hostd = NULL;
	InsLibDrvrDesc *drvrd = NULL;

	if (argc == 1) {
		display_usage(argv[0]);
		exit(1);
	}

	if (assert_xml_config_file(argv[1])) {
		printf("Can't open configuration file '%s'\n", argv[1]);
		exit(1);
	}


	hostd = InsLibParseInstallFile(argv[1], 1);
	if (InsLibErrorCount) {
		printf("Parser: %d Errors detected\n", InsLibErrorCount);
		exit(EXIT_FAILURE);
	} else
		printf("No errors detected\n");


	InsLibPrintHost(hostd);
	InsLibFreeHost(hostd);

	if (argc == 3) {
		drvrd = InsLibGetDriver(hostd, argv[2]);
		if (drvrd)
			printf("\nGet [%s] driver description with %d"
			       " modules.\n\n", argv[2], drvrd->ModuleCount);
	}


	exit(0);
}
