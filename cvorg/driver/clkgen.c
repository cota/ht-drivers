/**
 * clkgen.c
 *
 * Control functions for the AD9516-O clock generator
 * Copyright (c) 2009 Emilio G. Cota <cota@braap.org>
 *
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <cdcm/cdcm.h>
#include <cdcm/cdcmBoth.h>
#include <cdcm/cdcmIo.h>

#include <cvorg_priv.h>
#include <cvorg_hw.h>

#include <ad9516.h>
#include <ad9516_priv.h>

/*
 * @todo separate 100% CVORG code from AD9516 code.
 */

/*
 * define this if you want to debug the SPI interface with the AD9516
 */
/* #define DEBUG_CVORG_SPI */

#define CVORG_AD9516_CALIB_DIVIDER	2
#define CVORG_AD9516_SPI_SLEEP_US	5

/**
 * clkgen_ok - check if the data is up to date
 *
 * @rval:	CVORG_CLKCTL register value
 * @addr:	address on the clockgen to be checked
 *
 * return 1 - if OK
 * return 0 - if not OK (data not up to date, or address mismatch)
 */
static int clkgen_ok(uint32_t rval, unsigned int addr)
{
	unsigned int readaddr = (rval & CVORG_CLKCTL_ADDR) >> 8;

	return rval & CVORG_CLKCTL_UP2DATE && readaddr == addr;
}

static uint32_t clkgen_rval(struct cvorg *cvorg, unsigned int addr)
{
	uint32_t val;
	int i;

	cvorg_writew(cvorg, CVORG_CLKCTL, CVORG_AD9516_OP_READ | (addr << 8));

	/* try a few times, although normally 5us should be enough */
	for (i = 0; i < 3; i++) {
		/* delay to allow the serial transfer to happen */
		usec_sleep(CVORG_AD9516_SPI_SLEEP_US);
		val = cvorg_readw(cvorg, CVORG_CLKCTL);
		if (clkgen_ok(val, addr))
			goto out;
	}
 out:
	return val;
}

static uint32_t clkgen_rd(struct cvorg *cvorg, unsigned int addr)
{
	uint32_t rval = clkgen_rval(cvorg, addr);

	return rval & 0xff;
}

/*
 * debug_clkgen_wr - debug a write-to-clockgen operation
 * If we cannot read back the data we've written, then print a debug message
 */
#ifdef DEBUG_CVORG_SPI
static void debug_clkgen_wr(struct cvorg *cvorg, unsigned int addr, uint8_t data)
{
	uint32_t rval = clkgen_rval(cvorg, addr);

	if (!clkgen_ok(rval, addr)) {
		unsigned raddr = (rval & CVORG_CLKCTL_ADDR) >> 8;
		unsigned rdata = rval & CVORG_CLKCTL_DATA;

		SK_INFO("SPI to AD9516-O Error: (expected/read) addr: 0x%0x"
			 "/0x%x, data=0x%0x/0x%x", addr, raddr, data, rdata);
	}
}
#else
static void debug_clkgen_wr(struct cvorg *cvorg, unsigned int addr, uint8_t data)
{
}
#endif /* DEBUG_CVORG_SPI */

/**
 * clkgen_check_write - check if the write operation is finished properly.
 *
 * @cvorg:	CVORG module
 *
 * return 0 - if OK
 * return -1 - if not OK (data not up to date)
 */
int clkgen_check_write(struct cvorg *cvorg)
{
	int val;
	int times = 10;

	do {
		val = cvorg_readw(cvorg, CVORG_CLKCTL);
		if (val & CVORG_CLKCTL_UP2DATE) {
			goto check_ok;
		}
		times--;
		usec_sleep(1);

	} while(times);

	/* Fail waiting for the flag */
	return -1;

check_ok:
	return 0;

}

/**
 * clkgen_wr - write to the CVORG's clock generator
 *
 * @cvorg:	CVORG module
 * @addr:	address of the AD9516-O to write to (10 bits)
 * @data:	data to be written (8 bits)
 */
