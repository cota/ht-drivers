/* ********************************************************************************************* */
/*                                                                                               */
/* CTR (Controls Timing Receiver) Public include file containing driver structures, that         */
/* describe the CTR hardware layout.                                                             */
/*                                                                                               */
/* Julian Lewis 26th Aug 2003                                                                    */
/*                                                                                               */
/* ********************************************************************************************* */

#ifndef CTR_HARDWARE
#define CTR_HARDWARE

/* There are four possible hardware formats which influence the hardware resources */
/* Define one of these symbols in the make file accordingly.                       */

/* #define CTR_PMC    Must also define CTR_PCI PMC is a special case of PCI */
/* #define CTR_PCI    PCI module, could be PCI only or PMC, see above */
/* #define CTR_G64 */
/* #define CTR_VME */

/* Under the gcc we use on LynxOs either __LITTLE / BIG_ENDIAN__ is defined. */
/* Under Linux on the Intel platforms __i386__ is defined */

#ifdef __i386__
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif
#endif

/* ======================================================================================= */
/* Ctr basic event processing behaviour                                                    */
/* ======================================================================================= */

/* Each time an event arrives it is stored in the event history; except if its a telegram  */
/* or millisecond event. Telegram events are stored in one of the incomming telegram       */
/* buffers depending on the details of the header. Next the trigger table is searched      */
/* and if a frame match is found, any telegram conditions are next checked. If the         */
/* trigger fires, the appropriate counter configuration is overwritten if it is not locked.*/
/* If the counter history is not locked, the Frame, Trigger Index and Trigger Time fields  */
/* are set up, and the StartTime and OnZeroTime are cleared. When the counter is started   */
/* the StartTime is filled in, and when the counter reaches zero, the OnZeroTime is filled */
/* in and the LockHistory field in the counter control block is incremented, preventing it */
/* from being overwritten. Later the driver ISR may clear the LockHistory field once the   */
/* counter history and trigger information has been obtained.                              */

/* ========================================================== */
/* Counter definitions                                        */
/* ========================================================== */

/* Ways of addressing the Ctr counters and output channels    */
/* Counter zero is not a real counter but is used to denote a */
/* direct action resulting from a trigger. Eg a bus interrupt */
/* triggered from an incomming event frame. The trigger can   */
/* still produce bus interupts and hence looks like a counter */
/* in the counter history and configuration entries.          */

typedef enum {
   CtrDrvrCounter0,    /* 0: Counter zero the direct trigger */
   CtrDrvrCounter1,    /* 1: First counter is counter one */
   CtrDrvrCounter2,    /* 2: Second counter */
   CtrDrvrCounter3,    /* 3: Third counter */
   CtrDrvrCounter4,    /* 4: Fourth counter */
   CtrDrvrCounter5,    /* 5: Fifth counter */
   CtrDrvrCounter6,    /* 6: Sixth counter */
   CtrDrvrCounter7,    /* 7: Seventh counter */
   CtrDrvrCounter8,    /* 8: Eighth is last counter CtrDrvrCOUNTERS */
   CtrDrvrCOUNTERS     /* Number of counters = Counter 0 and Real counters */
 } CtrDrvrCounter;

/* Define a bit for each counter to be used in bit mask operations */
/* This is just (1 << counter) */
/* Then we decided to add straight through bits for 40MHz, X1 & X2 Clocks */

typedef enum {
   CtrDrvrCounterMask0  = 0x01,  /* Bit 0: Counter zero is a direct trigger bit */
   CtrDrvrCounterMask1  = 0x02,  /* Bit 1: Counter one bit */
   CtrDrvrCounterMask2  = 0x04,  /* Bit 2: Counter two bit */
   CtrDrvrCounterMask3  = 0x08,  /* Bit 3: Counter three bit */
   CtrDrvrCounterMask4  = 0x10,  /* Bit 4: Counter four bit */
   CtrDrvrCounterMask5  = 0x20,  /* Bit 5: Counter five bit */
   CtrDrvrCounterMask6  = 0x40,  /* Bit 6: Counter six bit */
   CtrDrvrCounterMask7  = 0x80,  /* Bit 7: Counter seven bit */
   CtrDrvrCounterMask8  = 0x100, /* Bit 8: Counter eight bit */

   /* These next signals can be routed directly to a counter output lemo */

   CtrDrvrCounterMask40 = 0x200, /* Bit 9: 40MHz clock bit */
   CtrDrvrCounterMaskX1 = 0x400, /* Bit 10: Ext-1 clock bit */
   CtrDrvrCounterMaskX2 = 0x800, /* Bit 11: Ext-2 clock bit */
 } CtrDrvrCounterMask;

/* In addition to the counters there are other sources of interrupt that can */
/* be enabled and connected to. */

#define CtrDrvrInterruptSOURCES 14

