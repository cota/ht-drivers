/**************************************************************************/
/* Command line stuff                                                     */
/**************************************************************************/

int Illegal();              /* llegal command */

int Quit();                 /* Quit test program  */
int Help();                 /* Help on commands   */
int News();                 /* Show GMT test news */
int History();              /* History            */
int Shell();                /* Shell command      */
int Sleep();                /* Sleep seconds      */
int Pause();                /* Pause keyboard     */
int Atoms();                /* Atom list commands */

#ifdef CTR_PCI
int ConfigIo();
int LocalIo();
int EpromEdit();
int EpromRead();
int EpromWrite();
int EpromReadFile();
int EpromWriteFile();
int EpromErase();
int PlxReConfig();
int Remap();
#endif

#ifdef CTR_VME
int GetSetOByte();
#endif

int GetVersion();
int ChangeEditor();
int ChangeDirectory();
int Module();
int NextModule();
int PrevModule();
int SwDeb();
int GetSetTmo();
int MemIo();
int HptdcSetupEdit();
int HptdcSetupIo();
int HptdcSetupReadFile();
int HptdcSetupWriteFile();
int HptdcStatusRead();
int HptdcControlEdit();
int HptdcControlIo();
int HptdcControlReadFile();
int HptdcControlWriteFile();
int ReloadJtag();
int ReloadAllJtag();
int GetSetQue();
int GetSetCableId();
int ListClients();
int Reset();
int GetStatus();
int GetSetUtc();
int WaitInterrupt();
int SimulateInterrupt();
int GetSetQue();
int GetSetTmo();
int GetSetActions();
int GetSetPtim();
int GetSetCtim();
int GetSetOutMask();
int GetSetRemote();
int SetRemoteCmd();
int GetSetEnable();
int GetSetInputDelay();
int GetSetConfig();
int GetCounterHistory();
int GetSetPll();
int SetBrutalPll();
int GetTelegram();
int GetEventHistory();
int SetDebHis();
int CtimRead();
int PtimReadFile();
int PtimWriteFile();
int LaunchClock();
int LaunchVideo();
int LaunchLookat();
int ListActions();
int Init();
int GetSetChannel();
int NextChannel();
int PrevChannel();
int GetLatency();
int MemoryTest();
int PlotPll();
int GetSetDac();
int AqLog();
int RecpErrs();
int IoStatus();
int GetBoardId();

static void IErr();

/* Jtag backend to public code */

FILE *inp;

void setPort(short p, short val);
void readByte();
unsigned char readTDOBit();
void waitTime();
void pulseClock();

