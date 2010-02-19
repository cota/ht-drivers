/* ==================================================================== */
/* Implement general stub library for generic devices                   */
/* Julian Lewis Wed 15th April 2009                                     */
/* ==================================================================== */

#ifndef STUB_LIB
#define STUB_LIB

/* ==================================================================== */
/* Some standard error codes                                            */

typedef enum {
   SLbErrSUCCESS,  /* All went OK, No error                       */
   SLbErrNOT_IMP,  /* Function is not implemented on this device  */
   SLbErrSTART,    /* Invalid Start value for this device         */
   SLbErrMODE,     /* Invalid Mode  value for this device         */
   SLbErrCLOCK,    /* Invalid Clock value for this device         */
   SLbErrOPEN,     /* Can't open a driver file handle, fatal      */
   SLbErrCONNECT,  /* Can't connect to that interrupt             */
   SLbErrWAIT,     /* No connections to wait for                  */
   SLbErrTIMEOUT,  /* Timeout in wait                             */
   SLbErrQFLAG,    /* Queue flag must be set 0 queueing is needed */
   SLbErrIO,       /* IO or BUS error                             */
   SLbErrNOT_ENAB, /* Module is not enabled                       */
   SLbErrSOURCE,   /* Not available for INTERNAL sources          */
   SLbErrVOR,      /* Value out of range                          */
   SLbErrDEVICE,   /* Bad device type                             */
   SLbErrADDRESS,  /* Bad address                                 */
   SLbErrMEMORY,   /* Can not allocate memory                     */
   SLbErrDLOPEN,   /* Shared library error, can't open object     */
   SLbErrDLSYM,    /* Shared library error, can't find symbol     */
   SLbErrDROPEN,   /* Can't open sampler device driver            */
   SLbErrHANDLE,   /* Invalid handle                              */
   SLbErrMODULE,   /* Invalid module number, not installed        */
   SLbErrCHANNEL,  /* Invalid channel number for this module      */

   SLbERRORS       /* Total errors */
 } SLbErr;

#define SLbErrSTRING_SIZE 80

/* ==================================================================== */
/* Set the debug level. The values in the first byte control the debug  */
/* print level on the console. Much of this information is prionted by  */
/* the driver and can slow things down a lot. The second byte controls  */
/* hardware and software emulation. This is for testing purposes only.  */

typedef enum {
   SLbDebugASSERTION   = 0x01,  /* All assertion violations (BUGS) printed */
   SLbDebugTRACE       = 0x02,  /* Trace all driver calls */
   SLbDebugWARNING     = 0x04,  /* Warnings such as bad call parameters */
   SLbDebugMODULE      = 0x08,  /* Hardware module errors, bus error etc */
   SLbDebugINFORMATION = 0x10,  /* Extra information */
   SLbDebugEMULATION   = 0x100, /* Turn on driver emulation, no hardware access */
 } SLbDebugFlag;

#define SLbDebugLEVELS 6

typedef struct {
   int          ClientPid;  /* 0 = me */
   SLbDebugFlag DebugFlag;  /* Debug flags */
 } SLbDebug;

/* ==================================================================== */
/* We sometimes need a bit mask to specify modules/channels when an     */
/* operation or command is applied to many at the same time.            */
/* Obviously we can declare more modules if needed.                     */

typedef enum {
   SLbModNONE = 0x0000,
   SLbMod01   = 0x0001,
   SLbMod02   = 0x0002,
   SLbMod03   = 0x0004,
   SLbMod04   = 0x0008,
   SLbMod05   = 0x0010,
   SLbMod06   = 0x0020,
   SLbMod07   = 0x0040,
   SLbMod08   = 0x0080,
   SLbMod09   = 0x0100,
   SLbMod10   = 0x0200,
   SLbMod11   = 0x0400,
   SLbMod12   = 0x0800,
   SLbMod13   = 0x1000,
   SLbMod14   = 0x2000,
   SLbMod15   = 0x4000,
   SLbMod16   = 0x8000,
   SLbModALL  = 0xFFFF
 } SLbMod;

