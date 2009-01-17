/* ==================================================================== */
/* Implement a library to handle reflective memory VMIC back end        */
/* Julian Lewis 9th/Feb/2005                                            */
/* ==================================================================== */

#include <xmemDrvr.h>
#include <errno.h>

/* ==================================================================== */
/* Static stuff.                                                        */

static int xmem = 0;
static void (*callback)(XmemCallbackStruct *cbs) = NULL;
static XmemEventMask callmask = 0;

/*****************************************************************/
/* Write segment table to the driver                             */

static XmemError VmicWriteSegTable() {

   if (seg_tab.Used) {
      if (ioctl(xmem,XmemDrvrSET_SEGMENT_TABLE,&seg_tab) < 0) return XmemErrorSYSTEM;
      else                                                    return XmemErrorSUCCESS;
   }
   return XmemErrorNO_TABLES;
}

/*****************************************************************/
/* Open the VMIC driver                                          */

static int VmicOpen() {

char fnm[32];
int  i, fn;

   for (i = 1; i <= XmemDrvrCLIENT_CONTEXTS; i++) {
      sprintf(fnm,"/dev/xmem.%1d",i);
      if ((fn = open(fnm,O_RDWR,0)) > 0) {
	 return(fn);
      }
   }
   XmemErrorCallback(XmemErrorSYSTEM,errno);
   return 0;
}

/* ==================================================================== */
/* Try to initialize the specified device                               */

