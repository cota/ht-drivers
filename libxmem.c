/**
 * @file libxmem.c
 *
 * @brief XMEM Reflective Memory Library Implementation
 *
 * @author Julian Lewis
 *
 * @date Created on 09/02/2005
 *
 * @version 1.1 Emilio G. Cota 16/01/2009
 *
 * @version 1.0 Julian Lewis
 */
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <stat.h>
#include <sched.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <mqueue.h>
#include <time.h>
#include <errno.h>
#include <sys/shm.h>
#include <ipc.h>
#include <sem.h>

#include <xmemDrvr.h>
#include <libxmem.h>

/*! @name device specific backend code
 *
 */
//@{
#define LN 128 //!< length of string (e.g. for full filenames)

static long 		*attach_address[XmemMAX_TABLES];

static XmemDrvrSegTable seg_tab;
static int 		segcnt = 0;

static XmemDrvrNodeTable node_tab;
static int 		nodecnt = 0;

static XmemNodeId 	my_nid = 0;	//!< My node ID
static int        	warm_start = 0; //!< Warm vs Cold start
/* see description in function XmemCheckForWarmStart for further info on this */
//@}

static char gbConfigPath[LN] = "";

/**
 * @brief Set the default path for initialisation files
 *
 * @param pbPath - Path where the configuration files are stored
 *
 * @return XmemError
 */
XmemError XmemSetPath(char *pbPath)
{
	if (pbPath == NULL)
		return XmemErrorSUCCESS;

	if (strlen(pbPath) < (LN - 20)) {
		strcpy(gbConfigPath, pbPath);
		if (gbConfigPath[strlen(gbConfigPath) - 1] != '/')
			strcat(gbConfigPath, "/");
	}

	return XmemErrorSUCCESS;
}

/**
 * @brief Form the full path of a given configuration file
 *
 * @param name - file name
 *
 * @return full path of the given file name
 */
char *XmemGetFile(char *name)
{
	static char path[LN];
	char        *configpath;

	configpath = strlen(gbConfigPath) > 0 ? gbConfigPath : XMEM_PATH;
	sprintf(path, "%s%s", configpath, name);
	return path;
}

/*! @name Static variables, constants and functions
 *
 * These are not exported to the users of this library.
 */
//@{
#include "./vmic/VmicLib.c"
#include "./network/NetworkLib.c"
#include "./shmem/ShmemLib.c"

typedef struct {
	XmemNodeId    (*GetAllNodeIds)();
	XmemError     (*RegisterCallback)();
	XmemEventMask (*Wait)();
	XmemEventMask (*Poll)();
	XmemError     (*SendTable)();
	XmemError     (*RecvTable)();
	XmemError     (*SendMessage)();
	XmemTableId   (*CheckTables)();
} XmemLibRoutines;

static int 		libinitialized = 0;
static XmemEventMask 	libcallmask = 0;
static XmemLibRoutines 	routines;
static void (*libcallback)(XmemCallbackStruct *cbs) = NULL;

static char *estr[XmemErrorCOUNT] = {
	"No Error",
	"Timeout expired while waiting for interrupt",
	"The Xmem library is not initialized",
	"Write access to that table is denied for this node",
	"Could not read table descriptors from file: " SEG_TABLE_NAME,
	"Syntax error in table descriptors file: " SEG_TABLE_NAME,
	"Could not read node descriptors from file: " NODE_TABLE_NAME,
	"Syntax error in node descriptors file: " NODE_TABLE_NAME,
	"There are currently no tables defined",
	"That table is not defined in: " SEG_TABLE_NAME,
	"That node is not defined in: " NODE_TABLE_NAME,
	"Illegal message ID",
	"A run time hardware IO error has occured, see: IOError",
	"System error, see: errno"
};


/**
 * XmemReadNodeTableFile - Reads the node table
 *
 * @param : none
 *
 * The node table is in the default place (NODE_TABLE_NAME).
 *
 * @return Appropriate error code (XmemError)
 */