#define SLbMODULES 16

typedef enum {
   SLbChnNONE = 0x0000,
   SLbChn01   = 0x0001,
   SLbChn02   = 0x0002,
   SLbChn03   = 0x0004,
   SLbChn04   = 0x0008,
   SLbChn05   = 0x0010,
   SLbChn06   = 0x0020,
   SLbChn07   = 0x0040,
   SLbChn08   = 0x0080,
   SLbChn09   = 0x0100,
   SLbChn10   = 0x0200,
   SLbChn11   = 0x0400,
   SLbChn12   = 0x0800,
   SLbChn13   = 0x1000,
   SLbChn14   = 0x2000,
   SLbChn15   = 0x4000,
   SLbChn16   = 0x8000,
   SLbChnALL  = 0xFFFF
 } SLbChn;

#define SLbCHANNELS 16

/* ==================================================================== */
/* Every thread that wants to use the library should open at least one  */
/* handle. Handles have nothing to do with sampler modules, one handle  */
/* can access all installed sampler hardware modules. The library uses  */
/* the handle to keep data for each thread. Some functions are provided */
/* to get data from a handle.                                           */

typedef void *SLbHandle;

/* ==================================================================== */
/* Data can be read back only when the state is IDLE                    */

typedef enum {
   SLbStateNOT_SET     = 0,   /* Not known/initialized */
   SLbStateIDLE        = 1,   /* Ready to do something */
   SLbStatePRETRIGGER  = 2,   /* Waiting for trigger   */
   SLbStatePOSTTRIGGER = 4,   /* Sampling, device busy */
   SLbSTATES           = 4
 } SLbState;

/* ============================================================= */
/* Standard status definitions: A value of ZERO is the BAD state */

typedef enum {
   SLbStatusDISABLED      = 0x001, /* Hardware is not enabled */
   SLbStatusHARDWARE_FAIL = 0x002, /* Hardware has failed */
   SLbStatusBUS_FAULT     = 0x004, /* Bus error(s) detected */
   SLbStatusEMULATION     = 0x008, /* Hardware emulation ON */
   SLbStatusNO_HARDWARE   = 0x010, /* Hardware has not been installed */
   SLbStatusHARDWARE_DBUG = 0x100  /* Hardware debug mode */
 } SLbStatus;

/* ============================================= */
/* The compilation dates in UTC time for driver  */
/* and whatever from the hardware module.        */

#define SLbVERSION_SIZE 64

typedef struct {
   unsigned int LibraryVersion;              /* DLL Library Compile date */
   unsigned int DriverVersion;               /* Drvr Compile date */
   char         ModVersion[SLbVERSION_SIZE]; /* Module version string */
 } SLbVersion;

/* ============================================================= */
/* Inputs can be specified as EXTERNAL in which case the EDGE    */
/* and TERMINATION of the signal SOURCE must be provided. When   */
/* the SOURCE is INTERNAL the EDGE and TERMINATION specification */
/* is internally determined by the module defaults.              */

typedef enum {
   SLbEdgeRISING,
   SLbEdgeFALLING,
   SLbEDGES
 } SLbEdge;

typedef enum {
   SLbTerminationNONE,
   SLbTermination50OHM,
   SLbTERMINATIONS
 } SLbTermination;

typedef enum {
   SLbSourceINTERNAL,
   SLbSourceEXTERNAL,
   SLbSOURCES
 } SLbSource;

/* ============================================================= */
/* A signal INPUT                                                */

typedef struct {
   SLbEdge        Edge;
   SLbTermination Termination;
   SLbSource      Source;
 } SLbInput;

/* ==================================================================== */
/* Sample buffers are read back from a sampler and contain the acquired */
/* data for a channel. The buffer pointer should be cast according to   */
/* the buffer data type.                                                */