static void clkgen_wr(struct cvorg *cvorg, unsigned int addr, uint8_t data)
{
	uint32_t cmd = CVORG_AD9516_OP_WRITE | (addr << 8) | data;

	cvorg_writew(cvorg, CVORG_CLKCTL, cmd);
	usec_sleep(CVORG_AD9516_SPI_SLEEP_US);
	if (clkgen_check_write(cvorg)) {
		SK_INFO("clkgen_wr : error writing on 0x%x, data 0x%x", addr, data);
		return;
	}

	debug_clkgen_wr(cvorg, addr, data);
}

static void clkgen_or(struct cvorg *cvorg, unsigned int addr, uint8_t data)
{
	uint32_t oldval = clkgen_rd(cvorg, addr) & 0xff;

	clkgen_wr(cvorg, addr, oldval | data);
}

static void clkgen_and(struct cvorg *cvorg, unsigned int addr, uint8_t data)
{
	uint32_t oldval = clkgen_rd(cvorg, addr) & 0xff;

	clkgen_wr(cvorg, addr, oldval & data);
}

/**
 * calib_vco_sleep - sleep until the calibration of the VCO has finished
 *
 * @cvorg:	CVORG module
 * @r:		configured parameter (divider) R of the AD9516's PLL
 * @freqref:	reference frequence (REFIN), in Hz
 *
 * The following math is derived from pg.40, 'VCO Calibration' of the datasheet
 *
 * fcal_clock = freqref / (R * cal_divider) [cycles/s]
 *	--> hence fcal_clock / 1000 = fcal_clock_ms [cycles/ms]
 *
 * 4400 calibration cycles are needed: how much is that in ms?
 *	--> 4400 [cycles] / fcal_clock [cycles/ms]
 *
 * re-arranging the operations to avoid overflow:
 *	--> calibration_time [ms] = 4400 * R * cal_divider / (fcal_clock_ms)
 *
 * return 0 - if the callibration was successful
 * return -1 - if the callibration didn't happen in time
 */
static int calib_vco_sleep(struct cvorg *cvorg, int r, int freqref)
{
	int interval;

	/* first convert the reference to cycles/ms, rounding up */
	freqref = freqref / 1000 + !!(freqref % 1000);

	/* calculate the calibration time interval in ms, rounding up */
	interval = 4400 * r * CVORG_AD9516_CALIB_DIVIDER;
	interval = interval / freqref + !!(interval % freqref);

	/* convert to tens of ms, rounding up */
	interval = interval / 10 + !!(interval % 10);

	/*
	 * sleep for, at least, 'interval' tens of ms.
	 * We add one to the number of ticks to ensure that in a worst
	 * case scenario (the first tick arrives immediately after tswait
	 * is called) we still wait for at least 'interval' tens of ms.
	 */
	tswait(NULL, SEM_SIGIGNORE, interval + 1);

	/* check for completion */
	return clkgen_rd(cvorg, AD9516_PLLREADBACK) & 0x40 ? 0 : -1;
}

static int clkgen_read_dvco(struct cvorg *cvorg)
{
	int dvco = clkgen_rd(cvorg, AD9516_VCO_DIVIDER);
	int inputclks = clkgen_rd(cvorg, AD9516_INPUT_CLKS);
	int retval;

	/* is it bypassed? */
	if (inputclks & 0x1)
		return 1;

	retval = (dvco & 0x7) + 2;

	if (retval >= 7) {
		SK_INFO("Suspicious dvco value: 0x%x", retval);
		retval = 1;
	}

	return retval;
}

/*
 * get the value of a divider
 * @nr - number of the divider (1 or 2)
 *
 * M - Low cycles divider
 * N - High cycles divider
 */
static int clkgen_read_d(struct cvorg *cvorg, int offset, int nr)
{
	int val;
	int bypass = clkgen_rd(cvorg, offset + 3);
	int m, n;

	/* sanity check */
	if (!WITHIN_RANGE(1, nr, 2)) {
		SK_WARN("Invalid divider nr: %d. Setting to 1", nr);
		nr = 1;
	}

	offset += nr - 1;
	val = clkgen_rd(cvorg, offset);

	m = (val & 0xf0) >> 4;
	n = val & 0x0f;

	bypass &= 1 << (4 + (--nr));

	if (bypass)
		return 1;

	return m + n + 2;
}

