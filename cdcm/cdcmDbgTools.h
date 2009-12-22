/**
 * @file cdcmDbgTools.h
 *
 * @brief Various debug utilities and supplementary tools.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 27/03/2007
 *
 * Developer can use for timing measurements, hardware access tracking, using
 * VMETRO tracking utilities and definitions. Intended for debugging, device
 * driver tuning and timing measurements. VMOD-DOR module should be used as a
 * major debugging tool for timing measurements. Driver developer can use it
 * for debug purposes. Base module address should be correctly set before
 * using the modules. 
 *
 * @version $Id: cdcmDbgTools.h,v 1.2 2009/01/09 10:26:03 ygeorgie Exp $
 */
#ifndef _CDCM_DBG_TOOLS_H_INCLUDE_
#define _CDCM_DBG_TOOLS_H_INCLUDE_

/*--------------------- tools for VMOD-DOR tracking -------------------------*/
/* Dual Output Register (VMOD-DOR) */
union U_DORreg {
  unsigned short All; /* to access the line register, offset 0 */
  struct {
    unsigned char elt[1]; /* to access lines group, offset [0,1] */
  } group;
  struct {
    unsigned char elt[5]; /* to access nibble, offset [0..5] */
  } nibble;
} __attribute__((packed));

unsigned long vmod_dor_base;
volatile union U_DORreg *dor_ptr;

#define VMOD_DOR_INIT(ba)													 \
({																 \
  struct pdparam_master param; /* contains VME access parameters */								 \
  int value = 0;														 \
																 \
  bzero((char*)&param, sizeof(struct pdparam_master)); 										 \
  param.iack   = 1; /* no iack */												 \
  param.rdpref = 0; /* no VME read prefetch option */										 \
  param.wrpost = 0; /* no VME write posting option */										 \
  param.swap   = 1; /* VME auto swap option */											 \
  param.dum[0] = 0; /* window is sharable */											 \
																 \
  vmod_dor_base = (unsigned long)find_controller(ba, sizeof(union U_DORreg), 0x29, 0/*offset*/, 2/*size to test-read*/, &param); \
  if (vmod_dor_base == (unsigned long)(-1)) {											 \
    CPRNT(("fpipls: Cannot map VMOD-DOR device address space!\n"));								 \
    value = 0;															 \
  } else {															 \
    dor_ptr = (union U_DORreg*)vmod_dor_base;											 \
    value = 1;															 \
  }																 \
  value;															 \
})

#define VMOD_DOR_PULSE				\
do {						\
  dor_ptr->All = 0xff;				\
  dor_ptr->All = 0x00;				\
} while (0)

/*---------------------- tools for VMETRO tracking --------------------------*/
char      *vmetro_track_addr_1b; /* one   byte  addr ptr */
short     *vmetro_track_addr_2b; /* two   bytes addr ptr */
int       *vmetro_track_addr_4b; /* four  bytes addr ptr */
long long *vmetro_track_addr_8b; /* eight bytes addr ptr */

#define VMETRO_TRACK_R_1b(offset) (*(vmetro_track_addr_1b + offset))
#define VMETRO_TRACK_R_2b(offset) (*(vmetro_track_addr_2b + offset))
#define VMETRO_TRACK_R_4b(offset) (*(vmetro_track_addr_4b + offset))
#define VMETRO_TRACK_R_8b(offset) (*(vmetro_track_addr_8b + offset))

#define VMETRO_TRACK_W_1b(val, offset) (*(vmetro_track_addr_1b + offset) = val)
#define VMETRO_TRACK_W_2b(val, offset) (*(vmetro_track_addr_2b + offset) = val)
#define VMETRO_TRACK_W_4b(val, offset) (*(vmetro_track_addr_4b + offset) = val)
#define VMETRO_TRACK_W_8b(val, offset) (*(vmetro_track_addr_8b + offset) = val)

#endif /* _CDCM_DBG_TOOLS_H_INCLUDE_ */