typedef enum {
   SLbDatumSize08 = 1,  /* Cast buffer to char */
   SLbDatumSize16 = 2,  /* Cast buffer to short */
   SLbDatumSize32 = 4   /* Cast buffer to int */
 } SLbDatumSize;

typedef struct {
   void *Addr;  /* Address of alloocated sample memory */
   int   BSze;  /* Buffer size in samples (See DSize)  */
   int   Post;  /* Requested number of post samples    */
   int   Tpos;  /* Actual position of trigger          */
   int   ASze;  /* Actual number of samples in buffer  */
   int   Ptsr;  /* Number of 100K ticks since start    */
 } SLbBuffer;

/* ==================================================================== */
/* Interrupt source connection mask                                     */

typedef enum {
   SLbQueueFlagON,  /* 0 => Client interrupt queueing ON */
   SLbQueueFlagOFF  /* 1 => and OFF */
 } SLbQueueFlag;

typedef enum {
   SLbIntrMaskTRIGGER      = 0x1, /* Trigger interrupt */
   SLbIntrMaskACQUISITION  = 0x2, /* Data ready interrupt */
   SLbIntrMaskERR          = 0x4, /* Hardware error interrupt */
   SLbINTR_MASKS           = 3
 } SLbIntrMask;

typedef struct {
   SLbIntrMask     Mask;      /* Single interrupt source bit */
   struct timespec Time;      /* Time of trigger */
   unsigned int    Module;
   unsigned int    Channel;
   unsigned int    QueueSize; /* Number of remaining entries on queue */
   unsigned int    Missed;    /* Queue overflow missed count */
 } SLbIntr;

/* ==================================================================== */
/* Analogue trigger control                                             */

typedef enum {
   SLbAtrgOFF   = 0x0,
   SLbAtrgABOVE = 0x03,
   SLbAtrgBELOW = 0x04
 } SLbAtrg;

typedef struct {
   SLbAtrg AboveBelow;
   int     TrigLevel;
 } SLbAnalogTrig;

/* ============================================================================== */
/* Trigger configuration, trigger delay and min pre trigger samples               */

typedef struct {
   int TrigDelay;   /* Time to wait after trig in sample intervals */
   int MinPreTrig;  /* Mininimum number of pretrig samples */
 } SLbTrigConfig;

/* ============================================================================== */
/*                                                                                */
/* API for generic sampler. This API may be extended for a particular             */
/* device in which case these extra calls will conform to this ...                */
/*                                                                                */
/*    SLbErr <Dev>LibExtension(SLbHandle handle, .... );                          */
/* eg SLbErr SisLibSetVertical(SLbHandle handle, double fullscale ...);           */
/*                                                                                */
/* ============================================================================== */

/* ==================================================================== */
/* Convert an SLbError to a string suitable for printing.               */
/* This routine can not return a NULL pointer. If the error is out of   */
/* range it returns a string saying just that. The pointer returned is  */
/* thread safe, the allocated memory is kept in the callers address     */
/* space. If the handle pointer is NULL, the error text is kept in      */
/* static memory, and is not thread safe => copy it yourself.           */

char *SLbErrToStr(SLbHandle handle, SLbErr error);

/* ==================================================================== */
/* Each thread in an application should get a handle and use it in      */
/* all subsequent calls. A limited number of concurrently open handles  */
/* is available for a given driver. The client thread should close its  */
/* handle when it no longer needs it. The library will then free memory */
/* and close the file asociated file descriptor. The handle contains a  */
/* file descriptor that can be used from within a Linux select call.    */
/*                                                                      */
/* Example:                                                             */
/*                                                                      */
/*    SlbHandle my_handle;                                              */
/*    SlbErr    my_error;                                               */
/*                                                                      */
/*    my_error = SLbOpenHandle(&my_handle,"VD80","1.0");                */
/*    if (my_error != SLbErrSUCCESS) {                                  */
/*       stprintf(stderr,"Fatal: %s\n",SLbErrTopStr(NULL, my_error));   */
/*       exit((int) my_error);                                          */
/*    }                                                                 */
/*                                                                      */

