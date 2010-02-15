/* ******************************************************** */
/* Small program to download a VHDL program to a JTAG port. */
/* Julian Lewis Mon 7th April 2003                          */

#include <sys/types.h>
#include <ces/vmelib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>

#include <lenval.h>
#include <micro.h>
#include <ports.h>

void setPort(short p,short val);
void readByte(unsigned char *data);
unsigned char readTDOBit();
void waitTime(long microsec);
void pulseClock();

static int xsvf_iDebugLevel = 0;
static FILE *inp = NULL;
static short *jtagAddr = NULL;
static unsigned short jtag = 0x0F; /* Keep all jtag bits   */

#include <lenval.c>
#include <micro.c>
#include <ports.c>
#include <smemio.c>

/* ********************************************* */
/* Arguments: Filename VME-Address [Debug-Level] */

int main(int argc,char *argv[]) {

char fname[128], *cp, *ep, yn;
short *vmeAddress;
int cc;

   if ((argc < 3) || (argc > 4)) {
      printf("jtag: <filename> <vme address> [<debug level>]\n");
      printf("Examples:\n");
      printf("         nouchi.xsvf 0x1000\n");
      printf("         nouchi.xsvf 0x1000 4\n");
      exit(0);
   }

   strcpy(fname,argv[1]);
   cp = argv[2];
   vmeAddress = (short *) strtoul(cp,&ep,0);
   if (argc == 4) xsvf_iDebugLevel = strtoul(argv[3],&ep,0);

   printf("VHDL-Compiled BitStream Filename: %s VMEAddress: 0x%X DebugLevel: %d \n",
	  fname,
	  (int) vmeAddress,
	  (int) xsvf_iDebugLevel);

   printf("Continue (Y/N):"); yn = getchar();
   if ((yn != 'y') && (yn != 'Y')) exit(0);

   inp = fopen(fname,"r");
   if (inp) {

      if (GetJtagPort(vmeAddress)) {

	 cc = xsvfExecute(); /* Play the xsvf file */
	 if (cc) printf("Jtag: xsvfExecute: ReturnCode: %d Error\n",cc);
	 else    printf("Jtag: xsvfExecute: ReturnCode: %d All OK\n",cc);
	 fclose(inp);
	 exit(0);
      }
   } else {
      perror("fopen");
      printf("Could not open the file: %s for reading\n",fname);
   }
   exit(0);
}