static int clkgen_read_a(struct cvorg *cvorg)
{
	return clkgen_rd(cvorg, AD9516_ACOUNT) & 0x3f;
}

static int clkgen_read_b(struct cvorg *cvorg)
{
	return	  clkgen_rd(cvorg, AD9516_BCOUNT_LSB) |
		((clkgen_rd(cvorg, AD9516_BCOUNT_MSB) & 0x1f) << 8);
}

static int clkgen_read_r(struct cvorg *cvorg)
{
	return	  clkgen_rd(cvorg, AD9516_RCOUNT_LSB) |
		((clkgen_rd(cvorg, AD9516_RCOUNT_MSB) & 0x3f) << 8);
}

static int clkgen_read_p(struct cvorg *cvorg)
{
	int prescaler = clkgen_rd(cvorg, AD9516_PLL1) & 0x7;
	int ret = 1;

	if (prescaler == 5)
		ret = 16;
	else if (prescaler == 6)
		ret = 32;
	else
		SK_INFO("Prescaler P not 16 nor 32");

	return ret;
}

/*
 * this one is not used at the moment, but might be useful in the future,
 * so we leave it around.
 */
void get_pll_conf(struct cvorg *cvorg, struct ad9516_pll *pll)
{
	/* get the PLL parameters */
	pll->a = clkgen_read_a(cvorg);
	pll->b = clkgen_read_b(cvorg);
	pll->r = clkgen_read_r(cvorg);
	pll->p = clkgen_read_p(cvorg);

	/* get non-pll dividers */
	pll->dvco = clkgen_read_dvco(cvorg);
	pll->d1 = clkgen_read_d(cvorg, AD9516_CMOSDIV3_1, 1);
	pll->d2 = clkgen_read_d(cvorg, AD9516_CMOSDIV3_1, 2);
}

/*
 * make the 'bypassed?' test easier: it will just check for !val.
 */
static void check_pll_dividers(struct ad9516_pll *pll)
{
	if (pll->r == 1)
		pll->r = 0;
	if (pll->dvco == 1)
		pll->dvco = 0;

	if (pll->d1 == 1)
		pll->d1 = 0;
	if (pll->d2 == 1)
		pll->d2 = 0;

	if (!pll->d1 && !pll->d2)
		return; /* the checks below don't apply */
	/*
	 * pg. 45 of the datasheet: if only one divider is bypassed,
	 * it must be d2.
	 */
	if (!pll->d1 && pll->d2) {
		pll->d1 = pll->d2;
		pll->d2 = 0;
		return; /* do not clash with the check below */
	}

	/*
	 * pg. 45: if only one divider has an even divide by, it must be d2.
	 */
	if (pll->d1 % 2 && !(pll->d2 % 2)) {
		int tmp = pll->d1;

		pll->d1 = pll->d2;
		pll->d2 = tmp;
	}
}

static void clkgen_write_a(struct cvorg *cvorg, int a)
{
	clkgen_wr(cvorg, AD9516_ACOUNT, a & 0x3f);
}

static void clkgen_write_b(struct cvorg *cvorg, int b)
{
	clkgen_wr(cvorg, AD9516_BCOUNT_LSB, b & 0xff);
	clkgen_wr(cvorg, AD9516_BCOUNT_MSB, (b >> 8) & 0x3f);
}

static void clkgen_write_r(struct cvorg *cvorg, int r)
{
	clkgen_wr(cvorg, AD9516_RCOUNT_LSB, r & 0xff);
	clkgen_wr(cvorg, AD9516_RCOUNT_MSB, (r >> 8) & 0x3f);
}

static void clkgen_write_p(struct cvorg *cvorg, int p)
{
	int prescaler = p == 16 ? 0x5 : 0x6; /* see the module's docs */

	/* reset P */
	clkgen_and(cvorg, AD9516_PLL1, ~0x07);

	/* write new P value */
	clkgen_or(cvorg, AD9516_PLL1, prescaler);
}

