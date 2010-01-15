/**
 * @file gp.h
 *
 * @brief gnuplot data files parsing API functions
 *
 * <long description>
 *
 * @author Copyright (C) 2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 13/01/2010
 *
 * @section license_sec License
 *          Released under the GPL
 */
#ifndef _GP_H_INCLUDE_
#define _GP_H_INCLUDE_

#define MAX_GPFN_SZ 128 /* maximum .gp gnuplot filename size */
#define MAX_GP_EL 1024	/* MAX elements in .gp file */
/* holds original && converted gnuplot values */
struct gpv {
	float orig; /* original */
	short conv; /* converted */
};

/* gnuplot file data */
struct gpfd {
	char gpfn[MAX_GPFN_SZ];	   /* gnuplot filename */
	int  elam;		   /* number of valid elements */
	struct gpv gpv[MAX_GP_EL]; /* element data */
};

/* API functions */
struct gpfd* open_gpf(char *);
int          load_gpf(struct gpfd *);
int          do_conv_gp_file(struct gpfd *, int);

#endif	/* _GP_H_INCLUDE_ */
