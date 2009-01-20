/**
 * @file libxmem.h
 *
 * @brief XMEM Reflective Memory Library
 *
 * @author Julian Lewis
 *
 * @date Created on 09/02/2005
 *
 * This is the basic interface needed to implement an application layer over
 * distributed memory. I would expect users of this library to write a daemon
 * on top of this. A daemon is nothing more than a WAIT loop with a callback.
 * The callback then implements the application layer code.
 *
 * In this library, and the Xmem driver use 32-Bit long values to store Node
 * and Table identifiers so there is a fundamental limit of 32 for both.
 *
 * When both VMIC and TCP backends are used then a TCP bridging daemon must
 * run on a VMIC node. Each bridging daemon adds another 32 nodes.
 *
 * NOTE. The library (and subsequent driver) assume that the size of a long
 * in both user and kernel space is 4 bytes; this is only true for 32-bit
 * machines. Therefore 64-bit platforms are simply _not_ supported. This
 * was a design mistake from the early beginning and has not been fixed
 * (up to v1.1) because we have other priorities.
 *
 * @version 1.1 Emilio G. Cota 16/01/2009
 *
 * @version 1.0 Julian Lewis
 */
#ifndef XMEMLIB
#define XMEMLIB

#ifdef __linux__
#define XMEM_PATH "/dsc/data/xmem/"
#else /* LynxOs */
#define XMEM_PATH "/dsc/data/xmem/"
#endif /* !__linux__ */

#define XMEM_SEMAPHORE "XMEMSEM"
#define SEG_TABLE_NAME "Xmem.segs"
#define NODE_TABLE_NAME "Xmem.nodes"
#define CONFIG_FILE_NAME "Xmem.conf"



/*
 * Note. Documentation for each of the functions is in the .c file.
 * Doxygen automatically will take the documentation from there
 */



/*! @name Error codes returned from Xmem functions
 */
//@{
typedef enum {
  XmemErrorSUCCESS,           //!< All went OK, No error
  XmemErrorTIMEOUT,           //!< Timeout
  XmemErrorNOT_INITIALIZED,   //!< The library has not been initialized
  XmemErrorWRITE_PROTECTED,   //!< You don't have write segment access
  XmemErrorSEG_TABLE_READ,    //!< Could not read segment table file
  XmemErrorSEG_TABLE_SYNTAX,  //!< Syntax error in segment table file
  XmemErrorNODE_TABLE_READ,   //!< Could not read node table file
  XmemErrorNODE_TABLE_SYNTAX, //!< Syntax error in node table file
  XmemErrorNO_TABLES,         //!< No tables are defined
  XmemErrorNO_SUCH_TABLE,     //!< That table is undefined
  XmemErrorNO_SUCH_NODE,      //!< That node is undefined
  XmemErrorNO_SUCH_MESSAGE,   //!< Illegal message
  XmemErrorIO,                //!< An XmemDrvr IO error, see errno
  XmemErrorSYSTEM,            //!< System error

  XmemErrorCOUNT
} XmemError;

#define XmemErrorSTRING_SIZE 64
//@}


/*! @name Initialise the library to use the given device
 */
//@{
typedef enum {
  XmemDeviceANY,     //!< Use any device available
  XmemDeviceVMIC,    //!< Use VMIC 6656 reflective memory
  XmemDeviceSHMEM,   //!< Local shared memory
  XmemDeviceNETWORK, //!< Use ethernet TCP/IP and UDP protocols

  XmemDeviceCOUNT    //!< Number of available devices
} XmemDevice;
//@}


/*! @name Handle host and table names and attributes
 */
//@{
#define XmemNAME_SIZE 32
typedef char XmemName[XmemNAME_SIZE];
//@}


/*! @name Xmem Nodes
 *
 * Nodes are either hosts or VMIC modules. They are identified by a single
 * bit, and they have names, normally it's the hostname.
*/
//@{
#define XmemMAX_NODES 32         //!< One bit for each node
#define XmemALL_NODES 0xFFFFFFFF //!< All nodes mask

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
//@}


/*! @name XMEM Tables
 *
 * Tables are chunks of reflective memory. They are identified by a single
 * bit, and they have names.
 *
 * Get the given tables size in 'longs' (i.e. 4 bytes), and the nodes with write
 * access to it.
 *
 * WARNING: only 'long' accesses can be made, the longs parameter contains the
 * size of the table in longs. To get the real size multiply by sizeof(long).
 */
//@{
#define XmemMAX_TABLES 32         //!< One bit for each table
#define XmemALL_TABLES 0xFFFFFFFF //!< All tables mask

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
//@}


/*! @name Callback routines returning values
 *
 * The callback routines can be called for different reasons
 */
//@{
#define XmemEventMASKS 9

typedef enum {
  XmemEventMaskTIMEOUT      = 0x001, //!< IO timeout
  XmemEventMaskSEND_TABLE   = 0x002, //!< Send table(s), see TableId mask
  XmemEventMaskUSER         = 0x004, //!< User interrupt, see data
  XmemEventMaskTABLE_UPDATE = 0x008, //!< Table has been updated, see TableId
  XmemEventMaskINITIALIZED  = 0x010, //!< Node initialized, see node

  XmemEventMaskIO           = 0x020,
  //!< Some IO error, Data contains XmemIoError

  XmemEventMaskKILL         = 0x040, //!< Kill yourself

  XmemEventMaskSOFTWARE     = 0x080,
  //!< Software error, Data contains XmemIoError

  XmemEventMaskSYSTEM       = 0x100,
  //!< System error, Data contains system errno

  XmemEventMaskMASK         = 0x1FF  //!< Mask bits
} XmemEventMask;
//@}