static void clkgen_write_dvco(struct cvorg *cvorg, struct ad9516_pll *pll)
{
	if (!pll->dvco) {
		if (pll->external) { /* bypassing is OK */
			clkgen_or(cvorg, AD9516_INPUT_CLKS, 0x1);
			return;
		} else {
			SK_WARN("Cannot bypass the VCO divider: setting to 2");
			pll->dvco = 2;
		}
	}

	if (!WITHIN_RANGE(2, pll->dvco, 6)) {
		SK_WARN("Invalid d_vco: %d ([2-6]). Setting to 2", pll->dvco);
		pll->dvco = 2;
	}

	/* Note: the (-2) should be there, check the datasheet */
	clkgen_wr(cvorg, AD9516_VCO_DIVIDER, (pll->dvco - 2) & 0x7);
	/* no bypass */
	clkgen_and(cvorg, AD9516_INPUT_CLKS, ~0x1);
}

/**
 * clkgen_write_d - write to memory the values of a CMOS divider
 *
 * @cvorg:	CVORG module
 * @divider:	3 or 4
 * @nr:		1 or 2
 * @d:		value of the divider to be written
 *
 * M - Low cycles divider
 * N - High cycles divider
 * More info in the datasheet: pg 44 'Channel Dividers - LVDS/CMOS Outputs'
 */
static void clkgen_write_d(struct cvorg *cvorg, int divider, int nr, int d)
{
	unsigned int m, n;
	unsigned int divider_addr, bypass_addr, bypass_mask;

	/* sanity checks */
	if (!WITHIN_RANGE(3, divider, 4)) {
		SK_INFO("Invalid divider: %d [3-4]. Ignoring.", divider);
		return;
	}
	if (!WITHIN_RANGE(1, nr, 2)) {
		SK_INFO("Invalid divider nr: %d [1-2]. Ignoring.", nr);
		return;
	}

	/* get the correct offsets within the AD9516 memory map */
	if (divider == 3) {
		bypass_addr = AD9516_CMOSDIV3_BYPASS;
		if (nr == 1) {
			divider_addr = AD9516_CMOSDIV3_1;
			bypass_mask = 0x10;
		} else {
			divider_addr = AD9516_CMOSDIV3_2;
			bypass_mask = 0x20;
		}
	} else {
		bypass_addr = AD9516_CMOSDIV4_BYPASS;
		if (nr == 1) {
			divider_addr = AD9516_CMOSDIV4_1;
			bypass_mask = 0x10;
		} else {
			divider_addr = AD9516_CMOSDIV4_2;
			bypass_mask = 0x20;
		}
	}

	/* bypass? */
	if (!d) {
		clkgen_or(cvorg, bypass_addr, bypass_mask);
		return;
	}

	d -= 2; /* D = M + N + 2 */
	m = d / 2 + d % 2; /* pg. 45: An odd D must be set as M = N + 1 */
	n = d / 2;
	clkgen_wr(cvorg, divider_addr, ((m & 0xf) << 4) | (n & 0xf));
	/* clear the bypass bit */
	clkgen_and(cvorg, bypass_addr, ~bypass_mask);
}

int clkgen_apply_pll_conf(struct cvorg *cvorg)
{
	int ret = 0;
	struct ad9516_pll *pll = &cvorg->pll;

	check_pll_dividers(pll);

	if (pll->external)
		clkgen_wr(cvorg, AD9516_INPUT_CLKS, 0x9);
	else
		clkgen_wr(cvorg, AD9516_INPUT_CLKS, 0x2);

	clkgen_write_a(cvorg, pll->a);
	clkgen_write_b(cvorg, pll->b);
	clkgen_write_r(cvorg, pll->r);
	clkgen_write_p(cvorg, pll->p);

	clkgen_write_dvco(cvorg, pll);

	clkgen_write_d(cvorg, 3, 1, pll->d1);
	clkgen_write_d(cvorg, 3, 2, pll->d2);
	clkgen_write_d(cvorg, 4, 1, pll->d1);
	clkgen_write_d(cvorg, 4, 2, pll->d2);

	clkgen_wr(cvorg, AD9516_PLL3,		0x00);
	clkgen_wr(cvorg, AD9516_UPDATE_ALL,	0x01);
	clkgen_wr(cvorg, AD9516_PLL3,		0x01);
	clkgen_wr(cvorg, AD9516_UPDATE_ALL,	0x01);

	if (calib_vco_sleep(cvorg, pll->r, AD9516_OSCILLATOR_FREQ)) {
		SK_DEBUG("VCO calibration failed [PLL: A=%d B=%d P=%d "
			"R=%d d1=%d d2=%d dvco=%d]", pll->a, pll->b,
			pll->p, pll->r, pll->d1, pll->d2, pll->dvco);
		pseterr(EAGAIN);
		ret = -1;
	}

	return ret;
}

