/* ========================================================================== */
/* Include file for XMEM Reflective Memory Library                            */
/* Julian Lewis 9th/Feb/2005                                                  */
/*                                                                            */
/* This is the basic interface needed to implement an application layer over  */
/* distributed memory. I would expect users of this library to write a daemon */
/* on top of this. A daemon is nothing more than a WAIT loop with a callback. */
/* The callback then implements the application layer code.                   */
/*                                                                            */
/* In this library, and the Xmem driver use 32-Bit long values to store Node  */
/* and Table identifiers so there is a fundamental limit of 32 for both.      */
/*                                                                            */
/* When both VMIC and TCP backends are used then a TCP bridging daemon must   */
/* run on a VMIC node. Each bridging daemon adds another 32 nodes.            */
/* ========================================================================== */

#ifndef XMEMLIB
#define XMEMLIB

#ifdef __linux__
#define XMEM_PATH "/dsc/data/xmem/"
#else
#define XMEM_PATH "/dsc/data/xmem/"
#endif

#define XMEM_SEMAPHORE "XMEMSEM"
#define SEG_TABLE_NAME "Xmem.segs"
#define NODE_TABLE_NAME "Xmem.nodes"
#define CONFIG_FILE_NAME "Xmem.conf"

/* ========================================================================== */
/* Access configuration data base to get full path of a file.                 */
/* Name is the name of the file to get, the result is the full path.          */

char *XmemGetFile(char *name);

/* ========================================================================== */
/* Error codes returned from Xmem functions                                   */

typedef enum {
   XmemErrorSUCCESS,           /* All went OK, No error */
   XmemErrorTIMEOUT,           /* Timeout */
   XmemErrorNOT_INITIALIZED,   /* The library has not been initialized */
   XmemErrorWRITE_PROTECTED,   /* You don't have write segment access */
   XmemErrorSEG_TABLE_READ,    /* Could not read segment table file */
   XmemErrorSEG_TABLE_SYNTAX,  /* Syntax error in segment table file */
   XmemErrorNODE_TABLE_READ,   /* Could not read node table file */
   XmemErrorNODE_TABLE_SYNTAX, /* Syntax error in node table file */
   XmemErrorNO_TABLES,         /* No tables are defined */
   XmemErrorNO_SUCH_TABLE,     /* That table is undefined */
   XmemErrorNO_SUCH_NODE,      /* That node is undefined */
   XmemErrorNO_SUCH_MESSAGE,   /* Illegal message */
   XmemErrorIO,                /* An XmemDrvr IO error, see errno */
   XmemErrorSYSTEM,            /* System error */

   XmemErrorCOUNT
 } XmemError;

#define XmemErrorSTRING_SIZE 64

/* Convert an error code into a printable string */

char *XmemErrorToString(XmemError err);

/* ========================================================================== */
/* Initialize the library to use the given device                             */

typedef enum {
   XmemDeviceANY,     /* Use any device available */
   XmemDeviceVMIC,    /* Use VMIC 6656 reflective memory */
   XmemDeviceSHMEM,   /* Local shared memory */
   XmemDeviceNETWORK, /* Use ethernet TCP/IP and UDP protocols */

   XmemDeviceCOUNT    /* Number of available devices */
 } XmemDevice;

/* Set the device to be used by the library and */
/* perform library initialization code. */

XmemError XmemInitialize(XmemDevice device);

/* ==================================================================== */
/* Check for warm/cold start up. Returns non zero if this is a warm     */
/* start and returns 0 if its a cold start.                             */
/* Daemons need to know if they are starting warm or cold to correctly  */
/* initialize tables and reflective memory.                             */
/* Once the driver tables have been loaded, subsequent instances of any */
/* programs using XmemLib will always receive the warm start status,    */
/* cold start status occurs only when the driver tables have not been   */
/* loaded just after a reboot and a fresh driver install.               */

int XmemCheckForWarmStart(void);

/* ========================================================================== */
/* Handle host and table names and attributes                                 */

#define XmemNAME_SIZE 32
typedef char XmemName[XmemNAME_SIZE];

/* ========================================================================== */
/* Nodes are either hosts or VMIC modules. They are identified by a single    */
/* bit, and they have names, mostly its the host name.                        */

#define XmemMAX_NODES 32         /* One bit for each node */
#define XmemALL_NODES 0xFFFFFFFF /* All nodes mask */