typedef enum {
   CtrDrvrInterruptMaskCOUNTER_0     = 0x01,
   CtrDrvrInterruptMaskCOUNTER_1     = 0x02,
   CtrDrvrInterruptMaskCOUNTER_2     = 0x04,
   CtrDrvrInterruptMaskCOUNTER_3     = 0x08,
   CtrDrvrInterruptMaskCOUNTER_4     = 0x10,
   CtrDrvrInterruptMaskCOUNTER_5     = 0x20,
   CtrDrvrInterruptMaskCOUNTER_6     = 0x40,
   CtrDrvrInterruptMaskCOUNTER_7     = 0x80,
   CtrDrvrInterruptMaskCOUNTER_8     = 0x100,
   CtrDrvrInterruptMaskPLL_ITERATION = 0x200,
   CtrDrvrInterruptMaskGMT_EVENT_IN  = 0x400,
   CtrDrvrInterruptMaskPPS           = 0x800,
   CtrDrvrInterruptMask1KHZ          = 0x1000,
   CtrDrvrInterruptMaskMATCHED       = 0x2000
 } CtrDrvrInterruptMask;

/* Different possibilities for counter start. For the PMC version access to the */
/* external starts and clocks is via a seperate connector on the PMVC board.    */

typedef enum {
   CtrDrvrCounterStartNORMAL,       /* 0: The next millisecond tick starts the counter */
   CtrDrvrCounterStartEXT1,         /* 1: The counter uses external start number one */
   CtrDrvrCounterStartEXT2,         /* 2: The counter uses external start number two */
   CtrDrvrCounterStartCHAINED,      /* 3: The output of the previous counter is the start */
   CtrDrvrCounterStartSELF,         /* 4: The output of this counter is the start (Divide) */
   CtrDrvrCounterStartREMOTE,       /* 5: The counter waits for a remote start, see later */
   CtrDrvrCounterStartPPS,          /* 6: The counter waits for the PPS */
   CtrDrvrCounterStartCHAINED_STOP, /* 7: The counter is started by previous and stopped by next */

   CtrDrvrCounterSTARTS             /* 8: The number of possible counter starts */
 } CtrDrvrCounterStart;

/* Different counter configuration modes for burst and multi start */
/* Divide can be obtained from SELF start and MULTIPLE mode. */
/* When the start is SELF, the first start is like NORMAL to start the loop. */

#define CtrDrvrCounterStopBURST CtrDrvrCounterStartEXT2

typedef enum {
   CtrDrvrCounterModeNORMAL,     /* 0: One load, one start and one output mode */
   CtrDrvrCounterModeMULTIPLE,   /* 1: One load, multiple starts, multiple outputs */
   CtrDrvrCounterModeBURST,      /* 2: Like MULTIPLE, but terminated by external start two */
   CtrDrvrCounterModeMULT_BURST, /* 3: Both mode 1 & 2 = Multiple bursts */

   CtrDrvrCounterMODES           /* 4: There are 4 possible counter configuration modes */
 } CtrDrvrCounterMode;

/* Different sources for the counter clock.For the PMC version access to the */
/* external starts and clocks is via a seperate connector on the PMVC board. */

typedef enum {
   CtrDrvrCounterClock1KHZ,    /* 0: The 1KHZ CTrain clock from the 01 events */
   CtrDrvrCounterClock10MHZ,   /* 1: Divided down 40MHZ phase synchronous with 1KHZ */
   CtrDrvrCounterClock40MHZ,   /* 2: Recovered 40MHZ by the PLL from the data edges */
   CtrDrvrCounterClockEXT1,    /* 3: External clock one */
   CtrDrvrCounterClockEXT2,    /* 4: External clock two */
   CtrDrvrCounterClockCHAINED, /* 5: The output of the previous counter is the clock */

   CtrDrvrCounterCLOCKS        /* 6: The number of possible clock sources */
 } CtrDrvrCounterClock;

/* What happens when the counter reaches zero, possible actions are... */
/* (00) No output or interrupt N.B. Counter output can still be used in chaining */
/* (01) Bus interrupt only     */
/* (10) = External output      */
/* (11) = Output and interrupt */

typedef enum {
   CtrDrvrCounterOnZeroBUS = 0x01, /* Bit 0: On zero make a bus interrupt */
   CtrDrvrCounterOnZeroOUT = 0x02  /* Bit 1: On zero make an output pulse */
 } CtrDrvrCounterOnZero;

/* ==================================================================================== */
/* Counter configuration structure                                                      */
/* ==================================================================================== */

/* The counter configuration struc. The ctr has one active counter configuration block  */
/* for each counter, these structures are updated either from the host processor, when  */
/* the counter is under remote control, or, they can be overwritten when an incomming   */
/* timing frames triggers. For each possible trigger, a RAM stored configuration block  */
/* is copied to the counters active configuration block when the trigger fires.         */
/* Note that counter zero uses only the field "OnZero".                                 */

typedef struct {
   CtrDrvrCounterOnZero OnZero;    /* What to do on reaching zero. */
   CtrDrvrCounterStart  Start;     /* The counters start. */
   CtrDrvrCounterMode   Mode;      /* The counters operating mode. */
   CtrDrvrCounterClock  Clock;     /* Clock specification. */
   unsigned int         PulsWidth; /* Number of 40MHz ticks, 0 = as fast as possible. */
   unsigned int         Delay;     /* 32 bit delay to load into counter. */
 } CtrDrvrCounterConfiguration;

/* N.B. If the PulseWidth is large, then the output starts to look like a level */
/* or a gate. The maximum PulsWidth is therefore important. */

