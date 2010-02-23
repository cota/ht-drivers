/**
 * \file libvd80.h
 * \brief VD80 access user library interface
 * \author Sébastien Dugué
 * \date 04/02/2009
 *
 * This library gives userspace applications access to the VD80 board.
 *
 * Copyright (c) 2009 \em Sébastien \em Dugué
 *
 * \par License:
 *      This program is free software; you can redistribute  it and/or
 *      modify it under  the terms of  the GNU General  Public License as
 *      published by the Free Software Foundation;  either version 2 of the
 *      License, or (at your option) any later version.
 *
 */

#ifndef _LIBVD80_H
#define _LIBVD80_H

#include <vd80.h>

/**
 * \name CR/CSR address space mapping parameters
 * \{
 */
/** CR/CSR base address */
#define VD80_SETUP_BASE	0x600000
/** CR/CSR size */
#define VD80_SETUP_SIZE	0x80000
/** CR/CSR access address modifier */
#define VD80_SETUP_AM	VME_CR_CSR
/** CR/CSR access data width */
#define VD80_SETUP_DW	VME_D32
/* \}*/

/**
 * \name Operational registers mapping parameters
 * \{
 */
/** Registers base address */
#define VD80_OPER_BASE	0x600000
/** Registers size */
#define VD80_OPER_SIZE	0x100
/** Registers access address modifier */
#define VD80_OPER_AM	VME_A24_USER_DATA_SCT
/** Registers access data width */
#define VD80_OPER_DW	VME_D32
/* \}*/

/**
 * \name Operational DMA parameters
 * \{
 */
/** DMA base address */
#define VD80_DMA_BASE	VD80_OPER_BASE + VD80_MEMOUT
/** DMA address modifier */
#define VD80_DMA_AM	VME_A24_USER_MBLT
/** DMA data width */
#define VD80_DMA_DW	VME_D32
/* \}*/

extern int vd80_cmd_start();
extern int vd80_cmd_stop();
extern int vd80_cmd_trig();
extern int vd80_cmd_substart();
extern int vd80_cmd_substop();
extern int vd80_cmd_read(unsigned int);
extern int vd80_cmd_single();
extern int vd80_cmd_clear();
extern int vd80_cmd_setbig(int);
extern int vd80_cmd_setdebug(int);
extern int vd80_set_postsamples(unsigned int);
extern int vd80_get_postsamples();
extern int vd80_get_pre_length();
extern int vd80_get_post_length();
extern int vd80_get_shot_length();
extern int vd80_set_readstart(unsigned int);
extern int vd80_set_readlen(unsigned int);
extern int vd80_get_sample();
extern int vd80_map();
extern int vd80_init();
extern int vd80_exit();

#endif /* _LIBVD80_H */
