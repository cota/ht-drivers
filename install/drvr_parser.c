/*************************************************************************/
/* Parse a standard command line for a driver install program.           */
/* Julian Lewis                                                          */
/* Mon 13th Sept 1993                                                    */
/*************************************************************************/

/***************************************************************************/
/* Usage:                                                                  */
/* install *{-A/B/C/M<address> -O<offset> -V<vector> -L<level> -X<extra>}  */
/*                                                                         */
/* All parameters are optional except <address>. The actual address that   */
/* is returned is modified depending on which platform the driver is to    */
/* be installed on. The "Size" field in the structure returned denotes the */
/* number of modules actually found.                                       */
/*                                                                         */
/* #include <drvr_parser.c>                                                */
/*                                                                         */
/* int main(argv,argc) char **argv; int argc; {                            */
/*                                                                         */
/* DrvrModuleAddresses addresses;                                          */
/* int i;                                                                  */
/*                                                                         */
/* DrvrParser(argv,argc,&addresses);                                       */
/* for (i=0; i<addresses.Size; i++) {                                      */
/*    printf("Module : %1d\n", i+1);                                       */
/*    printf("Address: %x\n",addresses.Addresses[i].Address);              */
/*    printf("Tag    : %x\n",addresses.Addresses[i].AddressTag);           */
/*    printf("Offest : %x\n",addresses.Addresses[i].Offset);               */
/*    printf("Vector : %x\n",addresses.Addresses[i].InterruptVector);      */
/*    printf("Level  : %x\n",addresses.Addresses[i].InterruptLevel);       */
/*    printf("Extra  : %x\n",addresses.Addresses[i].ExtraParameter);       */
/*    . . . rest of modules install procedure . . .                        */
/*                                                                         */
/***************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __68k__
#include <mem.h>        /* Memory layout addresses VMEBASE */
#endif
#include <io.h>
#include <sys/file.h>
#include <tgm/con.h>
#include <drvr_parser.h>

/***************************************************************************/
/* Parse command line and fill addresses structure.                        */
/***************************************************************************/

void DrvrParser(argc,argv,addresses)
long argc;
char **argv;
DrvrModuleAddresses *addresses; {

int i, index, temp;
char *cpd, *cpp;

   if (addresses == NULL) return;
   bzero((void *) addresses,(long) sizeof(DrvrModuleAddresses));
   index = 0;

   for (i=1; i<argc; i++) {
      cpd = &(argv[i][2]);

      /* -A/B/C/M<address> For each VME address increment module count */

      if ( (strncmp(argv[i],"-A",2) == 0)
      ||   (strncmp(argv[i],"-B",2) == 0)
      ||   (strncmp(argv[i],"-C",2) == 0)
      ||   (strncmp(argv[i],"-M",2) == 0) ) {
	 if (addresses->Size >= DrvrMODULE_ADDRESSES) return;
	 index = addresses->Size;       /* Index = size -1 */
	 addresses->Size += 1;
	 addresses->Addresses[index].Address = strtoul(cpd,&cpp,0);
	 addresses->Addresses[index].AddressTag = argv[i][1] - 'A';
      };

      /* -V<vector> Get the interrupt vector */

      if (strncmp(argv[i],"-V",2) == 0)
	 addresses->Addresses[index].InterruptVector = strtoul(cpd,&cpp,0);

      /* -L<level> The interrupt level must be ranged 1..6 */

      if (strncmp(argv[i],"-L",2) == 0) {
	 temp = strtoul(cpd,&cpp,0);
	 if (temp < 1) temp = 1; if (temp > 6) temp = 6;
	 addresses->Addresses[index].InterruptLevel = temp;
      };

      /* -X<extra> Get module dependent extra parameter */

      if (strncmp(argv[i],"-X",2) == 0)
	 addresses->Addresses[index].ExtraParameter = strtoul(cpd,&cpp,0);
   };
   return;
}
