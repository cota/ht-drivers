#ifndef _SIS33_LIBINTERNAL_H_
#define _SIS33_LIBINTERNAL_H_

#include <libsis33.h>
#include "sis33_errno.h"

extern int __sis33_loglevel;
extern int __sis33_errno;
extern int __sis33_init;

void __sis33_internal_error(int err);
void __sis33_param_error(int err);
void __sis33_libc_error(const char *string);
void __sis33_lib_error(int err);

#define LIBSIS33_DEBUG(level, format, args...) \
	do { \
		if (__sis33_loglevel >= (level)) {	\
			fprintf(stderr, "%s: " format,	\
				__func__, ##args);	\
		}					\
	} while (0)

#endif /* _SIS33_LIBINTERNAL_H_ */
