/**
 * @file drvr_vid_did.h
 *
 * @brief Contains vendor && device id's of the CERN PCI modules.
 *
 * @author Georgievskiy Yury. CERN AB/CO.
 *
 * @date June, 2008
 *
 * @version 1.0  ygeorgie  03/06/2008  Initial version.
 */
#ifndef _MIL1553_HEADER_H_INCLUDE_
#define _MIL1553_HEADER_H_INCLUDE_

/* device && vendor ID's used in the drivers, installation and test programs */
#define CERN_PCI_VENDOR_ID 0x10dc

/* ctri */
#define CTRP_VENDOR_ID CERN_PCI_VENDOR_ID
#define CTRP_DEVICE_ID 0x0300

/* mil1553 */
#define MIL1553_PCI_VENDOR_ID CERN_PCI_VENDOR_ID
#define MIL1553_PCI_DEVICE_ID 0x0301

#endif /* _MIL1553_HEADER_H_INCLUDE_ */