typedef enum {
   XmemNodeId_01 = 0x00000001, XmemNodeId_02 = 0x00000002,
   XmemNodeId_03 = 0x00000004, XmemNodeId_04 = 0x00000008,
   XmemNodeId_05 = 0x00000010, XmemNodeId_06 = 0x00000020,
   XmemNodeId_07 = 0x00000040, XmemNodeId_08 = 0x00000080,
   XmemNodeId_09 = 0x00000100, XmemNodeId_10 = 0x00000200,
   XmemNodeId_11 = 0x00000400, XmemNodeId_12 = 0x00000800,
   XmemNodeId_13 = 0x00001000, XmemNodeId_14 = 0x00002000,
   XmemNodeId_15 = 0x00004000, XmemNodeId_16 = 0x00008000,
   XmemNodeId_17 = 0x00010000, XmemNodeId_18 = 0x00020000,
   XmemNodeId_19 = 0x00040000, XmemNodeId_20 = 0x00080000,
   XmemNodeId_21 = 0x00100000, XmemNodeId_22 = 0x00200000,
   XmemNodeId_23 = 0x00400000, XmemNodeId_24 = 0x00800000,
   XmemNodeId_25 = 0x01000000, XmemNodeId_26 = 0x02000000,
   XmemNodeId_27 = 0x04000000, XmemNodeId_28 = 0x08000000,
   XmemNodeId_29 = 0x10000000, XmemNodeId_30 = 0x20000000,
   XmemNodeId_31 = 0x40000000, XmemNodeId_32 = 0x80000000
 } XmemNodeId;

XmemNodeId XmemGetAllNodeIds();              /* Bit mask of all ACTIVE nodes (**) */
char      *XmemGetNodeName(XmemNodeId node); /* Get the name of a node */
XmemNodeId XmemGetNodeId(XmemName name);     /* Get the node of a name */
XmemNodeId XmemWhoAmI(void);                     /* Get my node Id */

/* (**) When there is no driver installed on a DSC, there will be no responce from */
/*      that DSC to the PENDING_INIT interrupt, so the node will be removed from   */
/*      the list of ACTIVE nodes. When the node comes back up, it sends out a new  */
/*      non-zero PENDING_INIT broadcast, and all nodes will respond restablishing  */
/*      the ACTIVE node list in every node. This is part of the driver ISR logic.  */
/*      The library event XmemEventMaskINITIALIZED corresponds to PENDING_INIT.    */

/* ========================================================================== */
/* Tables are chunks of reflective memory. They are identified by a single    */
/* bit, and they have names.                                                  */

#define XmemMAX_TABLES 32         /* One bit for each table */
#define XmemALL_TABLES 0xFFFFFFFF /* All tables mask */

typedef enum {
   XmemTableId_01 = 0x00000001, XmemTableId_02 = 0x00000002,
   XmemTableId_03 = 0x00000004, XmemTableId_04 = 0x00000008,
   XmemTableId_05 = 0x00000010, XmemTableId_06 = 0x00000020,
   XmemTableId_07 = 0x00000040, XmemTableId_08 = 0x00000080,
   XmemTableId_09 = 0x00000100, XmemTableId_10 = 0x00000200,
   XmemTableId_11 = 0x00000400, XmemTableId_12 = 0x00000800,
   XmemTableId_13 = 0x00001000, XmemTableId_14 = 0x00002000,
   XmemTableId_15 = 0x00004000, XmemTableId_16 = 0x00008000,
   XmemTableId_17 = 0x00010000, XmemTableId_18 = 0x00020000,
   XmemTableId_19 = 0x00040000, XmemTableId_20 = 0x00080000,
   XmemTableId_21 = 0x00100000, XmemTableId_22 = 0x00200000,
   XmemTableId_23 = 0x00400000, XmemTableId_24 = 0x00800000,
   XmemTableId_25 = 0x01000000, XmemTableId_26 = 0x02000000,
   XmemTableId_27 = 0x04000000, XmemTableId_28 = 0x08000000,
   XmemTableId_29 = 0x10000000, XmemTableId_30 = 0x20000000,
   XmemTableId_31 = 0x40000000, XmemTableId_32 = 0x80000000
 } XmemTableId;

