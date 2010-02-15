/*******************************************************/
/* file: ports.h                                       */
/* abstract:  This file contains extern declarations   */
/*            for providing stimulus to the JTAG ports.*/
/*******************************************************/

#ifndef ports_dot_h
#define ports_dot_h

/* these constants are used to send the appropriate ports to setPort */
/* they should be enumerated types, but some of the microcontroller  */
/* compilers don't like enumerated types */

#define TCK (short) 0
#define TMS (short) 1
#define TDI (short) 2

#define MttDrvrJTAG_TDO     0x01   /* Data Out */
#define MttDrvrJTAG_TDO_BAR 0x10

#define MttDrvrJTAG_TDI     0x02   /* Data In */
#define MttDrvrJTAG_TDI_BAR 0x20

#define MttDrvrJTAG_TMS     0x04   /* Mode Select */
#define MttDrvrJTAG_TMS_BAR 0x40

#define MttDrvrJTAG_TCK     0x08   /* Clock */
#define MttDrvrJTAG_TCK_BAR 0x80

#endif
