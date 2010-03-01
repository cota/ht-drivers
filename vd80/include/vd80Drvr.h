/* ======================================= */
/* VD80 Sampler public user interface.     */
/* Tue 10th Feb 2009 BE/CO/HT Julian Lewis */
/* ======================================= */

#ifndef VD80DRVR
#define VD80DRVR

#include <skeluser.h>

/* ============================================= */

/* WO is a vlue that you must supply */
/* RO is a value returned to you by the driver */
/* RW is WO followed by RO */

/* The driver will try to supply the requested data, in any case */
/* it will adjust the values of TrigPosition and Samples as best it can */
/* You should have set up the number of post-samples SET_POSTSAMPLES */
/* prior to calling READ_SAMPLE. */

typedef struct {
   int Channel;        /* WO Channel to read from */
   int BufSizeSamples; /* WO Target buffer size (In samples) */
   int TrigPosition;   /* RW Position of trigger in buf */
   int Samples;        /* RO On return: The number of samples read */
   int PreTrigStat;    /* RO Pre-Trigger Status Register */
   short *SampleBuf;   /* RO Buffer where samples will be stored */
 } Vd80SampleBuf;

typedef enum {
   Vd80DrvrClockINTERNAL,
   Vd80DrvrClockEXTERNAL,
   Vd80DrvrCLOCKS
 } Vd80DrvrClock;

typedef enum {
   Vd80DrvrDivideModeDIVIDE,
   Vd80DrvrDivideModeSUBSAMPLE,
   Vd80DrvrDivideMODES
 } Vd80DrvrDivideMode;

typedef enum {
   Vd80DrvrEdgeRISING,
   Vd80DrvrEdgeFALLING,
   Vd80DrvrEDGES
 } Vd80DrvrEdge;

typedef enum {
   Vd80DrvrTerminationNONE,
   Vd80DrvrTermination50OHM,
   Vd80DrvrTERMINATIONS
 } Vd80DrvrTermination;

typedef enum {
   Vd80DrvrTriggerINTERNAL,
   Vd80DrvrTriggerEXTERNAL,
   Vd80DrvrTRIGGERS
 } Vd80DrvrTrigger;

typedef enum {
   Vd80DrvrStateIDLE        = 0x1,
   Vd80DrvrStatePRETRIGGER  = 0x2,
   Vd80DrvrStatePOSTTRIGGER = 0x4,
   Vd80DrvrSTATES           = 3
 } Vd80DrvrState;

typedef enum {
   Vd80DrvrCommandSTOP     = 0xE,
   Vd80DrvrCommandSTART    = 0xB,
   Vd80DrvrCommandSUBSTOP  = 0x8,
   Vd80DrvrCommandSUBSTART = 0x9,
   Vd80DrvrCommandTRIGGER  = 0x2,
   Vd80DrvrCommandREAD     = 0x4,
   Vd80DrvrCommandSINGLE   = 0x1,
   Vd80DrvrCommandCLEAR    = 0xC,
   Vd80DrvrCOMMANDS        = 8
 } Vd80DrvrCommand;

typedef enum {
   Vd80DrvrATrigDISABLED   = 0x0,
   Vd80DrvrATrigBELOW      = 0x3,
   Vd80DrvrATrigABOVE      = 0x4,
 } Vd80DrvrATrig;


typedef struct {
   int           Channel;     /* Channel to define the trigger on */
   Vd80DrvrATrig Control;     /* Control options for trigger */
   short         Level;       /* Trigger level */
 } Vd80DrvrAnalogTrig;

typedef struct {
   int           TrigDelay;   /* Time to wait after trig in sample intervals */
   int           MinPreTrig;  /* Mininimum number of pretrig samples */
 } Vd80DrvrTrigConfig;

/* ============================================= */
/* Define the VD80 specific IOCTL function codes */

typedef enum {

   Vd80NumFIRST,

   Vd80NumSET_CLOCK,              /* Set to a Vd80 clock */
   Vd80NumSET_CLOCK_DIVIDE_MODE,  /* Sub sample or clock */
   Vd80NumSET_CLOCK_DIVISOR,      /* A 16 bit integer so the lowest frequency is */
				  /*    200000/65535 (16bits 0xFFFF) ~3.05Hz     */
   Vd80NumSET_CLOCK_EDGE,         /* Set to a Vd80 edge */
   Vd80NumSET_CLOCK_TERMINATION,  /* Set to a Vd80 termination */

   Vd80NumGET_CLOCK,
   Vd80NumGET_CLOCK_DIVIDE_MODE,
   Vd80NumGET_CLOCK_DIVISOR,
   Vd80NumGET_CLOCK_EDGE,
   Vd80NumGET_CLOCK_TERMINATION,

   Vd80NumSET_TRIGGER,
   Vd80NumSET_TRIGGER_EDGE,
   Vd80NumSET_TRIGGER_TERMINATION,

   Vd80NumGET_TRIGGER,
   Vd80NumGET_TRIGGER_EDGE,
   Vd80NumGET_TRIGGER_TERMINATION,

   Vd80NumSET_ANALOGUE_TRIGGER,   /* Vd80DrvrAnalogTrig */
   Vd80NumGET_ANALOGUE_TRIGGER,   /* Vd80DrvrAnalogTrig */

   Vd80NumGET_STATE,              /* Returns a Vd80 state */
   Vd80NumSET_COMMAND,            /* Set to a Vd80 command */

   Vd80NumREAD_ADC,
   Vd80NumREAD_SAMPLE,

   Vd80NumGET_POSTSAMPLES, /* Get the number of post samples you set */
   Vd80NumSET_POSTSAMPLES, /* Set the number of post samples you want */

   Vd80NumGET_TRIGGER_CONFIG,     /* Get Trig delay and min pre trig samples */
   Vd80NumSET_TRIGGER_CONFIG,     /* Set Trig delay and min pre trig samples */

   Vd80NumLAST                                    /* The last IOCTL */

 } Vd80Num;

#define Vd80DrvrSPECIFIC_IOCTL_CALLS (Vd80NumLAST - Vd80NumFIRST + 1)

#endif
