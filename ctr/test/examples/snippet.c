#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>        /* Error numbers */
#include <sys/file.h>
#include <a.out.h>
#include <ctype.h>

#include <TimLib.h>     /* See /dsrc/drivers/ctr/src/lib/TimLib.[ch] */

int main(int argc,char *argv[])

TimLibClass    iclss;   /* Class of interrupt */
unsigned long  equip;   /* Equipment or hardware mask */
unsigned long  module;  /* For HARD or CTIM classes */
unsigned long  plnum,   /* Ptim line number 1..n or 0 */
TimLibHardware source,  /* Hardware source of interrupt */
TimLibTime     onzero,  /* Time of interrupt/output */
TimLibTime     trigger, /* Time of counters load */
TimLibTime     start,   /* Time of counters start */
unsigned long  ctim,    /* CTIM trigger equipment ID */
unsigned long  payload, /* Payload of trigger event */
unsigned long  module,  /* Module that interrupted */
unsigned long  missed,  /* Number of missed interrupts */
unsigned long  qsize);  /* Remaining interrupts on queue */

   /* This routine performs the following initialization functions...      */
   /*    1) Opens a connection to the driver                               */
   /*    2) Checks the Firmware/VHDL version against the latest revision   */
   /*       Some EProms/FPGAs may need updating, this takes a while.       */
   /*    3) Load all relavent CTIM and PTIM definitions if needed.         */

   err =  TimLibInitialize(); /* Initialize hardware/software */
   if (err) {
      printf("Error: %s\n",TimLibErrorToString(err));
      exit(err);
   }

   /* Connect to an interrupt. If you are connecting to either a CTIM      */
   /* interrupt or to a hardware interrupt, you may need to specify on     */
   /* which device the interrupt should be connected. This is achieved by  */
   /* the module parameter. If the module is zero, the system will decide  */
   /* which device to use, otherwise module contains a value between 1 and */
   /* the number of installed timing receiver cards. For PTIM objects the  */
   /* module parameter must be set to zero or the real module on which the */
   /* PTIM object is implemented. On PTIM objects the module is implicit.  */

   iclss = TimLibClassPTIM;
   equip = 1001;            /* This will be declared at install time. the 10HZ you need */
   module = 0;              /* Don't care which module, use the one I need for above PTIM */

   err = TimLibConnect(iclss, equip, module);
   if (err) {
      printf("Error: %s\n",TimLibErrorToString(err));
      exit(err);
   }

   /* Wait for an interrupt. The parameters are all returned from the call */
   /* so you can know which interrupt it was that came back. Note, when    */
   /* waiting for a hardware interrupt from either CTIM or from a counter, */
   /* it is the CTIM or PTIM object that caused the interrupt returned.    */
   /* The telegram will have been read already by the high prioity task    */
   /* get_tgm_ctr/tg8, be aware of the race condition here, hence payload. */
   /* This routine is a blocking call, it waits for interrupt or timeout.  */
   /* Any NULL argument  is permitted, and no value will be returned.      */

   /* Arguments:                                                           */
   /*    iclss:   The class of the interrupt CTIM, PTIM, or hardware       */
   /*    equip:   The PTIM, CTIM equipment, or hardware mask               */
   /*    plnum:   If class is PTIM this is the PLS line number             */
   /*    source:  The hardware source of the interrupt                     */
   /*    onzero:  The time of the interrupt                                */
   /*    trigger: The arrival time of the event that triggered the action  */
   /*    start:   The time the start of the counter occured                */
   /*    ctim:    The CTIM equipment number of the triggering event        */
   /*    payload: The payload of the triggering event                      */
   /*    module:  The module number 1..n of the timing receiver card       */
   /*    missed:  The number of missed events since the last wait          */
   /*    qsize:   The number of remaining interrupts on the queue          */

   while (1) {
      err = TimLibWait(&class,&equip,&plnum,&source,&onzero,
		       &trigger,&start,&ctim,&payload,&module,
		       &missed,&qsize);
      if (err) {
	 printf("Error: %s\n",TimLibErrorToString(err));
	 exit(err);
      }

      /* Here is all the crap you can do things with after waiting */

      printf("Class:   %d\n",(int) class);
      printf("Equip:   %d\n",(int) equip);
      printf("Plnum:   %d\n",(int) plnum);
      printf("Source:  0x%X\n",(int) source);

      printf("OnZero:  %ds %dns C%d Mch:%d\n",
	    (int) onzero.Second,
	    (int) onzero.Nano,
	    (int) onzero.CTrain,
	    (int) onzero.Machine);

      printf("Trigger: %ds %dns C%d Mch:%d\n",
	    (int) trigger.Second,
	    (int) trigger.Nano,
	    (int) trigger.CTrain,
	    (int) trigger.Machine);

      printf("Start:   %ds %dns C%d Mch:%d\n",
	    (int) start.Second,
	    (int) start.Nano,
	    (int) start.CTrain,
	    (int) start.Machine);

      printf("CTim:    %d\n",(int) ctim);
      printf("Payload: %d\n",(int) payload);
      printf("Module:  %d\n",(int) module);
      printf("Missed:  %d\n",(int) missed);
      printf("QSize:   %d\n",(int) qsize);
   }
}