/*! @name XmemIoError
 *
 * The XmemEventMask{IO,SOFTWARE} events carry an XmemIoError code so that a
 * subscribed callback can get extra information on just what went wrong.
 */
//@{
#define XmemIoERRORS 5

typedef enum {
  XmemIoErrorPARITY   = 0x01,    //!< Parity error
  XmemIoErrorROGUE    = 0x02,    //!< Rogue packet killed
  XmemIoErrorCONTACT  = 0x04,    //!< Lost contact with network
  XmemIoErrorHARDWARE = 0x08,    //!< Hardware error, this should never happen
  XmemIoErrorSOFTWARE = 0x10,    //!< Software error, this should never happen

  XmemIoErrorMASK     = 0x1F
} XmemIoError;
//@}


/*! @name Event: mask initialized
 *
 * The XmemEventMaskINITIALIZED event carries a message with it from the
 * originating node.
 * The value 0 is used as an acknowledge from other nodes saying I have seen
 * your request. This is sent automatically by the library in response to an
 * Init message.
 * The value 1 is sent automatically whenever any device RESET logic is called.
 * Other values are user defined.
 */
//@{
typedef enum {
  XmemInitMessageSEEN_BY,
  //!< Zero is used to establish the network topology

  XmemInitMessageRESET,        //!< The node hase been reset
  XmemInitMessageUSER_DEFINED, //!< A user defined message

  XmemInitMessageMESSAGES
} XmemInitMessage;
//@}


/*! This structure is delivered to your callback on receipt of an interrupt
 */
typedef struct {
  XmemEventMask Mask;        //!< One bit only can be set on call
  XmemTableId   Table;       //!< Table when relevant
  XmemNodeId    Node;        //!< Node  when relevant
  unsigned long Data;        //!< Users data
} XmemCallbackStruct;


/*! @name Messages to other hosts (and to itself)
 *
 * N.B. SEND_TABLE will only work if there is a daemon active at the receiver
 * node connected to the SEND_TABLE interrupt which then writes the table.
 * N.B. INITIALIZE_ME is used to get nodes that own tables to write/flush
 * their tables to DSCs doing a cold start. This needs daemon support to work.
 * When many nodes own a table, the highest ACTIVE daemon ID flushes it.
 */
//@{
typedef enum {
  XmemMessageTypeSEND_TABLE,    //!< Send me a table
  XmemMessageTypeUSER,          //!< Send a USER message
  XmemMessageTypeTABLE_UPDATE,  //!< Send a table update message
  XmemMessageTypeINITIALIZE_ME, //!< Send "I need to be initialized"
  XmemMessageTypeKILL,          //!< Send "Kill yourself"

  XmemMessageTypeCOUNT
} XmemMessageType;


/*! Xmem Message
 *
 * For sending messages to other hosts (and to itself)
 */
typedef struct {
  XmemMessageType MessageType; //!< See XmemMessageType
  unsigned long   Data;        //!< TableId/UserData
} XmemMessage;
//@}



/*
 * Public function declarations
 * Remember that the documentation is generated by Doxygen from
 * the comments in the .c library file.
 */


char 		*XmemGetFile(char *name);

char 		*XmemErrorToString(XmemError err);

XmemError 	XmemInitialize(XmemDevice device);

int 		XmemCheckForWarmStart(void);

XmemNodeId 	XmemGetAllNodeIds();
char      	*XmemGetNodeName(XmemNodeId node);
XmemNodeId 	XmemGetNodeId(XmemName name);

XmemNodeId	XmemWhoAmI(void);

XmemTableId 	XmemGetAllTableIds();
char       	*XmemGetTableName(XmemTableId table);
XmemTableId 	XmemGetTableId(XmemName name);
XmemError 	XmemGetTableDesc(XmemTableId table, unsigned long *longs,
				 XmemNodeId *nodes, unsigned long *user);

XmemError 	XmemRegisterCallback(void (*cb)(XmemCallbackStruct *cbs),
				     XmemEventMask mask);

XmemEventMask 	XmemGetRegisteredEvents(void);

XmemEventMask 	XmemWait(int timeout);
XmemEventMask 	XmemPoll(void);

XmemError 	XmemSendTable(XmemTableId table, long *buf, unsigned long longs,
			      unsigned long offset, unsigned long upflag);
XmemError 	XmemRecvTable(XmemTableId table, long *buf, unsigned long longs,
			      unsigned long offset);

XmemError 	XmemSendMessage(XmemNodeId nodes, XmemMessage *mess);

XmemTableId 	XmemCheckTables(void);

XmemError 	XmemErrorCallback(XmemError err, unsigned long ioe);

int 		XmemGetKey(char *name);

long 		*XmemGetSharedMemory(XmemTableId tid);

XmemError 	XmemReadTableFile(XmemTableId tid);
XmemError 	XmemWriteTableFile(XmemTableId tid);

XmemError 	XmemBlockCallbacks();
XmemError 	XmemUnBlockCallbacks();
int		XmemCallbacksBlocked();

#endif