XmemTableId XmemGetAllTableIds();                /* Bit mask of all defined tables */
char       *XmemGetTableName(XmemTableId table); /* Get the name of a node */
XmemTableId XmemGetTableId(XmemName name );      /* Get the node of a name */

/* ========================================================================== */
/* Get the given tables size in longs, and the nodes with write access to it. */
/* WARNING: only long accesses can be made, the longs parameter contains the  */
/* size of the table in longs. To get the real size multiply by sizeof(long). */

XmemError XmemGetTableDesc(XmemTableId    table,
			   unsigned long *longs,
			   XmemNodeId    *nodes,
			   unsigned long *user);    /* Your User data */

/* ========================================================================== */
/* The callback routines can be called for different reasons...               */

typedef enum {
   XmemEventMaskTIMEOUT      = 0x001, /* IO timeout */
   XmemEventMaskSEND_TABLE   = 0x002, /* Send table(s), see TableId mask */
   XmemEventMaskUSER         = 0x004, /* User interrupt, see data */
   XmemEventMaskTABLE_UPDATE = 0x008, /* Table has been updated, see TableId */
   XmemEventMaskINITIALIZED  = 0x010, /* Node initialized, see node */
   XmemEventMaskIO           = 0x020, /* Some IO error, Data contains XmemIoError */
   XmemEventMaskKILL         = 0x040, /* Kill your self */
   XmemEventMaskSOFTWARE     = 0x080, /* Software error, Data contains XmemIoError */
   XmemEventMaskSYSTEM       = 0x100, /* System error, Data contains system errno */

   XmemEventMaskMASK         = 0x1FF  /* Mask bits */
 } XmemEventMask;

#define XmemEventMASKS 9

/* The XmemEventMaskIO and XmemEventMaskSOFTWARE events carry an XmemIoError code so that, */
/* a subscribed callback can get extra information on just what went wrong.                */

typedef enum {
   XmemIoErrorPARITY   = 0x01,    /* Parity error */
   XmemIoErrorROGUE    = 0x02,    /* Rogue packet killed */
   XmemIoErrorCONTACT  = 0x04,    /* Lost contact with network */
   XmemIoErrorHARDWARE = 0x08,    /* Hardware error, this should never happen */
   XmemIoErrorSOFTWARE = 0x10,    /* Software error, this should never happen */

   XmemIoErrorMASK     = 0x1F
 } XmemIoError;

#define XmemIoERRORS 5

/* The XmemEventMaskINITIALIZED event carries a message with it from the originating node. */
/* The value 0 is used as an acknowledge from other nodes saying I have seen your request, */
/* this is sent automatically by the library in response to an Init message.               */
/* The value 1 is sent automatically whenever any device RESET logic is called.            */
/* Other values are user defined.                                                          */

typedef enum {
   XmemInitMessageSEEN_BY,      /* Zero is used to establish the network topology */
   XmemInitMessageRESET,        /* The node hase been reset */
   XmemInitMessageUSER_DEFINED, /* A user defined message */

   XmemInitMessageMESSAGES
 } XmemInitMessage;

/* This structure is delivered to your callback on receipt of an interrupt */

typedef struct {
   XmemEventMask Mask;        /* One bit only can be set on call */
   XmemTableId   Table;       /* Table when relavent */
   XmemNodeId    Node;        /* Node  when relavent */
   unsigned long Data;        /* Users data */
 } XmemCallbackStruct;

/* If you set a callback, it gets called with the above structure. */
/* Calling with NULL deletes any previous callback that was installed. */
/* Calling the Wait or Poll library functions may cause the call back to be envoked. */
/* Implementing a daemon is nothing more than writing a callback. */

/* Register your callback function, and              */
/* subscribe to the things you want delivered to it. */
/* The callback should be like this: void callback(XmemCallbackStruct *cbs) */

XmemError XmemRegisterCallback(void (*cb)(XmemCallbackStruct *cbs), XmemEventMask mask);

/* Lets you know which events are registered in your callback */

XmemEventMask XmemGetRegisteredEvents(void);

/* Blocking wait, or just a Poll, both may call your callback */

XmemEventMask XmemWait(int timeout); /* timeout = 0 means forever, else seconds to wait */
XmemEventMask XmemPoll(void);

