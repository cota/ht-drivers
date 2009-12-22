/*******************************************************/
/* file: ports.c                                       */
/* actrract:  This file contains the routines to       */
/*            output values on the JTAG ports, to read */
/*            the TDO bit, and to read a byte of data  */
/*            from the prom                            */
/*                                                     */
/*******************************************************/

#include "ports.h"

/*******************************************************/
/* Write one bit to selected Jtag bit                  */
/* p is the jtag port TMS, TDI or TCK                  */
/* val contains the bit value one or zero              */

static unsigned long jtag = 0x0F; /* Keep all jtag bits   */
static unsigned long dito = 0;
#define DITO 200000

void setPort(short p, short val)
{
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

   if (xsvf_iDebugLevel > 1) {
      printf("Jtag: 0x%2x ",(int) jtag);
      if (jtag & JTAG_TMS) printf("TMS ");
      if (jtag & JTAG_TDI) printf("TDI ");
      if (jtag & JTAG_TCK) printf("TCK ");
      if (jtag & JTAG_TMS_BAR) printf("tms ");
      if (jtag & JTAG_TDI_BAR) printf("tdi ");
      if (jtag & JTAG_TCK_BAR) printf("tck ");
      printf("\n");
   }

   if (ioctl(ctr,JTAG_WRITE_BYTE,&jtag) < 0)
      IErr("JTAG_WRITE_BYTE",NULL);

   if ((dito++ % DITO) == 0) {
      printf("\7.");
      fflush(stdout);
   }
}

/*******************************************************/
/* read in a byte of data from the input stream        */

void readByte(unsigned char *data)
{
   *data = (unsigned char) fgetc(inp);

   if (xsvf_iDebugLevel > 1) printf("Data: 0x%2x\n",(int) *data);
}

/*******************************************************/
/* read the TDO bit from port                          */

unsigned char readTDOBit(unsigned long rback)
{
   if (ioctl(ctr,JTAG_READ_BYTE,&rback) < 0)
      IErr("JTAG_READ_BYTE",NULL);

   if (xsvf_iDebugLevel > 1) {
      printf("Rback: 0x%2x ",(int) rback);
      if (rback & JTAG_TDO) printf("TDO ");
      if (rback & JTAG_TDO_BAR) printf("tdo ");
      printf("\n");
   }
   if (rback & JTAG_TDO) return (unsigned char) 1;
   else                         return (unsigned char) 0;
}

/*****************************************************************************/
/* Wait at least the specified number of microsec.                           */

void waitTime(long microsec)
{
/* Fix problem for STMicro, set clock low prior to wait */
/* See application not acn2003-01 from www.xilinx.com web site */

   setPort(TCK,0);
   usleep(microsec);
}

/*******************************************************/
/* Pulse the TCK clock                                 */

void pulseClock(void)
{
   setPort(TCK,0);  /* set the TCK port to low  */
   setPort(TCK,1);  /* set the TCK port to high */
}
