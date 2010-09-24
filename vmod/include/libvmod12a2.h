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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get a handle for a vmod12a2 device
 *
 * A board is selected by lun, and a handle is returned for further
 * refrence to it.

 * @param lun - LUN of vmod card
 *
 * @return >0 - on success, device file descriptor number
 */
int vmod12a2_get_handle(unsigned int lun);

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

#ifdef __cplusplus
}
#endif
#endif /* _LIBVMOD12A2_H_ */



