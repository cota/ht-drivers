/**
 * @file libxmem.h
 *
 * @brief XMEM Reflective Memory Library
 *
 * @author Julian Lewis
 *
 * @date Created on 09/02/2005
 *
 * Xmem Library
 *
 * Description
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
 * Warning
 *
 * The library (and subsequent driver) assume that the size of a long
 * in both user and kernel space is 4 bytes; this is only true for 32-bit
 * machines. Therefore @b 64-bit @b platforms are simply @b _not_ @b supported.
 * This was a design mistake from the early beginning and has not been fixed
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
 */





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
char *XmemGetFile(char *name);




/**
 * XmemErrorToString - Convert an error into a string.
 *
 * @param err: XmemError to be converted
 *
 * @return static (global) string
 */
char *XmemErrorToString(XmemError err);




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
XmemError XmemInitialize(XmemDevice device);




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
int XmemCheckForWarmStart(void);





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
XmemNodeId XmemGetAllNodeIds();





/**
 * XmemGetNodeName - Get the name of a node from its node id
 *
 * @param node: node id
 *
 * This is usually the name of the host on which the node is implemented.
 *
 * @return node name on success; NULL otherwise.
 */
char *XmemGetNodeName(XmemNodeId node);




/**
 * XmemGetNodeId - Get the id of a node from its name
 *
 * @param name: node's name
 *
 * @return node's id on success; 0 otherwise
 */
XmemNodeId XmemGetNodeId(XmemName name);



/**
 * XmemWhoAmI - Get the node ID of this node
 *
 * @param : none
 *
 * @return node ID of this node
 */
XmemNodeId XmemWhoAmI(void);



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
XmemTableId XmemGetAllTableIds();




/**
 * XmemGetTableName - Get the name of a table from its table id
 *
 * @param table: table id
 *
 * Tables are defined in a configuration file
 *
 * @return table's name on success; NULL otherwise
 */
char *XmemGetTableName(XmemTableId table);




/**
 * XmemGetTableId - get the id of a table from its name
 *
 * @param name: table's name
 *
 * @return table id on success; 0 otherwise
 */
XmemTableId XmemGetTableId(XmemName name);




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
				 XmemNodeId *nodes, unsigned long *user);



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
				     XmemEventMask mask);




/**
 * XmemGetRegisteredEvents - Get currently registered callback's mask
 *
 * @param : none
 *
 * @return mask of currently registered callback
 */
XmemEventMask XmemGetRegisteredEvents(void);





/**
 * XmemWait - Wait for an Xmem Event with timeout
 *
 * @param timeout: in intervals of 10ms; 0 means forever.
 *
 * An incoming event will call any registered callback
 *
 * @return Wait handler on success; 0 otherwise
 */
XmemEventMask XmemWait(int timeout);




/**
 * XmemPoll - poll for an Xmem Event
 *
 * @param : none
 *
 * An incoming event will call any registered callback for that event
 *
 * @return Poll handled on success; 0 otherwise
 */
XmemEventMask XmemPoll(void);




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
			      unsigned long offset, unsigned long upflag);




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
			      unsigned long offset);



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
XmemError XmemSendMessage(XmemNodeId nodes, XmemMessage *mess);




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
XmemTableId XmemCheckTables(void);




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
XmemError XmemErrorCallback(XmemError err, unsigned long ioe);




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
int XmemGetKey(char *name);




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
long *XmemGetSharedMemory(XmemTableId tid);




/**
 * XmemReadTableFile - read tables from files into shared memory segments
 *
 * @param tid: table id
 *
 * @return Appropriate error code (XmemError)
 */
XmemError XmemReadTableFile(XmemTableId tid);




/**
 * XmemWriteTableFile - Write tables from shared memory segments to files
 *
 * @param tid: table id
 *
 * @return Appropriate error code (XmemError)
 */
XmemError XmemWriteTableFile(XmemTableId tid);




/**
 * XmemBlockCallbacks - Block all client callbacks while the caller is running
 *
 * @param : none
 *
 * @return Appropriate error code (XmemError)
 */
XmemError XmemBlockCallbacks();




/**
 * XmemUnBlockCallbacks - Unblock client callbacks while the caller is running
 *
 * @param : none
 *
 * @return Appropriate error code (XmemError)
 */
XmemError XmemUnBlockCallbacks();




/**
 * XmemCallbacksBlocked - Checks that callbacks are blocked
 *
 * @param : none
 *
 * @return 1 if blocked; 0 otherwise
 */
int XmemCallbacksBlocked();

#endif
