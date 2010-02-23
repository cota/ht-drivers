/**
 * \file libvd80.c
 * \brief VD80 access user library
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <libvmebus.h>
#include <libvd80.h>

unsigned int *regs;
struct vme_mapping regs_desc;

/**
 * \brief Start sampling
 *
 * \return 0 on success else 1 on a VME bus error.
 */
int vd80_cmd_start(void)
{
	regs[VD80_GCR1/4] |= swapbe32(VD80_COMMAND_START);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error starting sampling\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Stop sampling
 *
 * \return 0 on success else 1 on a VME bus error.
 */
int vd80_cmd_stop(void)
{
	regs[VD80_GCR1/4] |= swapbe32(VD80_COMMAND_STOP);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error stopping sampling\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Generate an internal trigger
 *
 * \return 0 on success else 1 on a VME bus error.
 */
int vd80_cmd_trig(void)
{
	regs[VD80_GCR1/4] |= swapbe32(VD80_COMMAND_TRIG);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error triggering sampling\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Start subsampling
 *
 * \return 0 on success else 1 on a VME bus error.
 */
int vd80_cmd_substart(void)
{
	regs[VD80_GCR1/4] |= swapbe32(VD80_COMMAND_SUBSTART);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error starting sub sampling\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Stop subsampling
 *
 * \return 0 on success else 1 on a VME bus error.
 */
int vd80_cmd_substop(void)
{
	regs[VD80_GCR1/4] |= swapbe32(VD80_COMMAND_SUBSTOP);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error stopping sub sampling\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Enable readout of the sample buffer
 * \param channel Channel to read samples from (1 to 16)
 *
 * \return 0 on success else 1 on a VME bus error.
 */
int vd80_cmd_read(unsigned int channel)
{
	regs[VD80_GCR1/4] |= swapbe32((VD80_COMMAND_READ |
				       ((channel << VD80_OPERANT_SHIFT) &
					VD80_OPERANT_MASK)));

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error setting READ for channel %d\n", channel);
		return 1;
	}

	return 0;
}

/**
 * \brief Record only a single sample
 *
 * \return 0 on success else 1 on a VME bus error.
 */
int vd80_cmd_single(void)
{
	regs[VD80_GCR1/4] |= swapbe32(VD80_COMMAND_SINGLE);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error setting SINGLE\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Clear the pre-trigger counter
 *
 * \return 0 on success else 1 on a VME bus error.
 */
int vd80_cmd_clear(void)
{
	regs[VD80_GCR1/4] |= swapbe32(VD80_COMMAND_CLEAR);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error setting CLEAR\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Set little or big endian samples recording
 * \param bigend When set, samples are stored in big endian ordering, else in
 *               little endian
 *
 * \return 0 on success else 1 on a VME bus error.
 */
int vd80_cmd_setbig(int bigend)
{
	if (bigend)
		regs[VD80_GCR2/4] |= swapbe32(VD80_BIGEND);
	else
		regs[VD80_GCR2/4] &= swapbe32(~VD80_BIGEND);	

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error setting samples endianess\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Enable/Disable generation of the test waveform
 * \param debug When set, enable generation of the test waveform, else disable
 *
 * \return 0 on success else 1 on a VME bus error.
 *
 * The test waveform consists in a digital ramp.
 */
int vd80_cmd_setdebug(int debug)
{
	if (debug)
		regs[VD80_GCR2/4] |= swapbe32(VD80_DBCNTSEL);
	else
		regs[VD80_GCR2/4] &= swapbe32(~VD80_DBCNTSEL);	

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error setting debug counter select\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Set the number of post-trigger samples
 * \param num Number of post-trigger samples to store
 *
 * \return 0 on success or -1 on a VME bus error.
 *
 * Set the number of samples recorded between the trigger and sampling end.
 */
int vd80_set_postsamples(unsigned int num)
{

	regs[VD80_TCR2/4] = swapbe32(num & 0x7ffff);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error setting POSTSAMPLES\n");
		return -1;
	}

	return 0;
}

/**
 * \brief Get the number of post-trigger samples
 *
 * \return the number of requested samples on success or -1 on a VME bus error.
 *
 * Get the number of post-trigger samples requested.
 */
int vd80_get_postsamples(void)
{
	int val = swapbe32(regs[VD80_TCR2/4]);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error reading POSTSAMPLES\n");
		return -1;
	}

	return val;
}

/**
 * \brief Get the number of pre-trigger samples
 *
 * \return the number of pre-trigger samples or -1 on a VME bus error.
 *
 * Get the number of samples recorded between the sampling start and the
 * trigger.
 */
int vd80_get_pre_length(void)
{
	int reg = swapbe32(regs[VD80_PTSR/4]);
	int val = (reg & VD80_PRESAMPLES_MASK) >>
		VD80_PRESAMPLES_SHIFT;

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error reading PRESAMPLES\n");
		return -1;
	}

	return val;
}

/**
 * \brief Get the number of post-trigger samples
 *
 * \return the number of post-trigger samples or -1 on a VME bus error.
 *
 * Get the number of samples recorded between the trigger and the sampling
 * end.
 */
int vd80_get_post_length(void)
{
	int reg = swapbe32(regs[VD80_TSR/4]);
	int val = (reg & VD80_ACTPOSTSAMPLES_MASK) >>
		VD80_ACTPOSTSAMPLES_SHIFT;

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error reading ACTPOSTSAMPLES\n");
		return -1;
	}

	return val;
}

/**
 * \brief Get the total shot length
 *
 * \return the total number of samples or -1 on a VME bus error.
 *
 * Get the total number of samples taken during a shot
 * (pre-samples + post-samples).
 */
int vd80_get_shot_length(void)
{
	int reg = swapbe32(regs[VD80_SSR/4]);
	int val = (reg & VD80_SHOTLEN_MASK) >>
		VD80_SHOTLEN_SHIFT;

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error reading SHOTLEN\n");
		return -1;
	}

	return val;
}

/**
 * \brief Set the readout starting sample
 * \param readstart READSTART value to set
 *
 * \return 0 on success else 1 on a VME bus error.
 *
 * \note READSTART counts in steps of 32 samples. Therefore the number of
 * samples is READSTART * 32.
 */
int vd80_set_readstart(unsigned int readstart)
{
	regs[VD80_MCR1/4] = swapbe32((readstart << VD80_READSTART_SHIFT) &
				     VD80_READSTART_MASK);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error setting READSTART\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Set the number of samples that must be read
 * \param readlen READLEN value to set
 *
 * \return 0 on success else 1 on a VME bus error.
 *
 * \note READLEN counts in steps of 32 samples. Therefore the number of samples
 * is (READLEN + 1) * 32 and the minimum number of samples is 32
 */
int vd80_set_readlen(unsigned int readlen)
{
	regs[VD80_MCR2/4] = swapbe32((readlen << VD80_READLEN_SHIFT) &
				     VD80_READLEN_MASK);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error setting READLEN\n");
		return 1;
	}

	return 0;
}

/**
 * \brief Get one sample from the sample memory
 *
 * 
 */
int vd80_get_sample(void)
{
	return regs[VD80_MEMOUT/4];
}

/**
 * \brief Probe and configure the VD80
 *
 * \return 0 on success else 1 on failure.
 *
 * This function probes and configures the VD80 using the CR/CSR address space.
 *
 * It performs the following operations:
 *
 * \li map the VME CR/CSR address space of the board
 * \li check we're talking to a VD80 board by checking the module ID
 * \li configure the function 0 Address Decode Compare register (\a ADER0) to
 *     map the operational registers on an A24/D32 address space at base
 *     address 0x600000. Used for operational registers access.
 * \li configure the function 1 Address Decode Compare register (\a ADER1) to
 *     map the operational registers on an A24-BLT/D32 address space at base
 *     address 0x600000. Used for DMA reading of the samples memory using
 *     VME block transfer.
 *
 * \note The VME CR/CSR address space is unmapped on function return.
 */
int vd80_map(void)
{
	int rc = 1;
	struct vme_mapping crcsr_desc;
	unsigned int *crcsr;
	int i;
	unsigned int val;
	char cr_sig[2];
	char board_id[5];
	char rev_id[5];
	unsigned int desc_offset;
	char board_desc[512];

	/*
	 * Open a mapping into the VD80 CR/CSR (0x2f) address space for
	 * configuring the board.
	 */
	memset(&crcsr_desc, 0, sizeof(struct vme_mapping));

	crcsr_desc.window_num = 0;
	crcsr_desc.am = VD80_SETUP_AM;
	crcsr_desc.read_prefetch_enabled = 0;
	crcsr_desc.data_width = VD80_SETUP_DW;
	crcsr_desc.sizeu = 0;
	crcsr_desc.sizel = VD80_SETUP_SIZE;
	crcsr_desc.vme_addru = 0;
	crcsr_desc.vme_addrl = VD80_SETUP_BASE;

	if ((crcsr = vme_map(&crcsr_desc, 1)) == NULL) {
		printf("Failed to map CR/CSR: %s\n", strerror(errno));
		return 1;
	}

	/* First check we're accessing a configuration ROM */
	cr_sig[0] = swapbe32(crcsr[VD80_CR_SIG1/4]) & 0xff;

	if (vme_bus_error_check(&crcsr_desc)) {
		printf("Bus error reading CR signature byte 0 at %08x\n",
		       VD80_SETUP_BASE + VD80_CR_SIG1);
		goto out;
	}

	cr_sig[1] = swapbe32(crcsr[VD80_CR_SIG2/4]) & 0xff;

	if (vme_bus_error_check(&crcsr_desc)) {
		printf("Bus error reading CR signature byte 1 at %08x\n",
		       VD80_SETUP_BASE + VD80_CR_SIG2);
		goto out;
	}

	if ((cr_sig[0] != 'C') && (cr_sig[1] != 'R')) {
		printf("Not a Configuration ROM at 0x%x (%08x %08x)\n",
		       VD80_SETUP_BASE, swapbe32(crcsr[VD80_CR_SIG1/4]),
		       swapbe32(crcsr[VD80_CR_SIG2/4]));
		goto out;
	}

	/* Try to read module ID - offset 0x30 */
	for (i = 0; i < VD80_CR_BOARD_ID_LEN; i++) {
		val = swapbe32(crcsr[VD80_CR_BOARD_ID/4 + i]);
		board_id[i] = (char)(val & 0xff);
	}
	board_id[4] = '\0';

	if (vme_bus_error_check(&crcsr_desc)) {
		printf("Bus error reading board ID\n");
		goto out;
	}

	for (i = 0; i < VD80_CR_REV_ID_LEN; i++) {
		val = swapbe32(crcsr[VD80_CR_REV_ID/4 + i]);
		rev_id[i] = (char)(val & 0xff);
	}
	rev_id[4] = '\0';

	if (vme_bus_error_check(&crcsr_desc)) {
		printf("Bus error reading revision ID\n");
		goto out;
	}

	if (strncmp(board_id, "VD80", 4)) {
		printf("No VD80 board found at base address 0x%x\n",
		       VD80_SETUP_BASE);
		goto out;
	}

	printf("Found board ID %s - revision ID: %s\n", board_id, rev_id);

	/* Read board string */
	desc_offset = 0;

	for (i = 0; i < VD80_CR_DESC_PTR_LEN; i++) {
		desc_offset <<= 8;
		desc_offset |= swapbe32(crcsr[VD80_CR_DESC_PTR/4 + i]) & 0xff;
	}

	desc_offset &= ~3;

	if (desc_offset != 0) {
		for (i = 0; i < VD80_UCR_BOARD_DESC_LEN; i++) {
			val = swapbe32(crcsr[0x1040/4 + i]);
			board_desc[i] = (char)(val & 0xff);

			if (board_desc[i] == '\0')
				break;
		}
		board_desc[320] = '\0';
	}

	if (vme_bus_error_check(&crcsr_desc)) {
		printf("Bus error reading board description\n");
		goto out;
	}

	printf("Board name: %s\n", board_desc);
	printf("\n");

	/* Configure ADER0 for A24 USER DATA access (AM=0x39 BA=0x600000) */
	crcsr[VD80_CSR_ADER0/4] = swapbe32((VD80_OPER_BASE >> 24) & 0xff);
	crcsr[VD80_CSR_ADER0/4 + 1] = swapbe32((VD80_OPER_BASE >> 16) & 0xff);
	crcsr[VD80_CSR_ADER0/4 + 2] = swapbe32((VD80_OPER_BASE >> 8) & 0xff);
	crcsr[VD80_CSR_ADER0/4 + 3] = swapbe32(VD80_OPER_AM * 4);

	if (vme_bus_error_check(&crcsr_desc)) {
		printf("Bus error setting ADER0\n");
		goto out;
	}

	/* Configure ADER1 for A24 USER block transfer (AM=0x3b BA=0x600000) */
	crcsr[VD80_CSR_ADER1/4] = swapbe32((VD80_OPER_BASE >> 24) & 0xff);
	crcsr[VD80_CSR_ADER1/4 + 1] = swapbe32((VD80_OPER_BASE >> 16) & 0xff);
	crcsr[VD80_CSR_ADER1/4 + 2] = swapbe32((VD80_OPER_BASE >> 8) & 0xff);
	crcsr[VD80_CSR_ADER1/4 + 3] = swapbe32(VD80_DMA_AM * 4);

	if (vme_bus_error_check(&crcsr_desc)) {
		printf("Bus error setting ADER1\n");
		goto out;
	}

	/*
	 * Clear address counter enable for function 1 to use it for block
	 * transfer of the sample memory.
	 */
	val = swapbe32(crcsr[VD80_CSR_FUNC_ACEN/4]) | 0x02;
	crcsr[VD80_CSR_FUNC_ACEN/4] = swapbe32(0);

	if (vme_bus_error_check(&crcsr_desc)) {
		printf("Bus error setting ACEN\n");
		goto out;
	}

	/* Enable the module */
	crcsr[VD80_CSR_BITSET/4] = swapbe32(VD80_CSR_ENABLE_MODULE);

	if (vme_bus_error_check(&crcsr_desc)) {
		printf("Bus error enabling module\n");
		goto out;
	}

	rc = 0;

out:
	if (vme_unmap(&crcsr_desc, 1))
		printf("Failed to unmap CR/CSR space: %s\n",
		       strerror(errno));

	return rc;
}

/**
 * \brief Map and initialize the VD80 for operation
 *
 * \return 0 on success else 1 on failure.
 *
 * This functions performs the necessary initializations to prepare the
 * board for sampling:
 *
 * \li map the operational registers into the user address space
 * \li clear any interrupts
 * \li disable interrupts
 * \li configure the sampling clock source to use the internal clock divided
 *     by 2 which yields a 100kHz sampling clock
 * \li configure the trigger source to use the internal trigger
 * \li set the number of post samples to 0
 * \li setup the pretrigger control to clear the pretrigger counter on a
 *     \a START command, to clear the \a PRESAMPLES counter on reading the
 *      pretrigger status register and to unse the sample clock for the
 *      pretrigger counter
 * \li enable generation of the test waveform
 *
 * \note Those settings are \b testing settings and must be adapted for
 *       operational use.
 *
 */
int vd80_init(void)
{
	int rc = 1;

	/* Open a mapping for the VD80 operational registers */
	memset(&regs_desc, 0, sizeof(struct vme_mapping));

	regs_desc.window_num = 0;
	regs_desc.am = VD80_OPER_AM;
	regs_desc.read_prefetch_enabled = 0;
	regs_desc.data_width = VD80_OPER_DW;
	regs_desc.sizeu = 0;
	regs_desc.sizel = VD80_OPER_SIZE;
	regs_desc.vme_addru = 0;
	regs_desc.vme_addrl = VD80_OPER_BASE;

	if ((regs = vme_map(&regs_desc, 1)) == NULL) {
		printf("Failed to map VD80 operational registers: %s\n",
		       strerror(errno));
		printf("  addr: %x size: %x am: %x dw: %d\n", VD80_OPER_BASE,
		       VD80_OPER_SIZE, VD80_OPER_AM, VD80_OPER_DW);
		return 1;
	}

	/* Clear interrupts */
	regs[VD80_GCR1/4] |= swapbe32(VD80_INTCLR | VD80_IOERRCLR);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error initializing GCR1\n");
		goto out;
	}

	/*
	 * Disable interrupts, set little endian mode and disable debug
	 * counter (Clear all bits).
	 */
	regs[VD80_GCR2/4] = 0;

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error initializing GCR2\n");
		goto out;
	}

	/*
	 * Set sampling clock:
	 *    - Internal clock
	 *    - No VXI clock output
	 *    - Divide mode
	 *    - Rising edge
	 *    - No 50 ohm termination
	 *    - Clkdiv = 2 (I want a 100kHz clock from the 200 kHz internal
	 *                  clock - which yields a 10 us sampling period)
	 */
	regs[VD80_CCR/4] = swapbe32(1 << VD80_CLKDIV_SHIFT);

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error initializing CCR\n");
		goto out;
	}

	/*
	 * Set trigger:
	 *    - Internal trigger
	 *    - No VXI trigger output
	 *    - No trigger output
	 *    - Rising edge
	 *    - No 50 ohm termination
	 *    - No trigger synchronization
	 */
	regs[VD80_TCR1/4] = 0;

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error initializing TCR1\n");
		goto out;
	}

	/* Set number of post samples to 0 */
	
	if (vd80_set_postsamples(0))
		goto out;

	/*
	 * Setup the Pretrigger Control Register:
	 *    - Clear pretrigger counter on START
	 *    - Clear PRESAMPLES on Pretrigger Status Register read
	 *    - Clock the pretrigger counter on the sample clock
	 */
	regs[VD80_PTCR/4] |= swapbe32(VD80_PSCNTCLRONSTART |
				      VD80_PSREGCLRONREAD |
				      VD80_PSCNTCLKONSMPL);
	

	if (vme_bus_error_check(&regs_desc)) {
		printf("Bus error initializing PTCR\n");
		goto out;
	}

	/* Enable generation of debug waveform */
	if ((rc = vd80_cmd_setdebug(1)))
		goto out;

	return 0;

out:
	if (vme_unmap(&regs_desc, 1))
		printf("Failed to unmap operational registers: %s\n",
		       strerror(errno));

	return rc;
}

/**
 * \brief Unmap the VD80 operational address space
 *
 * \return 0 on success else 1 on failure
 */
int vd80_exit(void)
{
	int rc = 0;

	if ((rc = vme_unmap(&regs_desc, 1)))
		printf("Failed to return_controler: %s\n", strerror(errno));

	printf("Unmapped VD80 registers\n");
	return rc;
}