static XmemError XmemReadNodeTableFile()
{
	int 	i;
	int 	fc;
	FILE 	*fp;

	nodecnt = 0;
	umask(0);
	fp = fopen(XmemGetFile(NODE_TABLE_NAME), "r");
	if (NULL == fp)
		return XmemErrorCallback(XmemErrorNODE_TABLE_READ, 0);

	node_tab.Used = 0;
	for (i = 0; i < XmemDrvrNODES; i++) {
		fc = fscanf(fp,"{ %s 0x%x }\n",
			(char *)&node_tab.Descriptors[i].Name[0],
			(int  *)&node_tab.Descriptors[i].Id);
		if (EOF == fc)
			break;
		if (2 != fc)
			return XmemErrorCallback(XmemErrorNODE_TABLE_SYNTAX, 0);
		node_tab.Used |= node_tab.Descriptors[i].Id;
		nodecnt++;
	}
	return XmemErrorSUCCESS;
}


/**
 * XmemReadSegTableFile - Reads the segment table
 *
 * @param : none
 *
 * The segment table is in the default place (SEG_TABLE_NAME).
 *
 * @return Appropriate error code (XmemError)
 */
static XmemError XmemReadSegTableFile()
{
	int 	i;
	int 	fc;
	FILE 	*fp;

	segcnt = 0;
	umask(0);
	fp = fopen(XmemGetFile(SEG_TABLE_NAME), "r");
	if (NULL == fp)
		return XmemErrorCallback(XmemErrorSEG_TABLE_READ, 0);
	seg_tab.Used = 0;
	for (i = 0; i < XmemDrvrSEGMENTS; i++) {
		fc = fscanf(fp,"{ %s 0x%x 0x%x 0x%x 0x%x 0x%x }\n",
			(char *)&seg_tab.Descriptors[i].Name[0],
			(int  *)&seg_tab.Descriptors[i].Id,
			(int  *)&seg_tab.Descriptors[i].Size,
			(int  *)&seg_tab.Descriptors[i].Address,
			(int  *)&seg_tab.Descriptors[i].Nodes,
			(int  *)&seg_tab.Descriptors[i].User);
		if (fc == EOF)
			break;
		if (fc != 6)
			return XmemErrorCallback(XmemErrorSEG_TABLE_SYNTAX, 0);
		seg_tab.Used |= seg_tab.Descriptors[i].Id;
		segcnt++;
	}
	return XmemErrorSUCCESS;
}


/**
 * InitDevice - Local routine to initialise one real device
 *
 * @param device: type of device
 *
 * Remember that device can be VMIC, SHMEM or NETWORK.
 *
 * @return device initialisation on success; Not initialised error otherwise.
 */
static XmemError InitDevice(XmemDevice device)
{
	switch (device) {

	case XmemDeviceVMIC:
		routines.GetAllNodeIds    = VmicGetAllNodeIds;
		routines.RegisterCallback = VmicRegisterCallback;
		routines.Wait             = VmicWait;
		routines.Poll             = VmicPoll;
		routines.SendTable        = VmicSendTable;
		routines.RecvTable        = VmicRecvTable;
		routines.SendMessage      = VmicSendMessage;
		routines.CheckTables      = VmicCheckTables;
		return VmicInitialize();

	case XmemDeviceSHMEM:
		routines.GetAllNodeIds    = ShmemGetAllNodeIds;
		routines.RegisterCallback = ShmemRegisterCallback;
		routines.Wait             = ShmemWait;
		routines.Poll             = ShmemPoll;
		routines.SendTable        = ShmemSendTable;
		routines.RecvTable        = ShmemRecvTable;
		routines.SendMessage      = ShmemSendMessage;
		routines.CheckTables      = ShmemCheckTables;
		return ShmemInitialize();

	case XmemDeviceNETWORK:
		routines.GetAllNodeIds    = NetworkGetAllNodeIds;
		routines.RegisterCallback = NetworkRegisterCallback;
		routines.Wait             = NetworkWait;
		routines.Poll             = NetworkPoll;
		routines.SendTable        = NetworkSendTable;
		routines.RecvTable        = NetworkRecvTable;
		routines.SendMessage      = NetworkSendMessage;
		routines.CheckTables      = NetworkCheckTables;
		return NetworkInitialize();

	default:
		break;

	}
	return XmemErrorNOT_INITIALIZED;
}
//@}