/* ========================================================================== */
/* Send and receive tables from reflective memory.                            */
/* When writing the last time to the table, set the upflag non zero so that a */
/* table update message is sent out to all nodes.                             */
/* If SendTable is called with a NULL buffer, the driver will flush the       */
/* reflective memory segment to the network by copying it onto itself. This   */
/* is rather slow compared to the usual copy because DMA can not be used, and */
/* two PCI-bus accesses ~1us are used for each long word.                     */

/* WARNING: Only 32 bit accesses can be made. So the buf parameter must be on */
/* a long boundary, the offset is in longs, and longs is the number of longs  */
/* to be transfered. This can be tricky because the size of the buffer to be  */
/* allocated must be multiplied by sizeof(long).                              */

XmemError XmemSendTable(XmemTableId   table,    /* Table to be written to     */
				 long *buf,     /* Buffer containing table    */
			unsigned long longs,    /* Number of longs to transfer*/
			unsigned long offset,   /* Offset of transfer         */
			unsigned long upflag);  /* Update message flag        */

XmemError XmemRecvTable(XmemTableId   table,    /* Table to be read from      */
				 long *buf,     /* Buffer containing table    */
			unsigned long longs,    /* Number of longs to transfer*/
			unsigned long offset);  /* Offset of transfer         */

/* ========================================================================== */
/* Send messages to other hosts, and myself                                   */
/* N.B. SEND_TABLE will only work if there is a daemon active at the receiver */
/* node connected to the SEND_TABLE interrupt which then writes the table.    */
/* N.B. INITIALIZE_ME is used to get nodes that own tables to write/flush     */
/* their tables to DSCs doing a cold start. This needs daemon support to work.*/
/* When many nodes own a table, the highest ACTIVE daemon ID flushes it.      */

typedef enum {
   XmemMessageTypeSEND_TABLE,    /* Send me a table */
   XmemMessageTypeUSER,          /* Send a USER message */
   XmemMessageTypeTABLE_UPDATE,  /* Send a table update message */
   XmemMessageTypeINITIALIZE_ME, /* Send "I need to be initialized" */
   XmemMessageTypeKILL,          /* Send "Kill yourself" */

   XmemMessageTypeCOUNT
 } XmemMessageType;

typedef struct {
   XmemMessageType MessageType; /* As above */
   unsigned long   Data;        /* TableId/UserData */
 } XmemMessage;

/* Multicast to nodes, if nodes = XmemALL_NODES (0xFFFFFFFF) broadcast is used. */

XmemError XmemSendMessage(XmemNodeId nodes, XmemMessage *mess);

/* ========================================================================== */
/* Check to see which tables have been updated since the last call. For each  */
/* table that has been modified, its ID bit is set in the returned TableId.   */
/* For each client the call maintains the set of tables that the client has   */
/* not yet seen, and this set is zeroed after each call.                      */
/* This will not envoke your registered callback even if you have registered  */
/* for TABLE_UPDATE interrupts.                                               */

XmemTableId XmemCheckTables(void);

/* ==================================================================== */
/* This routine permits invoking your callback for errors.              */
/* The err parameter contains one error, and ioe contains the sub-error */
/* code as defined above in XmemEventMask description.                  */

XmemError XmemErrorCallback(XmemError err, unsigned long ioe);

/* ==================================================================== */
/* This is general enough to warrant putting in the library.            */
/* This hash algorithum is needed for handling shared memory keys.      */

int XmemGetKey(char *name);

/* ========================================================================== */
/* Create/Get a shared memory segment to hold the given table, and initialize */
/* it by reading from reflective memory. The shared memory segment is opened  */
/* with read and write access. If it exists already then a pointer to it is   */
/* returned, otherwise it is created and initialized. The tid parameter must  */
/* have only one bit set in it. On error a null is returned, and any callback */
/* you have registered for errors will be called, in any case errno contains  */
/* the system error number for the failure.                                   */

long *XmemGetSharedMemory(XmemTableId tid);

/* ==================================================================== */
/* Read tables from files into shared memory segments                   */

XmemError XmemReadTableFile(XmemTableId tid);

/* ==================================================================== */
/* Write tables from shared memory segments to files                    */

XmemError XmemWriteTableFile(XmemTableId tid);

/* ==================================================================== */
/* Block and unblock all client callbacks while the daemon is running   */

XmemError XmemBlockCallbacks();
XmemError XmemUnBlockCallbacks();
int       XmemCallbacksBlocked();

#endif
