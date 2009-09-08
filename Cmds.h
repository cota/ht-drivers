/**************************************************************************/
/* Command line stuff                                                     */
/**************************************************************************/

int Illegal();              /* llegal command */

int Quit();                 /* Quit test program  */
int Help();                 /* Help on commands   */
int History();              /* History            */
int Shell();                /* Shell command      */
int Sleep();                /* Sleep seconds      */
int Pause();                /* Pause keyboard     */
int Atoms();                /* Atom list commands */

int ChangeEditor();
int Module();
int NextModule();
int PrevModule();
int SwDeb();
int GetSetTmo();
int RfmIo();
int RawIo();
int ConfigIo();
int LocalIo();
int GetSetQue();
int ListClients();
int Reset();
int SetCommand();
int GetVersion();
int ShowNetwork();
int ReadNodeTableFile();
int WaitInterrupt();

int WriteSegTable();
int ReadSegTable();
int EditSegTable();
int WriteSegTableFile();
int ReadSegTableFile();
int ShowSegTable();
int FlushSegs();
int SegIoDma();

int CheckSeg();
int EditSeg();
int WriteSegFile();
int ReadSegFile();
int Segment();
int NextSegment();
int PrevSegment();

int SendMessage();
int SendSoftWakeup();
int DmaThreshold();
int SetSegPattern();
int SearchPattern();

static void IErr();

FILE *inp;

typedef enum {

   CmdNOCM,    /* llegal command */

   CmdQUIT,    /* Quit test program  */
   CmdHELP,    /* Help on commands   */
   CmdHIST,    /* History            */
   CmdSHELL,   /* Shell command      */
   CmdSLEEP,   /* Sleep seconds      */
   CmdPAUSE,   /* Pause keyboard     */
   CmdATOMS,   /* Atom list commands */

   CmdCHNGE,   /* Change editor to next program */
   CmdMODULE,  /* Module selection */
   CmdNEXT,    /* Next module */
   CmdPREV,    /* Previous module */
   CmdDEBUG,   /* Get Set debug mode */
   CmdTMO,     /* Timeout control */
   CmdRFMIO,   /* RFM register IO */
   CmdRAWIO,   /* Raw SDRAM memory IO */
   CmdCNFIO,   /* Plx9656 Configuration registers edit */
   CmdLOCIO,   /* Plx9656 Local Configuration registers edit */
   CmdQFL,     /* Queue flag on off */
   CmdLSCL,    /* List clients */
   CmdRESET,   /* Reset module */
   CmdCOMMAND, /* Send a command */
   CmdVER,     /* Get versions */
   CmdWINT,    /* Wait for interrupt */
   CmdFRNDTAB, /* Read the node table from file */
   CmdNETWORK, /* Show connected nodes network */

   CmdWSEGTAB,  /* Write segment table to hardware */
   CmdRSEGTAB,  /* Read segment table from hardware */
   CmdESEGTAB,  /* Edit segment table copy */
   CmdFWSEGTAB, /* File write save segment table */
   CmdFRSEGTAB, /* File read load segment table */
   CmdSHOWSEGS, /* Show the segment table */

   CmdFSEGS,    /* Flush segments */
   CmdSGDMA,    /* Do a DMA transfer from to segment */
   CmdCSEG,     /* Check segment for updates */
   CmdESEG,     /* Edit copy of segment */
   CmdSEGFILL,  /* Fill a gegment with a value */
   CmdSCHSEG,   /* Search segment */
   CmdFWSEG,    /* File write segment */
   CmdFRSEG,    /* File read segment */
   CmdSEG,      /* Set/Get working segment */
   CmdNSEG,     /* Next segment */
   CmdPSEG,     /* Previous segment */

   CmdSMS,      /* Send a message */
   CmdWAKEUP,   /* soft wake-up test */
   CmdDMA,      /*Set/Get drivers Dma threshold */

   CmdCMDS } CmdId;

typedef struct {
   CmdId  Id;
   char  *Name;
   char  *Help;
   char  *Optns;
   int  (*Proc)(); } Cmd;

