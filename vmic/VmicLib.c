/**
 * @file VmicLib.c
 *
 * @brief Library on top of the VMIC xmem driver
 *
 * @author Julian Lewis
 *
 * @date Created on 09/02/2005
 *
 * @version 1.1 Emilio G. Cota 22/01/2009
 *
 * @version 1.0 Julian Lewis
 */
#include <xmemDrvr.h>
#include <errno.h>


static int xmem = 0; //!< device file handler
static void (*callback)(XmemCallbackStruct *cbs) = NULL;
static XmemEventMask callmask = 0;

/**
 * VmicWriteSegTable - Set the list of all segments into the driver
 *
 * @param : none
 *
 * Tables need to have already been loaded; otherwise it will fail.
 *
 * @return XmemErrorSUCCESS on success
 * @return XmemErrorSYSTEM if tables had been loaded but operation fails
 * @return XmemErrorNO_TABLES if no tables had been loaded
 */
static XmemError VmicWriteSegTable()
{
	if (!seg_tab.Used)
		return XmemErrorNO_TABLES;
	/* write segment table into the driver */
	if (ioctl(xmem, XmemDrvrSET_SEGMENT_TABLE, &seg_tab) < 0)
		return XmemErrorSYSTEM;
	else
		return XmemErrorSUCCESS;
}


/**
 * VmicOpen - Open a VMIC device + driver
 *
 * @param : none
 *
 * @return opened file descriptor on success
 * @return 0 if no device was available
 */
static int VmicOpen()
{
	int  	i, fn;
	char 	fnm[32];

	for (i = 1; i <= XmemDrvrCLIENT_CONTEXTS; i++) {
		sprintf(fnm, "/dev/xmem.%1d", i);
		fn = open(fnm, O_RDWR, 0);
		if (fn > 0)
			return fn;
	}
	XmemErrorCallback(XmemErrorSYSTEM, errno);
	return 0;
}


/**
 * VmicInitialize - Initialize the device
 *
 * @param : none
 *
 * @return XmemErrorSUCCESS on success
 * @return XmemErrorNOT_INITIALIZED otherwise
 */
XmemError VmicInitialize()
{
	int debug = 0;
	XmemError 		err;
	XmemDrvrSegTable 	stb;
	XmemDrvrConnection 	con;
	XmemDrvrModuleDescriptor mdesc;

	if (xmem)
		return XmemErrorSUCCESS;
	xmem = VmicOpen();
	if (!xmem)
		goto notinit;

	bzero((void *)&stb, sizeof(XmemDrvrSegTable));
	/* read segment table into stb */
	if (ioctl(xmem, XmemDrvrGET_SEGMENT_TABLE, &stb) >= 0)
		warm_start = stb.Used; /* this is a warm start */

	err = VmicWriteSegTable();
	if (err != XmemErrorSUCCESS)
		goto notinit;
	/*
	 * BUG / FIXME
	 * setting con.Module = 0 --> the IOCTL handler will take the module
	 * from ccon->ModuleIndex, which is only set by the IOCTL SET_MODULE.
	 * This IOCTL is never called from the library; therefore we'll always
	 * connect to the interrupt from the same module (ModuleIndex = 0).
	 *
	 * (i.e. the library does not support 2 modules on the same machine!)
	 *
	 */
	con.Module = 0;
	con.Mask   = XmemDrvrIntrPENDING_INIT;
	if (ioctl(xmem, XmemDrvrCONNECT, &con) < 0)
		goto notinit;
	if (ioctl(xmem, XmemDrvrGET_MODULE_DESCRIPTOR, &mdesc) < 0)
		goto notinit;
	my_nid = 1 << (mdesc.NodeId - 1);

	/* RESET initialises the module if connected to PENDING_INIT */
	if (!warm_start)
		if (ioctl(xmem, XmemDrvrRESET, NULL) < 0)
			goto notinit;
	if (debug)
		if (ioctl(xmem,XmemDrvrSET_SW_DEBUG,&debug) < 0)
			goto notinit;
	return XmemErrorSUCCESS;
notinit:
	xmem = 0;
	return XmemErrorNOT_INITIALIZED;
}


/**
 * VmicGetAllNodeIds - Get all currently up and running nodes
 *
 * @param : none
 *
 * The set of up and running nodes is determined dynamically through a
 * handshake protocol between nodes.
 *
 * @return nodes connected to PENDING_INIT
 * @return 0 if there's an error
 */
XmemNodeId VmicGetAllNodeIds()
{
	XmemNodeId nodes;

	if (!xmem)
		return 0;
	/* Get the node map of all connected to PENDING_INIT */
	if (ioctl(xmem,XmemDrvrGET_NODES, &nodes) < 0) {
		XmemErrorCallback(XmemErrorSYSTEM, errno);
		return 0;
	}
	return nodes;
}


