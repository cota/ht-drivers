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


/*! @name configuration path
 *
 * Used to get a file configuration path
 */
//@{
char *defaultconfigpath = XMEM_PATH CONFIG_FILE_NAME;
char *configpath = NULL;

/**
 * XmemGetFile - returns the path to a configuration file
 *
 * @param name: desired configuration file
 *
 * This functions access the main configuration file, reading and returning the
 * correct path to the file described in *name.
 * NB. A config file looks like the following:
 *
 * ;File name            ;Location of File            ;Comment
 *
 * ;Reflective memory
 *
 * xmemtest               /usr/drivers/xmem/          ;Xmem test program
 *
 * xmemdiag               /usr/xmem/                  ;Xmem daemon test program
 *
 * @return string with full path to the config file on success; NULL otherwise.
 */
char *XmemGetFile(char *name)
{
  int 		i;
  int		j;
  char 		txt[LN];
  FILE 		*gpath = NULL;
  static char 	path[LN];


  if (configpath) {
    gpath = fopen(configpath, "r");
    if (gpath == NULL) {
      configpath = NULL;
    }
  }

  if (configpath == NULL) {
    configpath = defaultconfigpath;
    gpath = fopen(configpath, "r");
    if (gpath == NULL) { /* could not open main conf file */
      configpath = NULL;
      sprintf(path, "./%s", name); /* then try in the working directory */
      return path;
    }
  }

  /* now gpath contains a valid FILE pointer to the main conf file */
  bzero((void *)path, LN);

  while (1) {

    if (fgets(txt, LN, gpath) == NULL) /* read a line from the file */
      break;

    if (strncmp(name, txt, strlen(name)) != 0)
      continue; /* this line is not the one we're searching for */

    for (i = strlen(name); i < strlen(txt); i++)
      if (txt[i] != ' ')
	break; /* get rid of the whitespaces between name and path */

    j = 0;
    while (txt[i] != ' ' && txt[i] != '\0' && txt[i] != '\n')
      path[j++] = txt[i++];

    /* now 'path' contains the path we want */
    strcat(path, name);

    fclose(gpath);
    return path;
  }

  fclose(gpath);
  return NULL;
}
//@}

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

  if (fp == NULL)
    return XmemErrorCallback(XmemErrorNODE_TABLE_READ, 0);

  node_tab.Used = 0;

  for (i = 0; i < XmemDrvrNODES; i++) {

    fc = fscanf(fp,"{ %s 0x%x }\n",
		(char *)&node_tab.Descriptors[i].Name[0],
		(int  *)&node_tab.Descriptors[i].Id);

    if (fc == EOF)
      break;
    if (fc != 2)
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
  if (fp == NULL)
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



//<! The following are exported (non-static) Xmem Lib functions


/**
 * XmemInitialize - Try to initialise the specified device
 *
 * @param device: device to initialise
 *
 * Set the device to be used by the library and
 * perform library initialization code.
 *
 * @return Appropriate error code (XmemError)
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


/**
 * XmemWhoAmI - Get the node ID of this node
 *
 * @param : none
 *
 * @return node ID of this node
 */
XmemNodeId XmemWhoAmI()
{
  return my_nid;
}


/**
 * XmemCheckForWarmStart - Check for warm/cold start-up.
 *
 * @param : none
 *
 * Clients might need to know if they are starting warm or cold to correctly
 * initialize tables and reflective memory.
 * Once the driver tables have been loaded, subsequent instances of any
 * programs using XmemLib will always receive the warm start status,
 * cold start status occurs only when the driver tables have not been
 * loaded just after a reboot and a fresh driver install.
 *
 * @return 0 if it's a cold start; non-zero if it's a warm start.
 */
int XmemCheckForWarmStart()
{
  return warm_start;
}


/**
 * XmemErrorToString - Convert an error into a string.
 *
 * @param err: XmemError to be converted
 *
 * @return static (global) string
 */
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


/**
 * XmemGetAllNodeIds - Get all currently up and running nodes
 *
 * @param : none
 *
 * The set of up and running nodes is determined dynamically through a
 * handshake protocol between nodes.
 *
 * When there is no driver installed on a DSC, there will be no response from
 * that DSC to the PENDING_INIT interrupt, so the node will be removed from
 * the list of ACTIVE nodes. When the node comes back up, it sends out a new
 * non-zero PENDING_INIT broadcast, and all nodes will respond restablishing
 * the ACTIVE node list in every node. This is part of the driver ISR logic.
 * The library event XmemEventMaskINITIALIZED corresponds to PENDING_INIT.
 *
 * @return initialised node id's.
 */
XmemNodeId XmemGetAllNodeIds()
{
  if (libinitialized)
    return routines.GetAllNodeIds();
  return 0;
}


/**
 * XmemRegisterCallback - Register callback to handle Xmem Events.
 *
 * @param cb: callback descriptor
 * @param mask: event to subscribe to
 *
 * This is the way clients can connect to events.
 *
 * Calling with NULL deletes any previous callback that was installed.
 *
 * Calling the Wait or Poll library functions may cause the call back
 * to be invoked.
 *
 * Implementing a daemon is nothing more than writing a callback.
 *
 * Register your callback function, and subscribe to the things you want
 * delivered to it.
 *
 * @return Appropriate error message (XmemError)
 */
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


/**
 * XmemGetRegisteredEvents - Get currently registered callback's mask
 *
 * @param : none
 *
 * @return mask of currently registered callback
 */
XmemEventMask XmemGetRegisteredEvents()
{
  return (XmemEventMask)libcallmask;
}


/**
 * XmemWait - Wait for an Xmem Event with timeout
 *
 * @param timeout: in intervals of 10ms; 0 means forever.
 *
 * An incoming event will call any registered callback
 *
 * @return Wait handler on success; 0 otherwise
 */
XmemEventMask XmemWait(int timeout)
{
  if (libinitialized)
    return routines.Wait(timeout);
  return 0;
}


/**
 * XmemPoll - poll for an Xmem Event
 *
 * @param : none
 *
 * An incoming event will call any registered callback for that event
 *
 * @return Poll handled on success; 0 otherwise
 */
XmemEventMask XmemPoll()
{
  if (libinitialized)
    return routines.Poll();
  return 0;
}


/**
 * XmemSendTable - Send a client's buffer to reflective memory table
 *
 * @param table: table to be written to
 * @param buf: client's buffer
 * @param longs: number of 'longs' (4 bytes) to transfer
 * @param offset: offset of transfer
 * @param upflag: update message flag
 *
 * When writing the last time to the table, set the upflag non zero so that a
 * table update message is sent out to all nodes.
 *
 * If SendTable is called with a NULL buffer, the driver will flush the
 * reflective memory segment to the network by copying it onto itself. This
 * is rather slow compared to the usual copy because DMA can not be used, and
 * two PCI-bus accesses ~1us are used for each long word.
 *
 * WARNING: Only 32 bit accesses can be made. So the buf parameter must be on
 * a 'long' boundary, the offset is in longs, and longs is the number of 'longs'
 * to be transfered. This can be tricky because the size of the buffer to be
 * allocated must be multiplied by sizeof(long).
 *
 * @return Appropriate error message (XmemError)
 */
XmemError XmemSendTable(XmemTableId table, long *buf, unsigned long longs,
			unsigned long offset, unsigned long upflag)
{
  if (libinitialized)
    return routines.SendTable(table, buf, longs, offset, upflag);
  return XmemErrorNOT_INITIALIZED;
}


/**
 * XmemRecvTable - Update a client's buffer from a reflective memory table
 *
 * @param table: table to read from
 * @param buf: client's buffer
 * @param longs: number of 'longs' (i.e. 4 bytes) to transfer
 * @param offset: offset of transfer
 *
 * See comments for XmemSendTable.
 *
 * @return Appropriate error message (XmemError)
 */
XmemError XmemRecvTable(XmemTableId table, long *buf, unsigned long longs,
			unsigned long offset)
{
  if (libinitialized)
    return routines.RecvTable(table, buf, longs, offset);
  return XmemErrorNOT_INITIALIZED;
}


/**
 * XmemSendMessage - Send a message to other nodes
 *
 * @param nodes: receiving nodes' mask
 * @param mess: message
 *
 * If the node parameter is zero, then nothing happens. If it's equal to
 * 0xFFFFFFFF the message is sent via broadcast, else multicast or unicast
 * is used.
 *
 * @return Appropriate error code (XmemError)
 */
XmemError XmemSendMessage(XmemNodeId nodes, XmemMessage *mess)
{
  if (libinitialized)
    return routines.SendMessage(nodes, mess);
  return XmemErrorNOT_INITIALIZED;
}


/**
 * XmemCheckTables - Check which tables were updated
 *
 * @param : none
 *
 * Check to see which tables have been updated since the last call. For each
 * table that has been modified, its ID bit is set in the returned TableId.
 *
 * For each client the call maintains the set of tables that the client has
 * not yet seen, and this set is zeroed after each call.
 *
 * This will not invoke your registered callback even if you have registered
 * for TABLE_UPDATE interrupts.
 *
 * @return CheckTables handler on success; XmemErrorNOT_INITIALIZED otherwise
 */
XmemTableId XmemCheckTables()
{
  if (libinitialized)
    return routines.CheckTables();
  return XmemErrorNOT_INITIALIZED;
}


/**
 * XmemErrorCallback - Invokes callback for errors
 *
 * @param err: error code
 * @param ioe: data to append
 *
 * This routine permits invoking your callback for errors.
 *
 * The err parameter contains one error, and ioe contains the sub-error
 * code as defined above in XmemEventMask description.
 *
 * @return Appropriate error code (XmemError)
 */
XmemError XmemErrorCallback(XmemError err, unsigned long ioe)
{
  XmemCallbackStruct cbs;

  if (! libcallback)
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


/**
 * XmemGetKey - generate a key from name
 *
 * @param name: name to generate a key from
 *
 * This is general enough to be included in the library.
 * This hash algorithm is needed for handling shared memory keys.
 *
 * @return generated key
 */
int XmemGetKey(char *name)
{
  int 	i;
  int	key;

  key = 0;

  if (name == NULL)
    return key;

  for (i = 0; i < strlen(name); i++)
    key = (key << 1) + (int)name[i];

  return key;
}


/**
 * XmemGetSharedMemory - Create/Get a shared memory segment
 *
 * @param tid: table id
 *
 * Create/Get a shared memory segment to hold the given table, and initialize
 * it by reading from reflective memory. The shared memory segment is opened
 * with read and write access.
 * If it exists already then a pointer to it is returned, otherwise it is
 * created an initialised.
 * The tid parameter must have only one bit set in it.
 * On error a null is returned, and any callback registered for errors will be
 * called, in any case errno contains the system error number for the failure.
 *
 * @return pointer to segment on success; NULL otherwise.
 */
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
    if (msk & tid) {
      if (attach_address[tnum])
	return attach_address[tnum];
      else
	break; /* it doesn't exist yet */
    }
  }


  cp = XmemGetTableName(tid);
  if (! cp)
    goto error;

  err = XmemGetTableDesc(tid, &longs, &nid, &user);
  if (err != XmemErrorSUCCESS)
    goto error;

  bytes = longs * sizeof(long);
  smemky = XmemGetKey(cp);
  smemid = shmget(smemky, bytes, 0666);

  if (smemid == -1) {

    if (errno != ENOENT)
      goto error;

    /* segment does not exist; create it */
    smemid = shmget(smemky, bytes, IPC_CREAT | 0666);
    if (smemid == -1)
      goto error;

    /* attach memory segment to smemid */
    table = (long *)shmat(smemid, (char *)0, 0);
    if ((int)table == -1)
      goto error;

    if (tnum < XmemMAX_TABLES)
      attach_address[tnum] = table;

    err = XmemRecvTable(tid, table, longs, 0);

    if (err != XmemErrorSUCCESS)
      goto error;

    return table;
  }
  else { /* segment was already there */

    table = (long *)shmat(smemid, (char *)0, 0);
    if ((int)table == -1)
      goto error;

    if (tnum < XmemMAX_TABLES)
      attach_address[tnum] = table;

    return table;
  }

 error:
  XmemErrorCallback(XmemErrorSYSTEM, errno);
  return NULL;
}


