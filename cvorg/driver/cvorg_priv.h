/*
 * cvorg_priv.h
 *
 * private CVORG definitions
 */

#ifndef _CVORG_PRIV_H_
#define _CVORG_PRIV_H

#include <skeldrvrP.h> /* for SkelDrvrModuleContext */
#include <cvorg.h>
#include <cdcm/cdcmBoth.h>
#include <cdcm/cdcmIo.h>

#include <general_both.h>

#include <ad9516.h>
#include <ad9516_priv.h>

/* define this to debug I/O accesses */
#undef CVORG_DEBUG_IO

/**
 * struct cvorg_channel - internal channel structure
 * @inpol:	input polarity
 * @outoff:	analog output offset
 * @outgain:	analog output gain
 * @reg_offset:	offset of the channel's registers within the @parent module.
 * @out_enabled:Set to 1 when the output is enabled. 0 otherwise.
 * @parent:	parent CVORG module
 */
struct cvorg_channel {
	int		inpol;
	int		outoff;
	int		outgain;
	unsigned int	reg_offset;
	int		out_enabled;
	struct cvorg	*parent;
};

/**
 * struct cvorg - description of a CVORG module
 * @hw_rev:	hardware revision number
 * @lock:	module's lock
 * @owner:	struct file that owns the module
 * @iomap:	kernel virtual address of the module's mapped I/O region
 * @pll:	PLL configuration
 * @channels:	array containing the contexts of the two channels
 */
struct cvorg {
	unsigned int		hw_rev;
	struct cdcm_mutex	lock;
	struct cdcm_file	*owner;
	void			*iomap;
	struct ad9516_pll	pll;
	struct cvorg_channel	channels[2];
};


static inline int __cvorg_check_perm(struct cvorg *cvorg, struct cdcm_file *file)
{
	return cvorg->owner && cvorg->owner != file;
}

static inline struct cvorg *cvorg_get(SkelDrvrClientContext *ccon)
{
	SkelDrvrModuleContext *mcon = get_mcon(ccon->ModuleNumber);

	return mcon->UserData;
}

static inline struct cvorg_channel
*cvorg_get_channel(SkelDrvrModuleContext *mcon, int nr)
{
	struct cvorg *cvorg = mcon->UserData;

	return &cvorg->channels[nr];
}

static inline struct cvorg_channel
*cvorg_get_channel_ccon(SkelDrvrClientContext *ccon)
{
	SkelDrvrModuleContext *mcon = get_mcon(ccon->ModuleNumber);

	return cvorg_get_channel(mcon, ccon->ChannelNr);
}

static inline uint32_t __cvorg_readw(struct cvorg *cvorg, unsigned int offset)
{
	return cdcm_ioread32be(cvorg->iomap + offset);
}

static inline uint32_t cvorg_readw(struct cvorg *cvorg, unsigned int offset)
{
	uint32_t value;

	value = __cvorg_readw(cvorg, offset);
#ifdef CVORG_DEBUG_IO
	SK_INFO("read:\t0x%8x @ 0x%8x", value, offset);
#endif
	return value;
}

static inline void
__cvorg_writew(struct cvorg *cvorg, unsigned int offset, uint32_t value)
{
	cdcm_iowrite32be(value, cvorg->iomap + offset);
}

static inline void
cvorg_writew(struct cvorg *cvorg, unsigned int offset, uint32_t value)
{
#ifdef CVORG_DEBUG_IO
	SK_INFO("write:\t\t0x%8x @ 0x%8x", value, offset);
#endif
	__cvorg_writew(cvorg, offset, value);
}

/* Read from a channel's register */
static inline uint32_t
__cvorg_rchan(struct cvorg_channel *channel, unsigned int offset)
{
	return __cvorg_readw(channel->parent, channel->reg_offset + offset);
}

static inline uint32_t
cvorg_rchan(struct cvorg_channel *channel, unsigned int offset)
{
	return cvorg_readw(channel->parent, channel->reg_offset + offset);
}

/* Write to a channel's register */
static inline void
__cvorg_wchan(struct cvorg_channel *channel, unsigned int offset, uint32_t value)
{
	return __cvorg_writew(channel->parent, channel->reg_offset + offset, value);
}

static inline void
cvorg_wchan(struct cvorg_channel *channel, unsigned int offset, uint32_t value)
{
	return cvorg_writew(channel->parent, channel->reg_offset + offset, value);
}

/* OR a channel's register */
static inline void
cvorg_ochan(struct cvorg_channel *channel, unsigned int offset, uint32_t value)
{
	uint32_t regval;

	regval = cvorg_rchan(channel, offset);
	cvorg_wchan(channel, offset, regval | value);
}

/* AND a channel's register */
static inline void
cvorg_achan(struct cvorg_channel *channel, unsigned int offset, uint32_t value)
{
	uint32_t regval;

	regval = cvorg_rchan(channel, offset);
	cvorg_wchan(channel, offset, regval & value);
}

/* Update only certain bits of a channel's register */
static inline void cvorg_uchan(struct cvorg_channel *channel,
			unsigned int offset, uint32_t value, uint32_t mask)
{
	cvorg_achan(channel, offset, ~mask);
	cvorg_ochan(channel, offset, value);
}

/* write to a channel's register, without swapping */
static inline void
cvorg_wchan_noswap(struct cvorg_channel *channel, unsigned int offset, uint32_t value)
{
	cdcm_iowrite32(value, channel->parent->iomap + channel->reg_offset + offset);
}

/* read from a channel's register, without swapping */
static inline uint32_t
cvorg_rchan_noswap(struct cvorg_channel *channel, unsigned int offset)
{
	return cdcm_ioread32(channel->parent->iomap + channel->reg_offset + offset);
}

/* Note: the clkgen functions that may fail shall set errno appropriately */
int clkgen_check_pll(struct ad9516_pll *pll);
void clkgen_startup(struct cvorg *cvorg);
int clkgen_apply_pll_conf(struct cvorg *cvorg);
void clkgen_get_pll_conf(struct cvorg *cvorg, struct ad9516_pll *pll);

#endif /* _CVORG_PRIV_H_ */