/**
 * VmicRegisterCallback - Register Callback to handle Xmem Events
 *
 * @param cb: callback descriptor (pointer to the user's callback handler)
 * @param mask: contains the events to subscribe to
 *
 * If the mask is valid, the client subscribes to the appropriate interrupts
 * and registers its callback handler cb to be called accordingly.
 *
 * @return Appropriate XmemError code
 */
XmemError VmicRegisterCallback(void (*cb)(XmemCallbackStruct *cbs),
			       XmemEventMask mask)
{
	int 		i;
	unsigned long 	msk, qflg;
	XmemDrvrConnection con;

	if (!xmem)
		goto notinit;
	con.Module = 0;
	con.Mask   = 0;
	if (! (mask & XmemEventMaskMASK)) {
		/* invalid or empty mask */
		con.Module = 0;
		con.Mask   = 0;
#if 0
		if (ioctl(xmem,XmemDrvrDISCONNECT,&con) < 0)
			return XmemErrorCallback(XmemErrorSYSTEM, errno);
#endif
		callback = NULL;
		callmask = 0;
		return XmemErrorSUCCESS;
	}
	for (i = 0; i < XmemEventMASKS; i++) {
		msk = 1 << i;
		if (! (msk & mask))
			continue;
		switch (msk) {
		case XmemEventMaskSEND_TABLE:
			con.Mask |= XmemDrvrIntrINT_2;
			callmask |= msk;
			break;
		case XmemEventMaskUSER:
			con.Mask |= XmemDrvrIntrINT_1;
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
		case XmemEventMaskSOFTWAKEUP:
			con.Mask |= XmemDrvrIntrSOFTWAKEUP;
			callmask |= msk;
			break;
		default:
			break;
		}
	}
	if (callmask) {
		qflg = 0; /* set queueing ON */
		if (ioctl(xmem, XmemDrvrSET_QUEUE_FLAG, &qflg) < 0)
			return XmemErrorCallback(XmemErrorSYSTEM, errno);
		if (ioctl(xmem, XmemDrvrCONNECT, &con) < 0)
			return XmemErrorCallback(XmemErrorSYSTEM, errno);
		callback = cb;
	}
	return XmemErrorSUCCESS;
notinit:
	return XmemErrorCallback(XmemErrorNOT_INITIALIZED, 0);
}


/**
 * VmicWait - Wait for an Event with timeout
 *
 * @param timeout: desired timeout for the wait (in chunks of 10ms)
 *
 * When there's an incoming event, the appropriate registered callback will be
 * executed.
 *
 * @return Mask with the incoming events handled (including timeout)
 * @return 0 if there was an error
 */
