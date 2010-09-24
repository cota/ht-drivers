/**
 * @file libvmod12e16.h
 *
 * @brief VMOD12E16 driver library interface
 *
 * This file describes the external interface to the VMOD12E16
 * driver and provides the definitions for proper communication
 * with the device
 * 
 * Copyright (c) 2009 CERN
 * @author Juan David Gonzalez Cobas <dcobas@cern.ch>
 * @date Sun Oct 18 11:07:39 CEST 2009
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifndef _LIBVMOD12E16_H_
#define _LIBVMOD12E16_H_

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @brief Symbolic constants for amplification factors
 * 
 * The input to vmod12e16 ADCs is led by an amplification stage
 * that can be configured to scale values by 1, 10, 100, 1000.
 * Given that the observed precision of the card is rather poor, it
 * use of factors greater than 1 is seriously discouraged
 */

enum vmod12e16_amplification {
	VMOD12E16_AMPLIFICATION_1 = 0,
	VMOD12E16_AMPLIFICATION_10,
	VMOD12E16_AMPLIFICATION_100,
	VMOD12E16_AMPLIFICATION_1000
};


/**
 * @brief Get a handle for a vmod12e16 device
 *
 * @param carrier_lun - carrier LUN for vmod card
 *
 * @return >0 - on success, device file descriptor number identifying 
 *		a mezzanine card
 * @return <0 - if failure
 *
 */
int vmod12e16_get_handle(unsigned int carrier_lun);

/**
 * @brief perform an analog-to-digital conversion 
 * 
 * Start ADC conversion using 12E16 module addressed by handle fd.
 * Analog data are read from the channel given as a parameter
 * The desired amplification
 * factor is applied to it. At the address specified by value we get the
 * result of conversion.  The possible values of 12E16 instrumentation
 * amplifier are given by the symbolic constants values of factor, as
 * per enum vmod16a2_amplification.

 * Given the limited precision of the card, use of amplification factors
 * greater than 1 is probably uninteresting
 * 
 * @param  fd    	Device handle
 * @param  channel	Channel to read from
 * @param  factor	Amplification factor
 * @param  value	Return value 
 * 
 * @return 0 on success, <0 on failure
 */
int vmod12e16_convert(int fd, int channel,
	enum vmod12e16_amplification factor, int *value);

/**
 *
 * @brief close a handle
 *
 * @param fd Handle to close
 */
int vmod12e16_close(int fd);

#ifdef __cplusplus
}
#endif

#endif /* _LIBVMOD12E16_H_ */