/**
 * XmemGetAllTableIds - get all currently defined table id's
 *
 * @param : none
 *
 * There can be up to 32 tables each ones' id corresponds to a single bit in
 * a double word.
 *
 * @return used segment tables mask
 */
XmemTableId XmemGetAllTableIds()
{
  return seg_tab.Used;
}


/**
 * XmemGetNodeName - Get the name of a node from its node id
 *
 * @param node: node id
 *
 * This is usually the name of the host on which the node is implemented.
 *
 * @return node name on success; NULL otherwise.
 */
char * XmemGetNodeName(XmemNodeId node)
{
  int i;
  unsigned long msk;

  if (! xmem) /* comes from VmicLib.c, means a VMIC device is open */
    return (char *)0;

  for (i = 0; i < XmemMAX_NODES; i++) {
    msk = 1 << i;
    if (node_tab.Used & msk && node == node_tab.Descriptors[i].Id)
      return node_tab.Descriptors[i].Name;
  }

  XmemErrorCallback(XmemErrorNO_SUCH_NODE, 0);
  return (char *)0;
}


/**
 * XmemGetNodeId - Get the id of a node from its name
 *
 * @param name: node's name
 *
 * @return node's id on success; 0 otherwise
 */
XmemNodeId XmemGetNodeId(XmemName name)
{
  int 		i;
  unsigned long	msk;

  if (! xmem)
    return 0;

  for (i = 0; i < XmemMAX_NODES; i++) {
    msk = 1 << i;
    if (node_tab.Used & msk && strcmp(name, node_tab.Descriptors[i].Name) == 0)
      return node_tab.Descriptors[i].Id;
  }

  XmemErrorCallback(XmemErrorNO_SUCH_NODE, 0);
  return 0;
}


