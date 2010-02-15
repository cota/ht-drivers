/*******************************************************/
/* file: ports.c                                       */
/* abstract:  This file contains the routines to       */
/*            output values on the JTAG ports, to read */
/*            the TDO bit, and to read a byte of data  */
/*            from the prom                            */
/*                                                     */
/*******************************************************/
#include "ports.h"

#define Ctx1DrvrJTAG_TDO     0x01   /* Data Out */
#define Ctx1DrvrJTAG_TDO_BAR 0x10

#define Ctx1DrvrJTAG_TDI     0x02   /* Data In */
#define Ctx1DrvrJTAG_TDI_BAR 0x20

#define Ctx1DrvrJTAG_TMS     0x04   /* Mode Select */
#define Ctx1DrvrJTAG_TMS_BAR 0x40

#define Ctx1DrvrJTAG_TCK     0x08   /* Clock */
#define Ctx1DrvrJTAG_TCK_BAR 0x80

/*******************************************************/
/* Write one bit to selected Jtag bit                  */
/* p is the jtag port TMS, TDI or TCK                  */
/* val contains the bit value one or zero              */

void setPort(short p,short val) {
   if (val) {
      if      (p == TMS) { jtag |=  Ctx1DrvrJTAG_TMS;
			   jtag &= ~Ctx1DrvrJTAG_TMS_BAR; }
      else if (p == TDI) { jtag |=  Ctx1DrvrJTAG_TDI;
			   jtag &= ~Ctx1DrvrJTAG_TDI_BAR; }
      else if (p == TCK) { jtag |=  Ctx1DrvrJTAG_TCK;
			   jtag &= ~Ctx1DrvrJTAG_TCK_BAR; }
      else return;
   } else {
      if      (p == TMS) { jtag |=  Ctx1DrvrJTAG_TMS_BAR;
			   jtag &= ~Ctx1DrvrJTAG_TMS; }
      else if (p == TDI) { jtag |=  Ctx1DrvrJTAG_TDI_BAR;
			   jtag &= ~Ctx1DrvrJTAG_TDI; }
      else if (p == TCK) { jtag |=  Ctx1DrvrJTAG_TCK_BAR;
			   jtag &= ~Ctx1DrvrJTAG_TCK; }
      else return;
   }
   if (jtagAddr) *jtagAddr = jtag;
}

/*******************************************************/
/* read in a byte of data from the input stream        */

void readByte(unsigned char *data) {

    *data = (unsigned char) fgetc(inp);
}

/*******************************************************/
/* read the TDO bit from port                          */

unsigned char readTDOBit() {
unsigned short rback;

   if (jtagAddr) rback = *jtagAddr;
   if (rback & Ctx1DrvrJTAG_TDO)
      return (unsigned char) 1;
   return (unsigned char) 0;
}

/*****************************************************************************/
/* Wait at least the specified number of microsec.                           */

void waitTime(long microsec) {
   setPort(TCK,0);  /* set the TCK port to low  */
   usleep(microsec);
}

/*******************************************************/
/* Pulse the TCK clock                                 */

void pulseClock() {

    setPort(TCK,0);  /* set the TCK port to low  */
    setPort(TCK,1);  /* set the TCK port to high */
}
