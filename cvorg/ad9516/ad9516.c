#include <math.h>

#include <ad9516.h>

/*
 * given a desired output frequency and constant f_vco, check if just playing
 * with the output dividers can suffice. If it does, then return 0. Otherwise,
 * return -1, keeping the best dividers found.
 */
static int ad9516_get_dividers(int freq, struct ad9516_pll *pll)
{
	double diff;
	double mindiff = 1e10;
	int dvco, d1, d2;
	int i, j, k;

	/*
	 * Note: this loop is sub-optimal, since sometimes combinations
	 * that lead to the same result are tried.
	 * For more info on this, check the octave (.m) files in /scripts.
	 */
	for (i = 2; i <= 6; i++) {
		for (j = 1; j <= 32; j++) {
			for (k = j; k <= 32; k++) {
				diff = (double)AD9516_VCO_FREQ / i / j / k - freq;
				if (fabs(diff) <= (double)AD9516_MAXDIFF)
					goto found;
				if (fabs(diff) < fabs(mindiff)) {
					mindiff = diff;
					dvco = i;
					d1 = j;
					d2 = k;
				}
			}
		}
	}
	pll->dvco = dvco;
	pll->d1 = d1;
	pll->d2 = d2;
	return -1;
found:
	pll->dvco = i;
	pll->d1 = j;
	pll->d2 = k;
	return 0;
}


/*
 * Here we consider the dividers fixed, and play with N, R to calculate
 * a satisfactory f_vco, given a certain required output frequency @freq
 */
static void ad9516_calc_pll_params(int freq, struct ad9516_pll *pll)
{
	double n, nr;
	/*
	 * What's the required N / R ?
	 * N / R = Dividers * freq / fref, where Dividers = dvco * d1 * d1
	 * and fref = AD9516_OSCILLATOR_FREQ
	 */
	nr = (double)pll->dvco * pll->d1 * pll->d2 * freq / AD9516_OSCILLATOR_FREQ;

	n = nr * pll->r;
	/*
	 * No need to touch R or P; let's just re-calculate B and A
	 */
	pll->b = ((int)n) / pll->p;
	pll->a = ((int)n) % pll->p;
}

static int ad9516_check_pll(struct ad9516_pll *pll)
{
	int err = 0;
	double fvco = (double)AD9516_OSCILLATOR_FREQ *
		(pll->p * pll->b + pll->a) / pll->r;

	if (pll->a > pll->b)
		err = -1;

	if (pll->a > 63)
		err = -1;

	if (pll->p != 16 && pll->p != 32)
		err = -1;

	if (pll->b <= 0 || pll->b > 8191)
		err = -1;

	if (pll->r <= 0 || pll->r > 16383)
		err = -1;

	if (fvco < 2.55e9 || fvco > 2.95e9)
		err = -1;

	return err;
}

int ad9516_fill_pll_conf(unsigned int freq, struct ad9516_pll *pll)
{
	if (!freq) {
		pll->a = 0;
		pll->b = 4375;
		pll->p = 16;
		pll->r = 1000;
		pll->dvco = 1;
		pll->d1 = 1;
		pll->d2 = 1;
		pll->external = 1;
		return 0;
	}

	/* initial sanity check */
	if (freq < AD9516_MINFREQ)
		return -1;

	/*
	 * Algorithm for adjusting the PLL
	 * 1. set f_vco = 2.8 GHz
	 *	N = (p * b) + a -> N = 70000
	 *	f_vco = f_ref * N / R = 40e6 * 70000 = 2.8e9 Hz
	 */
	pll->a = 0;
	pll->b = 4375;
	pll->p = 16;
	pll->r = 1000;
	pll->external = 0;

	/*
	 * 2. check if playing with the dividers we can achieve the required
	 *    output frequency (@freq) without changing the f_vco above.
	 *    If a exact match is not possible, then return the closest result.
	 */
	if (ad9516_get_dividers(freq, pll)) {
		/*
		 * 2.1 Playing with the dividers is not enough: tune the VCO
		 *     by adjusting the relation N/R.
		 */
		ad9516_calc_pll_params(freq, pll);
	}

	/* 3. Check the calculated PLL configuration is feasible */
	return ad9516_check_pll(pll);
}

