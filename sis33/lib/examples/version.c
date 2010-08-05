/*
 * version.c
 *
 * print the version of the library
 */

#include <stdio.h>

#include <libsis33.h>
#include "my_stringify.h"

int main(int argc, char *argv[])
{
	printf("%s\n", libsis33_version ? libsis33_version : "Unknown");
	return 0;
}