/*
 * The following are exported (non-static) Xmem Lib functions
 * These are documented in the header file.
 */


XmemError XmemInitialize(XmemDevice device)
{
	XmemDevice 	fdev;
	XmemDevice 	ldev;
	XmemDevice 	dev;
	XmemError 	err;

	if (libinitialized)
		return XmemErrorSUCCESS;
	bzero((void *)attach_address, XmemMAX_TABLES * sizeof(long *));
	bzero((void *)&node_tab, sizeof(XmemDrvrNodeTable));

	err = XmemReadNodeTableFile();
	if (err != XmemErrorSUCCESS)
		return err;

	bzero((void *) &seg_tab, sizeof(XmemDrvrSegTable));
	err = XmemReadSegTableFile();
	if (err != XmemErrorSUCCESS)
		return err;

	if (device == XmemDeviceANY) {
		fdev = XmemDeviceVMIC;
		ldev = XmemDeviceNETWORK;
	}
	else {
		fdev = device;
		ldev = device;
	}
	for (dev = fdev; dev <= ldev; dev++) {
		err = InitDevice(dev);
		if (err == XmemErrorSUCCESS) {
			libinitialized = 1;
			return err;
		}
	}
	return XmemErrorNOT_INITIALIZED;
}



XmemNodeId XmemWhoAmI()
{
	return my_nid;
}



int XmemCheckForWarmStart()
{
	return warm_start;
}


char *XmemErrorToString(XmemError err)
{
	char 		*cp;
	static char 	result[XmemErrorSTRING_SIZE];

	if (err < 0 || err >= XmemErrorCOUNT)
		cp = "No such error number";
	else
		cp = estr[(int)err]; /* estr: global error string array */
	bzero((void *)result, XmemErrorSTRING_SIZE);
	strcpy(result, cp);
	return result;
}


XmemNodeId XmemGetAllNodeIds()
{
	if (libinitialized)
		return routines.GetAllNodeIds();
	return 0;
}


XmemError XmemRegisterCallback(void (*cb)(XmemCallbackStruct *cbs),
			XmemEventMask mask)
{
	XmemError err;

	if (! libinitialized)
		return XmemErrorNOT_INITIALIZED;
	err = routines.RegisterCallback(cb, mask);
	if (err == XmemErrorSUCCESS) {
		if (mask) {
			libcallmask |= mask;
			libcallback = cb;
		}
		else {
			libcallmask = 0;
			libcallback = NULL;
		}
	}
	return err;
}


XmemEventMask XmemGetRegisteredEvents()
{
	return (XmemEventMask)libcallmask;
}


XmemEventMask XmemWait(int timeout)
{
	if (libinitialized)
		return routines.Wait(timeout);
	return 0;
}


XmemEventMask XmemPoll()
{
	if (libinitialized)
		return routines.Poll();
	return 0;
}


XmemError XmemSendTable(XmemTableId table, long *buf, unsigned long longs,
			unsigned long offset, unsigned long upflag)
{
	if (libinitialized)
		return routines.SendTable(table, buf, longs, offset, upflag);
	return XmemErrorNOT_INITIALIZED;
}


XmemError XmemRecvTable(XmemTableId table, long *buf, unsigned long longs,
			unsigned long offset)
{
	if (libinitialized)
		return routines.RecvTable(table, buf, longs, offset);
	return XmemErrorNOT_INITIALIZED;
}


