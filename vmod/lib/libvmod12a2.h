/**
 * @file libvmod12a2.h
 *
 * @brief VMOD12a2 driver library interface
 *
 * This file describes the external interface to the VMOD12A2
 * driver and provides the definitions for proper communication
 * with the device
 * 
 * Copyright (c) 2009 CERN
 * @author Juan David Gonzalez Cobas <dcobas@cern.ch>
 * @date Oc 19th 2009
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifndef _LIBVMOD12A2_H_
#define _LIBVMOD12A2_H_

/**
 * @brief Get a handle for a vmod12a2 device and channel
 *
 * A channel is selected by giving the LUN for the board
 * and the channel number. The handle returned allows
 * further reference to the same channel
 *
 * @param carrier_lun - carrier LUN for vmod card
 * @param channel     - selected channel for reading
 *
 * @return >0 - on success, device file descriptor number
 * @return <0 - if failure
 *
 */
int vmod12a2_get_handle(unsigned int carrier_lun);

/**
 * @brief perform a digital-to-analog conversion
 *
 * The output voltage depends on hardware configuration (default
 * -10V..10V, configurable by jumpers to 0-10V range).
 *
 * @param  fd    	Device handle identifying board
 * @param  channel    	Channel on the board to write to
 * @param  datum	Digital 12-bit value (0x000-0xfff)
 *
 * @return 0 on success, <0 on failure
 */
int vmod12a2_convert(int fd, int channel, int datum);

/**
 * @brief close a channel handle
 *
 * @param fd - Handle to close
 */
int vmod12a2_close(int fd);
#endif /* _LIBVMOD12A2_H_ */



