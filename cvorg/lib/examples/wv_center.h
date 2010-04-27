#ifndef _CVORG_WV_CENTER_H_
#define _CVORG_WV_CENTER_H_

#include <stdint.h>

/*
 * Move up a wave so that its middle point is at the middle of the
 * resulting output.
 */
static inline void cvorg_center_output(uint16_t *wv, int len, int fullscale)
{
	int i;

	if (fullscale == 0xffff)
		return;

	for (i = 0; i < len; i++)
		wv[i] += (0xffff - fullscale) / 2;
}

#endif /* _CVORG_WV_CENTER_H_ */