XmemError XmemSendMessage(XmemNodeId nodes, XmemMessage *mess)
{
	if (libinitialized)
		return routines.SendMessage(nodes, mess);
	return XmemErrorNOT_INITIALIZED;
}


XmemTableId XmemCheckTables()
{
	if (libinitialized)
		return routines.CheckTables();
	return XmemErrorNOT_INITIALIZED;
}


XmemError XmemErrorCallback(XmemError err, unsigned long ioe)
{
	XmemCallbackStruct cbs;

	if (!libcallback)
		return err;
	bzero((void *)&cbs, sizeof(XmemCallbackStruct));
	switch (err) {

	case XmemErrorSUCCESS:
		break;

	case XmemErrorTIMEOUT:

		cbs.Mask = XmemEventMaskTIMEOUT;
		if (libcallmask & XmemEventMaskTIMEOUT)
			libcallback(&cbs);
		break;

	case XmemErrorIO:

		cbs.Mask = XmemEventMaskIO;
		cbs.Data = ioe;
		if (libcallmask & XmemEventMaskIO)
			libcallback(&cbs);
		break;

	case XmemErrorSYSTEM:

		cbs.Mask = XmemEventMaskSYSTEM;
		cbs.Data = ioe;
		if (libcallmask & XmemEventMaskSYSTEM)
			libcallback(&cbs);
		break;

	default:

		cbs.Mask = XmemEventMaskSOFTWARE;
		cbs.Data = (unsigned long)err;
		if (libcallmask & XmemEventMaskSOFTWARE)
			libcallback(&cbs);
		break;
	}
	return err;
}


int XmemGetKey(char *name)
{
	int 	i;
	int	key;

	key = 0;
	if (NULL == name)
		return key;
	for (i = 0; i < strlen(name); i++)
		key = (key << 1) + (int)name[i];
	return key;
}


long *XmemGetSharedMemory(XmemTableId tid)
{
	int 		tnum, msk;
	unsigned long longs, user, bytes, smemid;
	key_t 	smemky;
	XmemError 	err;
	XmemNodeId 	nid;
	unsigned long	*table;
	char 		*cp;

	if (! libinitialized)
		goto error;
	for (tnum = 0; tnum < XmemMAX_TABLES; tnum++) {
		msk = 1 << tnum;
		if (! (msk & tid))
			continue;
		if (attach_address[tnum])
			return attach_address[tnum];
		else
			break; /* it doesn't exist yet */
	}


	cp = XmemGetTableName(tid);
	if (!cp)
		goto error;

	err = XmemGetTableDesc(tid, &longs, &nid, &user);
	if (err != XmemErrorSUCCESS)
		goto error;

	bytes = longs * sizeof(long);
	smemky = XmemGetKey(cp);
	smemid = shmget(smemky, bytes, 0666);

	if (smemid == -1) {
		if (ENOENT != errno)
			goto error;
		/* segment does not exist; create it */
		smemid = shmget(smemky, bytes, IPC_CREAT | 0666);
		if (-1 == smemid)
			goto error;
		/* attach memory segment to smemid */
		table = (long *)shmat(smemid, (char *)0, 0);
		if (-1 == (int)table)
			goto error;
		if (tnum < XmemMAX_TABLES)
			attach_address[tnum] = table;

		err = XmemRecvTable(tid, table, longs, 0);
		if (XmemErrorSUCCESS != err)
			goto error;
		return table;
	}
	else { /* segment was already there */
		table = (long *)shmat(smemid, (char *)0, 0);
		if (-1 == (int)table)
			goto error;
		if (tnum < XmemMAX_TABLES)
			attach_address[tnum] = table;
		return table;
	}
error:
	XmemErrorCallback(XmemErrorSYSTEM, errno);
	return NULL;
}


XmemTableId XmemGetAllTableIds()
{
	return seg_tab.Used;
}