/* OK The above configuration structure is too big, so using bit-fields we will make a  */
/* more compact version occupying only two 32-bit ints. */

#define CtrDrvrCounterConfigON_ZERO_MASK     0xC0000000  /* 2-Bits 0/Out/Bus/OutBus */
#define CtrDrvrCounterConfigSTART_MASK       0x38000000  /* 3-Bits Nor/X1/X2/Chain/Self/Rem/Chain_stop */
#define CtrDrvrCounterConfigMODE_MASK        0x06000000  /* 2-Bits Nor/Mult/Burst/Mult+Burst */
#define CtrDrvrCounterConfigCLOCK_MASK       0x01C00000  /* 3-Bits 1K/10M/40M/X1/X2/Chain */
#define CtrDrvrCounterConfigPULSE_WIDTH_MASK 0x003FFFFF  /* 22-Bits 0..104 ms */

typedef struct {
   unsigned int Config;         /* Comprised of the above bit fields. */
   unsigned int Delay;          /* 32 bit delay to load into counter. */
} CtrDrvrHwCounterConfiguration; /* Compact form in two 32-bit ints */

/* ==================================================================================== */
/* The counter configuration table contains all the counter configurations that can be  */
/* copied to the active configuration when they are triggered. This data is located in  */
/* the RAM along with the TriggerTable, see below.                                      */
/* ==================================================================================== */

/* Both the Configuration table and the Trigger table have the same size and they are   */
/* both located in RAM, they share the same index. */

#define CtrDrvrRamTableSIZE 2048

/* Ram base configuration table, each configuration has a corresponding entry in the */
/* trigger table with the same index, it is also located in Ram. */

typedef CtrDrvrHwCounterConfiguration CtrDrvrRamConfigurationTable[CtrDrvrRamTableSIZE];

/* =============================================================================== */
/* Remote control bits to operate counters directly from host.                     */
/* The ctrp can do a lot of usefull things even if it is not connected to the      */
/* timing cable. It can still drive counters, make outputs and trains and is able  */
/* to provoke bus interrupts.                                                      */
/* =============================================================================== */

/* When LockConfig is set in the control block, the triggers can't overwrite the */
/* counter configuration. It is completely under remote control, however, it is  */
/* always possible to control the counter this way, even if not locked.          */

/* These bits are set directly from the host in the counters control block  */
/* located in the FPGA, see below. The corresponding FPGA location is write */
/* only, a rising edge on one of these bits provokes the corresponding      */
/* action on the counter. */

#define CtrDrvrREMOTES 5

typedef enum {
   CtrDrvrRemoteLOAD  = 0x01,   /* Bit 1: Load the counter remotely */
   CtrDrvrRemoteSTOP  = 0x02,   /* Bit 2: Remote kill counter no matter what its state */
   CtrDrvrRemoteSTART = 0x04,   /* Bit 3: Start the counter remotely */
   CtrDrvrRemoteOUT   = 0x08,   /* Bit 4: Make output on output pin, counter not disturbed */
   CtrDrvrRemoteBUS   = 0x10,   /* Bit 5: Make a bus interrupt now,  counter not disturbed */
   CtrDrvrRemoteEXEC  = 0x20    /* Bit 6: Execute remote command else set mask */
 } CtrDrvrRemote;

/* =============================================================================== */
/* Event frames                                                                    */
/* =============================================================================== */

/* Layout of an event frame, it occupies one 32bit frame on the timing cable. */

typedef unsigned int CtrDrvrFrameLong;

#ifdef __LITTLE_ENDIAN__
typedef struct {
   unsigned short Value;    /* Can be a WILD card in which case value is ignored */
   unsigned char  Code;
   unsigned char  Header;   /* Like 01 for C-Train, usually Mch:4 Type:4 */
 } CtrDrvrFrameStruct;
#else
typedef struct {
   unsigned char  Header;   /* Like 01 for C-Train, usually Mch:4 Type:4 */
   unsigned char  Code;
   unsigned short Value;    /* Can be a WILD card in which case value is ignored */
 } CtrDrvrFrameStruct;
#endif

typedef union {
   CtrDrvrFrameLong   Long;
   CtrDrvrFrameStruct Struct;
 } CtrDrvrEventFrame;

/* The "Value" field can be set to "WILD" in which case it is not included in the */
/* trigger test logic, however the Full event is stored in the counter history. */

#define CtrDrvrEventWILD 0xFFFF;

/* There are different accelerators on the CPS timing network. Here we make the */
/* definition of which accelerator corresponds to the 6 positions on the ctr    */
/* module. In general this assignement is in order of priority, highest energy  */
/* comes first, followed by its injectors. */

typedef enum {
   CtrDrvrMachineNONE = 0,     /* 0: No machine value */
   CtrDrvrMachineLHC  = 1,     /* 1: LHC */
   CtrDrvrMachineSPS  = 2,     /* 2: SPS new or legacy */
   CtrDrvrMachineCPS  = 3,     /* 3: PS or SCT */
   CtrDrvrMachinePSB  = 4,     /* 4: Booster or FCT */
   CtrDrvrMachineLEI  = 5,     /* 5: Leir Ion ring */
   CtrDrvrMachineADE  = 6,     /* 6: ADE Anti proton dec */
   CtrDrvrMachine777  = 7,     /* 7: Not used yet */

   CtrDrvrMachineMACHINES = 7  /* Number of machines, hence telegram buffers */
 } CtrDrvrMachine;

