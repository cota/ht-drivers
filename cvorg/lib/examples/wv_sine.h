#ifndef _CVORG_WV_SINE_H_
#define _CVORG_WV_SINE_H_

#include <stdint.h>
#include <math.h>

/*
 * ratio: 2 for 1 period, bigger for higher freqs
 */
static inline double __mysine(int len, int i, double ratio)
{
	return (sin(i * M_PI / (len / ratio)) + 1) / 2.0;
}

static void cvorg_sine_init(uint16_t *wv, int len, int fullscale)
{
	int i;

	for (i = 0; i < len; i++)
		wv[i] = (uint16_t)(__mysine(len, i, 2.0) * fullscale);
}

#endif /* _CVORG_WV_SINE_H_ */
