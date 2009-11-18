/*
$Source: /ps/dsc/src/drivers/icv196vme/lib/RCS/icv196vmelib.c,v $
$Header: icv196vmelib.c,v 1.2 97/09/10$
$Version$
$Author: dscps $
$Locker: dscps $
$Log:	icv196vmelib.c,v $
 * Revision 1.2  97/09/10  11:19:59  dscps
 * to get rid of warning at compile time
 * A.G.
 *
 * Revision 1.1  95/06/14  18:23:56  dscps
 * Initial revision
 *
*/

/* %%%( begining of file icv196vmelib.c                           _*_Mode: c; _*_
*/

/*
  __________________________________________________________________
  |                                                                 |
  |   file name :          icv196vmelib.c                           |
  |                                                                 |
  |   created: 25-mar-1992 Alain Gagnaire                           |
  |   updated: 26-mar-1992 A. G.                                    |
  |                                                                 |
  |_________________________________________________________________|
  |                                                                 |
  |   facilities to access icv196vme driver interface               |
  |                                                                 |
  |_________________________________________________________________|

  updated:
  =======

*/
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "icv196vmelib.h"


#ifndef OK
#define OK 0
#endif

	/* for source code management: RCS marker  */
static char rcsid[]="$Header: icv196vmelib.c,v 1.2 97/09/10$";

static int fdid_icvservice = (-1);	  /* currently open on service handle*/
static char Hdlservice[] = "/dev/icv196service";

/*
   ______________________
  |  icv196vme  library  |
  |                      |
  |      icv196GetInfo   |
  |______________________|

*/
int icv196GetInfo(m, buff_sz, buff)
                  int  m;
                  int  buff_sz;
                  struct icv196T_ModuleInfo *buff;
{
   int function;
   int cc;
	/*  */

   if (fdid_icvservice < 0) {
     if ((cc = open( Hdlservice, O_RDONLY, 0755)) < 0){
       perror("icv196GetSlotInfo:open");
       return (cc);
     }
     fdid_icvservice = cc;
   }
   function = ICVVME_getmoduleinfo;
		/* set up argument field for ioctl call  */
		/* Call the driver to perform the function */
   if ((cc = ioctl(fdid_icvservice, function, (char *)buff)) < 0){
     perror("icv196GetSlotInfo:ioctl:");
     return(cc);
   }
		/* Give back data in user array */
   return (OK);
 } /* GetSlotInfo */


/*
   ______________________
  |   icv196vme library  |
  |                      |
  |     icv196SetTO      |
  |______________________|

  set time out for waiting Event on read of synchronization channel
  the synchronization is requested by mean of the general purpose
  function gpevtconnect see in real time facilities: rtfclty/gpsynchrolib.c
*/
int icv196SetTO(fdid,val)
                  int fdid;	/* file Id returned by sdevtconnect */
                  int *val;	/* at call: new TO, at return the old one*/
{
   int function;
   int L_val;
   int cc;
	/*  */

   function = ICVVME_setupTO;
		/* set up argument field for ioctl call  */
   L_val = *val;
 		/* Call the driver to perform the function */
   if ((cc = ioctl(fdid,function,(char *)&L_val)) < 0){
     perror("icv196SetTO:ioctl:");
     return(cc);
   }
		/* Give back data in user array */
   *val = L_val;
   return (OK);
 } /* fpiSetTO */

/* )%%% End of file icv196vmelib.c
*/


