XmemError VmicInitialize() { /* Initialize hardware/software */

XmemDrvrModuleDescriptor mdesc;
XmemDrvrConnection con;
XmemError err;
XmemDrvrSegTable stb;
int debug = 0;

   if (xmem == 0) {
      xmem = VmicOpen();
      if (xmem) {
	 bzero((void *) &stb, sizeof(XmemDrvrSegTable));
	 if (ioctl(xmem,XmemDrvrGET_SEGMENT_TABLE,&stb) >=0) warm_start = stb.Used;
	 err = VmicWriteSegTable();
	 if (err == XmemErrorSUCCESS) {
	    con.Module = 0;
	    con.Mask   = XmemDrvrIntrPENDING_INIT;
	    if (ioctl(xmem,XmemDrvrCONNECT,&con) >= 0) {
	       ioctl(xmem,XmemDrvrGET_MODULE_DESCRIPTOR,&mdesc);
	       my_nid = 1 << (mdesc.NodeId -1);
	       if (warm_start == 0) ioctl(xmem,XmemDrvrRESET,NULL);
	       if (debug) ioctl(xmem,XmemDrvrSET_SW_DEBUG,&debug);
	       return XmemErrorSUCCESS;
	    }
	 }
      }
   } else return XmemErrorSUCCESS;
   xmem = 0;
   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Get all currently up and running nodes.                              */
/* The set of up and running nodes is determined dynamically through a  */
/* handshake protocol between nodes.                                    */

XmemNodeId VmicGetAllNodeIds() {

XmemNodeId nodes;

   if (xmem) {
      if (ioctl(xmem,XmemDrvrGET_NODES,&nodes) < 0) {
	 XmemErrorCallback(XmemErrorSYSTEM,errno);
	 return 0;
      }
      return nodes;
   }
   return 0;
}

/* ==================================================================== */
/* Register your call back to handle Xmem Events.                       */

XmemError VmicRegisterCallback(void (*cb)(XmemCallbackStruct *cbs),
			       XmemEventMask mask) {
XmemDrvrConnection con;
unsigned long msk, qflg;
int i;

   if (xmem) {

      con.Module = 0;
      con.Mask   = 0;

      if (mask & XmemEventMaskMASK) {
	 for (i=0; i<XmemEventMASKS; i++) {
	    msk = 1 << i;
	    if (msk & mask) {
	       switch (msk) {

		  case XmemEventMaskSEND_TABLE:
		     con.Mask |= XmemDrvrIntrINT_2;    /* I use this for Send-ME-a-Table */
		     callmask |= msk;
		  break;

		  case XmemEventMaskUSER:
		     con.Mask |= XmemDrvrIntrINT_1;    /* For library user */
		     callmask |= msk;
		  break;

		  case XmemEventMaskTABLE_UPDATE:
		     con.Mask |= XmemDrvrIntrSEGMENT_UPDATE;
		     callmask |= msk;
		  break;

		  case XmemEventMaskINITIALIZED:
		     con.Mask |= XmemDrvrIntrPENDING_INIT;
		     callmask |= msk;
		  break;

		  case XmemEventMaskIO:
		     con.Mask |= XmemDrvrIntrPARITY_ERROR   |
				 XmemDrvrIntrLOST_SYNC      |
				 XmemDrvrIntrRX_OVERFLOW    |
				 XmemDrvrIntrRX_ALMOST_FULL |
				 XmemDrvrIntrDATA_ERROR     |
				 XmemDrvrIntrROGUE_CLOBBER  ;
		     callmask |= msk;
		  break;

		  case XmemEventMaskKILL:
		     con.Mask |= XmemDrvrIntrREQUEST_RESET;
		     callmask |= msk;
		  break;

		  default: break;
	       }
	    }
	 }
	 if (callmask) {
	    qflg = 0; ioctl(xmem,XmemDrvrSET_QUEUE_FLAG,&qflg);
	    if (ioctl(xmem,XmemDrvrCONNECT,&con) < 0)
	       return XmemErrorCallback(XmemErrorSYSTEM,errno);

	    callback = cb;
	 }
      } else {
	 con.Module = 0;
	 con.Mask   = 0;
#if 0
	 if (ioctl(xmem,XmemDrvrDISCONNECT,&con) < 0)
	    return XmemErrorCallback(XmemErrorSYSTEM,errno);
#endif
	 callback = NULL;
	 callmask = 0;
      }
      return XmemErrorSUCCESS;
   }
   return XmemErrorCallback(XmemErrorNOT_INITIALIZED,0);
}

/* ==================================================================== */

static void VmicUsleep(int dly) {
   struct timespec rqtp, rmtp; /* 'nanosleep' time structure       */
   rqtp.tv_sec = 0;
   rqtp.tv_nsec = dly*1000;
   nanosleep(&rqtp, &rmtp);
}

/* ==================================================================== */

static XmemDrvrReadBuf drbf;
static int first_time = 1;
static int dqsze = 0;

int VmicDaemonWait(int timeout) {

   if (first_time) {
      if (ioctl(xmem,XmemDrvrSET_TIMEOUT,&timeout) >= 0) first_time = 0;
   }
   bzero((void *) &drbf, sizeof(XmemDrvrReadBuf));
   if (read(xmem,&drbf,sizeof(XmemDrvrReadBuf)) > 0) {
      ioctl(xmem,XmemDrvrGET_QUEUE_SIZE,&dqsze);
      return 1;
   }
   return 0;
}

/* ==================================================================== */

void VmicDaemonCall() {

int i;
XmemDrvrIntr imsk;
XmemEventMask emsk;
XmemCallbackStruct cbs;

   emsk = 0;
   for (i=0; i<XmemDrvrIntrSOURCES; i++) {
      imsk = 1 << i;
      bzero((void *) &cbs, sizeof(XmemCallbackStruct));
      switch (imsk & drbf.Mask) {

	 case XmemDrvrIntrPARITY_ERROR:
	    cbs.Mask = XmemEventMaskIO;
	    cbs.Data = XmemIoErrorPARITY;
	    emsk |= XmemEventMaskIO;
	    if (callmask & XmemEventMaskIO) callback(&cbs);
	 break;

	 case XmemDrvrIntrRX_OVERFLOW:
	 case XmemDrvrIntrRX_ALMOST_FULL:
	 case XmemDrvrIntrDATA_ERROR:
	    cbs.Mask = XmemEventMaskIO;
	    cbs.Data = XmemIoErrorHARDWARE;
	    emsk |= XmemEventMaskIO;
	    if (callmask & XmemEventMaskIO) callback(&cbs);
	 break;

	 case XmemDrvrIntrLOST_SYNC:
	    cbs.Mask = XmemEventMaskIO;
	    cbs.Data = XmemIoErrorCONTACT;
	    emsk |= XmemEventMaskIO;
	    if (callmask & XmemEventMaskIO) callback(&cbs);
	 break;

	 case XmemDrvrIntrROGUE_CLOBBER:
	    cbs.Mask = XmemEventMaskIO;
	    cbs.Data = XmemIoErrorROGUE;
	    emsk |= XmemEventMaskIO;
	    if (callmask & XmemEventMaskIO) callback(&cbs);
	 break;

	 case XmemDrvrIntrPENDING_INIT:
	    cbs.Mask = XmemEventMaskINITIALIZED;
	    cbs.Node = 1 << (drbf.NodeId[XmemDrvrIntIdxPENDING_INIT] -1);
	    cbs.Data = drbf.NdData[XmemDrvrIntIdxPENDING_INIT];
	    if (callmask & XmemEventMaskINITIALIZED) callback(&cbs);
	 break;

	 case XmemDrvrIntrREQUEST_RESET:
	    cbs.Mask = XmemEventMaskKILL;
	    emsk |= XmemEventMaskKILL;
	    if (callmask & XmemEventMaskKILL) callback(&cbs);
	 break;

	 case XmemDrvrIntrSEGMENT_UPDATE:
	    emsk |= XmemEventMaskTABLE_UPDATE;
	    cbs.Mask  = XmemEventMaskTABLE_UPDATE;
	    cbs.Node  = 1 << (drbf.NodeId[XmemDrvrIntIdxSEGMENT_UPDATE] -1);
	    cbs.Table = drbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE];
	    if (callmask & XmemEventMaskTABLE_UPDATE) callback(&cbs);
	 break;

	 case XmemDrvrIntrINT_2:
	    emsk |= XmemEventMaskSEND_TABLE;
	    cbs.Mask  = XmemEventMaskSEND_TABLE;
	    cbs.Node  = 1 << (drbf.NodeId[XmemDrvrIntIdxINT_2] -1);
	    cbs.Table = drbf.NdData[XmemDrvrIntIdxINT_2];
	    if (callmask & XmemEventMaskSEND_TABLE) callback(&cbs);
	 break;

	 case XmemDrvrIntrINT_1:
	    emsk |= XmemEventMaskUSER;
	    cbs.Mask  = XmemEventMaskUSER;
	    cbs.Node  = 1 << (drbf.NodeId[XmemDrvrIntIdxINT_1] -1);
	    cbs.Data  = drbf.NdData[XmemDrvrIntIdxINT_1];
	    if (callmask & XmemEventMaskUSER) callback(&cbs);
	 break;

	 default: break;
      }
   }
}

