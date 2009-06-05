/**
 * @file cdcmBoth.h
 *
 * @brief helpers for both Linux and Lynx
 *
 * Copyright (c) 2006-2009 CERN
 * @author Yury Georgievskiy <yury.georgievskiy@cern.ch>
 *
 * Copyright (c) 2009 CERN
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#include "cdcmBoth.h"

#ifdef __linux__

/**
 * @brief copy data from user-space to kernel-space
 *
 * @param to   - destination address, in kernel space
 * @param from - source address, in user space
 * @param size - number of bytes to copy
 *
 * @return 0 - on success
 * @return SYSERR - on failure
 */
int cdcm_copy_from_user(void *to, void *from, int size)
{
	if (copy_from_user(to, from, size))
		return SYSERR;
	return 0;
}

/**
 * @brief copy data from kernel-space to user-space
 *
 * @param to   - destination address, in user space
 * @param from - source address, in kernel space
 * @param size - number of bytes to copy
 *
 * @return 0 - on success
 * @return SYSERR - on failure
 */
int cdcm_copy_to_user(void *to, void *from, int size)
{
	if (copy_to_user(to, from, size))
		return SYSERR;
	return 0;
}


#else /* __Lynx__ */

int cdcm_copy_from_user(void *to, void *from, int size)
{
	if (rbounds((unsigned long)from) < size)
		return SYSERR;
	memcpy(to, from, size);
	return 0;
}

int cdcm_copy_to_user(void *to, void *from, int size)
{
	if (wbounds((unsigned long) to) < size)
		return SYSERR;
	memcpy(to, from, size);
	return 0;
}

#endif /* !__linux__ */