typedef enum {

   CmdNOCM,    /* llegal command */

   CmdQUIT,    /* Quit test program  */
   CmdHELP,    /* Help on commands   */
   CmdNEWS,    /* Show GMT test news */
   CmdHIST,    /* History            */
   CmdSHELL,   /* Shell command      */
   CmdSLEEP,   /* Sleep seconds      */
   CmdPAUSE,   /* Pause keyboard     */
   CmdATOMS,   /* Atom list commands */

   CmdVER,     /* Get versions */
   CmdCHNGE,   /* Change editor to next program */
   CmdCD,      /* Change working directory */
   CmdDEBUG,   /* Get Set debug mode */

   CmdMEMIO,   /* Raw memory IO          */

#ifdef CTR_PCI
   CmdCNFIO,   /* Plx9030 Configuration registers edit */
   CmdLOCIO,   /* Plx9030 Local Configuration registers edit */

   CmdPLXRC,   /* Reconfigure Plx9030 from 93lc56B */
   CmdREMAP,   /* Rebuild PCI tree and reinstall driver */

   CmdEPMED,   /* 93lc56B EEPROM edit PLX registers */
   CmdEPMRD,   /* Read 93lc56B EEPROM */
   CmdEPMWR,   /* Write to 93lc56B EEPROM */
   CmdEPMXX,   /* Wipe the 93lc56b EEPROM clean */
   CmdEPMRF,   /* Read file containing 93lc56B EEPROM data */
   CmdEPMWF,   /* Write file containing 93lc56B EEPROM data */
#endif

#ifdef CTR_VME
   CmdOBYTE,   /* Get/Set Output VME byte on the P2 connector */
#endif

   CmdHPSED,   /* Edit CERN HPTDC Setup registers */
   CmdHPSIO,   /* Write/Read HPTDC setup */
   CmdHPSRF,   /* Read file containing HPTDC setup */
   CmdHPSWF,   /* Write HPTDC setup to file */

   CmdHPTRS,   /* Read Status from HPTDC chip */

   CmdHPCED,   /* Edit CERN HPTDC control registers */
   CmdHPCIO,   /* Write/Read HPTDC control */
   CmdHPCRF,   /* Read file containing HPTDC control */
   CmdHPCWF,   /* Write HPTDC control to file */

   CmdJTAG,    /* Reload VHDL bit stream accross Jtag */
   CmdALLJTAG, /* Reload all outdated modules with new VHDL */

   CmdMODULE,  /* Module selection */
   CmdNEXT,    /* Next module */
   CmdPREV,    /* Previous module */

   CmdLSCL,    /* List clients */
   CmdRESET,   /* Reset module */
   CmdSTATUS,  /* Get status */
   CmdUTC,     /* Get/Set UTC time */
   CmdWINT,    /* Wait for interrupt */
   CmdSINT,    /* Simulate an interrupt */
   CmdLAT,     /* Measure driver latency */
   CmdQFL,     /* Queue flag on off */
   CmdTMO,     /* Timeout control */
   CmdCLK,     /* Launch X-Window clock */
   CmdLKM,     /* Launch TimLookat */

   CmdACT,     /* Get and set actions */
   CmdLACT,    /* List actions */
   CmdPTIM,    /* Edit PTIM objects */
   CmdCTIM,    /* Edit CTIM objects */
   CmdCTMRF,   /* Read file of CTIM bindings and install them */
   CmdPTMRF,   /* Read file of PTIM bindings and install them */
   CmdPTMWF,   /* Write file of all defined PTIM bindings */
   CmdINIT,    /* Read all CTIM, PTIM and HPTDC files from standard path */

   CmdCH,      /* Set channel */
   CmdNC,      /* Next channel */
   CmdPC,      /* Previous channel */

   CmdENB,     /* GetSet event reception enable */
   CmdIND,     /* Get/Set input delay */
   CmdCNF,     /* Edit a counter configuration */
   CmdCHIS,    /* Counter history */
   CmdCBID,    /* Get/Set module cable ID */
   CmdPLL,     /* Edit Phase Locked Loop */
   CmdUPLL,    /* Set UTC PLL On Off */
   CmdDAC,     /* Set the PLL DAC value directly */
   CmdPLOT,    /* Plot the PLL */
   CmdRTG,     /* Read telegram */
   CmdVID,     /* Launch telegram video program */
   CmdEHIS,    /* Event history */
   CmdDBHIS,   /* Set Event History debug mode */
   CmdRCM,     /* Set remote counter command */
   CmdREM,     /* Get/Set Counter remote flag */
   CmdOMSK,    /* Get/Set Counter output mask */
   CmdMEMTST,  /* Performs an automatic RAM test */
   CmdAQLOG,   /* Log acquisitions between 2 events */

   CmdGRERS,   /* Get reception error log */
   CmdGIOS,    /* Get IO input values */
   CmdGBID,    /* Get 64-bit board ID */

   CmdCMDS } CmdId;

typedef struct {
   CmdId  Id;
   char  *Name;
   char  *Help;
   char  *Optns;
   int  (*Proc)(); } Cmd;