/* Machines currently in the CTF (CLIC Test Facility) network */
/* CLIC is the Compact LInear Collider and runs independently */

#define CtrDrvrMachineSCT CtrDrvrMachineCPS /* Slow CLIC telegram */
#define CtrDrvrMachineFCT CtrDrvrMachinePSB /* Fast CLIC telegram */

/* Most of these event types will be suppressed leaving only SIMPLE and TELEGRAM */
/* These types are active when the first nibble in the event frame has a machine */
/* code in it in the range 1..7                                                  */

typedef enum {
   CtrDrvrMachineEventTypeSC_NUM     = 0,
   CtrDrvrMachineEventTypeSPS_SIMPLE = 1,
   CtrDrvrMachineEventTypeSPS_SSC    = 2,
   CtrDrvrMachineEventTypeTELEGRAM   = 3,
   CtrDrvrMachineEventTypeSIMPLE     = 4,
   CtrDrvrMachineEventTypeTCU        = 5,

   CtrDrvrMachineEventTypeTYPES = 6
 } CtrDrvrMachineEventType;

/* Again most of this is legacy stuff, only MILLI_SECOND, CABLE_ID, and UTC will remain */
/* Special events occur when the first nibble is zero.                                  */

typedef enum {
   CtrDrvrSpecialEventTypeMILLI_SECOND = 1,
   CtrDrvrSpecialEventTypeSPS_SECOND   = 2,
   CtrDrvrSpecialEventTypeSPS_MINUTE   = 3,
   CtrDrvrSpecialEventTypeSPS_HOUR     = 4,
   CtrDrvrSpecialEventTypeSPS_DAY      = 5,
   CtrDrvrSpecialEventTypeSPS_MONTH    = 6,
   CtrDrvrSpecialEventTypeSPS_YEAR     = 7,
   CtrDrvrSpecialEventTypePS_DATE      = 8,
   CtrDrvrSpecialEventTypePS_TIME      = 9,
   CtrDrvrSpecialEventTypeCABLE_ID     = 10,
   CtrDrvrSpecialEventTypeUTC          = 11,

   CtrDrvrSpecialEventTypeTYPES = 11
 } CtrDrvrSpecialEventType;

/* =============================================================================== */
/* Special headers and codes recognized by hardware                                */
/* =============================================================================== */

/* Each millisecond the UTC time is incremented by 1ms, and the CTime register is */
/* set to the 24 bit binary value. All transactions and actions a synchronized to */
/* this clock. */

#define CtrDrvrMILLISECOND_HEADER 0x01

/* When the MTG_CABLE_ID event arrives, the "Value" is stored in the CableId */
/* FPGA register in the memory map. */

#define CtrDrvrMTG_CABLEID_HEADER 0x0A

/* The UTC time in seconds is transmitted in two frames 0xB5<MSW> 0xB6<LSW> */
/* the B6 event validates the time, B5 comes first in any millisecond. */

#define CtrDrvrUTC_TIME_MSW_CODE  0xB5  /* MS 16 bits of UTC second */
#define CtrDrvrUTC_TIME_LSW_CODE  0xB6  /* Validates the time */

/* Store the "Value" of these events in the incomming telegram buffer for the */
/* machine nibble in the header, at the position indicated by the "Code". The */
/* first nibble is the machine, the second nibble in the header byte is the   */
/* fixed value "3" indicating its a telegram. */

#define CtrDrvrTELEGRAM_HEADER(machine) (0x03 & (machine<<4))

#define CtrDrvrTELEGRAM_LHC_HEADER 0x13
#define CtrDrvrTELEGRAM_SPS_HEADER 0x23
#define CtrDrvrTELEGRAM_CPS_HEADER 0x33
#define CtrDrvrTELEGRAM_PSB_HEADER 0x43
#define CtrDrvrTELEGRAM_LEI_HEADER 0x53
#define CtrDrvrTELEGRAM_ADE_HEADER 0x63
#define CtrDrvrTELEGRAM_777_HEADER 0x73

/* Put these in the event history, search the triggers. The "Code" is the event */
/* code and the "Value" may carry other data. */
/* Wild cards can be used to mask the "Value". */

#define CtrDrvrSIMPLE_HEADER(machine) (0x04 & (machine<<4))

#define CtrDrvrSIMPLE_LHC_HEADER 0x14
#define CtrDrvrSIMPLE_SPS_HEADER 0x24
#define CtrDrvrSIMPLE_CPS_HEADER 0x34
#define CtrDrvrSIMPLE_PSB_HEADER 0x44
#define CtrDrvrSIMPLE_LEI_HEADER 0x54
#define CtrDrvrSIMPLE_ADE_HEADER 0x64
#define CtrDrvrSIMPLE_777_HEADER 0x74