char *XmemGetNodeName(XmemNodeId node)
{
	int i;
	unsigned long msk;

	if (!xmem) /* global variable, means a device is open */
		return (char *)0;
	for (i = 0; i < XmemMAX_NODES; i++) {
		msk = 1 << i;
		if (node_tab.Used & msk && node == node_tab.Descriptors[i].Id)
			return node_tab.Descriptors[i].Name;
	}
	XmemErrorCallback(XmemErrorNO_SUCH_NODE, 0);
	return (char *)0;
}


XmemNodeId XmemGetNodeId(XmemName name)
{
	int 		i;
	unsigned long	msk;

	if (!xmem)
		return 0;
	for (i = 0; i < XmemMAX_NODES; i++) {
		msk = 1 << i;
		if (strcmp(name, node_tab.Descriptors[i].Name) == 0 &&
		    node_tab.Used & msk)
			return node_tab.Descriptors[i].Id;
	}
	XmemErrorCallback(XmemErrorNO_SUCH_NODE, 0);
	return 0;
}


char * XmemGetTableName(XmemTableId table)
{
	int 		i;
	unsigned long	msk;

	if (!xmem)
		return (char *)0;
	for (i = 0; i < XmemMAX_TABLES; i++) {
		msk = 1 << i;
		if (seg_tab.Used & msk && table == seg_tab.Descriptors[i].Id)
			return seg_tab.Descriptors[i].Name;
	}
	XmemErrorCallback(XmemErrorNO_SUCH_TABLE, 0);
	return (char *)0;
}



XmemTableId XmemGetTableId(XmemName name)
{
	int		i;
	unsigned long	msk;

	if (!xmem)
		return 0;
	for (i = 0; i < XmemMAX_TABLES; i++) {
		msk = 1 << i;
		if (strcmp(name,seg_tab.Descriptors[i].Name) == 0 &&
		    seg_tab.Used & msk)
			return seg_tab.Descriptors[i].Id;
	}
	XmemErrorCallback(XmemErrorNO_SUCH_TABLE, 0);
	return 0;
}



XmemError XmemGetTableDesc(XmemTableId table, unsigned long *longs,
			XmemNodeId *nodes, unsigned long *user)
{
	int 		i;
	unsigned long	msk;

	if (!xmem)
		return XmemErrorCallback(XmemErrorNOT_INITIALIZED, 0);
	for (i = 0; i < XmemMAX_TABLES; i++) {
		msk = 1 << i;
		if (seg_tab.Used & msk && table == seg_tab.Descriptors[i].Id) {
			*longs = seg_tab.Descriptors[i].Size / sizeof(long);
			*nodes = seg_tab.Descriptors[i].Nodes;
			*user  = seg_tab.Descriptors[i].User;
			return XmemErrorSUCCESS;
		}
	}
	return XmemErrorCallback(XmemErrorNO_SUCH_TABLE, 0);
}


XmemError XmemReadTableFile(XmemTableId tid)
{
	int 		i, cnt;
	unsigned long	msk;
	unsigned long user, longs, bytes;
	XmemNodeId 	nodes;
	XmemError	err;
	char 		*cp;
	char		tbnam[64];
	FILE 		*fp;
	unsigned long	*table;

	for (i = 0; i < XmemMAX_TABLES; i++) {
		msk = 1 << i;
		if (! (msk & tid))
			continue;
		cp = XmemGetTableName(msk);
		err = XmemGetTableDesc(msk, &longs, &nodes, &user);
		if (XmemErrorSUCCESS != err)
			return err;
		table = XmemGetSharedMemory(msk);
		if (!table)
			return XmemErrorNO_SUCH_TABLE;

		bzero((void *)tbnam, 64);
		strcat(tbnam, cp);
		umask(0);

		fp = fopen(XmemGetFile(tbnam), "r");
		if (!fp)
			return XmemErrorCallback(XmemErrorSYSTEM, errno);
		bytes = longs * sizeof(long);
		cnt = fread(table, bytes, 1, fp);
		if (cnt <= 0)
			err = XmemErrorCallback(XmemErrorSYSTEM, errno);
		fclose(fp);
		fp = NULL;

		if (XmemErrorSUCCESS != err)
			return err;
	}
	return XmemErrorSUCCESS;
}