/**
 * XmemGetTableName - Get the name of a table from its table id
 *
 * @param table: table id
 *
 * Tables are defined in a configuration file
 *
 * @return table's name on success; NULL otherwise
 */
char * XmemGetTableName(XmemTableId table)
{
  int 		i;
  unsigned long	msk;

  if (! xmem)
    return (char *)0;

  for (i = 0; i < XmemMAX_TABLES; i++) {
    msk = 1 << i;
    if (seg_tab.Used & msk && table == seg_tab.Descriptors[i].Id)
      return seg_tab.Descriptors[i].Name;
  }

  XmemErrorCallback(XmemErrorNO_SUCH_TABLE, 0);
  return (char *)0;
}


/**
 * XmemGetTableId - get the id of a table from its name
 *
 * @param name: table's name
 *
 * @return table id on success; 0 otherwise
 */
XmemTableId XmemGetTableId(XmemName name)
{
  int		i;
  unsigned long	msk;

  if (! xmem)
    return 0;

  for (i = 0; i < XmemMAX_TABLES; i++) {
    msk = 1 << i;
    if (seg_tab.Used & msk && strcmp(name,seg_tab.Descriptors[i].Name) == 0)
      return seg_tab.Descriptors[i].Id;
  }

  XmemErrorCallback(XmemErrorNO_SUCH_TABLE, 0);
  return 0;
}


