/* ==================================================================== */
/* VD80 Library                                                         */
/* This library exists in two forms as a ".so" shared object, and for   */
/* thoes who are unable to use a dynamic library there is a ".a" static */
/* version as well.                                                     */
/* Julian Lewis Wed 15th April 2009                                     */
/* ==================================================================== */

#ifndef VD80_LIB
#define VD80_LIB

#ifdef __cplusplus
extern "C"
{
#endif

/* ==================================================================== */
/* Some standard error codes                                            */

typedef enum {
   Vd80ErrSUCCESS,  /* All went OK, No error                       */
   Vd80ErrNOT_IMP,  /* Function is not implemented on this device  */
   Vd80ErrSTART,    /* Invalid Start value for this device         */
   Vd80ErrMODE,     /* Invalid Mode  value for this device         */
   Vd80ErrCLOCK,    /* Invalid Clock value for this device         */
   Vd80ErrOPEN,     /* Can't open a driver file handle, fatal      */
   Vd80ErrCONNECT,  /* Can't connect to that interrupt             */
   Vd80ErrWAIT,     /* No connections to wait for                  */
   Vd80ErrTIMEOUT,  /* Timeout in wait                             */
   Vd80ErrQFLAG,    /* Queue flag must be set 0 queueing is needed */
   Vd80ErrIO,       /* IO or BUS error                             */
   Vd80ErrNOT_ENAB, /* Module is not enabled                       */
   Vd80ErrSOURCE,   /* Not available for INTERNAL sources          */
   Vd80ErrVOR,      /* Value out of range                          */
   Vd80ErrDEVICE,   /* Bad device type                             */
   Vd80ErrADDRESS,  /* Bad address                                 */
   Vd80ErrMEMORY,   /* Can not allocate memory                     */
   Vd80ErrDLOPEN,   /* Shared library error, can't open object     */
   Vd80ErrDLSYM,    /* Shared library error, can't find symbol     */
   Vd80ErrDROPEN,   /* Can't open sampler device driver            */
   Vd80ErrHANDLE,   /* Invalid handle                              */
   Vd80ErrMODULE,   /* Invalid module number, not installed        */
   Vd80ErrCHANNEL,  /* Invalid channel number for this module      */

   Vd80ERRORS       /* Total errors */
 } Vd80Err;

#define Vd80ErrSTRING_SIZE 128

/* ==================================================================== */
/* Set the debug level. The values in the first byte control the debug  */
/* print level on the console. Much of this information is prionted by  */
/* the driver and can slow things down a lot. The second byte controls  */
/* hardware and software emulation. This is for testing purposes only.  */

typedef enum {
   Vd80DebugASSERTION   = 0x01,  /* All assertion violations (BUGS) printed */
   Vd80DebugTRACE       = 0x02,  /* Trace all driver calls */
   Vd80DebugWARNING     = 0x04,  /* Warnings such as bad call parameters */
   Vd80DebugMODULE      = 0x08,  /* Hardware module errors, bus error etc */
   Vd80DebugINFORMATION = 0x10,  /* Extra information */
   Vd80DebugEMULATION   = 0x100, /* Turn on driver emulation, no hardware access */
 } Vd80DebugFlag;

#define Vd80DebugLEVELS 6

typedef struct {
   int           ClientPid;  /* 0 = me */
   Vd80DebugFlag DebugFlag;  /* Debug flags */
 } Vd80Debug;

/* ==================================================================== */
/* We sometimes need a bit mask to specify modules/channels when an     */
/* operation or command is applied to many at the same time.            */
/* Obviously we can declare more modules if needed.                     */

typedef enum {
   Vd80ModNONE = 0x0000,
   Vd80Mod01   = 0x0001,
   Vd80Mod02   = 0x0002,
   Vd80Mod03   = 0x0004,
   Vd80Mod04   = 0x0008,
   Vd80Mod05   = 0x0010,
   Vd80Mod06   = 0x0020,
   Vd80Mod07   = 0x0040,
   Vd80Mod08   = 0x0080,
   Vd80Mod09   = 0x0100,
   Vd80Mod10   = 0x0200,
   Vd80Mod11   = 0x0400,
   Vd80Mod12   = 0x0800,
   Vd80Mod13   = 0x1000,
   Vd80Mod14   = 0x2000,
   Vd80Mod15   = 0x4000,
   Vd80Mod16   = 0x8000,
   Vd80ModALL  = 0xFFFF
 } Vd80Mod;

#define Vd80MODULES 16

typedef enum {
   Vd80ChnNONE = 0x0000,
   Vd80Chn01   = 0x0001,
   Vd80Chn02   = 0x0002,
   Vd80Chn03   = 0x0004,
   Vd80Chn04   = 0x0008,
   Vd80Chn05   = 0x0010,
   Vd80Chn06   = 0x0020,
   Vd80Chn07   = 0x0040,
   Vd80Chn08   = 0x0080,
   Vd80Chn09   = 0x0100,
   Vd80Chn10   = 0x0200,
   Vd80Chn11   = 0x0400,
   Vd80Chn12   = 0x0800,
   Vd80Chn13   = 0x1000,
   Vd80Chn14   = 0x2000,
   Vd80Chn15   = 0x4000,
   Vd80Chn16   = 0x8000,
   Vd80ChnALL  = 0xFFFF
 } Vd80Chn;

#define Vd80CHANNELS 16

/* ==================================================================== */
/* Data can be read back only when the state is IDLE                    */

typedef enum {
   Vd80StateNOT_SET     = 0,   /* Not known/initialized */
   Vd80StateIDLE        = 1,   /* Ready to do something */
   Vd80StatePRETRIGGER  = 2,   /* Waiting for trigger   */
   Vd80StatePOSTTRIGGER = 4,   /* Sampling, device busy */
   Vd80STATES           = 4
 } Vd80State;

/* ============================================================= */
/* Standard status definitions: A value of ZERO is the BAD state */

typedef enum {
   Vd80StatusDISABLED      = 0x001, /* Hardware is not enabled */
   Vd80StatusHARDWARE_FAIL = 0x002, /* Hardware has failed */
   Vd80StatusBUS_FAULT     = 0x004, /* Bus error(s) detected */
   Vd80StatusEMULATION     = 0x008, /* Hardware emulation ON */
   Vd80StatusNO_HARDWARE   = 0x010, /* Hardware has not been installed */
   Vd80StatusHARDWARE_DBUG = 0x100  /* Hardware debug mode */
 } Vd80Status;

/* ============================================= */
/* The compilation dates in UTC time for driver  */
/* and whatever from the hardware module.        */

#define Vd80VERSION_SIZE 64

typedef struct {
   unsigned int LibraryVersion;              /* DLL Library Compile date */
   unsigned int DriverVersion;               /* Drvr Compile date */
   char         ModVersion[Vd80VERSION_SIZE]; /* Module version string */
 } Vd80Version;

/* ============================================================= */
/* Inputs can be specified as EXTERNAL in which case the EDGE    */
/* and TERMINATION of the signal SOURCE must be provided. When   */
/* the SOURCE is INTERNAL the EDGE and TERMINATION specification */
/* is internally determined by the module defaults.              */

typedef enum {
   Vd80EdgeRISING,
   Vd80EdgeFALLING,
   Vd80EDGES
 } Vd80Edge;

typedef enum {
   Vd80TerminationNONE,
   Vd80Termination50OHM,
   Vd80TERMINATIONS
 } Vd80Termination;

typedef enum {
   Vd80SourceINTERNAL,
   Vd80SourceEXTERNAL,
   Vd80SOURCES
 } Vd80Source;

/* ============================================================= */
/* A signal INPUT                                                */

typedef struct {
   Vd80Edge        Edge;
   Vd80Termination Termination;
   Vd80Source      Source;
 } Vd80Input;

/* ==================================================================== */
/* Sample buffers are read back from a sampler and contain the acquired */
/* data for a channel. The buffer pointer should be cast according to   */
/* the buffer data type.                                                */

typedef enum {
   Vd80DatumSize08 = 1,  /* Cast buffer to char */
   Vd80DatumSize16 = 2,  /* Cast buffer to short */
   Vd80DatumSize32 = 4   /* Cast buffer to int */
 } Vd80DatumSize;

typedef struct {
   void *Addr;  /* Address of alloocated sample memory */
   int   BSze;  /* Buffer size in samples (See DSize)  */
   int   Post;  /* Requested number of post samples    */
   int   Tpos;  /* Actual position of trigger          */
   int   ASze;  /* Actual number of samples in buffer  */
   int   Ptsr;  /* Number of 100K ticks since start    */
 } Vd80Buffer;

/* ==================================================================== */
/* Interrupt source connection mask                                     */

typedef enum {
   Vd80QueueFlagON,  /* 0 => Client interrupt queueing ON */
   Vd80QueueFlagOFF  /* 1 => and OFF */
 } Vd80QueueFlag;

typedef enum {
   Vd80IntrMaskTRIGGER      = 0x1, /* Trigger interrupt */
   Vd80IntrMaskACQUISITION  = 0x2, /* Data ready interrupt */
   Vd80IntrMaskERR          = 0x4, /* Hardware error interrupt */
   Vd80INTR_MASKS           = 3
 } Vd80IntrMask;

typedef struct {
   Vd80IntrMask     Mask;      /* Single interrupt source bit */
   struct timespec Time;      /* Time of trigger */
   unsigned int    Module;
   unsigned int    Channel;
   unsigned int    QueueSize; /* Number of remaining entries on queue */
   unsigned int    Missed;    /* Queue overflow missed count */
 } Vd80Intr;

/* ==================================================================== */
/* Set the trigger level                                                */

typedef enum {
   Vd80AtrgOFF   = 0x0,
   Vd80AtrgABOVE = 0x03,
   Vd80AtrgBELOW = 0x04
 } Vd80Atrg;

typedef struct {
   Vd80Atrg AboveBelow;
   int      TrigLevel;
 } Vd80AnalogTrig;

/* ==================================================================== */
/* Trigger configuration, trigger delay and min pre trigger samples     */

typedef struct {
   int TrigDelay;   /* Time to wait after trig in sample intervals */
   int MinPreTrig;  /* Mininimum number of pretrig samples */
 } Vd80TrigConfig;

/* ==================================================================== */
/* Open and close driver handle                                         */

int vd80OpenHandle();
void vd80CloseHandle(int fd);

/* ==================================================================== */
/* Set/Get the users debug mask. Writing zero turns debug off.          */

Vd80Err vd80SetDebug(int fd, Vd80Debug *debug);
Vd80Err vd80GetDebug(int fd, Vd80Debug *debug);

/* ==================================================================== */
/* Reset is the equivalent to a VME bus reset. All settings and data on */
/* the module will be lost.                                             */

Vd80Err vd80ResetMod(int fd, int module);

/* ==================================================================== */
/* Calibrate requires that zero and Max volts are applied to an input   */
/* and the Adc value then read back. Once done the values obtained can  */
/* be used to calibrate the module.                                     */

Vd80Err vd80GetAdcValue(int fd, int module, int channel, int *adc);

/* ==================================================================== */
/* A channel has a state, the state must be IDLE to read back data.     */

Vd80Err vd80GetState(int fd, int module, int channel, Vd80State *state);

/* ==================================================================== */
/* Mod status indicates hardware failures, enable state etc.            */

Vd80Err vd80GetStatus(int fd, int module, Vd80Status *status);

/* ==================================================================== */
/* This is a fixed value for each device type.                          */

Vd80Err vd80GetDatumSize(int fd, Vd80DatumSize *datsze);

/* ==================================================================== */
/* Discover how many modules and channels are installed.                */

Vd80Err vd80GetModuleCount(int fd, int *count);
Vd80Err vd80GetChannelCount(int fd, int *count);

/* ==================================================================== */
/* This returns the hardware, driver and DLL library versions.          */

Vd80Err vd80GetVersion(int fd, int module, Vd80Version *version);

/* ==================================================================== */

Vd80Err vd80SetTrigger(int fd, Vd80Mod modules, Vd80Chn channels, Vd80Input *trigger);
Vd80Err vd80GetTrigger(int fd, int module, int channel, Vd80Input *trigger);

/* ==================================================================== */

Vd80Err vd80SetClock(int fd, Vd80Mod modules, Vd80Chn channels, Vd80Input *clock);
Vd80Err vd80GetClock(int fd, int module, int channel, Vd80Input *clock);

/* ==================================================================== */
/* Divide the clock frequency by an integer.                            */
/* One is added to the divisor, so 0 means divide by 1.                 */

Vd80Err vd80SetClockDivide(int fd, Vd80Mod modules, Vd80Chn channels, unsigned int divisor);
Vd80Err vd80GetClockDivide(int fd, int module, int channel, unsigned int *divisor);

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
/* module/channel masks.                                                */

Vd80Err vd80SetBufferSize(int fd, Vd80Mod modules, Vd80Chn channels, int  size, int  post);
Vd80Err vd80GetBufferSize(int fd, unsigned int mod, int *post);

/* ==================================================================== */
/* Basic commands start, trigger and stop. Ane error may be returned if */
/* you try to control an external source.                               */

Vd80Err vd80StrtAcquisition(int fd, Vd80Mod modules, Vd80Chn channels); /* Zero all channels */
Vd80Err vd80TrigAcquisition(int fd, Vd80Mod modules, Vd80Chn channels);
Vd80Err vd80StopAcquisition(int fd, Vd80Mod modules, Vd80Chn channels);
Vd80Err vd80ReadAcquisition(int fd, Vd80Mod modules, Vd80Chn channels);

/* ==================================================================== */
/* Connecting to zero disconnects from all previous connections         */

Vd80Err vd80Connect(int fd, Vd80Mod modules, Vd80Chn channels, Vd80IntrMask imask);
Vd80Err vd80SetTimeout(int fd, int  tmout);
Vd80Err vd80GetTimeout(int fd, int *tmout);
Vd80Err vd80SetQueueFlag(int fd, Vd80QueueFlag flag);
Vd80Err vd80GetQueueFlag(int fd, Vd80QueueFlag *flag);
Vd80Err vd80Wait(int fd, Vd80Intr *intr);
Vd80Err vd80SimulateInterrupt(int fd, Vd80Intr *intr);

/* ==================================================================== */
/* Get information about a buffer so that the data, if any, can be used */

Vd80Err vd80GetBuffer(int fd, int module, int channel, Vd80Buffer *buffer);

/* ==================================================================== */
/* Set/Get the trigger analogue levels                                  */

Vd80Err vd80SetTriggerLevels(int fd, unsigned int mods, unsigned int chns, Vd80AnalogTrig *atrg);
Vd80Err vd80GetTriggerLevels(int fd, int          mod,  int          chn,  Vd80AnalogTrig *atrg);

/* ==================================================================== */
/* Convert an error to a string                                         */

char *vd80ErrToStr(int fd, Vd80Err error);

/* ==================================================================== */
/* Get set trigger configuration params, delay and min pretrig samples  */

Vd80Err vd80SetTriggerConfig(int fd, unsigned int mods, unsigned int chns, Vd80TrigConfig *ctrg);
Vd80Err vd80GetTriggerConfig(int fd, int mod, int chn, Vd80TrigConfig *ctrg);

/* ================ */

#ifdef __cplusplus
}
#endif

#endif
