/*
 * error.c
 *
 * Error and log-level handling
 * This code has been adapted from comedilib:
 *
 *    COMEDILIB - Linux Control and Measurement Device Interface Library
 *    Copyright (C) 1997-2001 David A. Schleef <ds@schleef.org>
 *    License: GPL v2.1.
 *
 * Copyright (c) 2010 Emilio G. Cota <cota@braap.org>
 */

#include <stdio.h>
#include <string.h>

#include <general_both.h>

#include "libinternal.h"

char *__sis33_error_strings[] = {
	"No error",
	"Invalid argument",
	"No such device",
	"Feature not supported",
};
#define LIBSIS33_NR_ERRCODES ARRAY_SIZE(__sis33_error_strings)

int __sis33_loglevel = 1;
int __sis33_errno;

/**
 * @brief Change libsis33 logging properties
 * @param loglevel - loglevel to be set
 *
 * @return Previous loglevel
 *
 * This function controls the debugging output from libsis33. By increasing
 * the loglevel, additional debugging information will be printed.
 * Error and debugging messages are printed to the stream stderr.
 *
 * The default loglevel can be set by using the environment variable
 * LIBSIS33_LOGLEVEL. The default loglevel is 1.
 *
 * The meaning of the loglevels is as follows:
 * loglevel = 0 libsis33 prints nothing
 *
 * loglevel = 1 (default) libsis33 prints error messages
 * when there is a self-consistency error (i.e., an internal bug.)
 *
 * loglevel = 2 libsis33 prints an error message when an invalid
 * parameter is passed.
 *
 * loglevel = 3 libsis33 prints an error message whenever an error is
 * generated in the libsis33 library or in the C library, when called by
 * libsis33.
 *
 * loglevel = 4 libsis33 prints as much debugging info as it can
 */
int sis33_loglevel(int loglevel)
{
	int old_loglevel = __sis33_loglevel;

	if (old_loglevel >= 4 || loglevel >= 4) {
		fprintf(stderr, "%s: old_loglevel %d new %d\n",
			__func__, old_loglevel, loglevel);
	}
	__sis33_loglevel = loglevel;

	return old_loglevel;
}

/**
 * @brief Retrieve the number of the last libsis33 error
 *
 * @return Most recent libsis33 error
 *
 * When a libsis33 function fails, an internal library variable stores the
 * error number, which can be retrieved with this function. See
 * sis33_perror() and sis33_strerror() to convert this number into
 * a text string.
 */
int sis33_errno(void)
{
	return __sis33_errno;
}

/**
 * @brief Convert a libsis33 error number into human-readable form
 * @param errnum - Error number to convert
 *
 * @return String describing the given error number
 *
 * See sis33_errno() for further info.
 */
char *sis33_strerror(int errnum)
{
	if (errnum < LIBSIS33_ERROR_BASE ||
	    errnum >= LIBSIS33_ERROR_BASE + LIBSIS33_NR_ERRCODES) {
		return strerror(errnum);
	}
	return __sis33_error_strings[errnum - LIBSIS33_ERROR_BASE];
}

/**
 * @brief Print a libsis33 error message
 * @param string - Prepend this string to the error message
 *
 * This function prints an error message obtained from the current value
 * of SIS33's error number.  See sis33_errno() for further info.
 */
void sis33_perror(const char *string)
{
	if (!string)
		string = "libsis33";
	fprintf(stderr, "%s: %s\n", string, sis33_strerror(__sis33_errno));
}

void __sis33_internal_error(int err)
{
	__sis33_errno = err;
	if (__sis33_loglevel >= 1)
		sis33_perror("internal error");
}

void __sis33_param_error(int err)
{
	__sis33_errno = err;
	if (__sis33_loglevel >= 2)
		sis33_perror("parameter error");
}

void __sis33_libc_error(const char *string)
{
	__sis33_errno = errno;
	if (__sis33_loglevel >= 3) {
		char pre[64];

		snprintf(pre, sizeof(pre) - 1, "%s: libc error", string);
		pre[sizeof(pre) - 1] = '\0';
		sis33_perror(pre);
	}
}

void __sis33_lib_error(int err)
{
	__sis33_errno = err;
	if (__sis33_loglevel >= 3)
		sis33_perror("libsis33 error");
}