int clkgen_check_pll(struct ad9516_pll *pll)
{
	int err = 0;

	if (pll->a > pll->b)
		err = 1;
	if (pll->a > 63)
		err = 1;
	if (pll->p != 16 && pll->p != 32)
		err = 1;
	if (pll->b <= 0 || pll->b > 8191)
		err = 1;
	if (pll->r <= 0 || pll->r > 16383)
		err = 1;

	if (err) {
		pseterr(EINVAL);
		return err;
	}
	return 0;
}

void clkgen_startup(struct cvorg *cvorg)
{
	/*
	 * PFD polarity 0, CP current 4.8 mA, CP mode: Normal,
	 * PLL Operating mode: normal
	 */
	clkgen_wr(cvorg, AD9516_PDF_CP,	0x7c);

	/* CP Normal, No reset counters, No B bypass, P = 16 @ DM mode */
	clkgen_wr(cvorg, AD9516_PLL1,	0x05);

	/* STATUS: N divider output, Antibackslash pulse width: 2.9ns */
	clkgen_wr(cvorg, AD9516_PLL2,	0x04);

	/* Do nothing on !SYNC, PLL Divider delays: 0 ps */
	clkgen_wr(cvorg, AD9516_PLL4,	0x00);

	/*
	 * REF{1,2} frequency threshold: valid if freq is above the higher
	 * freq threshold, LD pin: Digital lock detech (high = lock)
	 */
	clkgen_wr(cvorg, AD9516_PLL5,	0x00);

	/*
	 * enable VCO freq monitor, disable REF2 freqmon, enable REF1 freqmon,
	 * REFMON: status of VCO frequency (active high)
	 */
	clkgen_wr(cvorg, AD9516_PLL6,	0xab);

	/*
	 * enable switchover deglitch, select REF1 for PLL, manual reference
	 * switchover, return to REF1 automatically when REF1 status is good
	 * again, REF2 power on, REF1 power on, differential referente mode
	 */
	clkgen_wr(cvorg, AD9516_PLL7,	0x07);

	/*
	 * PLL status register enable, disable LD pin comparator, holdover
	 * disabled, automatic holdover mode
	 */
	clkgen_wr(cvorg, AD9516_PLL8,	0x00);

	/*
	 * PFD cycles to determine lock: 5, high range lock detect window,
	 * normal lock detect operation, VCO calibration divider: 2
	 */
	clkgen_wr(cvorg, AD9516_PLL3,	0x00);

	/* power down the LVPECL outputs, they're not used */
	clkgen_wr(cvorg, AD9516_LVPECL_OUT0,	0x0b);
	clkgen_wr(cvorg, AD9516_LVPECL_OUT1,	0x0b);
	clkgen_wr(cvorg, AD9516_LVPECL_OUT2,	0x0b);
	clkgen_wr(cvorg, AD9516_LVPECL_OUT3,	0x0b);
	clkgen_wr(cvorg, AD9516_LVPECL_OUT4,	0x0b);
	clkgen_wr(cvorg, AD9516_LVPECL_OUT5,	0x0b);

	/*
	 * Configure CMOS outputs
	 *
	 * Non-inverting, turn off CMOS B output, LVDS logic level,
	 * current level: 3.5 mA (100 m-ohm), power on
	 */
	clkgen_wr(cvorg, AD9516_LVCMOS_OUT6,	0x42);
	clkgen_wr(cvorg, AD9516_LVCMOS_OUT7,	0x42);
	/*
	 * Non-inverting, turn off CMOS B output, CMOS logic level,
	 * current level: 1.75 mA (100 m-ohm), power on
	 */
	clkgen_wr(cvorg, AD9516_LVCMOS_OUT8,	0x48);
	clkgen_wr(cvorg, AD9516_LVCMOS_OUT9,	0x48);
}
