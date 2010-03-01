/**
 * @file libvmod16a2.h
 *
 * @brief VMOD16A2 driver library interface
 *
 * This file describes the external interface to the VMOD16A2
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

#ifndef _LIBVMOD16A2_H_
#define _LIBVMOD16A2_H_

/**
 * @brief Get a handle for a vmod16a2 device
 *
 * A channel is selected by giving the LUN for the board
 *
 * @param lun - LUN for vmod card
 *
 * @return >0 - on success, device file descriptor number
 * @return <0 - if failure
 *
 */
int vmod16a2_get_handle(unsigned int lun);

/**
 * @brief perform a digital-to-analog conversion
 *
 * The output voltage depends on hardware configuration (default
 * -10V..10V, configurable by jumpers to 0-10V range).
 *
 * @param  fd    	Device handle identifying board
 * @param  channel    	Channel on the board to write to
 * @param  datum	Digital 16-bit value (0x0000-0xffff)
 *
 * @return 0 on success, <0 on failure
 */
int vmod16a2_convert(int fd, int channel, int datum);

/**
 * @brief close a channel handle
 *
 * @param fd - Handle to close
 */
int vmod16a2_close(int fd);

#endif /* _LIBVMOD16A2_H_ */