XmemEventMask VmicWait(int timeout)
{
	int 		i, cc;
	XmemDrvrIntr 	imsk;
	XmemEventMask 	emsk;
	XmemDrvrReadBuf rbf;
	XmemCallbackStruct cbs;

	if (!callmask)
		return 0;
	if (!xmem)
		return XmemErrorCallback(XmemErrorNOT_INITIALIZED, 0);
	if (ioctl(xmem, XmemDrvrSET_TIMEOUT, &timeout) < 0) {
		XmemErrorCallback(XmemErrorSYSTEM, errno);
		return 0;
	}
	bzero((void *)&cbs, sizeof(XmemCallbackStruct));
	cc = read(xmem, &rbf, sizeof(XmemDrvrReadBuf));
	if (cc <= 0) {
		cbs.Mask = XmemEventMaskTIMEOUT;
		if (callmask & XmemEventMaskTIMEOUT)
			callback(&cbs);
		return XmemEventMaskTIMEOUT;
	}
	emsk = 0;
	for (i = 0; i < XmemDrvrIntrSOURCES; i++) {
		imsk = 1 << i;
		switch (imsk & rbf.Mask) {
		case XmemDrvrIntrPARITY_ERROR:
			cbs.Mask = XmemEventMaskIO;
			cbs.Data = XmemIoErrorPARITY;
			emsk |= XmemEventMaskIO;

			if (callmask & XmemEventMaskIO)
				callback(&cbs);
			break;
		case XmemDrvrIntrRX_OVERFLOW:
		case XmemDrvrIntrRX_ALMOST_FULL:
		case XmemDrvrIntrDATA_ERROR:
			cbs.Mask = XmemEventMaskIO;
			cbs.Data = XmemIoErrorHARDWARE;
			emsk |= XmemEventMaskIO;
			if (callmask & XmemEventMaskIO)
				callback(&cbs);
			break;
		case XmemDrvrIntrLOST_SYNC:
			cbs.Mask = XmemEventMaskIO;
			cbs.Data = XmemIoErrorCONTACT;
			emsk |= XmemEventMaskIO;

			if (callmask & XmemEventMaskIO)
				callback(&cbs);
			break;
		case XmemDrvrIntrROGUE_CLOBBER:
			cbs.Mask = XmemEventMaskIO;
			cbs.Data = XmemIoErrorROGUE;
			emsk |= XmemEventMaskIO;
			if (callmask & XmemEventMaskIO)
				callback(&cbs);
			break;
		case XmemDrvrIntrPENDING_INIT:
			cbs.Mask = XmemEventMaskINITIALIZED;
			cbs.Node = 1 << (rbf.NodeId[XmemDrvrIntIdxPENDING_INIT] - 1);
			cbs.Data = rbf.NdData[XmemDrvrIntIdxPENDING_INIT];
			if (callmask & XmemEventMaskINITIALIZED)
				callback(&cbs);
			break;
		case XmemDrvrIntrREQUEST_RESET:
			cbs.Mask = XmemEventMaskKILL;
			emsk |= XmemEventMaskKILL;
			if (callmask & XmemEventMaskKILL)
				callback(&cbs);
			break;
		case XmemDrvrIntrSEGMENT_UPDATE:
			emsk |= XmemEventMaskTABLE_UPDATE;
			cbs.Mask  = XmemEventMaskTABLE_UPDATE;
			cbs.Node  = 1 << (rbf.NodeId[XmemDrvrIntIdxSEGMENT_UPDATE] - 1);
			cbs.Table = rbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE];
			if (callmask & XmemEventMaskTABLE_UPDATE)
				callback(&cbs);
			break;
		case XmemDrvrIntrINT_2:
			emsk |= XmemEventMaskSEND_TABLE;
			cbs.Mask  = XmemEventMaskSEND_TABLE;
			cbs.Node  = 1 << (rbf.NodeId[XmemDrvrIntIdxINT_2] - 1);
			cbs.Table = rbf.NdData[XmemDrvrIntIdxINT_2];
			if (callmask & XmemEventMaskSEND_TABLE)
				callback(&cbs);
			break;
		case XmemDrvrIntrINT_1:
			emsk |= XmemEventMaskUSER;
			cbs.Mask  = XmemEventMaskUSER;
			cbs.Node  = 1 << (rbf.NodeId[XmemDrvrIntIdxINT_1] - 1);
			cbs.Data  = rbf.NdData[XmemDrvrIntIdxINT_1];
			if (callmask & XmemEventMaskUSER)
				callback(&cbs);
			break;
		case XmemDrvrIntrSOFTWAKEUP:
			emsk |= XmemEventMaskSOFTWAKEUP;
			cbs.Mask  = XmemEventMaskSOFTWAKEUP;
			cbs.Node  = 1 << (rbf.NodeId[XmemDrvrIntIdxSOFTWAKEUP] - 1);
			cbs.Table = rbf.NdData[XmemDrvrIntIdxSOFTWAKEUP];
			if (callmask & XmemEventMaskSOFTWAKEUP)
				callback(&cbs);
			break;
		default:
			break;
		}
	}
	return emsk;
}


/**
 * VmicPoll - Poll for any incoming Xmem Events
 *
 * @param : none
 *
 * An incoming event will call any callback that's registered for that event.
 *
 * This isn't good if there is a lot of stuff on the queue !!
 * See Jean-Claude Bau's implementation
 *
 * @return VmicWait() if there's data in the queue
 * @return 0 if there's an error or the queue is empty
 */
XmemEventMask VmicPoll()
{
	unsigned long qsize;

	if (!xmem)
		return 0;
	if (ioctl(xmem, XmemDrvrGET_QUEUE_SIZE, &qsize) < 0) {
		XmemErrorCallback(XmemErrorSYSTEM, errno);
		return 0;
	}
	if (qsize)
		return VmicWait(0);
	return 0;
}


/**
 * VmicSendMessage - Send a message to other nodes
 *
 * @param nodes: receiving nodes
 * @param mess: message
 *
 * Send a message to other nodes. If the node parameter is zero, then
 * nothing happens, if its equal to 0xFFFFFFFF the message is sent via
 * broadcast; else multicast or unicast is used.
 *
 * @return Appropriate error code (XmemError)
 */
