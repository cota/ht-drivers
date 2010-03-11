/*******************************************************/
/* file: ports.c                                       */
/* abstract:  This file contains the routines to       */
/*            output values on the JTAG ports, to read */
/*            the TDO bit, and to read a byte of data  */
/*            from the prom                            */
/*                                                     */
/*******************************************************/
#include "ports.h"

#define JTAG_TDO     0x01   /* Data Out */
#define JTAG_TDO_BAR 0x10

#define JTAG_TDI     0x02   /* Data In */
#define JTAG_TDI_BAR 0x20

#define JTAG_TMS     0x04   /* Mode Select */
#define JTAG_TMS_BAR 0x40

#define JTAG_TCK     0x08   /* Clock */
#define JTAG_TCK_BAR 0x80

static unsigned long dito = 0;
#define DITO 200000

/*******************************************************/
/* Swap bytes on little endian systems                 */

short swap(short word) {

char l,r;

   r = word & 0xFF;
   l = word >> 8;
   word = (r << 8) | (l & 0xFF);

   return word;
}

/*******************************************************/
/* Write one bit to selected Jtag bit                  */
/* p is the jtag port TMS, TDI or TCK                  */
/* val contains the bit value one or zero              */

void setPort(short p,short val) {
   if (val) {
      if      (p == TMS) { jtag |=  JTAG_TMS;
			   jtag &= ~JTAG_TMS_BAR; }
      else if (p == TDI) { jtag |=  JTAG_TDI;
			   jtag &= ~JTAG_TDI_BAR; }
      else if (p == TCK) { jtag |=  JTAG_TCK;
			   jtag &= ~JTAG_TCK_BAR; }
      else return;
   } else {
      if      (p == TMS) { jtag |=  JTAG_TMS_BAR;
			   jtag &= ~JTAG_TMS; }
      else if (p == TDI) { jtag |=  JTAG_TDI_BAR;
			   jtag &= ~JTAG_TDI; }
      else if (p == TCK) { jtag |=  JTAG_TCK_BAR;
			   jtag &= ~JTAG_TCK; }
      else return;
   }
   if (jtagAddr) *jtagAddr = swap(jtag);

   if ((dito++ % DITO) == 0) {
      printf("\7.");
      fflush(stdout);
   }
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

   if (jtagAddr) rback = swap(*jtagAddr);
   if (rback & JTAG_TDO)
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
