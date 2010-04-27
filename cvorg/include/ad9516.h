#ifndef _AD9516_H_
#define _AD9516_H_

#define AD9516_VCO_FREQ		2800000000UL
#define AD9516_MAXDIFF		0
#define AD9516_OSCILLATOR_FREQ	40000000UL

#define AD9516_MINFREQ		416000

/**
 * @brief PLL configuration structure
 * @a - [0-63]
 * @b - 13 bits [0-8191]
 * @p - 16 or 32
 * @r - 14 bits [0-16383]
 * @dvco - [1,6]
 * @d1 - [1,32]
 * @d2 - [1,32]
 * @external - set to 0 to use the internal PLL; 1 to use EXTCLK
 */
struct ad9516_pll {
	int a;
	int b;
	int p;
	int r;
	int dvco;
	int d1;
	int d2;
	int external;
};

#endif /* _AD9516_H_ */