static Cmd cmds[CmdCMDS] = {

   { CmdNOCM,   "???",    "Illegal command"          ,""           ,Illegal },

   { CmdQUIT,    "q" ,    "Quit test program"        ,""           ,Quit  },
   { CmdHELP,    "h" ,    "Help on commands"         ,""           ,Help  },
   { CmdHIST,    "his",   "History"                  ,""           ,History},
   { CmdSHELL,   "sh",    "Shell command"            ,"UnixCmd"    ,Shell },
   { CmdSLEEP,   "s" ,    "Sleep seconds"            ,"Seconds"    ,Sleep },
   { CmdPAUSE,   "z" ,    "Pause keyboard"           ,""           ,Pause },
   { CmdATOMS,   "a" ,    "Atom list commands"       ,""           ,Atoms },

   { CmdCHNGE,   "ce",    "Change text editor used"  ,""           ,ChangeEditor },
   { CmdMODULE,  "mo",    "Select/Show modules"      ,"Module"     ,Module },
   { CmdNEXT,    "nm",    "Next module"              ,""           ,NextModule },
   { CmdPREV,    "pm",    "Previous module"          ,""           ,PrevModule },
   { CmdDEBUG,   "deb",   "Get/Set driver debug mode","1|2|4/0"    ,SwDeb },
   { CmdTMO,     "tmo",   "Timeout control"          ,"Timeout"    ,GetSetTmo },

   { CmdRFMIO,   "rio",   "Rfm Registers IO"         ,"Address"     ,RfmIo},
   { CmdRAWIO,   "xio",   "Raw SDRAM IO"             ,"Address"     ,RawIo},
   { CmdCNFIO,   "cio",   "Plx9656 Config IO"        ,"Address"     ,ConfigIo},
   { CmdLOCIO,   "lio",   "Plx9656 Local Config IO"  ,"Address"     ,LocalIo},
   { CmdQFL,     "qf",    "Queue flag on off"        ,"1/0"         ,GetSetQue },
   { CmdLSCL,    "cl",    "List clients"             ,""            ,ListClients },
   { CmdRESET,   "reset", "Reset module"             ,""            ,Reset },
   { CmdCOMMAND, "cmd",   "Get/Set LCSR1 command"    ,"?|<CmdMsk>"  ,SetCommand },
   { CmdVER,     "ver",   "Get versions"             ,""            ,GetVersion },
   { CmdWINT,    "wi",    "Wait for interrupt"       ,"?|-|<IntMsk>",WaitInterrupt },
   { CmdFRNDTAB, "rntf",  "Read node table file"     ,"<path>"      ,ReadNodeTableFile },
   { CmdNETWORK, "net",   "Show node network"        ,""            ,ShowNetwork },

   { CmdWSEGTAB, "wst",   "Write segment table to hardware"   ,""       ,WriteSegTable },
   { CmdRSEGTAB, "rst",   "Read segment table from hardware"  ,""       ,ReadSegTable },
   { CmdESEGTAB, "est",   "Edit segment table file"           ,""       ,EditSegTable },
   { CmdFWSEGTAB,"wstf",  "File write save segment table"     ,"<Path>" ,WriteSegTableFile },
   { CmdFRSEGTAB,"rstf",  "File read load segment table"      ,"<Path>" ,ReadSegTableFile },
   { CmdSHOWSEGS,"ls",    "List segment table"                ,""       ,ShowSegTable },

   { CmdFSEGS,   "flsgs", "Flush segments out to all nodes","<SegMsk>"             ,FlushSegs },
   { CmdSGDMA,   "tdma",  "Test DMA on current segment"    ,""                     ,SegIoDma },
   { CmdCSEG,    "csg",   "Check segment for updates"      ,""                     ,CheckSeg },
   { CmdESEG,    "esg",   "Edit segment"                   ,"<Offset>"             ,EditSeg },
   { CmdSEGFILL, "fsgb",  "Fill segment with bytes"        ,"<Byte><Offset><Size>" ,SetSegPattern },
   { CmdSCHSEG,  "ssg",   "Search segment for a Pattern"   ,"?|[B|W|L]<Opr><Value>",SearchPattern },
   { CmdFWSEG,   "wsgf",  "File write segment"             ,"<Path>"               ,WriteSegFile },
   { CmdFRSEG,   "rsgf",  "File read segment"              ,"<Path>"               ,ReadSegFile },
   { CmdSEG,     "sg",    "Set/Get working segment"        ,"<Seg>"                ,Segment },
   { CmdNSEG,    "nsg",   "Next segment"                   ,""                     ,NextSegment },
   { CmdPSEG,    "psg",   "Previous segment"               ,""                     ,PrevSegment },

   { CmdSMS,     "sms",   "Send a message","?|<Typ><Cst><Data><Dest>"   ,SendMessage },
   { CmdWAKEUP,  "wakeup","Send 'Soft Wake-up'"            ,""          ,SendSoftWakeup },
   { CmdDMA,     "dma",   "Set/Get drivers Dma threshold"  ,"<Threshold>",DmaThreshold }
 };