SLbErr SLbOpenHandle(SLbHandle *handle, char *device, char *version);
SLbErr SLbCloseHandle(SLbHandle handle);

/* ==================================================================== */
/* Get the Driver file descriptor to be used within a select call.      */

SLbErr SLbGetFileDescriptor(SLbHandle handle, int *fd);

/* ==================================================================== */
/* Get the sampler device name string.                                  */

SLbErr SLbGetDeviceName(SLbHandle handle, char **dname);

/* ==================================================================== */
/* Set/Get the users debug mask. Writing zero turns debug off.          */

SLbErr SLbSetDebug(SLbHandle handle, SLbDebug *debug);
SLbErr SLbGetDebug(SLbHandle handle, SLbDebug *debug);

/* ==================================================================== */
/* Get the Dll Version string.                                          */

SLbErr SLbGetDllVersion(SLbHandle handle, char **dver);

/* ==================================================================== */
/* The address of any library function can be got by name. This can be  */
/* useful if you want to call a device library function directly, for   */
/* example a specific device function.                                  */
/*                                                                      */
/* Example:                                                             */
/*                                                                      */
/* int (*my_funct)(int p);                                              */
/* SLbErr my_error;                                                     */
/*                                                                      */
/*    my_error = SlbGetSymbol(my_handle,"Vd80LibSpecial",&my_funct);    */
/*    if (my_error != SLbErrSUCCESS) {                                  */
/*       sprintf(stderr,"Fatal: %s\n",SLbErrTopStr(NULL, my_error));    */
/*       exit((int) my_error);                                          */
/*    }                                                                 */
/*                                                                      */
/*    my_error = my_funct(1); // Direct call to Vd80LibSpecial(1)       */

SLbErr SLbGetSymbol(SLbHandle handle, char *name, void **func);

/* ==================================================================== */
/* Reset is the equivalent to a VME bus reset. All settings and data on */
/* the module will be lost.                                             */

SLbErr SLbResetMod(SLbHandle handle, int module);

/* ==================================================================== */
/* Calibrate requires that zero and Max volts are applied to an input   */
/* and the Adc value then read back. Once done the values obtained can  */
/* be used to calibrate the module.                                     */

SLbErr SLbGetAdcValue(SLbHandle handle, int module, int channel, int *adc);
SLbErr SLbCalibrateChn(SLbHandle handle, int module, int channel, int low, int high);

/* ==================================================================== */
/* A channel has a state, the state must be IDLE to read back data.     */

SLbErr SLbGetState(SLbHandle handle, int module, int channel, SLbState *state);

/* ==================================================================== */
/* Mod status indicates hardware failures, enable state etc.            */

SLbErr SLbGetStatus(SLbHandle handle, int module, SLbStatus *status);

/* ==================================================================== */
/* This is a fixed value for each device type.                          */

SLbErr SLbGetDatumSize(SLbHandle handle, SLbDatumSize *datsze);

/* ==================================================================== */
/* Discover how many modules and channels are installed.                */

SLbErr SLbGetModCount(SLbHandle handle, int *count);
SLbErr SLbGetChnCount(SLbHandle handle, int *count);

/* ==================================================================== */
/* This returns the hardware, driver and DLL library versions.          */

SLbErr SLbGetVersion(SLbHandle handle, int module, SLbVersion *version);

SLbErr SLbGetStubVersion(unsigned int *version);

/* ==================================================================== */

SLbErr SLbSetTrigger(SLbHandle handle, SLbMod modules, SLbChn channels, SLbInput *trigger);
SLbErr SLbGetTrigger(SLbHandle handle, int module, int channel, SLbInput *trigger);

/* ==================================================================== */

