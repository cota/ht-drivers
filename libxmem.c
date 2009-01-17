/* ========================================================================== */
/* Implement a library to handle reflective memory front end                  */
/* Julian Lewis 9th/Feb/2005                                                  */
/* ========================================================================== */

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

/*****************************************************************/
/* Include device specific backend code                          */

#define LN 128

static long *attach_address[XmemMAX_TABLES];

static XmemDrvrSegTable seg_tab;
static int segcnt = 0;

static XmemDrvrNodeTable node_tab;
static int nodecnt = 0;

static XmemNodeId my_nid = 0;       /* My node ID */
static int        warm_start = 0;   /* Warm vs Cold start */

/* ============================== */
/* Get a file configuration path  */
/* ============================== */

char *defaultconfigpath = XMEM_PATH CONFIG_FILE_NAME;
char *configpath = NULL;

char *XmemGetFile(char *name) {
FILE *gpath = NULL;
char txt[LN];
int i, j;

static char path[LN];

   if (configpath) {
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = NULL;
      }
   }

   if (configpath == NULL) {
      configpath = defaultconfigpath;
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = NULL;
	 sprintf(path,"./%s",name);
	 return path;
      }
   }

   bzero((void *) path,LN);

   while (1) {
      if (fgets(txt,LN,gpath) == NULL) break;
      if (strncmp(name,txt,strlen(name)) == 0) {
	 for (i=strlen(name); i<strlen(txt); i++) {
	    if (txt[i] != ' ') break;
	 }
	 j= 0;
	 while ((txt[i] != ' ') && (txt[i] != 0) && (txt[i] != '\n')) {
	    path[j] = txt[i];
	    j++; i++;
	 }
	 strcat(path,name);
	 fclose(gpath);
	 return path;
      }
   }
   fclose(gpath);
   return NULL;
}

/*****************************************************************/

#include "./vmic/VmicLib.c"
#include "./network/NetworkLib.c"
#include "./shmem/ShmemLib.c"

/*****************************************************************/
/* Static variables and constants                                */

static int libinitialized = 0;
static XmemEventMask libcallmask = 0;
static void (*libcallback)(XmemCallbackStruct *cbs) = NULL;

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

static XmemLibRoutines routines;

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

/*****************************************************************/
/* Read the node table from file                                 */

static XmemError XmemReadNodeTableFile(void) {

int i, fc;
FILE *fp;

   nodecnt = 0;

   umask(0);
   fp = fopen(XmemGetFile(NODE_TABLE_NAME),"r");
   if (fp) {
      node_tab.Used = 0;
      for (i=0; i<XmemDrvrNODES; i++) {
	 fc = fscanf(fp,"{ %s 0x%x }\n",
		    (char *) &node_tab.Descriptors[i].Name[0],
		    (int  *) &node_tab.Descriptors[i].Id);

	 if (fc == EOF) break;
	 if (fc != 2) return XmemErrorCallback(XmemErrorNODE_TABLE_SYNTAX,0);
	 node_tab.Used |= node_tab.Descriptors[i].Id;
	 nodecnt++;
      }
      return XmemErrorSUCCESS;
   }
   return XmemErrorCallback(XmemErrorNODE_TABLE_READ,0);
}

/*****************************************************************/
/* Read segment table from file                                  */

static XmemError XmemReadSegTableFile(void) {

int i, fc;
FILE *fp;

   segcnt = 0;

   umask(0);
   fp = fopen(XmemGetFile(SEG_TABLE_NAME),"r");
   if (fp) {
      seg_tab.Used = 0;
      for (i=0; i<XmemDrvrSEGMENTS; i++) {
	 fc = fscanf(fp,"{ %s 0x%x 0x%x 0x%x 0x%x 0x%x }\n",
		    (char *) &seg_tab.Descriptors[i].Name[0],
		    (int  *) &seg_tab.Descriptors[i].Id,
		    (int  *) &seg_tab.Descriptors[i].Size,
		    (int  *) &seg_tab.Descriptors[i].Address,
		    (int  *) &seg_tab.Descriptors[i].Nodes,
		    (int  *) &seg_tab.Descriptors[i].User);

	 if (fc == EOF) break;
	 if (fc != 6) return XmemErrorCallback(XmemErrorSEG_TABLE_SYNTAX,0);
	 seg_tab.Used |= seg_tab.Descriptors[i].Id;
	 segcnt++;
      }
      return XmemErrorSUCCESS;
   }
   return XmemErrorCallback(XmemErrorSEG_TABLE_READ,0);
}

