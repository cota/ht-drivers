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

#ifdef __cplusplus
extern "C" {
#endif

/*! \file */
/*!
 *  \mainpage VMOD-DOR Device Driver
 *  \author Samuel Iglesias Gonsalvez BE/CO/HT
 *  \version 27 May 2010
 *
 * The VMOD-DOR hardware module is an opto-isolated digital output
 * with 16 bit TTL-level.
 *
 * In this version of the driver the following capabilities are provided.
 *
 * 1) Open/Close the device.
 * 
 * 2) Write to the channel.
 */

/*! 
 * This struct is used as argument in vmoddor_write 
 */
struct vmoddor_warg{
        int             data; /*!< Data to be written */
        unsigned short  size; /*!< Data size (4, 8, 16 bits) */
        unsigned short  offset; /*!< Offset to the corresponding channel [0-5]:<br>
				*	16 bits: this value is not used.<br>
				*	8 bits: use 0 (D8...D15) or 1 (D0...D7).<br>	
				*	4 bits: use from 2 to 5 (both included). 
 * <table align="center" cellspacing="2" cellpadding="2" border="1">
 * <tr>
 *   <td colspan="4" align="left"><strong>Data Offset</strong></td>
 * </tr>

 * <tr>
 *      <td align="center">Channel</td>
 *      <td align="center">Offset</td>
 * </tr>
 * <tr>
 *      <td align="center">D0...D3</td>
 *      <td align="center">3</td>
 * </tr>
 * <tr>
 *      <td align="center">D4...D7</td>
 *      <td align="center">5</td>
 * </tr>
 * <tr>
 *      <td align="center">D8...D11</td>
 *      <td align="center">2</td>
 * </tr>
 * <tr>
 *      <td align="center">D12...D15</td>
 *      <td align="center">4</td>
 * </tr> </table>
 */


};

//! Open the device.
/*!
  \param lun an integer argument to select the device.
  \return returns 0 on success. On error, -1 is returned, and errno is set appropriately.
*/
int vmoddor_open(int lun);
//! Close the device.
/*!
  \param lun an integer argument to select the device.
  \return return 0 on success. On error, -1 is returned, and errno is set appropriately.
*/
int vmoddor_close(int lun);
//! Write a value to the output.
 /*!
  \param lun an integer argument to select the device.
  \param val struct that contains the data that will be written.
  \return returns 0 on success. On error, negative number is returned, and errno is set appropriately.
 */
int vmoddor_write(int lun, struct vmoddor_warg val);

#ifdef __cplusplus
}
#endif
