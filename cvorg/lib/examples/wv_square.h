#ifndef _CVORG_WV_SQUARE_H_
#define _CVORG_WV_SQUARE_H_

#include <stdint.h>

static inline void cvorg_square_init(uint16_t *wv, int len, int fullscale)
{
	int i;

	/* initialise the square test-wave */
	for (i = 0; i < len / 2; i++)
		wv[i] = fullscale;
	for (; i < len; i++)
		wv[i] = 0x0000;
}

#endif /* _CVORG_WV_SQUARE_H_ */