/**
 * XmemGetTableDesc - Get description of a table
 *
 * @param table: table id
 * @param longs: table size
 * @param nodes: nodes with write access
 * @param user: users of the segment
 *
 * Note that the description is returned by reference
 *
 * @return Appropriate error code (XmemError)
 */
XmemError XmemGetTableDesc(XmemTableId table, unsigned long *longs,
			   XmemNodeId *nodes, unsigned long *user)
{
  int 		i;
  unsigned long	msk;

  if (! xmem)
    return XmemErrorCallback(XmemErrorNOT_INITIALIZED, 0);

  for (i = 0; i < XmemMAX_TABLES; i++) {
    msk = 1 << i;
    if (seg_tab.Used & msk && table == seg_tab.Descriptors[i].Id) {
      *longs = seg_tab.Descriptors[i].Size/sizeof(long);
      *nodes = seg_tab.Descriptors[i].Nodes;
      *user  = seg_tab.Descriptors[i].User;
      return XmemErrorSUCCESS;
    }
  }
  return XmemErrorCallback(XmemErrorNO_SUCH_TABLE, 0);
}


/**
 * XmemReadTableFile - read tables from files into shared memory segments
 *
 * @param tid: table id
 *
 * @return Appropriate error code (XmemError)
 */
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
    if (err != XmemErrorSUCCESS)
      return err;

    table = XmemGetSharedMemory(msk);
    if (!table)
      return XmemErrorNO_SUCH_TABLE;

    bzero((void *)tbnam, 64);
    strcat(tbnam, cp);
    umask(0);

    fp = fopen(XmemGetFile(tbnam), "r");
    if (! fp)
      return XmemErrorCallback(XmemErrorSYSTEM, errno);

    bytes = longs * sizeof(long);
    cnt = fread(table, bytes, 1, fp);

    if (cnt <= 0)
      err = XmemErrorCallback(XmemErrorSYSTEM, errno);

    fclose(fp);
    fp = NULL;

    if (err != XmemErrorSUCCESS)
      return err;

  }
  return XmemErrorSUCCESS;
}