XmemError VmicSendMessage(XmemNodeId nodes, XmemMessage *mess)
{
	int 		i, cnt;
	unsigned long 	mtyp, mcst, nnum;
	XmemNodeId 	nmsk;
	XmemDrvrSendBuf msbf;

	if (!xmem)
		return XmemErrorCallback(XmemErrorNOT_INITIALIZED, 0);
	bzero((void *) &msbf, sizeof(XmemDrvrSendBuf));
	if (nodes == XmemALL_NODES)
		mcst = XmemDrvrNicBROADCAST;
	else { /* MULTICAST or UNICAST */
		cnt = 0;
		for (i = 0; i < XmemMAX_NODES; i++) {
			nmsk = 1 << i;
			if (nmsk & nodes) {
				cnt++;
				nnum = i + 1;
			}
			if (cnt > 1)
				break;
		}
		if (cnt == 1) {
			mcst = XmemDrvrNicUNICAST;
			msbf.UnicastNodeId = nnum;
		}
		else {
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
		return XmemErrorCallback(XmemErrorNO_SUCH_MESSAGE, 0);
	}
	msbf.Module        = 1;
	msbf.InterruptType = mtyp | mcst;
	if (ioctl(xmem, XmemDrvrSEND_INTERRUPT, &msbf) < 0)
		return XmemErrorCallback(XmemErrorSYSTEM, errno);
	return XmemErrorSUCCESS;
}


/**
 * VmicSendTable - Send a buffer to a reflective memory table
 *
 * @param tid: table to be written to
 * @param buf: buffer containing the data
 * @param elems: number of elements (4 bytes) to transfer
 * @param offset: offset whithin the table
 * @param upflag: update message flag
 *
 * A table update event is broadcast if the upflag contains a non zero value.
 *
 * If @buf is NULL, table @tid is flushed.
 *
 * @return Appropriate error code (XmemError)
 */
XmemError VmicSendTable(XmemTableId tid, void *buf, int elems,
			int offset, int upflag)
{
	XmemMessage mess;
	XmemDrvrSegIoDesc sgio;

	if (!xmem)
		return XmemErrorCallback(XmemErrorNOT_INITIALIZED, 0);
	if (buf == NULL) {
		if (ioctl(xmem, XmemDrvrFLUSH_SEGMENTS, &tid) < 0)
			return XmemErrorCallback(XmemErrorSYSTEM, errno);
	}
	else {
		sgio.Module    = 1;
		sgio.Id        = tid;
		sgio.Size      = elems * sizeof(uint32_t);
		sgio.Offset    = offset * sizeof(uint32_t);
		sgio.UserArray = buf;
		sgio.UpdateFlg = 0;
		if (ioctl(xmem, XmemDrvrWRITE_SEGMENT, &sgio) < 0) {
			if (errno == EACCES)
				return XmemErrorCallback(XmemErrorWRITE_PROTECTED, 0);
			else
				return XmemErrorCallback(XmemErrorSYSTEM, errno);
		}
	}
	if (upflag) {
		mess.MessageType = XmemMessageTypeTABLE_UPDATE;
		mess.Data        = tid;
		return VmicSendMessage(XmemALL_NODES, &mess);
	}
	return XmemErrorSUCCESS;
}


/**
 * VmicRecvTable - Update buffer from a reflective memory table
 *
 * @param tid: table to read from
 * @param buf: buffer to be updated
 * @param elems: number of elements (4 bytes) to transfer
 * @param offset: offset whithin the table
 *
 * @return Appropriate Error code (XmemError)
 */
XmemError VmicRecvTable(XmemTableId tid, void *buf, int elems,
			int offset)
{
	XmemDrvrSegIoDesc sgio;

	if (!xmem)
		return XmemErrorCallback(XmemErrorNOT_INITIALIZED, 0);
	sgio.Module    = 1;
	sgio.Id        = tid;
	sgio.Size      = elems * sizeof(uint32_t);
	sgio.Offset    = offset * sizeof(uint32_t);
	sgio.UserArray = buf;
	sgio.UpdateFlg = 0;
	if (ioctl(xmem, XmemDrvrREAD_SEGMENT, &sgio) < 0)
		return XmemErrorCallback(XmemErrorSYSTEM, errno);
	return XmemErrorSUCCESS;
}


/**
 * VmicCheckTables - Check which tables were updated
 *
 * @param : none
 *
 * @return mask with the updated tables
 * @return 0 if there was an error or no tables updated
 */
XmemTableId VmicCheckTables() {

	XmemTableId tmsk;

	if (!xmem)
		return 0;
	if (ioctl(xmem, XmemDrvrCHECK_SEGMENT, &tmsk) < 0) {
		XmemErrorCallback(XmemErrorSYSTEM, errno);
		return 0;
	}
	return tmsk;
}

XmemError VmicSendSoftWakeup(uint32_t nodeid, uint32_t data)
{
	XmemDrvrWriteBuf wbf = {
		.Module	= 1,
		.Data	= data,
		.NodeId	= nodeid,
	};

	if (write(xmem, &wbf, sizeof(XmemDrvrWriteBuf)) <= 0)
		return XmemErrorIO;
	return XmemErrorSUCCESS;
}
