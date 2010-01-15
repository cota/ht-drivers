/**
 * @file gp.c
 *
 * @brief Parse && adjust gnuplot data files
 *
 * User-interactive. Should only be used from test programs.
 *
 * @author Copyright (C) 2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 13/01/2010
 *
 * @section license_sec License
 *          Released under the GPL
 */
#define _GNU_SOURCE /* asprintf rocks */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>

/* directory management */
#ifdef __Lynx__
/* it is [direct] under lynx (outdated) */
#include <dir.h>
#else  /* linux */
/* it is [dirent] under linux (up-to-date) */
#include <sys/dir.h>
#endif
/* we'll use outdated [direct] because of lynx */

#include <gp.h>
#include <general_both.h>

#ifdef __Lynx__
extern int snprintf(char *str, size_t size, const char *format, ...);
#endif

/**
 * @brief Try to open *.gp gnuplot Table file
 *
 * @param dname -- dir to search for .gp files.
 *                 If NULL -- will search in $(cwd)/gp
 *
 * Returned pointer should be freed afterwards.
 *
 * @return NULL    -- FAILED
 * @return pointer -- SUCCESS
 */
#define MAX_GP_FILES 32
struct gpfd* open_gpf(char *dname)
{
        DIR *dir;
        struct direct *ent;
	char gpf[MAX_GP_FILES][MAX_GPFN_SZ] = { { 0 } }; /* search results */
	int fc = 0, i;
	char gpfn[MAX_GPFN_SZ] = { 0 }; /* vector table file name */
	char idx[MAX_GPFN_SZ] = { 0 }; /* vector table index */
	struct gpfd *gpfd = NULL;

	if (!dname)
		dname = "./gp";  /* will search in local ./gp dir */

	dir = opendir(dname);

	if (!dir) {
		printf("Can't open %s directory\n", dname);
		return NULL;
	}

	/* scan dir for .gp files */
        while ( (ent = readdir(dir)) && fc < MAX_GP_FILES)
                if (strstr(ent->d_name, ".gp"))
			strncpy(gpf[fc++], ent->d_name, sizeof(gpf[0]));
	closedir(dir);

 again:
	printf("Choose vector table index to load\n"
	       "or enter your own .gp filename to open:\n\n");
	for (i = 0; i < fc; i++)
		printf("[%d] -- %s\n", i, gpf[i]);

	printf("--> ");
	scanf("%[ 0-9]", idx);	   /* get index */
	scanf("%[ a-zA-Z]", gpfn); /* get user-provided filename */
	getchar();

	if (!gpfd)
		gpfd = calloc(1, sizeof(*gpfd));
	if (!gpfd) {
		printf("Can't allocate memory to store .gp file data\n");
		return NULL;
	}
	/* check, what user wants */
	if (strlen(idx)) {	/* user provide index */
		i = atoi(idx);
		if (!WITHIN_RANGE(0, i, fc)) {
			printf("index choosen is out-of-range\n");
			goto again;
		}
		snprintf(gpfd->gpfn, sizeof(gpfd->gpfn), "%s/%s", dname, gpf[i]);
	} else /* user-provided file */
		snprintf(gpfd->gpfn, sizeof(gpfd->gpfn), "%s/%s", dname, gpfn);

	return gpfd;
}

/**
 * @brief Load && adjust values from the *.gp data file
 *
 * @param fn -- already opened .gp filename
 *
 * Opened .gp file will be closed!1
 *
 * @return number of elements extracted
 */
int load_gpf(struct gpfd *gpfd)
{
	FILE *fd;
	int rc, i = 0;
	float fv[2];
	char  cv;
	struct gpv *data = gpfd->gpv;

        /* open vtf file */
        fd = fopen(gpfd->gpfn, "r");
        if (!fd) {
                printf("ERROR! Can't open [%s] Vector Table File for reading\n",
		       gpfd->gpfn);
		perror("fopen()");
                return 0;
        }

	/* get data from *.gp file into local structure */
        while ( (rc = fscanf(fd, "%f %f %c\n", &fv[0], &fv[1], &cv)) != EOF) {
                if (!rc) { /* skip matching failure */
                        rc = fscanf(fd, "%*[^\n]");
                        continue;
                }
		data[i].orig = fv[1];
		data[i].conv = data[i].orig * 1e+4;
                ++i;
		if (i == MAX_GP_EL)
			break;
        }

	fclose(fd); /* close .gp file */
	gpfd->elam = i;
	return gpfd->elam;
}

/**
 * @brief Creates file with adjusted float-2-short values in it
 *
 * @param gpf -- .gp filename
 * @param change -- if to modify filename with .conv suffix
 *                  1 -- yes
 *                  0 -- no
 *
 * Will create *.gp.conv file
 *
 * @return 1 -- ok
 * @return 0 -- failed
 */
int do_conv_gp_file(struct gpfd *gpfd, int change)
{
	char fn[MAX_GPFN_SZ + 3] = { 0 }; /* filename to open */
	FILE *f;
	int i, sz;

	if (change) {
		sz =  rindex(gpfd->gpfn, '.') - gpfd->gpfn;
		snprintf(fn, sizeof(fn), "%.*s.conv", sz, gpfd->gpfn);
	} else
		snprintf(fn, sizeof(fn), "%s", gpfd->gpfn);

	f = fopen(fn, "w");
	if (!f) {
		printf("Failed to open file [%s]\n", fn);
		perror("fopen()");
		return 0;
	}

	for (i = 0; i < gpfd->elam; i++)
		fprintf(f, "%hd\n", gpfd->gpv[i].conv);

	fclose(f);
	return 1;
}