/**
 * XmemWriteTableFile - Write tables from shared memory segments to files
 *
 * @param tid: table id
 *
 * @return Appropriate error code (XmemError)
 */
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
    if (err != XmemErrorSUCCESS)
      return err;

    table = XmemGetSharedMemory(msk);
    if (! table)
      return XmemErrorNO_SUCH_TABLE;

    bzero((void *)tbnam, 64);
    strcat(tbnam, cp);
    umask(0);

    fp = fopen(XmemGetFile(tbnam), "w");
    if (! fp)
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


/*! @name Client concurrency handling
 *
 * A client may block other client's callbacks while he's running
 */
//@{
#ifdef __linux__
union semun {
  int val;
  struct semid_ds *buff;
  unsigned short *array;
} arg;

#else /* LynxOS */
union semun arg;
#endif /* !__linux__ */

/**
 * XmemBlockCallbacks - Block all client callbacks while the caller is running
 *
 * @param : none
 *
 * @return Appropriate error code (XmemError)
 */
XmemError XmemBlockCallbacks()
{
  int	key;
  int 	sd;
  int 	cnt;
  int 	err;

  key = XmemGetKey(XMEM_SEMAPHORE);
  sd  = semget(key, 1, 0666 | IPC_CREAT);

  if (sd == -1)
    return XmemErrorSYSTEM;

  arg.val = 1;
  err = semctl(sd, 0, SETVAL, arg);
  cnt = semctl(sd, 0, GETVAL, arg);

  if (err == - 1) {
    perror("XmemBlockCallbacks");
    return XmemErrorSYSTEM;
  }

  return XmemErrorSUCCESS;
}


/**
 * XmemUnBlockCallbacks - Unblock client callbacks while the caller is running
 *
 * @param : none
 *
 * @return Appropriate error code (XmemError)
 */
XmemError XmemUnBlockCallbacks()
{
  int	key;
  int	sd;
  int	cnt;
  int	err;

  key = XmemGetKey(XMEM_SEMAPHORE);

  sd  = semget(key, 1, 0);
  if (sd == -1)
    return XmemErrorSYSTEM;

  arg.val = 0;
  err = semctl(sd, 0, SETVAL, arg);
  cnt = semctl(sd, 0, GETVAL, arg);

  if (err == -1) {
    perror("XmemUnBlockCallbacks");
    return XmemErrorSYSTEM;
  }

  return XmemErrorSUCCESS;
}


/**
 * XmemCallbacksBlocked - Checks that callbacks are blocked
 *
 * @param : none
 *
 * @return 1 if blocked; 0 otherwise
 */
int XmemCallbacksBlocked()
{
  int	key;
  int	sd;
  int	cnt;

  key = XmemGetKey(XMEM_SEMAPHORE);

  sd  = semget(key, 1, 0);
  if (sd == -1)
    return 0;

  cnt = semctl(sd, 0, GETVAL, arg);

  return cnt;
}
//@}


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
      return 0;			/* MttLibErrorNONE; Object table received */
    }

    XmemLibUsleep(20000);	/* Sleep 20 milliseconds */
  }

  return 3; /* MttLibErrorIO; Timeout */
}
//@}
