/*
 * error.c
 * Error and log-level handling
 *
 * This code has been taken from comedilib:
 *
 *    COMEDILIB - Linux Control and Measurement Device Interface Library
 *    Copyright (C) 1997-2001 David A. Schleef <ds@schleef.org>
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation, version 2.1
 *    of the License.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *    USA.
 *
 *    Copyright (C) 1997-2001 David A. Schleef <ds@schleef.org>
 */

#include <stdio.h>
#include <string.h>

#include <general_both.h>

#include "libinternal.h"

extern int snprintf(char *s, size_t n, const char *format, ...);

char *__cvorg_error_strings[] = {
	"No error",
	"Invalid frequency",
};
#define LIBCVORG_NR_ERRCODES ARRAY_SIZE(__cvorg_error_strings)

int __cvorg_loglevel = 1;
int __cvorg_errno;

/**
 * @brief Change libcvorg logging properties
 * @param loglevel - loglevel to be set
 *
 * @return Previous loglevel
 *
 * This function controls the debugging output from libcvorg. By increasing
 * the loglevel, additional debugging information will be printed.
 * Error and debugging messages are printed to the stream stderr.
 *
 * The default loglevel can be set by using the environment variable
 * LICVORG_LOGLEVEL. The default loglevel is 1.
 *
 * The meaning of the loglevels is as follows:
 * loglevel = 0 libcvorg prints nothing
 *
 * loglevel = 1 (default) libcvorg prints error messages
 * when there is a self-consistency error (i.e., an internal bug.)
 *
 * loglevel = 2 libcvorg prints an error message when an invalid
 * parameter is passed.
 *
 * loglevel = 3 libcvorg prints an error message whenever an error is
 * generated in the libcvorg library or in the C library, when called by
 * libcvorg.
 *
 * loglevel = 4 libcvorg prints as much debugging info as it can
 */
int cvorg_loglevel(int loglevel)
{
	int old_loglevel = __cvorg_loglevel;

	if (old_loglevel >= 4 || loglevel >= 4) {
		fprintf(stderr, "%s: old_loglevel %d new %d\n",
			__func__, old_loglevel, loglevel);
	}
	__cvorg_loglevel = loglevel;

	return old_loglevel;
}

/**
 * @brief Retrieve the number of the last libcvorg error
 *
 * @return Most recent libcvorg error
 *
 * When a libcvorg function fails, an internal library variable stores the
 * error number, which can be retrieved with this function. See
 * cvorg_perror() and cvorg_strerror() to convert this number into
 * a text string.
 */
int cvorg_errno(void)
{
	return __cvorg_errno;
}

/**
 * @brief Convert a libcvorg error number into human-readable form
 * @param errnum - Error number to convert
 *
 * @return String describing the given error number
 *
 * See cvorg_errno() for further info.
 */
char *cvorg_strerror(int errnum)
{
	if (errnum < LIBCVORG_ERROR_BASE ||
		errnum >= LIBCVORG_ERROR_BASE + LIBCVORG_NR_ERRCODES) {

		return strerror(errnum);
	}

	return __cvorg_error_strings[errnum - LIBCVORG_ERROR_BASE];
}

/**
 * @brief Print a libcvorg error message
 * @param string - Prepend this string to the error message
 *
 * This function prints an error message obtained from the current value
 * of CVORG's error number.  See cvorg_errno() for further info.
 */
void cvorg_perror(const char *string)
{
	if (__cvorg_loglevel >= 3) {
		fprintf(stderr, "cvorg_perror(): __cvorg_errno=%d\n",
			__cvorg_errno);
	}

	if (!string)
		string = "libcvorg";

	fprintf(stderr, "%s: %s\n", string, cvorg_strerror(__cvorg_errno));
}

void __cvorg_libc_error(const char *string)
{
	__cvorg_errno = errno;

	if (__cvorg_loglevel >= 2) {
		char pre[32];

		snprintf(pre, sizeof(pre), "%s: libc error", string);
		pre[sizeof(pre) - 1] = '\0';
		cvorg_perror(pre);
	}
}

void __cvorg_internal_error(int err)
{
	__cvorg_errno = err;

	if (__cvorg_loglevel >= 2)
		cvorg_perror("internal error");
}
