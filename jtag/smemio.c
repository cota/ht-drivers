/*******************************************************************/
/* Map the JTAG physical VME adderss onto a virtual memory address */
/* using a sharded memory segment.                                 */

#include <libvmebus.h>

static short *GetJtagPort(unsigned short *vmeAddress) { /* VME address */

struct pdparam_master param; /* For CES PowerPC */

unsigned long addr;          /* VME base address */

   if (!jtagAddr) {

      /* CES: build an address window (64 kbyte) for VME A16-D16 accesses */

      addr = (unsigned long) vmeAddress;
      addr &= 0x0000ffff;                      /* A16 */

      memset(&param, 0, sizeof(param));
      param.iack   = 1;                /* no iack */
      param.rdpref = 0;                /* no VME read prefetch option */
      param.wrpost = 0;                /* no VME write posting option */
      param.swap   = 1;                /* VME auto swap option */
      param.dum[0] = VME_PG_SHARED;    /* window is sharable */
      param.dum[1] = 0;                /* XPC ADP-type */
      param.dum[2] = 0;                /* window is sharable */

      jtagAddr = (void *) find_controller(
				       0,                       /* Vme base address */
				       (unsigned long) 0x10000, /* Module address space */
				       (unsigned long) 0x29,    /* Address modifier A16 */
				       0,                       /* Offset */
				       2,                       /* Size is D16 */
				       &param);                 /* Parameter block */
      if (jtagAddr == (void *)(-1)) {
	 printf("GetJtagPort: find_controller: ERROR: JTAG Addr:%x\n",(int) vmeAddress);
	 jtagAddr = NULL;
      }
   }
   jtagAddr = &(jtagAddr[addr >> 1]);
   return jtagAddr;
}