XmemError XmemWriteTableFile(XmemTableId tid)
{
	int 		i, cnt;
	unsigned long	msk;
	unsigned long	user, longs, bytes;
	XmemError 	err;
	XmemNodeId 	nodes;
	char 		*cp;
	char 		tbnam[64];
	unsigned long	*table;
	FILE		*fp;


	for (i = 0; i < XmemMAX_TABLES; i++) {
		msk = 1 << i;
		if (! (msk & tid))
			continue;
		cp = XmemGetTableName(msk);
		err = XmemGetTableDesc(msk, &longs, &nodes, &user);
		if (XmemErrorSUCCESS != err)
			return err;
		table = XmemGetSharedMemory(msk);
		if (!table)
			return XmemErrorNO_SUCH_TABLE;

		bzero((void *)tbnam, 64);
		strcat(tbnam, cp);
		umask(0);

		fp = fopen(XmemGetFile(tbnam), "w");
		if (!fp)
			return XmemErrorCallback(XmemErrorSYSTEM, errno);
		bytes = longs * sizeof(long);
		cnt = fwrite(table, bytes, 1, fp);
		if (cnt <= 0)
			err = XmemErrorCallback(XmemErrorSYSTEM,errno);
		fclose(fp);
		fp = NULL;

		if (err != XmemErrorSUCCESS)
			return err;
	}
	return XmemErrorSUCCESS;
}

/**
 * XmemLibUsleep - Sleep for 'delay' us.
 *
 * @param dly: desired delay, in us
 *
 */
void XmemLibUsleep(int dly)
{
	struct timespec rqtp, rmtp;

	rqtp.tv_sec = 0;
	rqtp.tv_nsec = dly * 1000;
	nanosleep(&rqtp, &rmtp);
}


/*! @name Mtt Table Compilation
 *
 * Compile an event table by sending a message to the table compiler
 * server message queue. The resulting object file is transfered to
 * the LHC MTGs accross reflective memory. This routine avoids the
 * need for issuing system calls within complex multithreaded FESA
 * classes. The table must be loaded as usual and completion must
 * be checked as usual by looking at the response xmem tables.
 * If the return code is an IO error this probably means the cmtsrv
 * task is not running. A SYS error indicates a message queue error.
 */
//@{
#define TableTskObjM 0x200

/**
 * MttLibCompileTable - Compile MTT Table
 *
 * @param name: name of the table
 *
 */
int MttLibCompileTable(char *name)
{
	int 		tmo, terr;
	ssize_t	ql;
	mqd_t 	q;
	XmemTableId 	tid;

	tid = XmemCheckTables(); /* Clear out any old stuff  */
	q = mq_open("/tmp/cmtsrv", O_WRONLY | O_NONBLOCK);
	if (q == (mqd_t)-1)
		return 3; /* MttLibErrorIO; */

	ql = mq_send(q, name, 32, 0); /* 32 == MttLibMAX_NAME_SIZE */
	terr = errno;
	mq_close(q);

	if (q == (mqd_t)-1) {
		if (terr == EAGAIN)
			return 3; /* MttLibErrorIO; */
		else
			return 4; /* MttLibErrorSYS; */
	}
	tmo = 0;
	while (tmo++ < 1000) {	/* 20 Seconds max wait */

		tid = XmemCheckTables();
		if (tid & TableTskObjM) {
			XmemLibUsleep(30000);	/* Sleep 30 milliseconds */
			return 0; /* MttLibErrorNONE; Object table received */
		}
		XmemLibUsleep(20000);	/* Sleep 20 milliseconds */
	}
	return 3; /* MttLibErrorIO; Timeout */
}
//@}