/* ==================================================================== */
/* Wait for an Xmem Event with time out, an incomming event will call   */
/* any registered callback.                                             */

XmemEventMask VmicWait(int timeout) {

int i, cc;
XmemDrvrIntr imsk;
XmemEventMask emsk;
XmemDrvrReadBuf rbf;
XmemCallbackStruct cbs;

   if (callmask == 0) return 0;

   if (xmem) {
      if (ioctl(xmem,XmemDrvrSET_TIMEOUT,&timeout) < 0) {
	 XmemErrorCallback(XmemErrorSYSTEM,errno);
	 return 0;
      }

      cc = read(xmem,&rbf,sizeof(XmemDrvrReadBuf));
      if (cc <= 0) {
	 bzero((void *) &cbs, sizeof(XmemCallbackStruct));
	 cbs.Mask = XmemEventMaskTIMEOUT;
	 if (callmask & XmemEventMaskTIMEOUT) callback(&cbs);
	 return XmemEventMaskTIMEOUT;
      }

      i = 0;
      while (XmemCallbacksBlocked()) {
	 VmicUsleep(1000);
	 if (i++ > 1000) {
	    bzero((void *) &cbs, sizeof(XmemCallbackStruct));
	    cbs.Mask = XmemEventMaskTIMEOUT;
	    if (callmask & XmemEventMaskTIMEOUT) callback(&cbs);
	    return XmemEventMaskTIMEOUT;
	 }
      }

      emsk = 0;
      for (i=0; i<XmemDrvrIntrSOURCES; i++) {
	 imsk = 1 << i;
	 bzero((void *) &cbs, sizeof(XmemCallbackStruct));
	 switch (imsk & rbf.Mask) {

	    case XmemDrvrIntrPARITY_ERROR:
	       cbs.Mask = XmemEventMaskIO;
	       cbs.Data = XmemIoErrorPARITY;
	       emsk |= XmemEventMaskIO;
	       if (callmask & XmemEventMaskIO) callback(&cbs);
	    break;

	    case XmemDrvrIntrRX_OVERFLOW:
	    case XmemDrvrIntrRX_ALMOST_FULL:
	    case XmemDrvrIntrDATA_ERROR:
	       cbs.Mask = XmemEventMaskIO;
	       cbs.Data = XmemIoErrorHARDWARE;
	       emsk |= XmemEventMaskIO;
	       if (callmask & XmemEventMaskIO) callback(&cbs);
	    break;

	    case XmemDrvrIntrLOST_SYNC:
	       cbs.Mask = XmemEventMaskIO;
	       cbs.Data = XmemIoErrorCONTACT;
	       emsk |= XmemEventMaskIO;
	       if (callmask & XmemEventMaskIO) callback(&cbs);
	    break;

	    case XmemDrvrIntrROGUE_CLOBBER:
	       cbs.Mask = XmemEventMaskIO;
	       cbs.Data = XmemIoErrorROGUE;
	       emsk |= XmemEventMaskIO;
	       if (callmask & XmemEventMaskIO) callback(&cbs);
	    break;

	    case XmemDrvrIntrPENDING_INIT:
	       cbs.Mask = XmemEventMaskINITIALIZED;
	       cbs.Node = 1 << (rbf.NodeId[XmemDrvrIntIdxPENDING_INIT] -1);
	       cbs.Data = rbf.NdData[XmemDrvrIntIdxPENDING_INIT];
	       if (callmask & XmemEventMaskINITIALIZED) callback(&cbs);
	    break;

	    case XmemDrvrIntrREQUEST_RESET:
	       cbs.Mask = XmemEventMaskKILL;
	       emsk |= XmemEventMaskKILL;
	       if (callmask & XmemEventMaskKILL) callback(&cbs);
	    break;

	    case XmemDrvrIntrSEGMENT_UPDATE:
	       emsk |= XmemEventMaskTABLE_UPDATE;
	       cbs.Mask  = XmemEventMaskTABLE_UPDATE;
	       cbs.Node  = 1 << (rbf.NodeId[XmemDrvrIntIdxSEGMENT_UPDATE] -1);
	       cbs.Table = rbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE];
	       if (callmask & XmemEventMaskTABLE_UPDATE) callback(&cbs);
	    break;

	    case XmemDrvrIntrINT_2:
	       emsk |= XmemEventMaskSEND_TABLE;
	       cbs.Mask  = XmemEventMaskSEND_TABLE;
	       cbs.Node  = 1 << (rbf.NodeId[XmemDrvrIntIdxINT_2] -1);
	       cbs.Table = rbf.NdData[XmemDrvrIntIdxINT_2];
	       if (callmask & XmemEventMaskSEND_TABLE) callback(&cbs);
	    break;

	    case XmemDrvrIntrINT_1:
	       emsk |= XmemEventMaskUSER;
	       cbs.Mask  = XmemEventMaskUSER;
	       cbs.Node  = 1 << (rbf.NodeId[XmemDrvrIntIdxINT_1] -1);
	       cbs.Data  = rbf.NdData[XmemDrvrIntIdxINT_1];
	       if (callmask & XmemEventMaskUSER) callback(&cbs);
	    break;

	    default: break;
	 }
      }
      return emsk;
   }
   return XmemErrorCallback(XmemErrorNOT_INITIALIZED,0);
}

