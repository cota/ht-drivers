/* ==================================================================== */
/* Implement a library to handle reflective memory Shmem back end.      */
/* This device emulates reflective memory via the symp driver.          */
/* Julian Lewis 14th Jan 2008                                           */
/* ==================================================================== */

/* Simple synchronization driver */

#include <sympdrvr.h>

int symp = 0;
static SympDrvrIoBuf iob;
static XmemCallbackStruct *cbs = (XmemCallbackStruct *) &(iob.IoBuf);
static XmemTableId tmsk = 0;

/* ==================================================================== */
/* Try to initialize the specified device                               */

XmemError ShmemInitialize() { /* Initialize hardware/software */

char fnm[32];
int  i, fn;

   if (xmem == 0) {
      for (i = 1; i <= SympDrvrCLIENT_CONTEXTS; i++) {
	 sprintf(fnm,"/dev/symp.%1d",i);
	 if ((fn = open(fnm,O_RDWR,0)) > 0) {
	    symp = fn;
	    xmem = symp;
	    my_nid = 1;
	    return XmemErrorSUCCESS;
	 }
      }
      XmemErrorCallback(XmemErrorNOT_INITIALIZED,0);
   }
   return XmemErrorSUCCESS;
}

/* ==================================================================== */
/* Get all currently up and running nodes.                              */
/* The set of up and running nodes is determined dynamically through a  */
/* handshake protocol between nodes.                                    */

XmemNodeId ShmemGetAllNodeIds() {

   return my_nid;
}

/* ==================================================================== */
/* Register your call back to handle Xmem Events.                       */

XmemError ShmemRegisterCallback(void (*cb)(XmemCallbackStruct *cbs),
			       XmemEventMask mask) {

   callmask = (unsigned long) mask;
   if (ioctl(symp,SympDrvrCONNECT,&callmask) < 0)
      return XmemErrorCallback(XmemErrorSYSTEM,errno);

   callback = cb;
   return XmemErrorSUCCESS;
}

/* ==================================================================== */
/* Wait for an Xmem Event with time out, an incomming event will call   */
/* any registered callback.                                             */

XmemEventMask ShmemWait(int timeout) {

int cc;

   if (callmask == 0) return 0;

   if (ioctl(symp,SympDrvrSET_TIMEOUT,&timeout) < 0)
      return XmemErrorCallback(XmemErrorSYSTEM,errno);

   cc = read(symp,&iob,sizeof(SympDrvrIoBuf));
   if (cc <= 0) {
      bzero((void *) cbs, sizeof(XmemCallbackStruct));
      cbs->Mask = XmemEventMaskTIMEOUT;
      if (callmask & XmemEventMaskTIMEOUT) callback(cbs);
      return XmemEventMaskTIMEOUT;
   }
   if (cbs->Mask & XmemEventMaskTABLE_UPDATE) {
      tmsk |= (1 << (cbs->Table -1));
   }

   callback(cbs);

   return (XmemEventMask) iob.Mask;
}

/* ==================================================================== */
/* Poll for any Xmem Events, an incomming event will call any callback  */
/* that is registered for that event.                                   */

XmemEventMask ShmemPoll() {

unsigned long qsize;

   if (ioctl(symp,SympDrvrGET_QUEUE_SIZE,&qsize) < 0) {
      XmemErrorCallback(XmemErrorSYSTEM,errno);
      return 0;
   }
   if (qsize) return ShmemWait(0);
   return 0;
}

/* ==================================================================== */
/* Send a message to other nodes. If the node parameter is zero, then   */
/* nothing happens, if its equal to 0xFFFFFFFF the message is sent via  */
/* broadcast, else multicast or unicast is used.                        */

XmemError ShmemSendMessage(XmemNodeId nodes, XmemMessage *mess) {

int cc;

   cbs->Node  = my_nid;

   switch (mess->MessageType) {

      case XmemMessageTypeSEND_TABLE:
	 cbs->Table = mess->Data;
	 cbs->Mask  = XmemEventMaskSEND_TABLE;
      break;

      case XmemMessageTypeUSER:
	 cbs->Data  = mess->Data;
	 cbs->Mask  = XmemEventMaskUSER;
      break;

      case XmemMessageTypeTABLE_UPDATE:
	 cbs->Table = mess->Data;
	 cbs->Mask  = XmemEventMaskTABLE_UPDATE;
      break;

      case XmemMessageTypeINITIALIZE_ME:
	 cbs->Data  = mess->Data;
	 cbs->Mask = XmemEventMaskINITIALIZED;
      break;

      case XmemMessageTypeKILL:
	 cbs->Mask = XmemEventMaskKILL;
      break;

      default:
	 return XmemErrorCallback(XmemErrorNO_SUCH_MESSAGE,0);
   }
   iob.Mask = cbs->Mask;
   cc = write(symp,&iob,sizeof(SympDrvrIoBuf));
   return XmemErrorSUCCESS;
}

/* ==================================================================== */
/* Send your buffer to a reflective memory table. A table update event  */
/* is broadcast automatically.                                          */

XmemError ShmemSendTable(XmemTableId   tid,     /* Table to be written to      */
				 void *buf,     /* Buffer containing table     */
				 int elems, /* Nr of double words to transfer */
				 int offset,   /* Offset of transfer          */
				 int upflag) { /* Update message flag         */

XmemMessage mess;

   if (upflag) {
      mess.MessageType = XmemMessageTypeTABLE_UPDATE;
      mess.Data        = tid;
      return ShmemSendMessage(my_nid,&mess);
   }
   return XmemErrorSUCCESS;
}

/* ==================================================================== */
/* Update your buffer from a reflective memory table.                   */

XmemError ShmemRecvTable(XmemTableId table, void *buf) {

   return XmemErrorSUCCESS;
}

/* ==================================================================== */
/* Check which tables were updated.                                     */

XmemTableId ShmemCheckTables() {
XmemTableId tid;

   tid = tmsk;
   tmsk = 0;
   return tid;
}

XmemError ShmemSendSoftWakeup(uint32_t nodeid, uint32_t data)
{
	return 0;
}
