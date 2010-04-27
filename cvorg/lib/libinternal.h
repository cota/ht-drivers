#ifndef _CVORG_LIBINTERNAL_H_
#define _CVORG_LIBINTERNAL_H_

#include <libcvorg.h>
#include "cvorg_errno.h"

extern int __cvorg_loglevel;
extern int __cvorg_errno;
extern int __cvorg_init;

void __cvorg_libc_error(const char *string);
void __cvorg_internal_error(int error_number);

#define LIBCVORG_DEBUG(level, format, args...) \
	do { \
		if (__cvorg_loglevel >= (level)) {	\
			fprintf(stderr, "%s: " format,	\
				__func__, ##args);	\
		}					\
	} while (0)

#endif /* _CVORG_LIBINTERNAL_H_ */