/* ==================================================================== */
/* Poll for any Xmem Events, an incomming event will call any callback  */
/* that is registered for that event.                                   */

/* This isn't good if there is a lot of stuff on the queue !! */
/* See JCB's implementatuion */

XmemEventMask VmicPoll() {

unsigned long qsize;

   if (xmem) {
      if (ioctl(xmem,XmemDrvrGET_QUEUE_SIZE,&qsize) < 0) {
	 XmemErrorCallback(XmemErrorSYSTEM,errno);
	 return 0;
      }
      if (qsize) return VmicWait(0);
   }
   return 0;
}

/* ==================================================================== */
/* Send a message to other nodes. If the node parameter is zero, then   */
/* nothing happens, if its equal to 0xFFFFFFFF the message is sent via  */
/* broadcast, else multicast or unicast is used.                        */

XmemError VmicSendMessage(XmemNodeId nodes, XmemMessage *mess) {

XmemDrvrSendBuf msbf;

int i, cnt;
XmemNodeId nmsk;
unsigned long mtyp, mcst, nnum;

   if (xmem) {

      bzero((void *) &msbf, sizeof(XmemDrvrSendBuf));

      if (nodes == XmemALL_NODES) mcst = XmemDrvrNicBROADCAST;
      else {
	 cnt = 0;
	 for (i=0; i<XmemMAX_NODES; i++) {
	    nmsk = 1 << i;
	    if (nmsk & nodes) {
	       cnt++;
	       nnum = i +1;
	    }
	    if (cnt > 1) break;
	 }

	 if (cnt == 1) {
	    mcst = XmemDrvrNicUNICAST;
	    msbf.UnicastNodeId = nnum;
	 } else {
	    mcst = XmemDrvrNicMULTICAST;
	    msbf.MulticastMask = nodes;
	 }
      }

      switch (mess->MessageType) {

	 case XmemMessageTypeSEND_TABLE:
	    mtyp = XmemDrvrNicINT_2;
	    msbf.Data = mess->Data;
	 break;

	 case XmemMessageTypeUSER:
	    mtyp = XmemDrvrNicINT_1;
	    msbf.Data = mess->Data;
	 break;

	 case XmemMessageTypeTABLE_UPDATE:
	    mtyp = XmemDrvrNicSEGMENT_UPDATE;
	    msbf.Data = mess->Data;
	 break;

	 case XmemMessageTypeINITIALIZE_ME:
	    mtyp = XmemDrvrNicINITIALIZED;
	    msbf.Data = mess->Data;
	 break;

	 case XmemMessageTypeKILL:
	    mtyp = XmemDrvrNicREQUEST_RESET;
	    msbf.Data = mess->Data;
	 break;

	 default:
	    return XmemErrorCallback(XmemErrorNO_SUCH_MESSAGE,0);
      }

      msbf.Module        = 1;
      msbf.InterruptType = mtyp | mcst;

      if (ioctl(xmem,XmemDrvrSEND_INTERRUPT,&msbf) < 0)
	 return XmemErrorCallback(XmemErrorSYSTEM,errno);

      return XmemErrorSUCCESS;
   }
   return XmemErrorCallback(XmemErrorNOT_INITIALIZED,0);
}