/* One special simple event, "TELEGRAM_READY" is used to swap between the incomming */
/* and active telegram buffers. The machine is in the first nibble of the header */
/* and the second nibble is set to "4". The code is set to "FE", and the "Value" */
/* can be ignored. */

#define CtrDrvrSIMPLE_TELEGRAM_READY_CODE 0xFE
#define CtrDrvrSIMPLE_TELEGRAM_READY_FRAME(machine) (0x04FE0000 & (machine<<28))

/* =============================================================================== */
/* We need to define time stamps, they include the UTC second and the nano second  */
/* in the UTC second, it also includes the C-Train machine time value.             */
/* =============================================================================== */

typedef struct {
   unsigned int Second;       /* UTC Second */
   unsigned int TicksHPTDC;   /* 25/32 ns ticks, zero if no HPTDC */
 } CtrDrvrTime;

/* Sometimes we want not just the UTC time but the milli second modulo value as well */

typedef struct {
   unsigned int CTrain;        /* Milli second modulo at Time */
   CtrDrvrTime  Time;
 } CtrDrvrCTime;

/* =============================================================================== */
/* These counter configurations are implemented in the FPGA and really control the */
/* counters actively. Writing here really affects the running counters, and this   */
/* permits controlling the counter remotely, and routing its output to the ctrp    */
/* out put pins on the connector.                                                  */
/* =============================================================================== */

/* These FPGA resident structures control counters. Each time a trigger occurs, */
/* the ctr examines first the LockConfig field, if it is non zero, the trigger  */
/* is completely ignored. If it is zero, then the ctr examines the LockHistory, */
/* if it is non zero, the counter history is not overwritten, but the counter   */
/* configuration is overwritten as usual. If the LockHistory is zero, then the  */
/* ctr fills in the counter history as usual. In both cases, zero and non zero, */
/* at OnZeroTime, it increments the LockHistory flag. It will be up to the host */
/* to read and clear the LockHistory flag. This flag is cleared by the CTR on   */
/* reading. If the host sets the LockConfig flag then the the counter is under  */
/* REMOTE control.                                                              */

/* Note the OutMask is used to rout a counters output to physical output pins.  */
/* This permits doing a wire-or inside the CTR. Note also that if the OutMask   */
/* contains a zero value the counter makes no output even if the OnZero field   */
/* has the OnZeroOUT bit set in its configuration.                              */

typedef struct {

   /* The two lock flags give priority to bus accesses */

   unsigned int               LockConfig;    /* Triggers do not overwrite the config  */
   unsigned int               LockHistory;   /* CLEAR ON READ: Dont overwrite history */
   CtrDrvrRemote              Remote;        /* Used to remote control the counter    */
   CtrDrvrCounterMask         OutMask;       /* Physical output pins                  */
 } CtrDrvrCounterControl;

/* Pack Remote and OutMask fields into a 32-bit int for the hardware */

typedef struct {
   unsigned int               LockConfig;    /* Triggers do not overwrite the config  */
   unsigned int               LockHistory;   /* CLEAR ON READ: Dont overwrite history */
   unsigned int               RemOutMask;    /* Remote and hardware mask combined */
 } CtrDrvrHwCounterControl;

#define CtrDrvrCntrCntrlREMOTE_MASK 0x3F
#define CtrDrvrCntrCntrlOUT_MASK    0x3FFC0
#define CtrDrvrCntrCntrlTTL_BAR     0x80000000

/* The counter history is only one entry per counter deep, it is protected by the    */
/* LockHistory flag in the control block. If this flag is non zero, their is a valid */
/* history to be read. If it is greater than one, the history is not up to date. The */
/* main use of history is within the host ISR, if interrupts come too fast, then     */
/* some histories will be missed.                                                    */

/* The incomming event frame that provoked the trigger needs to be stored. This  */
/* is because some interesting data may be contained in its value part when wild */
/* cards are in use. See Frame field */

typedef struct {
   unsigned int               Index;         /* Index into trigger table of loading trigger */
   CtrDrvrEventFrame          Frame;         /* The actual frame (without Wild Cards !) */
   CtrDrvrCTime               TriggerTime;   /* Time counter got loaded */
   CtrDrvrCTime               StartTime;     /* Time counter start arrived. */
   CtrDrvrCTime               OnZeroTime;    /* Time the counter reaced zero, or interrupted */
 } CtrDrvrCounterHistory;

/* The hardware active implementation in the FPGA of a counter, it contains a control */
/* block, a history, and the actual counter configuration. */

typedef struct {
   CtrDrvrHwCounterControl       Control;
   CtrDrvrCounterHistory         History;
   CtrDrvrHwCounterConfiguration Config;
 } CtrDrvrFpgaCounter;

/* Now we instantiate one Fpga configuration for each of the real counters and for  */
/* counter zero, when direct action triggers fire. Eg when an incomming event frame */
/* provokes a bus interrupt we use counter zero. */

typedef CtrDrvrFpgaCounter CtrDrvrCounterBlock[CtrDrvrCOUNTERS];

/* ============================================================================ */
/* Before we can define the trigger condition of a counter we must first define */
/* a few more enumerations.                                                     */
/* ============================================================================ */

