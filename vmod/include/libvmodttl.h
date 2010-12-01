/*-
 * Copyright (c) 2010 Samuel I. Gonsalvez <siglesia@cern.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*! \file */
/*!
 *  \mainpage VMOD-TTL Device Driver
 *  \author Samuel Iglesias Gonsalvez BE/CO/HT
 *  \version 21 September 2010
 *
 * The VMOD-TTL hardware module is an opto-isolated digital input/output
 * with 20 bit TTL-level. There are three channels: two of 8 bits and one of 4 bits.
 * This driver exposes a minimum functionality of the boards capabilities.
 * More functions can be added later if needed.
 *
 * In this version of the driver the following capabilities are provided.
 *
 * 1) Open/Close the device.
 * 
 * 2) Setup the channels: direction, logic, time of the data strobe, and the working mode: open 
 * drain/collector for output channels**. 
 * <b>NOTICE: It must be done before any read/write operation.</b>
 * 
 * 3) Read/Write in each channel.
 *
 * 4) Data strobe in channel C:
 *
 * <table align="center" cellspacing="2" cellpadding="2" border="1">
 * <tr>
 *   <td colspan="4" align="center"><strong>Data Strobe (Channel C)</strong></td>
 * </tr>

 * <tr>
 * 	<td align="center">Channel</td>
 * 	<td align="center">Operation</td>
 * 	<td align="center">Value</td>
 * </tr>
 * <tr>
 * 	<td align="center">A</td>
 * 	<td align="center">Read</td>
 * 	<td align="center">0x4</td>
 * </tr>
 * <tr>
 * 	<td align="center">B</td>
 * 	<td align="center">Read</td>
 * 	<td align="center">0x8</td>
 * </tr>
 * <tr>
 * 	<td align="center">A</td>
 * 	<td align="center">Write</td>
 * 	<td align="center">0x1</td>
 * </tr>
 * <tr>
 * 	<td align="center">B</td>
 * 	<td align="center">Write</td>
 * 	<td align="center">0x2</td>
 * </tr>
 * <tr>
 * 	<td align="center">A and B</td>
 * 	<td align="center">Write</td>
 * 	<td align="center">0x3</td>
 * </tr> </table>
 *
 * * Only for channel A and B, because channel C is configured as output for data strobe purpose.
 *
 * ** An input channel is configured as normal mode.
 * 
 */

#ifndef __LIBVMODTTL
#define __LIBVMODTTL

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_BITS 8

/*!  
 * This enum is used in struct vmodttl_config to select the direction of each channel.
 * It can be for input or for output.
 */
enum vmodttl_chan_conf{
	CHAN_IN = 0, /*!< Indicates that is an input channel */ 
	CHAN_OUT /*!< Indicates that is an output channel */
};

/*!  
 * This enum is used in vmodttl_pattern to configure the bits' pattern in each channel.
 * It can be for input or for output.
 */
enum vmodttl_conf_pattern{
	OFF = 0,
	ANY,
	ZERO,
	ONE,
	ONE_TO_ZERO,
	ZERO_TO_ONE	
};


/*!  
 * This enum is used to select the channel.
 * Can be possible to select one of the three channels available in the board.
 */
enum vmodttl_channel{
	 
	VMOD_TTL_CHANNEL_A, /*!< Channel A (8 bits) */
	VMOD_TTL_CHANNEL_B, /*!< Channel B (8 bits) */
	VMOD_TTL_CHANNELS_AB /*! Channels A and B (16 bits) */
};

/*!  
 * This enum is used to select the mode of the channel.
 */
enum vmodttl_chan_mode{
	TTL = 0, /*!< Indicates if the channel is defined as ttl (only for output) */
	OPEN_COLLECTOR /*!< Indicates if the channel is defined as open collector (only for output) */
};
	

/*!  
 * This enum is used to select logic used in all the channels.
 */
enum vmodttl_chan_logic{
	NON_INVERT = 0, /*!< Non-inverting logic */
	INVERT 		/*!< Inverting logic */
};

/*!  
 * This enum is used to select the pattern mode for each channel.
 */
enum vmodttl_pattern_mode{
	OR = 0, /*!< OR mode. There will be an IRQ if one bit matches its pattern. */
	AND 	/*!< AND mode. There will be an IRQ if all bits match their pattern at the same time. */
};

/*! 
 * This struct is used as argument in vmodttl_io_conf to setup each channel 
 */
