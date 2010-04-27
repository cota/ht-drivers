/*
 * PLL Configuration validator: check that the implemented algorithm
 * for configuring the AD9156's PLL is correct.
 *
 * Example:
 * ./validator minfreq step maxfreq, e.g.
 * ./validator 45e4    1e3  10e6
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <cvorg.h>
#include <ad9516.h>

/*
 * uncomment the line below if you want to keep track of the pair (freq, reladiff)
 * in a file (validation.dat)
 * #define FILE_STORE_RELADIFF
 */

#define TEST_MAXDIFF		AD9516_MAXDIFF
#define TEST_VCO_FREQ		AD9516_VCO_FREQ

struct validator_diff {
	double reladiff;
	double absdiff;
	double freq;
};

struct invalid {
	struct ad9516_pll pll;
	double fvco;
	double freq;
};

static struct validator_diff finalmax;
static nr_errors;

static int get_dividers(int freq, struct ad9516_pll *pll)
{
	double diff;
	double mindiff = 1e10;
	int dvco, d1, d2;
	int i, j, k;

	for (i = 2; i <= 6; i++) {
		for (j = 1; j <= 32; j++) {
			for (k = j; k <= 32; k++) {

				diff = (double)TEST_VCO_FREQ / i / j / k - freq;

				if (fabs(diff) <= (double)TEST_MAXDIFF)
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
	return 1;
found:
	pll->dvco = i;
	pll->d1 = j;
	pll->d2 = k;
	return 0;
}

static int first_approximation(int freq, struct ad9516_pll *pll)
{
	double nr, fvco;
	/*
	 * What's the required N / R ?
	 * N / R = Dividers * freq / fref, where Dividers = dvco * d1 * d1
	 * and fref = AD9516_OSCILLATOR_FREQ
	 */
	nr = (double) pll->dvco * pll->d1 * pll->d2 * freq / AD9516_OSCILLATOR_FREQ;

	/*
	 * No need to touch R or P; let's just re-calculate P and R
	 */
	nr *= pll->r; /* now nr is n */

	pll->b = ((int)nr) / pll->p;
	pll->a = ((int)nr) % 16;

	fvco = (double)AD9516_OSCILLATOR_FREQ *
		(pll->p * pll->b + pll->a) / pll->r;

	if (fvco < 2.55e9 || fvco > 2.95e9)
		return 1;

	return 0;
}

static void second_approx(int freq, struct ad9516_pll *pll)
{
#if 0
	double n = (double)pll->p * pll->b + pll->a;
	double nr;

	nr = n / pll->r;

	if (nr < 63.75) { /* decrease r */
		pll->r = n / 64;
	}
#endif
}

/*
 * Here we consider the dividers fixed, and play with N, R to calculate
 * a satisfactory f_vco, given a certain required output frequency @freq
 */
static void calc_pll_params(int freq, struct ad9516_pll *pll)
{
	if (first_approximation(freq, pll))
		second_approx(freq, pll);
}

static int fill_pll_conf(int freq, struct ad9516_pll *pll)
{
	/*
	 * Algorithm for adjusting the PLL
	 * 1. set f_vco = 2.8 GHz
	 *	N = (p * b) + a -> N = 70000
	 *	f_vco = f_ref * N / R = 40e6 * 70000 = 2.8e9
	 */
	pll->a = 0;
	pll->b = 4375;
	pll->p = 16;
	pll->r = 1000;

	/*
	 * 2. check if playing with the dividers, we can achieve the required
	 *    output frequency (@freq)
	 */
	if (get_dividers(freq, pll)) {
		/* 2.1 we're close to @freq, but need to play with f_vco */
		calc_pll_params(freq, pll);
	}

	return 0;
}

#ifdef FILE_STORE_RELADIFF
static void store_reladiff(double freq, double reladiff)
{
	FILE *fp = fopen("validation.dat", "a");

	fprintf(fp, "%g\t%g\n", freq, reladiff);

	fclose(fp);
}
#else
static void store_reladiff(double freq, double reladiff)
{
}
#endif

static void check_precision(double freq, struct ad9516_pll *pll)
{
	double fvco, reladiff, diff;

	fvco = (double)AD9516_OSCILLATOR_FREQ *
		(pll->p * pll->b + pll->a) / pll->r;

	diff = freq - fvco / pll->dvco / pll->d1 / pll->d2;
	reladiff = diff / freq;

	store_reladiff(freq, reladiff);

	if (fabs(reladiff) > fabs(finalmax.reladiff)) {
		finalmax.reladiff = reladiff;
		finalmax.freq = freq;
		finalmax.absdiff = diff;
	}
}

static void dump_pll(struct ad9516_pll *pll)
{
	fprintf(stderr, "PLL params: [P=%d, B=%d, A=%d, R = %d]\n"
		"\tdvco=%d, d1=%d, d2=%d\n",
		pll->p, pll->b, pll->a, pll->r, pll->dvco, pll->d1, pll->d2);
}

#define pll_warn(pll, format...) do {	\
	fprintf(stderr, format);	\
	dump_pll(pll);			\
	nr_errors++;			\
	} while (0)

static int pll_feasibility(struct ad9516_pll *pll)
{
	int err = 0;
	double fvco = (double)AD9516_OSCILLATOR_FREQ *
		(pll->p * pll->b + pll->a) / pll->r;

	if (pll->a > pll->b) {
		pll_warn(pll, "Warning: pll-a > pll->b\n");
		err = 1;
	}

	if (pll->a > 63) {
		pll_warn(pll, "Warning: pll->a out of range\n");
		err = 1;
	}

	if (pll->p != 16 && pll->p != 32) {
		pll_warn(pll, "Warning: pll-> p != {16,32}\n");
		err = 1;
	}

	if (pll->b <= 0 || pll->b > 8191) {
		pll_warn(pll, "Warning: b out of range\n");
		err = 1;
	}

	if (pll->r <= 0 || pll->r > 16383) {
		pll_warn(pll, "Warning: r out of range\n");
		err = 1;
	}

	if (fvco < 2.55e9 || fvco > 2.95e9) {
		pll_warn(pll, "Warning: fvco (%g) out of range\n", fvco);
		err = 1;
	}

	return err;
}

int main(int argc, char *argv[])
{
	double freq, minfreq, maxfreq, step;
	struct ad9516_pll pll;

	/* set defaults */
	minfreq = 416e3;
	maxfreq = 100e6;
	step = 1e3;
	if (--argc > 0) {
		int i;

		for (i = 1; i <= argc; i++) {
			argv++;
			switch (i) {
			case 1:
				minfreq = atof(*argv);
				break;
			case 2:
				step = atof(*argv);
				break;
			case 3:
				maxfreq = atof(*argv);
			default:
				break;
			}
		}
	}

	printf("CVORG's PLL Configuration Validator\n");
	printf("VCO Clock: 2.55GHz to 2.95GHz--minimum output freq: 416KHz\n");

	for (freq = minfreq; freq < maxfreq; freq += step) {
		fill_pll_conf(freq, &pll);
		check_precision(freq, &pll);
		if (pll_feasibility(&pll))
			fprintf(stderr, "Warning detected for freq %g\n", freq);
	}

	printf("Maximum discrepancy between required/output freq occurred "
		"at %g Hz, being %g Hz off (reldiff=%g%%)\n",
		finalmax.freq, finalmax.absdiff, finalmax.reladiff * 100);

	if (nr_errors) {
		printf("Errors occurred: %d\n", nr_errors);
		return 1;
	}

	return 0;
}