/* =========================================================================== */
/* Send your buffer to a reflective memory table. A table update event         */
/* is broadcast if the upflag contains a non zero value.                       */
/* This logic may look a bit strange, but I need to do it this way because    */
/* the pages of shared memory are not usually contiguous, and so the DMA has  */
/* to be set up bage by page.                                                 */

XmemError VmicSendTable(XmemTableId   tid,      /* Table to be written to      */
				 long *buf,     /* Buffer containing table     */
			unsigned long longs,    /* Number of longs to transfer */
			unsigned long offset,   /* Offset of transfer          */
			unsigned long upflag) { /* Update message flag         */

XmemMessage mess;
XmemDrvrSegIoDesc sgio;

   if (xmem) {
      if (buf == NULL) {
	 if (ioctl(xmem,XmemDrvrFLUSH_SEGMENTS,&tid) < 0) {
	    return XmemErrorCallback(XmemErrorSYSTEM,errno);
	 }
      } else {

	 sgio.Module    = 1;
	 sgio.Id        = tid;
	 sgio.Size      = longs*sizeof (long);  /* Size in bytes */
	 sgio.Offset    = offset*sizeof(long);
	 sgio.UserArray = (char *) buf;
	 sgio.UpdateFlg = 0;

	 if (ioctl(xmem,XmemDrvrWRITE_SEGMENT,&sgio) < 0) {
	    if (errno == EINVAL) return XmemErrorCallback(XmemErrorWRITE_PROTECTED,0);
	    else                 return XmemErrorCallback(XmemErrorSYSTEM,errno);
	 }
      }
      if (upflag) {
	 mess.MessageType = XmemMessageTypeTABLE_UPDATE;
	 mess.Data        = tid;
	 return VmicSendMessage(XmemALL_NODES,&mess);
      }
      return XmemErrorSUCCESS;
   }
   return XmemErrorCallback(XmemErrorNOT_INITIALIZED,0);
}

/* ========================================================================== */
/* Update your buffer from a reflective memory table.                         */
/* This logic may look a bit strange, but I need to do it this way because    */
/* the pages of shared memory are not usually contiguous, and so the DMA has  */
/* to be set up bage by page.                                                 */

XmemError VmicRecvTable(XmemTableId   tid,      /* Table to be read from      */
				 long *buf,     /* Buffer containing table    */
			unsigned long longs,    /* Number of longs to transfer*/
			unsigned long offset) { /* Offset of transfer         */

XmemDrvrSegIoDesc sgio;

   if (xmem) {

      sgio.Module    = 1;
      sgio.Id        = tid;
      sgio.Size      = longs*sizeof (long);  /* Size in bytes */
      sgio.Offset    = offset*sizeof(long);
      sgio.UserArray = (char *) buf;
      sgio.UpdateFlg = 0;

      if (ioctl(xmem,XmemDrvrREAD_SEGMENT,&sgio) < 0)
	 return XmemErrorCallback(XmemErrorSYSTEM,errno);

      return XmemErrorSUCCESS;
   }
   return XmemErrorCallback(XmemErrorNOT_INITIALIZED,0);
}

/* ==================================================================== */
/* Check which tables were updated.                                     */

XmemTableId VmicCheckTables() {

XmemTableId tmsk;

   if (xmem) {
      if (ioctl(xmem,XmemDrvrCHECK_SEGMENT,&tmsk) < 0) {
	 XmemErrorCallback(XmemErrorSYSTEM,errno);
	 return 0;
      }
      return tmsk;
   }
   return 0;
}