typedef enum {
   CtrDrvrTriggerConditionNO_CHECK, /* Do not check the telegram */
   CtrDrvrTriggerConditionEQUALITY, /* Trigger telegram condition must equal the telegram */
   CtrDrvrTriggerConditionAND,      /* Trigger telegram condition AND telegram must be non zero */

   CtrDrvrTriggerCONDITIONS         /* There are 3 kinds of trigger conditions */
 } CtrDrvrTriggerCondition;

/* We need to define a telegram at this point. A telegram describes the current */
/* machine cycle as a sequential list of parameters transmitted over the timing */
/* network each basic period. Each machine has its own telegram.   */

#define CtrDrvrTgmGROUP_VALUES 64

typedef unsigned short CtrDrvrTgm[CtrDrvrTgmGROUP_VALUES];      /* 64x16 bit values */
typedef CtrDrvrTgm     CtrDrvrTgmBlock[CtrDrvrMachineMACHINES]; /* One telegram per machine */

typedef struct {
   unsigned short GroupNumber;  /* The serial position in the telegram starting at one */
   unsigned short GroupValue;   /* The value of the parameter at this position */
 } CtrDrvrTgmGroup;

/* OK now we define a trigger. This consists of an event frame and a telegram condition. */
/* This trigger is too big to fit into the FPGA, so we keep this structure for the */
/* driver API, and declare a second version packed into bit fields. */

typedef struct {
   unsigned int            Ctim;               /* Ctim equipment number */
   CtrDrvrEventFrame       Frame;              /* Timing frame with optional wild card */
   CtrDrvrTriggerCondition TriggerCondition;   /* Type of trigger */
   CtrDrvrMachine          Machine;            /* Which telegram if used */
   CtrDrvrCounter          Counter;            /* Which counter to configure */
   CtrDrvrTgmGroup         Group;              /* Trigger telegram condition */
 } CtrDrvrTrigger;                             /* Used in driver API */

/* Now define a compact form of trigger as packed bits in two 32-bit ints. */
/* CtrDrvrTriggerCondition needs 2-bits 0 (No Check) 1 (Equal) 2 (And) */
/* CtrDrvrMachine needs 3 bits 0(No) 1(LHC) 2(SPS) ... 7(Not used yet) */
/* CtrDrvrCounter needs 4 bits 0(NO) 1(1) ... 8(8) */
/* The number of Tgm groups is 64 needs 6 bits, we will use 7 */
/* The group value is 16 bits */

#define CtrDrvrTrigCONDITION_MASK    0xC0000000
#define CtrDrvrTrigMACHINE_MASK      0x38000000
#define CtrDrvrTrigCOUNTER_MASK      0x07800000
#define CtrDrvrTrigGROUP_NUMBER_MASK 0x007F0000
#define CtrDrvrTrigGROUP_VALUE_MASK  0x0000FFFF

typedef struct {
   CtrDrvrEventFrame Frame;     /* Timing frame with optional wild card */
   unsigned int      Trigger;   /* Packed trigger bits as above */
 } CtrDrvrHwTrigger;            /* Compact hardware form of a trigger */

/* This trigger table is searched by the ctr hardware for each incomming event.    */
/* If the trigger fires, the corresponding counter configuration from the counter  */
/* configuration table is copied the the active counter configuration specified by */
/* counter. */

typedef CtrDrvrHwTrigger CtrDrvrRamTriggerTable[CtrDrvrRamTableSIZE];

/* ==================================================== */
/* This structure is very low on the whish list, so if  */
/* we have time and hardware resources enough, then OK, */
/* otherwise we don't need it. It will be in RAM.       */
/* ==================================================== */

#define CtrDrvrHISTORY_TABLE_SIZE 1024

/* Each entry in the history buffer contains the frame  */
/* excluding telegram and millisecond events. An entry  */
/* also contains the UTC and Millisecond modulo arrival */
/* time of the events. */

typedef struct {
   CtrDrvrEventFrame Frame;
   CtrDrvrCTime      CTime;
 } CtrDrgvrEventHistoryEntry;

/* The Index field points to the next write address, it  */
/* behaves like a ring buffer after entry 1023, the next */
/* entry address is zero. */

typedef struct {
   unsigned int Index;
   CtrDrgvrEventHistoryEntry Entries[CtrDrvrHISTORY_TABLE_SIZE];
 } CtrDrvrEventHistory;

/* ==================================================== */
/* The status and alarm bit meanings                    */
/* ==================================================== */

/* Alarms and Status bits */

#define CtrDrvrSTATAE 12
#define CtrDrvrHwStatusMASK 0x3FF
#define CtrDrvrTypeStatusMask 0x380

