/*
 * Redefine Linux macros for IOCTL's in Lynx -- which uses a BSD scheme
 */
#ifndef __linux__ /* define these for Lynx */

#ifndef _LYNX_GENERIC_IOCTL_H
#define _LYNX_GENERIC_IOCTL_H

#include <sys/ioctl.h>

/* direction */
#define _IOC_NONE  IOC_VOID
#define _IOC_READ  IOC_OUT
#define _IOC_WRITE IOC_IN

#define _IOC_DIR(nr) ((nr) & IOC_DIRMASK)

/* type */
#define _IOC_TYPE IOCGROUP

/* number */
#define _IOC_NR(nr) ((nr) & 0xff)

/* size */
#define _IOC_SIZE IOCPARM_LEN

/*
 * TYPECHECK -- provoke a compile error if the
 * size needs more than 13 bits
 */

/* provoke compile error for invalid uses of size argument */
extern unsigned int __invalid_size_argument_for_IOC;
#define _IOC_TYPECHECK(t) \
	((sizeof(t) == sizeof(t[1]) && sizeof(t) < (1 << 13)) ? \
	  sizeof(t) : __invalid_size_argument_for_IOC)

#undef _IOR
#define _IOR(g,n,t)	_IOC(IOC_OUT,	(g), (n), (_IOC_TYPECHECK(t)))
#undef _IOW
#define _IOW(g,n,t)	_IOC(IOC_IN,	(g), (n), (_IOC_TYPECHECK(t)))
#undef _IOWR
#define _IOWR(g,n,t)	_IOC(IOC_INOUT,	(g), (n), (_IOC_TYPECHECK(t)))


#endif /* _LYNX_GENERIC_IOCTL_H */

#endif /* !__linux__ */
