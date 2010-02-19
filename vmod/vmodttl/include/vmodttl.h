
/**
 * @file vmodttl.h
 *
 * @brief VMODTTL I/O card definitions
 *
 * The VMOD-TTL mezzanine is a 20-bit parallel I/O card for
 * which this driver provides support if installed in a MOD-PCI carrier
 * board. So this we provide here definitions for easy access to hardware
 * registers on both MOD-PCI carrier and VMODTTL
 * 
 * Copyright (c) 2009 CERN
 * @author Juan David Gonzalez Cobas <dcobas@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifndef _VMOD_TTL_H_
#define _VMOD_TTL_H_

#include <mod-pci.h>

/* VMOD TTL registers */
#define VMOD_TTL_PORT_C		0x00	/**< rw: data register C (4 bit) */
#define VMOD_TTL_PORT_B		0x02	/**< rw: data register B (8 bit) */
#define VMOD_TTL_PORT_A		0x04	/**< rw: data register A (8 bit) */
#define VMOD_TTL_CONTROL	0x06	/**< rw: Z8536 control register */

/**
 * @brief access to mezzanine register addresses 
 * 	as mapped on MOD-PCI board
 */
#ifndef PP_TEST
struct vmodttl_registers {
	uint16_t* data_a;	/**< rw: data register C (8 bit) */
	uint16_t* data_b;       /**< rw: data register B (8 bit) */
	uint16_t* data_c;       /**< rw: data register A (4 bit) */
	uint16_t* control;      /**< rw: Z8536 control register */
};
#endif /* PP_TEST */

enum vmodttl_register_numbers {
	VMOD_TTL_CHANNEL_A = 0,
	VMOD_TTL_CHANNEL_B,
	VMOD_TTL_CHANNEL_C,
	VMOD_TTL_CHANNEL_CONTROL,
};

enum vmodttl_register_configs {
	VMOD_TTL_INPUT = 0,
	VMOD_TTL_OUTPUT,
};

/**
 * @brief driver's internal description of a register status
 *
 * Track is to be kept of the hardware configurarion of a register
 * (output or input, exclusively); an output register keeps track of the
 * last value written to it; and an input register need not keep any
 * state, because it can be read as needed and *never* ever has to be
 * written to. Never.
 */
struct vmodttl_register {
	int 	slot;		/**< MOD-PCI slot the card lives in (0..1) */
	enum vmodttl_register_numbers
		number;		/**< register to r/w from/to (0..2 = A..C) */
	enum vmodttl_register_configs
		hw_setting;	/**< register is only input or output */
	int data;               /**< register contents read or written */
};                             

#endif /* _VMODTTL_H_ */