/*****************************************************************/
/* Local routine to initialize one real device                   */

static XmemError InitDevice(XmemDevice device) {

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

      default: break;
   }
   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Exported non static Xmem Lib functions                               */

/* ==================================================================== */
/* Try to initialize the specified device                               */

XmemError XmemInitialize(XmemDevice device) { /* Initialize hardware/software */
XmemDevice fdev, ldev, dev;
XmemError err;

   if (libinitialized == 0) {

      bzero((void *) attach_address, XmemMAX_TABLES*sizeof(long *));

      bzero((void *) &node_tab, sizeof(XmemDrvrNodeTable));
      err = XmemReadNodeTableFile();
      if (err != XmemErrorSUCCESS) return err;

      bzero((void *) &seg_tab, sizeof(XmemDrvrSegTable));
      err = XmemReadSegTableFile();
      if (err != XmemErrorSUCCESS) return err;

      if (device == XmemDeviceANY) {
	 fdev = XmemDeviceVMIC;
	 ldev = XmemDeviceNETWORK;
      } else {
	 fdev = device;
	 ldev = device;
      }

      for (dev=fdev; dev<=ldev; dev++) {
	 err = InitDevice(dev);
	 if (err == XmemErrorSUCCESS) {
	    libinitialized = 1;
	    return err;
	 }
      }
   } else return XmemErrorSUCCESS;

   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Get the node ID for this node                                        */

XmemNodeId XmemWhoAmI() {
   return my_nid;
}

/* ==================================================================== */
/* Check for warm/cold start up. Returns non zero if this is a warm     */
/* start and returns 0 if its a cold start.                             */
/* Daemons need to know if they are starting warm or cold to correctly  */
/* initialize tables and reflective memory.                             */
/* Once the driver tables have been loaded, subsequent instances of any */
/* programs using XmemLib will always receive the warm start status,    */
/* cold start status occurs only when the driver tables have not been   */
/* loaded just after a reboot and a fresh driver install.               */

int XmemCheckForWarmStart() {
   return warm_start;
}

/* ==================================================================== */
/* Convert an error into a string. The result is a static string.       */

char *XmemErrorToString(XmemError err) {

static char result[XmemErrorSTRING_SIZE];
char *cp;

   if ((err < 0) || (err >= XmemErrorCOUNT)) cp = "No such error number";
   else                                      cp = estr[(int) err];
   bzero((void *) result, XmemErrorSTRING_SIZE);
   strcpy(result,cp);
   return result;
}

/* ==================================================================== */
/* Get all currently up and running nodes.                              */
/* The set of up and running nodes is determined dynamically through a  */
/* handshake protocol between nodes.                                    */

XmemNodeId  XmemGetAllNodeIds() {

   if (libinitialized) return routines.GetAllNodeIds();
   return 0;
}

/* ==================================================================== */
/* Register your call back to handle Xmem Events.                       */

XmemError XmemRegisterCallback(void (*cb)(XmemCallbackStruct *cbs),
			       XmemEventMask mask) {
XmemError err;

   if (libinitialized) {
      err = routines.RegisterCallback(cb,mask);
      if (err == XmemErrorSUCCESS) {
	 if (mask) { libcallmask |= mask; libcallback = cb;   }
	 else      { libcallmask = 0;     libcallback = NULL; }
      }
      return err;
   }
   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Get current registered callback events mask                          */

XmemEventMask XmemGetRegisteredEvents() {
   return (XmemEventMask) libcallmask;
}

/* ==================================================================== */
/* Wait for an Xmem Event with time out, an incomming event will call   */
/* any registered callback.                                             */

XmemEventMask XmemWait(int timeout) {

   if (libinitialized) return routines.Wait(timeout);
   return 0;
}

/* ==================================================================== */
/* Poll for any Xmem Events, an incomming event will call any callback  */
/* that is registered for that event.                                   */

XmemEventMask XmemPoll() {

   if (libinitialized) return routines.Poll();
   return 0;
}

/* ==================================================================== */
/* Send your buffer to a reflective memory table. A table update event  */
/* is broadcast automatically.                                          */

XmemError XmemSendTable(XmemTableId   table,    /* Table to be written to     */
				 long *buf,     /* Buffer containing table    */
			unsigned long longs,    /* Number of longs to transfer*/
			unsigned long offset,   /* Offset of transfer         */
			unsigned long upflag) { /* Update message flag        */

   if (libinitialized) return routines.SendTable(table,buf,longs,offset,upflag);
   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Update your buffer from a reflective memory table.                   */

XmemError XmemRecvTable(XmemTableId   table,    /* Table to be read from      */
				 long *buf,     /* Buffer containing table    */
			unsigned long longs,    /* Number of longs to transfer*/
			unsigned long offset) { /* Offset of transfer         */

   if (libinitialized) return routines.RecvTable(table,buf,longs,offset);
   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Send a message to other nodes. If the node parameter is zero, then   */
/* nothing happens, if its equal to 0xFFFFFFFF the message is sent via  */
/* broadcast, else multicast or unicast is used.                        */

XmemError XmemSendMessage(XmemNodeId nodes, XmemMessage *mess) {

   if (libinitialized) return routines.SendMessage(nodes,mess);
   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Check which tables were updated.                                     */

XmemTableId XmemCheckTables() {

   if (libinitialized) return routines.CheckTables();
   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* This routine permits invoking your callback for errors.              */

XmemError XmemErrorCallback(XmemError err, unsigned long ioe) {

XmemCallbackStruct cbs;

   if (libcallback) {
      bzero((void *) &cbs, sizeof(XmemCallbackStruct));
      switch (err) {

	 case XmemErrorSUCCESS:
	 break;

	 case XmemErrorTIMEOUT:
	    cbs.Mask = XmemEventMaskTIMEOUT;
	    if (libcallmask & XmemEventMaskTIMEOUT) libcallback(&cbs);
	 break;

	 case XmemErrorIO:
	    cbs.Mask = XmemEventMaskIO;
	    cbs.Data = ioe;
	    if (libcallmask & XmemEventMaskIO) libcallback(&cbs);
	 break;

	 case XmemErrorSYSTEM:
	    cbs.Mask = XmemEventMaskSYSTEM;
	    cbs.Data = ioe;
	    if (libcallmask & XmemEventMaskSYSTEM) libcallback(&cbs);
	 break;

	 default:
	    cbs.Mask = XmemEventMaskSOFTWARE;
	    cbs.Data = (unsigned long) err;
	    if (libcallmask & XmemEventMaskSOFTWARE) libcallback(&cbs);
	 break;
      }
   }
   return err;
}

/* ==================================================================== */
/* This is general enough to warrant putting in the library.            */
/* This hash algorithum is needed for handling shared memory keys.      */

int XmemGetKey(char *name) {
int i, key;

   key = 0;
   if (name != NULL)
      for (i=0; i<strlen(name); i++)
	 key = (key << 1) + (int) name[i];
   return(key);
}

/* ========================================================================== */
/* Create/Get a shared memory segment to hold the given table, and initialize */
/* it by reading from reflective memory. The shared memory segment is opened  */
/* with read and write access. If it exists already then a pointer to it is   */
/* returned, otherwise it is created and initialized. The tid parameter must  */
/* have only one bit set in it. On error a null is returned, and any callback */
/* you have registered for errors will be called, in any case errno contains  */
/* the system error number for the failure.                                   */

long *XmemGetSharedMemory(XmemTableId tid) {

key_t smemky;
unsigned long longs, user, bytes, smemid, *table;
char *cp;
XmemError err;
XmemNodeId nid;
int tnum, msk;


   if (libinitialized) {

      for (tnum=0; tnum<XmemMAX_TABLES; tnum++) {
	 msk = 1<<tnum;
	 if (msk & tid) {
	    if (attach_address[tnum]) return attach_address[tnum];
	    else break;
	 }
      }

      cp = XmemGetTableName(tid);
      if (cp) {
	 err = XmemGetTableDesc(tid, &longs, &nid, &user);
	 if (err == XmemErrorSUCCESS) {
	    bytes = longs*sizeof(long);
	    smemky = XmemGetKey(cp);
	    smemid = shmget(smemky,bytes,0666);
	    if (smemid == -1) {
	       if (errno == ENOENT) {
		  smemid = shmget(smemky,bytes,IPC_CREAT | 0666);
		  if (smemid != -1) {
		     table = (long *) shmat(smemid,(char *) 0,0);
		     if ((int) table != -1) {
			if (tnum < XmemMAX_TABLES) attach_address[tnum] = table;
			err = XmemRecvTable(tid,table,longs,0);
			if (err == XmemErrorSUCCESS) return table;
		     }
		  }
	       }
	    } else {
	       table = (long *) shmat(smemid,(char *) 0,0);
	       if ((int) table != -1) {
		  if (tnum < XmemMAX_TABLES) attach_address[tnum] = table;
		  return table;
	       }
	    }
	 }
      }
   }
   XmemErrorCallback(XmemErrorSYSTEM,errno);
   return NULL;
}

/* ==================================================================== */
/* Get all currently defined table Ids. There can be up to 32 tables    */
/* each ones Id corresponds to a single bit in an unsigned long.        */

XmemTableId XmemGetAllTableIds() {

   return seg_tab.Used;
}

/* ==================================================================== */
/* Get the name of a node from its node Id, this is usually the name of */
/* the host on which the node is implemented.                           */

char * XmemGetNodeName(XmemNodeId node) {

int i;
unsigned long msk;

   if (xmem) {
      for (i=0; i<XmemMAX_NODES; i++) {
	 msk = 1 << i;
	 if (node_tab.Used & msk) {
	    if (node == node_tab.Descriptors[i].Id)
	       return node_tab.Descriptors[i].Name;
	 }
      }
      XmemErrorCallback(XmemErrorNO_SUCH_NODE,0);
   }
   return (char *) 0;
}

/* ==================================================================== */
/* Get the Id of a node from its name.                                  */

XmemNodeId XmemGetNodeId(XmemName name) {

int i;
unsigned long msk;

   if (xmem) {
      for (i=0; i<XmemMAX_NODES; i++) {
	 msk = 1 << i;
	 if (node_tab.Used & msk) {
	    if (strcmp(name,node_tab.Descriptors[i].Name)==0)
	       return node_tab.Descriptors[i].Id;
	 }
      }
      XmemErrorCallback(XmemErrorNO_SUCH_NODE,0);
   }
   return 0;
}

/* ==================================================================== */
/* Get the name of a table from its table Id. Tables are defined from   */
/* a configuration file.                                                */

char * XmemGetTableName(XmemTableId table) {

int i;
unsigned long msk;

   if (xmem) {
      for (i=0; i<XmemMAX_TABLES; i++) {
	 msk = 1 << i;
	 if (seg_tab.Used & msk) {
	    if (table == seg_tab.Descriptors[i].Id)
	       return seg_tab.Descriptors[i].Name;
	 }
      }
      XmemErrorCallback(XmemErrorNO_SUCH_TABLE,0);
   }
   return (char *) 0;
}

/* ==================================================================== */
/* Get the Id of a table from its name.                                 */

XmemTableId XmemGetTableId(XmemName name ) {

int i;
unsigned long msk;

   if (xmem) {
      for (i=0; i<XmemMAX_TABLES; i++) {
	 msk = 1 << i;
	 if (seg_tab.Used & msk) {
	    if (strcmp(name,seg_tab.Descriptors[i].Name)==0)
	       return seg_tab.Descriptors[i].Id;
	 }
      }
      XmemErrorCallback(XmemErrorNO_SUCH_TABLE,0);
   }
   return 0;
}

/* ==================================================================== */
/* Get the description of a table, namley its size, and which nodes can */
/* write to it.                                                         */

XmemError XmemGetTableDesc(XmemTableId    table,
			   unsigned long  *longs,
			   XmemNodeId     *nodes,
			   unsigned long  *user) {

int i;
unsigned long msk;

   if (xmem) {
      for (i=0; i<XmemMAX_TABLES; i++) {
	 msk = 1 << i;
	 if (seg_tab.Used & msk) {
	    if (table == seg_tab.Descriptors[i].Id) {
	       *longs = seg_tab.Descriptors[i].Size/sizeof(long);
	       *nodes = seg_tab.Descriptors[i].Nodes;
	       *user  = seg_tab.Descriptors[i].User;
	       return XmemErrorSUCCESS;
	    }
	 }
      }
      return XmemErrorCallback(XmemErrorNO_SUCH_TABLE,0);
   }
   return XmemErrorCallback(XmemErrorNOT_INITIALIZED,0);
}

/* ==================================================================== */
/* Read tables from files into shared memory segments                   */

XmemError XmemReadTableFile(XmemTableId tid) {

int i, cnt;
unsigned long msk;
char *cp;
FILE *fp;
unsigned long user, longs, bytes;
XmemNodeId nodes;
unsigned long *table;
XmemError err;
char tbnam[64];

   for (i=0; i<XmemMAX_TABLES; i++) {
      msk = 1 << i;
      if (msk & tid) {
	 cp = XmemGetTableName(msk);
	 err = XmemGetTableDesc(msk,&longs,&nodes,&user);
	 if (err != XmemErrorSUCCESS) return err;
	 table = XmemGetSharedMemory(msk);
	 if (table) {
	    bzero((void *) tbnam, 64);
	    strcat(tbnam,cp);
	    umask(0);
	    fp = fopen(XmemGetFile(tbnam),"r");
	    if (fp) {
	       bytes = longs*sizeof(long);
	       cnt = fread(table,bytes,1,fp);
	       if (cnt <= 0) err = XmemErrorCallback(XmemErrorSYSTEM,errno);
	       fclose(fp); fp = 0;
	       if (err != XmemErrorSUCCESS) return err;
	    } else return XmemErrorCallback(XmemErrorSYSTEM,errno);
	 } else return XmemErrorNO_SUCH_TABLE;
      }
   }
   return XmemErrorSUCCESS;
}

/* ==================================================================== */
/* Write tables from shared memory segments to files                    */

XmemError XmemWriteTableFile(XmemTableId tid) {

int i, cnt;
unsigned long msk;
char *cp;
FILE *fp;
unsigned long user, longs, bytes;
XmemNodeId nodes;
unsigned long *table;
XmemError err;
char tbnam[64];

   for (i=0; i<XmemMAX_TABLES; i++) {
      msk = 1 << i;
      if (msk & tid) {
	 cp = XmemGetTableName(msk);
	 err = XmemGetTableDesc(msk,&longs,&nodes,&user);
	 if (err != XmemErrorSUCCESS) return err;
	 table = XmemGetSharedMemory(msk);
	 if (table) {
	    bzero((void *) tbnam, 64);
	    strcat(tbnam,cp);
	    umask(0);
	    fp = fopen(XmemGetFile(tbnam),"w");
	    if (fp) {
	       bytes = longs*sizeof(long);
	       cnt = fwrite(table,bytes,1,fp);
	       if (cnt <= 0) err = XmemErrorCallback(XmemErrorSYSTEM,errno);
	       fclose(fp); fp = 0;
	       if (err != XmemErrorSUCCESS) return err;
	    } else return XmemErrorCallback(XmemErrorSYSTEM,errno);
	 } else return XmemErrorNO_SUCH_TABLE;
      }
   }
   return XmemErrorSUCCESS;
}

/* ==================================================================== */
/* Block all client callbacks while the daemon is running               */

#ifdef __linux__

union semun {
   int val;
   struct semid_ds *buff;
   unsigned short *array;
} arg;

#else

union semun arg;

#endif

XmemError XmemBlockCallbacks() {

int key;
int sd;
int cnt;
int err;

   key = XmemGetKey(XMEM_SEMAPHORE);
   sd  = semget(key,1,0666|IPC_CREAT);
   if (sd == -1) return XmemErrorSYSTEM;

   arg.val = 1;
   err = semctl(sd,0,SETVAL,arg);
   cnt = semctl(sd,0,GETVAL,arg);
   if (err == -1) {
      perror("XmemBlockCallbacks");
      return XmemErrorSYSTEM;
   }
   return XmemErrorSUCCESS;
}

/* ==================================================================== */
/* UnBlock all client callbacks while the daemon is running             */

XmemError XmemUnBlockCallbacks() {

int key;
int sd;
int cnt;
int err;

   key = XmemGetKey(XMEM_SEMAPHORE);
   sd  = semget(key,1,0);
   if (sd == -1) return XmemErrorSYSTEM;

   arg.val = 0;
   err = semctl(sd,0,SETVAL,arg);
   cnt = semctl(sd,0,GETVAL,arg);
   if (err == -1) {
      perror("XmemUnBlockCallbacks");
      return XmemErrorSYSTEM;
   }

   return XmemErrorSUCCESS;
}

/* ==================================================================== */
/* Test to see if client callbacks are blocked                          */

int XmemCallbacksBlocked() {

int key;
int sd;
int cnt;

  key = XmemGetKey(XMEM_SEMAPHORE);
  sd  = semget(key,1,0);
  if (sd == -1) return 0;

  cnt = semctl(sd,0,GETVAL,arg);
  return cnt;
}

/* =========================== */

void XmemLibUsleep(int dly) {
   struct timespec rqtp, rmtp; /* 'nanosleep' time structure */
   rqtp.tv_sec = 0;
   rqtp.tv_nsec = dly*1000;
   nanosleep(&rqtp, &rmtp);
}

/* ================================================================= */
/* Compile an event table by sending a message to the table compiler */
/* server message queue. The resulting object file is transfered to  */
/* the LHC MTGs accross reflective memory. This routine avoids the   */
/* need for issuing system calls within complex multithreaded FESA   */
/* classes. The table must be loaded as usual and completion must    */
/* be checked as usual by looking at the response xmem tables.       */
/* If the return code is an IO error this probably means the cmtsrv  */
/* task is not running. A SYS error indicates a message queue error. */

#define TableTskObjM 0x200

/* ================================================================= */

int /* MttLibError */ MttLibCompileTable(char *name) {

ssize_t ql;
mqd_t q;
int tmo, terr;

XmemTableId tid;

   tid = XmemCheckTables();             /* Clear out any old stuff  */

   q = mq_open("/tmp/cmtsrv",(O_WRONLY | O_NONBLOCK));
   if (q == (mqd_t) -1) return 3; /* MttLibErrorIO; */

   ql = mq_send(q,name,32 /* MttLibMAX_NAME_SIZE */,0);
   terr = errno;
   mq_close(q);
   if (q == (mqd_t) -1) {
      if (terr == EAGAIN) return 3; /* MttLibErrorIO; */
      else                return 4; /* MttLibErrorSYS; */
   }

   tmo = 0;
   while (tmo++ < 1000) {               /* 20 Seconds max wait */
      tid = XmemCheckTables();
      if (tid & TableTskObjM) {
	 XmemLibUsleep(30000);          /* Sleep 30 milliseconds */
	 return 0;                      /* MttLibErrorNONE; Object table received */
      }
      XmemLibUsleep(20000);             /* Sleep 20 milliseconds */
   }

   return 3; /* MttLibErrorIO; Timeout */
}
