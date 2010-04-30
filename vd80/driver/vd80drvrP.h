#include <vd80hard.h>

typedef struct {   /* mcon->Registers */

   U32 GCR1;
   U32 GCR2;
   U32 GSR;


 } GeneralRegs;


typedef struct {   /* mcon->UserData */

   char revis_id[VD80_CR_REV_ID_LEN +1]; /* The revis_id and terminator byte */

 } UserData;

#define VD80_KLUDGE_DELAY 100000	/* fixing the delay in vd80 module */
