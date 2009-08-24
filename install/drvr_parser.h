/*************************************************************************/
/* Parse a standard command line for a driver install program.           */
/* Julian Lewis                                                          */
/* Mon 13th Sept 1993                                                    */
/*************************************************************************/

#include <con.h>

/* Maximum number of modules in one VME crate seen by one device driver */

#define DrvrMODULE_ADDRESSES 24

typedef struct {
   unsigned long Address;               /* Address with modifiers added */
   unsigned long AddressTag;            /* 0=A 1=B 2=C 13=M */
   unsigned long Offset;                /* Second address */
   unsigned long InterruptVector;       /* Vector used by module */
   unsigned long InterruptLevel;        /* Level */
   unsigned long ExtraParameter;        /* Module dependant parameter */
 } DrvrModuleAddress;

typedef struct {
   Cardinal Size;                                       /* Number found */
   DrvrModuleAddress Addresses[DrvrMODULE_ADDRESSES];   /* Address array */
 } DrvrModuleAddresses;

void DrvrParser(long argc, char **argv, DrvrModuleAddresses *addresses);
