/* ======================================================================================== */
/* Data used to provide VMIC 5565 field names and addresses of configuration registers      */
/* Julian Lewis 26th Nov 2003. CERN AB/CO/HT Geneva                                         */
/* Julian.Lewis@cern.ch                                                                     */
/* ======================================================================================== */

#include <iofield.h>

#define VmicRFM_REGISTERS 30
#define VmicRFM_MAX_ADR 0x3e

#define VmicSDRAM_MAX_ADR 0x3fffffc

IoField RfmRegs[VmicRFM_REGISTERS] = {

   { 1, "Brv",      0x00, "Board Revision RO" },
   { 1, "Bid",      0x01, "Board ID = 65  RO" },
   { 2, "Short02",  0x02, "Reserved" },
   { 1, "Nid",      0x04, "Node ID 1..256 RO" },
   { 1, "Byte05",   0x05, "Reserved" },
   { 2, "Short06",  0x06, "Reserved" },
   { 4, "Lcsr1",    0x08, "Local control and Status 1" },
   { 4, "Long0C",   0x0C, "Reserved" },
   { 4, "Lisr",     0x10, "Local Interrupt Status Register" },
   { 4, "Leir",     0x14, "Local Interrupt Enable Register" },
   { 4, "Ntd",      0x18, "Network Target Data" },
   { 1, "Ntn",      0x1C, "Network Target Node" },
   { 1, "Nic",      0x1D, "Network Interrupt Command" },
   { 2, "Short1E",  0x1E, "Reserved" },
   { 4, "Isd1",     0x20, "Int-1 Sender Data" },
   { 1, "Sid1",     0x24, "Int-1 Sender ID" },
   { 1, "Byte25",   0x25, "Reserved" },
   { 2, "Short26",  0x26, "Reserved" },
   { 4, "Isd2",     0x28, "Int-2 Sender Data" },
   { 1, "Sid2",     0x2C, "Int-2 Sender ID" },
   { 1, "Byte2D",   0x2D, "Reserved" },
   { 2, "Short2E",  0x2E, "Reserved" },
   { 4, "Isd3",     0x30, "Int-3 Sender Data" },
   { 1, "Sid3",     0x34, "Int-3 Sender ID" },
   { 1, "Byte35",   0x35, "Reserved" },
   { 2, "Short36",  0x36, "Reserved" },
   { 4, "Initd",    0x38, "Initialized Node Data" },
   { 1, "Initn",    0x3C, "Initialized Node ID" },
   { 1, "Byte3D",   0x3D, "Reserved" },
   { 2, "Short3E",  0x3E, "Reserved" }
 };
