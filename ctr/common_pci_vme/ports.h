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

#define CtrDrvrJTAG_TDO     0x01   /* Data Out */
#define CtrDrvrJTAG_TDO_BAR 0x10

#define CtrDrvrJTAG_TDI     0x02   /* Data In */
#define CtrDrvrJTAG_TDI_BAR 0x20

#define CtrDrvrJTAG_TMS     0x04   /* Mode Select */
#define CtrDrvrJTAG_TMS_BAR 0x40

#define CtrDrvrJTAG_TCK     0x08   /* Clock */
#define CtrDrvrJTAG_TCK_BAR 0x80

#endif