typedef enum {

   OprNOOP,

   OprNE,  OprEQ,  OprGT,  OprGE,  OprLT , OprLE,   OprAS,
   OprPL,  OprMI,  OprTI,  OprDI,  OprAND, OprOR,   OprXOR,
   OprNOT, OprNEG, OprLSH, OprRSH, OprINC, OprDECR, OprPOP,
   OprSTM,

   OprOPRS } OprId;

typedef struct {
   OprId  Id;
   char  *Name;
   char  *Help; } Opr;

static Opr oprs[OprOPRS] = {
   { OprNOOP, "?"  ,"Not an operator"       },
   { OprNE,   "#"  ,"Not equal"             },
   { OprEQ,   "="  ,"Equal"                 },
   { OprGT,   ">"  ,"Greater than"          },
   { OprGE,   ">=" ,"Greater than or equal" },
   { OprLT,   "<"  ,"Less than"             },
   { OprLE,   "<=" ,"Less than or equal"    },
   { OprAS,   ":=" ,"Becomes equal"         },
   { OprPL,   "+"  ,"Add"                   },
   { OprMI,   "-"  ,"Subtract"              },
   { OprTI,   "*"  ,"Multiply"              },
   { OprDI,   "/"  ,"Divide"                },
   { OprAND,  "&"  ,"AND"                   },
   { OprOR,   "!"  ,"OR"                    },
   { OprXOR,  "!!" ,"XOR"                   },
   { OprNOT,  "##" ,"Ones Compliment"       },
   { OprNEG,  "#-" ,"Twos compliment"       },
   { OprLSH,  "<<" ,"Left shift"            },
   { OprRSH,  ">>" ,"Right shift"           },
   { OprINC,  "++" ,"Increment"             },
   { OprDECR, "--" ,"Decrement"             },
   { OprPOP,  ";"  ,"POP"                   },
   { OprSTM,  "->" ,"PUSH"                  } };

static char atomhash[256] = {
  10,9,9,9,9,9,9,9,9,0,0,9,9,0,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  0 ,1,9,1,9,4,1,9,2,3,1,1,0,1,11,1,5,5,5,5,5,5,5,5,5,5,1,1,1,1,1,1,
  10,6,6,6,6,6,6,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6,6,6,6,6,7,9,8,9,6,
  9 ,6,6,6,6,6,6,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6,6,6,6,6,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 };

typedef enum {
   Seperator=0,Operator=1,Open=2,Close=3,Comment=4,Numeric=5,Alpha=6,
   Open_index=7,Close_index=8,Illegal_char=9,Terminator=10,Bit=11,
 } AtomType;

#define MAX_ARG_LENGTH  32
#define MAX_ARG_COUNT   16
#define MAX_ARG_HISTORY 16

typedef struct {
   int      Pos;
   int      Number;
   AtomType Type;
   char     Text[MAX_ARG_LENGTH];
   CmdId    CId;
   OprId    OId;
} ArgVal;

static int pcnt = 0;
static ArgVal val_bufs[MAX_ARG_HISTORY][MAX_ARG_COUNT];
static ArgVal *vals = val_bufs[0];

#if 0
#define True 1
#define False 0
#endif
