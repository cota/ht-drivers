/**
 * @file icv196vmelib.c
 *
 * @brief icv196 library interface
 *
 * <long description>
 *
 * @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 18/12/2009
 *
 * @section license_sec License
 *          Released under the GPL
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

static int fdid_icvservice = -1; /* currently open on service handle */
static char Hdlservice[] = "/dev/icv196service";

int icv196_get_handle()
{
	return -1;
}

int icv196_put_handle()
{
	return -1;
}

int icv196_connect()
{
	return -1;
}

int icv196_disconnect()
{
	return -1;
}

int icv196_read()
{
	return -1;
}

int icv196_write()
{
	return -1;
}

int icv196_get_info(int m, int buff_sz, struct icv196T_ModuleInfo *buff)
{
	int function;
	int cc;

   if (fdid_icvservice < 0) {
     if ((cc = open( Hdlservice, O_RDONLY, 0755)) < 0){
       perror("icv196GetSlotInfo:open");
       return -1;
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
   return 0;
}

/**
 * @brief
 *
 * @param fdid -- file Id returned by sdevtconnect
 * @param val  -- at call: new TO, at return the old one
 *
 * Set time out for waiting Event on read of synchronization channel
 * the synchronization is requested by mean of the general purpose
 * function gpevtconnect see in real time facilities: rtfclty/gpsynchrolib.c
 *
 * @return <ReturnValue>
 */
int icv196_set_to(int fdid, int *val)
{
   int function;
   int L_val;
   int cc;

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
   return 0;
}


