SLbErr SLbSetTriggerLevels(SLbHandle handle, unsigned int modules, unsigned int channels, SLbAnalogTrig *atrg);
SLbErr SLbGetTriggerLevels(SLbHandle handle, int module, int channel, SLbAnalogTrig *atrg);

/* ==================================================================== */

SLbErr SLbSetClock(SLbHandle handle, SLbMod modules, SLbChn channels, SLbInput *clock);
SLbErr SLbGetClock(SLbHandle handle, int module, int channel, SLbInput *clock);

/* ==================================================================== */
/* Some hardware has an external stop input.                            */

SLbErr SLbSetStop(SLbHandle handle, SLbMod modules, SLbChn channels, SLbInput *stop);
SLbErr SLbGetStop(SLbHandle handle, int module, int channel, SLbInput *stop);

/* ==================================================================== */
/* Divide the clock frequency by an integer.                            */
/* One is added to the divisor, so 0 means divide by 1.                 */

SLbErr SLbSetClockDivide(SLbHandle handle, SLbMod modules, SLbChn channels, unsigned int divisor);
SLbErr SLbGetClockDivide(SLbHandle handle, int module, int channel, unsigned int *divisor);

/* ==================================================================== */
/* Buffers contain samples that occur before and after the trigger.     */
/* The buffer is allocated by the library in an optimum way for DMA.    */
/* Any byte ordering operations are performed by the library.           */
/* The user specifies the number of post trigger samples and the size   */
/* of the buffer. In some hardware such as the VD80 the DMA transfers   */
/* are carried out in chunks. Thus the exact triigger position within   */
/* the buffer is adjusted to the nearset size/post boundary. Hence the  */
/* actual number of pre/post trigger samples may vary.                  */
/* One buffer is allocated for each module/channel specified in the     */
/* module/channel masks. The returned size, and post values will be     */
/* rounded up to the nearest chunk.                                     */

SLbErr SLbSetBufferSize(SLbHandle handle, SLbMod modules, SLbChn channels, int  *size, int  *post);
SLbErr SLbGetBufferSize(SLbHandle handle, int module, int channel, int *size, int *post);

/* ==================================================================== */
/* Basic commands start, trigger and stop. Ane error may be returned if */
/* you try to control an external source.                               */

SLbErr SLbStrtAcquisition(SLbHandle handle, SLbMod modules, SLbChn channels); /* Zero all channels */
SLbErr SLbTrigAcquisition(SLbHandle handle, SLbMod modules, SLbChn channels);
SLbErr SLbStopAcquisition(SLbHandle handle, SLbMod modules, SLbChn channels);
SLbErr SLbReadAcquisition(SLbHandle handle, SLbMod modules, SLbChn channels);

/* ==================================================================== */
/* Connecting to zero disconnects from all previous connections         */

SLbErr SLbConnect(SLbHandle handle, SLbMod modules, SLbChn channels, SLbIntrMask imask);
SLbErr SLbSetTimeout(SLbHandle handle, int  tmout);
SLbErr SLbGetTimeout(SLbHandle handle, int *tmout);
SLbErr SLbSetQueueFlag(SLbHandle handle, SLbQueueFlag flag);
SLbErr SLbGetQueueFlag(SLbHandle handle, SLbQueueFlag *flag);
SLbErr SLbWait(SLbHandle handle, SLbIntr *intr);
SLbErr SLbSimulateInterrupt(SLbHandle handle, SLbIntr *intr);

/* ==================================================================== */
/* Get information about a buffer so that the data, if any, can be used */

SLbErr SLbGetBuffer(SLbHandle handle, int module, int channel, SLbBuffer *buffer);

/* ==================================================================== */
/* Get set trigger configuration params, delay and min pretrig samples  */

SLbErr SLbSetTriggerConfig(SLbHandle handle, unsigned int mods, unsigned int chns, SLbTrigConfig *ctrg);
SLbErr SLbGetTriggerConfig(SLbHandle handle, int mod, int chn, SLbTrigConfig *ctrg);

#endif