typedef enum {
   CtrDrvrStatusGMT_OK       = 0x01,   /* No GMT errors */
   CtrDrvrStatusPLL_OK       = 0x02,   /* PLL locked on GMT */
   CtrDrvrStatusEXT_CLK_1_OK = 0x04,   /* Ext clock present */
   CtrDrvrStatusEXT_CLK_2_OK = 0x08,   /* Ext clock present */
   CtrDrvrStatusSELF_TEST_OK = 0x10,   /* Self test OK */
   CtrDrvrStatusENABLED      = 0x20,   /* Module enabled */
   CtrDrvrStatusHPTDC_IN     = 0x40,   /* HPTDC chip installed */
   CtrDrvrStatusCTRI         = 0x80,   /* This is a CTRI module */
   CtrDrvrStatusCTRP         = 0x100,  /* This is a CTRP module */
   CtrDrvrStatusCTRV         = 0x200,  /* This is a CTRV module */

   /* Software status bits */

   CtrDrvrStatusNO_LOST_INTERRUPTS = 0x400,  /* Interrupts too fast */
   CtrDrvrStatusNO_BUS_ERROR       = 0x800  /* Module has not seen a bus error */

 } CtrDrvrStatus;

typedef enum {
   CtrDrvrIoStatusCTRXE   = 0x0001, /* Beam Energy extension */
   CtrDrvrIoStatusCTRXI   = 0x0002, /* IO extension */
   CtrDrvrIoStatusV1_PCB  = 0x0004, /* PCB version 1 */
   CtrDrvrIoStatusV2_PCB  = 0x0008, /* PCB version 2 */

   CtrDrvrIoStatusS1      = 0x0010, /* External start 1 input value */
   CtrDrvrIoStatusS2      = 0x0020, /* External start 2 input value */
   CtrDrvrIoStatusX1      = 0x0040, /* External clock 1 input value */
   CtrDrvrIoStatusX2      = 0x0080, /* External clock 2 input value */

   CtrDrvrIoStatusO1      = 0x0100, /* Lemo output 1 state */
   CtrDrvrIoStatusO2      = 0x0200, /* Lemo output 2 state */
   CtrDrvrIoStatusO3      = 0x0400, /* Lemo output 3 state */
   CtrDrvrIoStatusO4      = 0x0800, /* Lemo output 4 state */

   CtrDrvrIoStatusO5      = 0x1000, /* Lemo output 5 state */
   CtrDrvrIoStatusO6      = 0x2000, /* Lemo output 6 state */
   CtrDrvrIoStatusO7      = 0x4000, /* Lemo output 7 state */
   CtrDrvrIoStatusO8      = 0x8000, /* Lemo output 8 state */
   
   /* Added by pablo */

   CtrDrvrIOStatusIDOkP            = 0x10000, /* PCB Id read Ok */
   CtrDrvrIOStatusDebugHistory     = 0x20000, /* Debug History Mode On */
   CtrDrvrIOStatusUtcPllEnabled    = 0x40000, /* Utc Pll Enabled 0=Brutal 1=Soft lock */
   CtrDrvrIOStatusExtendedMemory   = 0x80000, /* Extended memory for VHDL revision March 2008 */
   CtrDrvrIOStatusTemperatureOk    = 0x100000 /* Sensor present */

 } CtrDrvrIoStatus;

#define CtrDrvrIoSTATAE 21

/* ==================================================== */
/* Command bits for ctrp module                         */
/* ==================================================== */

typedef enum {
   CtrDrvrCommandRESET     = 0x01,      /* Do a hardware reset */
   CtrDrvrCommandENABLE    = 0x02,      /* Enable timing input */
   CtrDrvrCommandDISABLE   = 0x04,      /* Disable timing input */
   CtrDrvrCommandLATCH_UTC = 0x08,      /* Prior to reading it */
   CtrDrvrCommandSET_UTC   = 0x10,      /* When no timing connected */
   CtrDrvrCommandSET_HPTDC = 0x20,      /* Say HPTDC detected */
   
   /* Added by pablo */

   CtrDrvrCommandDISABLE_HPTDC = 0x40,  /* Say HPTDC  disabled */
   CtrDrvrCommandDebugHisOn    = 0x80,  /* Enable Debug History Mode */
   CtrDrvrCommandDebugHisOff   = 0x100, /* Disable Debug History Mode */
   CtrDrvrCommandUtcPllOff     = 0x200, /* Disable Utc Pll Brutal On */
   CtrDrvrCommandUtcPllOn      = 0x400  /* Enable Utc Pll  Brutal Off */
 } CtrDrvrCommand;

/* ==================================================== */
/* Pll Debug information                                */
/* ==================================================== */

typedef struct {
   unsigned int  Error;       /* RO: Pll phase error */
   unsigned int  Integrator;  /* RW: Pll Integrator */
   unsigned int  Dac;         /* RO: Value applied to DAC */
   unsigned int  LastItLen;   /* RO: Last iteration length */
   unsigned int  Phase;       /* RW: Pll phase */
   unsigned int  NumAverage;  /* RW: Numeric average */
   unsigned int  KP;          /* RW: Constant of proportionality */
   unsigned int  KI;          /* RW: Constant of integration */
 } CtrDrvrPll;

/* The PLL asynchronous period is needed to convert PLL units into */
/* time values. This value is kept a a constant in the driver to   */
/* be used in PLL calculations as follows ...                      */

/* ErrorNs = (Error*PllAsyncPeriodNs)/NumAverage                   */
/* PhaseNs = (Phase*PllAsyncPeriodNs)/NumAverage                   */

typedef float CtrDrvrPllAsyncPeriodNs;

/* ==================================================== */
/* Module statistics                                    */
/* ==================================================== */