struct vmodttl_config {
	enum vmodttl_chan_conf 		dir_a; /*!< Configure the direction of channel A. */
	enum vmodttl_chan_conf 		dir_b; /*!< Configure the direction of channel B. */
	enum vmodttl_chan_logic		inverting_logic; /*!< Select invert/non-invert logic for all the channels */
	int 				us_pulse; /*!< Time of the data strobe's pulse (us). Minimum 1 us */
	enum vmodttl_chan_mode		mode_a; /*!< Configure the mode of the channel A (TTL/open collector) */
	enum vmodttl_chan_mode		mode_b; /*!< Configure the mode of the channel B (TTL/open collector) */
	enum vmodttl_chan_mode		mode_c; /*!< Configure the mode of the channel C (TTL/open collector) */
	enum vmodttl_pattern_mode	pattern_mode_a; /*!< Configure the pattern mode of the channel A */
	enum vmodttl_pattern_mode	pattern_mode_b; /*!< Configure the pattern mode of the channel B */
	enum vmodttl_conf_pattern	conf_pattern_a[NUM_BITS]; /*!< Configure the bit pattern of the channel A */
	enum vmodttl_conf_pattern	conf_pattern_b[NUM_BITS]; /*!< Configure the bit pattern of the channel B */
};

//! Open the device.
/*!
  \param lun an integer argument to select the device.
  \return returns 0 on success. On error, -1 is returned, and errno is set appropriately.
*/
int vmodttl_open(int lun);

//! Close the device.
/*!
  \param lun an integer argument to select the device.
  \return returns 0 on success. On error, -1 is returned, and errno is set appropriately.
*/
int vmodttl_close(int lun);

//! Configuration of each channel.
/*!
  \brief <b>NOTICE: It must be done after a vmodttl_open and before any read/write.</b>
  \param lun an integer argument to select the device.
  \param conf a struct vmodttl_config to setup the desired configuration.
  \return returns 0 on success. On error, negative number is returned, and errno is set appropriately.
*/
int vmodttl_io_config(int lun, struct vmodttl_config conf);

//! Configuration of each channel.
/*!
  \brief <b>NOTICE: It must be done after a vmodttl_open and before any read/write.</b>
  \param lun an integer argument to select the device.
  \param chan an enum vmodttl_channel to select the desired channel.
  \param conf a struct vmodttl_config to setup the desired configuration.
  \return returns 0 on success. On error, negative number is returned, and errno is set appropriately.
*/
int vmodttl_io_chan_config(int lun, enum vmodttl_channel chan, struct vmodttl_config conf);

//! Write a value to a channel.
/*!
  \param lun an integer argument to select the device.
  \param chan an enum vmodttl_channel to select the desired channel.
  \param val the integer value that have to be written.
  \return returns 0 on success. On error, -1 is returned, and errno is set appropriately.
*/
int vmodttl_write(int lun, enum vmodttl_channel chan, int val);

//! Read from a channel.                 
/*!                                                                                       
  \param lun an integer argument to select the device.
  \param chan an enum vmodttl_channel to select the desired channel.
  \param val a pointer to the integer variable in which the channel value will be written to.
  \return returns 0 on success. On error, -1 is returned, and errno is set appropriately.
*/
int vmodttl_read(int lun, enum vmodttl_channel chan, int *val);

//! Configure the pattern in a channel. <b>Not implemented</b>                 
/*!                                                                                       
  \param lun an integer argument to select the device.
  \param chan an enum vmodttl_channel (only A or B are allowed) to select the desired input channel.
  \param pos bit number inside the port (from 0 to 7).
  \param bit_pattern an enum vmodttl_pattern to select the desired pattern. 
  \return returns 0 on success. On error, -1 is returned, and errno is set appropriately.
*/
int vmodttl_pattern(int lun, enum vmodttl_channel chan, int pos, enum vmodttl_conf_pattern bit_pattern);

//! Configuration of each channel. <b>Not implemented in PPC4</b>
/*!
  \param lun an integer argument to select the device.
  \param conf a struct vmodttl_config to copy the I/O configuration of the device.
  \return returns 0 on success. On error, negative number is returned, and errno is set appropriately.
*/
int vmodttl_read_config(int lun, struct vmodttl_config *conf);

//! Read the device (to be notified of interrupts). 
/*!
  \brief It will be blocked until an IRQ arrives. <b>Not implemented</b>
  \param lun an integer argument to select the device.
  \param buffer unsigned char array to save the channel in the first position and the value in the second.
  \return returns 0 on success. On error, negative number is returned, and errno is set appropriately.
*/
int vmodttl_read_device(int lun, unsigned char buffer[2]);

#ifdef __cplusplus
}
#endif

#endif /* __LIBVMODTTL */