static Cmd cmds[CmdCMDS] = {

   { CmdNOCM,   "???",    "Illegal command"          ,""                   ,Illegal },

   { CmdQUIT,    "q" ,    "Quit test program"        ,""                   ,Quit  },
   { CmdHELP,    "h" ,    "Help on commands"         ,""                   ,Help  },
   { CmdNEWS,    "news",  "Show GMT test news"       ,""                   ,News  },
   { CmdHIST,    "his",   "History"                  ,""                   ,History},
   { CmdSHELL,   "sh",    "Shell command"            ,"UnixCmd"            ,Shell },
   { CmdSLEEP,   "s" ,    "Sleep seconds"            ,"Seconds"            ,Sleep },
   { CmdPAUSE,   "z" ,    "Pause keyboard"           ,""                   ,Pause },
   { CmdATOMS,   "a" ,    "Atom list commands"       ,""                   ,Atoms },

   { CmdVER,     "ver",   "Get versions"             ,""                   ,GetVersion },
   { CmdCHNGE,   "ce",    "Change text editor used"  ,""                   ,ChangeEditor },
   { CmdCD,      "cd",    "Change working directory" ,""                   ,ChangeDirectory },
   { CmdDEBUG,   "deb",   "Get/Set driver debug mode","1|2|4/0"            ,SwDeb },

   { CmdMEMIO,   "rio",   "Raw memory IO"            ,"Address"            ,MemIo},

#ifdef CTR_PCI
   { CmdCNFIO,   "cio",   "Plx9030 Config IO"        ,"Address"            ,ConfigIo},
   { CmdLOCIO,   "lio",   "Plx9030 Local Config IO"  ,"Address"            ,LocalIo},

   { CmdPLXRC,   "plxrc", "Reconfig Plx from EPROM"  ,""                   ,PlxReConfig},
   { CmdREMAP,   "remap", "Remap CTRP after config"  ,""                   ,Remap},

   { CmdEPMED,   "eped",  "EPROM: Edit image"            ,""               ,EpromEdit},
   { CmdEPMRD,   "eprd",  "EPROM: Read image from chip"  ,""               ,EpromRead},
   { CmdEPMWR,   "epwr",  "EPROM: Write image to chip"   ,""               ,EpromWrite},
   { CmdEPMXX,   "epxx",  "EPROM: Wipe chip clean"       ,""               ,EpromErase},
   { CmdEPMRF,   "eprf",  "EPROM: Read image file"       ,"FileName"       ,EpromReadFile},
   { CmdEPMWF,   "epwf",  "EPROM: Write image file"      ,"FileName"       ,EpromWriteFile},
#endif

#ifdef CTR_VME
   { CmdOBYTE,   "oby",   "Get/Set Output P2 VME byte"   ,"0..8"           ,GetSetOByte},
#endif

   { CmdHPSED,   "hpsed", "HPTDC: Edit Setup registers"  ,"RegNum"         ,HptdcSetupEdit},
   { CmdHPSIO,   "hpsio", "HPTDC: Write/Read Setup"      ,""               ,HptdcSetupIo},
   { CmdHPSRF,   "hpsrf", "HPTDC: Read Setup file"       ,"FileName"       ,HptdcSetupReadFile},
   { CmdHPSWF,   "hpswf", "HPTDC: Write Setup file"      ,"FileName"       ,HptdcSetupWriteFile},

   { CmdHPTRS,   "hprs",  "HPTDC: Read Status from chip" ,""               ,HptdcStatusRead},

   { CmdHPCED,   "hpced", "HPTDC: Edit Control registers","RegNum"         ,HptdcControlEdit},
   { CmdHPCIO,   "hpcio", "HPTDC: Write/Read Control"    ,""               ,HptdcControlIo},
   { CmdHPCRF,   "hpcrf", "HPTDC: Read Control file"     ,"FileName"       ,HptdcControlReadFile},
   { CmdHPCWF,   "hpcwf", "HPTDC: Write Control file"    ,"FileName"       ,HptdcControlWriteFile},

   { CmdJTAG,    "jtag",  "Jtag, VHDL reload"         ,"FileName"          ,ReloadJtag},
   { CmdALLJTAG, "Xjtag", "Check/Update VHDL modules" ,"U(update)"         ,ReloadAllJtag},

   { CmdMODULE,  "mo",    "Select/Show modules"      ,"Module"             ,Module },
   { CmdNEXT,    "nm",    "Next module"              ,""                   ,NextModule },
   { CmdPREV,    "pm",    "Previous module"          ,""                   ,PrevModule },

   { CmdLSCL,    "cl",    "List clients"             ,""                   ,ListClients },
   { CmdRESET,   "reset", "Reset module"             ,""                   ,Reset },
   { CmdSTATUS,  "rst",   "Get status"               ,""                   ,GetStatus },
   { CmdUTC,     "utc",   "Get/Set UTC time"         ,"1"                  ,GetSetUtc },
   { CmdWINT,    "wi",    "Wait for interrupt"       ,"P<n>|C<n>|Msk|?"    ,WaitInterrupt },
   { CmdSINT,    "si",    "Simulate interrupt"       ,"T<n>P<n>|C<n>|Msk|?",SimulateInterrupt },
   { CmdLAT,     "lat",   "Measure driver latency"   ,"<trials>"           ,GetLatency },
   { CmdQFL,     "qf",    "Queue flag on off"        ,"1/0"                ,GetSetQue },
   { CmdTMO,     "tmo",   "Timeout control"          ,"Timeout"            ,GetSetTmo },
   { CmdCLK,     "clk",   "Launch CtrClock"          ,""                   ,LaunchClock },
   { CmdLKM,     "lkm",   "Launch TimLookat"         ,"C/P/H Id"           ,LaunchLookat },

   { CmdACT,     "ea",    "Edit actions"             ,"Act,Count"          ,GetSetActions },
   { CmdLACT,    "la",    "List actions"             ,"Act,Count"          ,ListActions },
   { CmdPTIM,    "ptm",   "Edit PTIM objects"        ,"Ptim Id"            ,GetSetPtim },
   { CmdCTIM,    "ctm",   "Edit CTIM objects"        ,"Ctim Id"            ,GetSetCtim },
   { CmdCTMRF,   "ctmr",  "Get CTIM objects from Tgv",""                   ,CtimRead },
   { CmdPTMRF,   "ptmrf", "Install PTIM objects"     ,"FileName"           ,PtimReadFile },
   { CmdPTMWF,   "ptmwf", "Save all PTIM objects"    ,"FileName"           ,PtimWriteFile },
   { CmdINIT,    "load",  "Load all standard objects","1/Hptdc alone"      ,Init },
   { CmdCH,      "ch",    "Get/Set counter"          ,"Ch"                 ,GetSetChannel },
   { CmdNC,      "nch",   "Next counter"             ,""                   ,NextChannel },
   { CmdPC,      "pch",   "Previous counter"         ,""                   ,PrevChannel },
   { CmdENB,     "enb",   "GetSet reception enable"  ,"1/0"                ,GetSetEnable },
   { CmdIND,     "ind",   "Get/Set input delay"      ,"Delay"              ,GetSetInputDelay },
   { CmdCNF,     "cnf",   "Edit counter config"      ,""                   ,GetSetConfig },
   { CmdCHIS,    "chis",  "Show Counter history"     ,""                   ,GetCounterHistory },
   { CmdCBID,    "cbid",  "Get/Set module cable ID"  ,"[id]"               ,GetSetCableId },

   { CmdPLL,     "pll",   "Edit Phase Locked Loop"   ,"0/Ki|Kp|Nav|Phas"      ,GetSetPll },
   { CmdUPLL,    "upll",  "Set UTC PLL On Off"       ,"1/0"                   ,SetBrutalPll },
   { CmdDAC,     "dac",   "Get/Set PLL DAC value"    ,"[-]<value>"            ,GetSetDac },
   { CmdPLOT,    "plot",  "Set PLL and plot curves"  ,"0/Pnts|Ki|Kp|Nav|Phas" ,PlotPll },

   { CmdRTG,     "rtg",   "Read telegram"            ,"Mch"                ,GetTelegram },
   { CmdVID,     "vid",   "Launch telegram video"    ,"Mch"                ,LaunchVideo },
   { CmdEHIS,    "reh",   "Show Event history"       ,"Count"              ,GetEventHistory },
   { CmdDBHIS,   "deh",   "Event History debug mode" ,"0/1"                ,SetDebHis },

   { CmdRCM,     "rcm",   "Remote counter command"   ,"Cmd"                ,SetRemoteCmd },
   { CmdREM,     "rem",   "Get/Set counter remote"   ,"1/0"                ,GetSetRemote },
   { CmdOMSK,    "omsk",  "Get/Set outmask, polarity","Mask,1(-ve)/0(+ve)" ,GetSetOutMask },
   { CmdMEMTST,  "memtst","Memory test"              ,""                   ,MemoryTest },
   { CmdAQLOG,   "aqlog", "Log aqns over interval"   ,"[StCtm,[EnCtm]]"    ,AqLog },
   { CmdGRERS,   "grel",  "Get reception error log"  ,""                   ,RecpErrs},
   { CmdGIOS,    "gio",   "Get IO input values"      ,""                   ,IoStatus},
   { CmdGBID,    "bid",   "Get 64-bit board ID"      ,""                   ,GetBoardId}
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
   { OprNOOP, "?"  ,"???     Not an operator"       },
   { OprNE,   "#"  ,"Test:   Not equal"             },
   { OprEQ,   "="  ,"Test:   Equal"                 },
   { OprGT,   ">"  ,"Test:   Greater than"          },
   { OprGE,   ">=" ,"Test:   Greater than or equal" },
   { OprLT,   "<"  ,"Test:   Less than"             },
   { OprLE,   "<=" ,"Test:   Less than or equal"    },
   { OprAS,   ":=" ,"Assign: Becomes equal"         },
   { OprPL,   "+"  ,"Arith:  Add"                   },
   { OprMI,   "-"  ,"Arith:  Subtract"              },
   { OprTI,   "*"  ,"Arith:  Multiply"              },
   { OprDI,   "/"  ,"Arith:  Divide"                },
   { OprAND,  "&"  ,"Bits:   AND"                   },
   { OprOR,   "!"  ,"Bits:   OR"                    },
   { OprXOR,  "!!" ,"Bits:   XOR"                   },
   { OprNOT,  "##" ,"Bits:   Ones Compliment"       },
   { OprNEG,  "#-" ,"Arith:  Twos compliment"       },
   { OprLSH,  "<<" ,"Bits:   Left shift"            },
   { OprRSH,  ">>" ,"Bits:   Right shift"           },
   { OprINC,  "++" ,"Arith:  Increment"             },
   { OprDECR, "--" ,"Arith:  Decrement"             },
   { OprPOP,  ";"  ,"Stack:  POP"                   },
   { OprSTM,  "->" ,"Stack:  PUSH"                  } };

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

#ifndef True
#define True 1
#define False 0
#endif