typedef struct {
   unsigned int PllErrorThreshold;     /* Pll error threshold to generate a pll error RW */
   unsigned int PllDacLowPassValue;    /* Low Passed DAC value used if the GMT is missed (CIC filter) RO */
   unsigned int PllDacCICConstant;     /* Log2 of the number of avarages of the DAC Value CIC low pass filter RW */
   unsigned int PllMonitorCICConstant; /* Log2 of the interrupt reduction factor (averages over this count)  RW */
   unsigned int PhaseDCM;              /* (Not used yet) Phase of the spartan DLL. Not used in the standard VHDL RW */
   unsigned int UtcPllPhaseError;      /* Phase error Utc */
   unsigned int Temperature;           /* Card temperature. Only valid in CTRIs V3 RO */
   unsigned int MsMissedErrs;          /* Number of millisencods missed since last power up. (Latches the time)  RO */
   CtrDrvrTime   LastMsMissed;
   
   unsigned int PllErrors;             /* Number of pll errors since last power up (past threshold ) RO */
   CtrDrvrTime  LastPllError;
   
   unsigned int MissedFrames;          /* Number of missed frames since last power up */
   CtrDrvrTime  LastFrameMissed;

   unsigned int BadReceptionCycles;    /* Number of bad reception cycles since last power up */
   unsigned int ReceivedFrames;        /* Number of received frames since last power up */
   unsigned int SentFramesEvent;       /* Last Sent Frames Event */
   unsigned int UtcPllErrs;            /* Number of utc pll errors since last reset */
   
   CtrDrvrTime   LastExt1Start;         /* External Start 1 time tag */
 } CtrDrvrModuleStats;

/* ==================================================== */
/* The ctrp memory map                                  */
/* ==================================================== */

/* N.B. For efficiency in the ISR, the source field must be the first. */

typedef struct {

   /* FPGA resident structures */

   CtrDrvrInterruptMask InterruptSource;    /* Cleared on read */
   CtrDrvrInterruptMask InterruptEnable;    /* Enable interrupts */

   unsigned int         HptdcJtag;          /* Jtag control reg for HpTdc */

   /* See below for the meaning of these fields */

   unsigned int        InputDelay;  /* Specified in 40MHz ticks */
   unsigned int        CableId;     /* ID of CTG piloting GMT */
   unsigned int        VhdlVersion; /* UTC time of VHDL build */
   unsigned int        OutputByte;  /* Output byte number 1..8 on VME P2 + Enable front pannel */

   CtrDrvrStatus       Status;      /* Current status */
   CtrDrvrCommand      Command;     /* Write command to module */

   CtrDrvrPll          Pll;         /* Pll parameters */

   CtrDrvrCTime        ReadTime;    /* Latched date time and CTrain */
   unsigned int        SetTime;     /* Used to set UTC if no cable */

   CtrDrvrCounterBlock Counters;    /* The counter configurations and status */
   CtrDrvrTgmBlock     Telegrams;   /* Active telegrams */

   /* RAM resident structures, they must be read/write */

   CtrDrvrRamTriggerTable       Trigs;        /* 2048 Trigger conditions */
   CtrDrvrRamConfigurationTable Configs;      /* 2048 Counter configurations */
   CtrDrvrEventHistory          EventHistory; /* 1024 Events deep */

   unsigned int Setup;          /* VME Level and Interrupt vector */

   unsigned int LastReset;      /* UTC Second of last reset */
   unsigned int PartityErrs;    /* Number of parity errors since last reset */
   unsigned int SyncErrs;       /* Number of frame synchronization errors since last reset */
   unsigned int TotalErrs;      /* Total number of IO errors since last reset */
   unsigned int CodeViolErrs;   /* Number of code violations since last reset */
   unsigned int QueueErrs;      /* Number of input Queue overflows since last reset */

   /* New registers used only in recent VHDL, old makes a bus error ! */

   CtrDrvrIoStatus IoStat;      /* IO status */

   unsigned int IdLSL;          /* ID Chip value Least Sig 32-bits */
   unsigned int IdMSL;          /* ID Chip value Most  Sig 32-bits */
  
   CtrDrvrModuleStats ModStats; /* Module statistics */

 } CtrDrvrMemoryMap;

/* ===================================================================== */
/* InputDelay: This is the delay in 40MHz ticks used to compensate for   */
/* the transmission delay from the CTGU to this particular card. The     */
/* CTGU advances UTC time be 100us, and hence the InputDelay can be      */
/* calculated as: InputDelay = 100us - TransmissionDelay.                */
/* ===================================================================== */
/* CableId: This number is transmitted over the timing cable to indicate */
/* which one of the eight cables the CTR is attached. See CtrDrvrMachine */
/* ===================================================================== */
/* VhdlVersion: This is the UTC second when the VHDL program was         */
/* compiled into the FPGA bit stream.                                    */
/* ===================================================================== */
/* OutputByte: In the VME version of the CTR, the eight counter outputs  */
/* can be placed on one byte of the P2 connector. If this value is zero  */
/* the CTR does not drive the P2 connector, a value between 1..4 selects */
/* the byte number in the 64bit P2 VME bus.                              */
/* ===================================================================== */

#endif
