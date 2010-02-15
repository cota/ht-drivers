/**************************************************************************/
/* Command line stuff                                                     */
/**************************************************************************/

int Illegal();              /* llegal command */

int Quit();                 /* Quit test program  */
int Help();                 /* Help on commands */
int News();                 /* Show GMT test news */
int History();              /* History */
int Shell();                /* Shell command */
int Sleep();                /* Sleep seconds */
int Pause();                /* Pause keyboard */
int Atoms();                /* Atom list commands */

int ChangeEditor();         /* Switch text editor */
int ChangeDirectory();      /* Change configuration path */

int GetSetSwDeb();          /* Get/Set driver debug level */
int GetVersion();           /* Get all version data */
int GetSetQue();            /* Get/Set the queue flag */
int GetSetModule();         /* Get/Set the working module */
int NextModule();           /* Select the next module */
int PrevModule();           /* Select the previous module */
int GetSetCableId();        /* Get/Set the cable ID */
int GetSetTmo();            /* Get/Set the timeout value */
int WaitInterrupt();        /* Connect and wait for an interrupt */
int SimulateInterrupt();    /* Simulate an interrupt */
int ListClients();          /* List connected clients */
int GetSetEnable();         /* Get/Set event output enable */
int Reset();                /* Reset the module */
int GetSetUtcSending();     /* Get/Set UTC sending */
int GetSetExt1Khz();        /* Get/Set 1KHz external clock selection */
int GetSetOutDelay();       /* Get/Set the output delay */
int GetSetSyncPeriod();     /* Get/Set the sync period */
int GetSetUtc();            /* Get/Set the UTC time */
int GetStatus();            /* Get the module status */
int SendEvent();            /* Send and event out now */

int GetSetTask();           /* Get/Set the working task */
int NextTask();             /* Select the next task */
int PrevTask();             /* Select the previous task */

int StartTasks();           /* Start tasks */
int StopTasks();            /* Stop tasks */
int ContTask();             /* Continue tasks */
int GetRunningTasks();      /* Get the list of running tasks */
int SaveRunningTasks();     /* Save all running tasks and registers */

int GetTcb();               /* Get the working tasks TCB */
int GetSetLa();             /* Get/Set working tasks load address */
int GetSetIc();             /* Get/Set working tasks instruction count */
int GetSetPs();             /* Get/Set working tasks PC Start address */
int GetSetPo();             /* Get/Set working tasks PC Offset */
int GetSetPc();             /* Get/Set working tasks PC */
int GetSetTaskName();       /* Get/Set working tasks Name */
int EditLreg();             /* Edit the working tasks local register */

int EditGreg();             /* Edit global registers */

int GetProgram();           /* Read program from working module */
int SetProgram();           /* Write program to working module */
int GetTaskProgram();       /* Get program for a given task */
int Asm();                  /* Edit and assemble program */
int DisAsm();               /* Disassemble program */
int EmuProg();              /* Emulate program object */

int CompileTable();         /* Compile an event table */
int FilterTable();          /* Filter the CTR event history by table */

int ReloadJtag();           /* Upload VHDL bit stream */
int MemIo();                /* Raw IO */

int CtrTest();              /* Ctr hardware test program */
int TimTest();              /* TimLib test program */

int LaunchVideo();          /* Launch Video displays */

int ListFiles();            /* Execute ls -l in CWD */

int MakeCtfTable();         /* Make a CTF timing table */

static void IErr();

/* Jtag backend to public code */

FILE *jtag_input;

void setPort(short p, short val);
void readByte();
unsigned char readTDOBit();
void waitTime();
void pulseClock();

typedef enum {

   CmdNOCM,                 /* llegal command */

   CmdQUIT,                 /* Quit test program  */
   CmdHELP,                 /* Help on commands */
   CmdNEWS,                 /* Show GMT test news */
   CmdHIST,                 /* History */
   CmdSHELL,                /* Shell command */
   CmdSLEEP,                /* Sleep seconds */
   CmdPAUSE,                /* Pause keyboard */
   CmdATOMS,                /* Atom list commands */

   CmdCE,                   /* Switch text editor */
   CmdCD,                   /* Change configuration path */
   CmdVID,                  /* Launch Video displays */

   CmdDEBUG,                /* Get/Set driver debug level */
   CmdVER,                  /* Get all version data */
   CmdQFL,                  /* Get/Set the queue flag */
   CmdMOD,                  /* Get/Set the working module */
   CmdNMOD,                 /* Select the next module */
   CmdPMOD,                 /* Select the previous module */
   CmdCABLE,                /* Get/Set the cable ID */
   CmdTMO,                  /* Get/Set the timeout value */
   CmdWAIT,                 /* Connect and wait for an interrupt */
   CmdSIM,                  /* Simulate an interrupt */
   CmdCLIST,                /* List connected clients */
   CmdENABLE,               /* Get/Set event output enable */
   CmdRESET,                /* Reset the module */
   CmdUTCSND,               /* Get/Set UTC sending */
   CmdEX1KHZ,               /* Get/Set 1KHz external clock selection */
   CmdODLY,                 /* Get/Set the output delay */
   CmdSYNCP,                /* Get/Set the sync period */
   CmdUTC,                  /* Get/Set the UTC time */
   CmdSTATUS,               /* Get the module status */
   CmdSEND,                 /* Send and event out now */

   CmdTASK,                 /* Get/Set the working task */
   CmdNTASK,                /* Select the next task */
   CmdPTASK,                /* Select the previous task */

   CmdSTART,                /* Start tasks */
   CmdSTOP,                 /* Stop tasks */
   CmdCONT,                 /* Continue tasks */
   CmdRUNING,               /* Get the list of running tasks */
   CmdSAVERUN,              /* Save running tasks and registers */

   CmdTCB,                  /* Get the working tasks TCB */
   CmdLA,                   /* Get/Set task LoadAddrs */
   CmdIC,                   /* Get/Set task InstCount */
   CmdPCS,                  /* Get/Set task PcStart */
   CmdPCO,                  /* Get/Set task PcOffset */
   CmdPC,                   /* Get/Set task PC */
   CmdTNAME,                /* Get/Set task name */
   CmdLREG,                 /* Get/Set the working tasks local register */

   CmdGREG,                 /* Get/Set global registers */

   CmdREADP,                /* Read program from working module */
   CmdWRITEP,               /* Write program to working module */
   CmdREADT,                /* Read program for task */
   CmdASM,                  /* Edit/Assemble program */
   CmdDASM,                 /* Disassemble program */
   CmdEMUL,                 /* Emulate program binary */
   CmdCOMPLE,               /* Compile event table */
   CmdFILTER,               /* Filter CTR event history by table */
   CmdCTFT,                 /* Build a CTF timing table */

   CmdJTAG,                 /* Upload VHDL bit stream */
   CmdRIO,                  /* Raw IO */

   CmdCTRT,                 /* Launch ctrtest hardware test program */
   CmdTIMT,                 /* Launch timtest timlib test program */

   CmdLSF,                  /* List files */

   CmdCMDS } CmdId;

typedef struct {
   CmdId  Id;
   char  *Name;
   char  *Help;
   char  *Optns;
   int  (*Proc)(); } Cmd;

static Cmd cmds[CmdCMDS] = {

   { CmdNOCM,   "???",    "Illegal command"          ,""                   ,Illegal },

   { CmdQUIT,    "q" ,    "Quit test program"        ,""                   ,Quit    },
   { CmdHELP,    "h" ,    "Help on commands"         ,""                   ,Help    },
   { CmdNEWS,    "news",  "Show GMT test news"       ,""                   ,News    },
   { CmdHIST,    "his",   "History"                  ,""                   ,History },
   { CmdSHELL,   "sh",    "Shell command"            ,"UnixCmd"            ,Shell   },
   { CmdSLEEP,   "s" ,    "Sleep seconds"            ,"Seconds"            ,Sleep   },
   { CmdPAUSE,   "z" ,    "Pause keyboard"           ,""                   ,Pause   },
   { CmdATOMS,   "a" ,    "Atom list commands"       ,""                   ,Atoms   },

   { CmdCE,      "ce",    "Change text editor used"  ,""                   ,ChangeEditor    },
   { CmdCD,      "cd",    "Change working directory" ,""                   ,ChangeDirectory },
   { CmdVID,     "vid",   "Launch display programs"  ,"Machine"            ,LaunchVideo     },

   { CmdDEBUG,   "deb",   "Get/Set driver debug mode","1|2|4/0"            ,GetSetSwDeb       },
   { CmdVER,     "ver",   "Get versions"             ,""                   ,GetVersion        },
   { CmdQFL,     "qf",    "Queue flag on off"        ,"1/0"                ,GetSetQue         },
   { CmdMOD,     "mo",    "Select/Show modules"      ,"Module"             ,GetSetModule      },
   { CmdNMOD,    "nm",    "Next module"              ,""                   ,NextModule        },
   { CmdPMOD,    "pm",    "Previous module"          ,""                   ,PrevModule        },
   { CmdCABLE,   "id",    "Get/Set cable ID"         ,"Id"                 ,GetSetCableId     },
   { CmdTMO,     "tmo",   "Timeout control"          ,"Timeout"            ,GetSetTmo         },
   { CmdWAIT,    "wi",    "Wait for interrupt"       ,"Msk|?"              ,WaitInterrupt     },
   { CmdSIM,     "si",    "Simulate interrupt"       ,"Msk|?"              ,SimulateInterrupt },
   { CmdCLIST,   "cl",    "List clients"             ,""                   ,ListClients       },
   { CmdENABLE,  "eno",   "GetSet output enable"     ,"1/0"                ,GetSetEnable      },
   { CmdRESET,   "reset", "Reset module"             ,""                   ,Reset             },
   { CmdUTCSND,  "autc",  "Get/Set auto UTC sending" ,"1/0"                ,GetSetUtcSending  },
   { CmdEX1KHZ,  "khz",   "Khz clock source"         ,"1/0"                ,GetSetExt1Khz     },
   { CmdODLY,    "dly",   "Output delay"             ,"Delay"              ,GetSetOutDelay    },
   { CmdSYNCP,   "bp",    "Get/Set basic-period"     ,"Period"             ,GetSetSyncPeriod  },
   { CmdUTC,     "utc",   "Get/Set UTC time"         ,"1"                  ,GetSetUtc         },
   { CmdSTATUS,  "rst",   "Get status"               ,""                   ,GetStatus         },
   { CmdSEND,    "sev",   "Send and event out now"   ,"Frame"              ,SendEvent         },

   { CmdTASK,    "tn",    "Get/Set the working task" ,"Task"               ,GetSetTask },
   { CmdNTASK,   "nt",    "Select the next task"     ,""                   ,NextTask   },
   { CmdPTASK,   "pt",    "Select the previous task" ,""                   ,PrevTask   },

   { CmdSTART,   "start", "Start tasks"              ,"T|Msk"              ,StartTasks      },
   { CmdSTOP,    "stop",  "Stop tasks"               ,"T|Msk"              ,StopTasks       },
   { CmdCONT,    "cont",  "Continue tasks"           ,"T|Msk"              ,ContTask        },
   { CmdRUNING,  "rnt",   "Get list of running tasks",""                   ,GetRunningTasks },
   { CmdSAVERUN, "srnt",  "Save running tasks & regs","FileNmae"           ,SaveRunningTasks},

   { CmdTCB,     "tcb",   "Get the task TCB"         ,"Task"               ,GetTcb   },
   { CmdLA,      "la",    "Get/Set task LoadAddrs"   ,"Address"            ,GetSetLa },
   { CmdIC,      "ic",    "Get/Set task InstCount"   ,"Count"              ,GetSetIc },
   { CmdPCS,     "pcs",   "Get/Set task PcStart"     ,"Address"            ,GetSetPs },
   { CmdPCO,     "pco",   "Get/Set task PcOffset"    ,"Address"            ,GetSetPo },
   { CmdPC,      "pc",    "Get/Set task PC"          ,"Address"            ,GetSetPc },
   { CmdTNAME,   "tname", "Get/Set task Name"        ,"Name|clear"         ,GetSetTaskName },
   { CmdLREG,    "lr",    "Edit task registers"      ,"RegId"              ,EditLreg },

   { CmdGREG,    "gr",    "Edit global registers"    ,"RegId"              ,EditGreg },

   { CmdREADP,   "rp",    "Read program"             ,"Address,Count"         ,GetProgram     },
   { CmdWRITEP,  "wp",    "Write program"            ,"Task,Address,FileName" ,SetProgram     },
   { CmdREADT,   "rtp",   "Read program for task"    ,"Task"                  ,GetTaskProgram },
   { CmdASM,     "as",    "Edit/Assemble program"    ,"FileName"              ,Asm            },
   { CmdDASM,    "ds",    "Disassemble program"      ,"FileName"              ,DisAsm         },
   { CmdEMUL,    "em",    "Emulate program binary"   ,"FileName"              ,EmuProg        },
   { CmdCOMPLE,  "ct",    "Compile event table"      ,"FileName"              ,CompileTable   },
   { CmdFILTER,  "ft",    "Look for table in history","FileName"              ,FilterTable    },
   { CmdCTFT,    "ctf",   "Build a CTF timing table" ,"Prod,Meas,Intv"        ,MakeCtfTable   },

   { CmdJTAG,    "jtag",  "Jtag, VHDL reload"        ,"FileName"           ,ReloadJtag },
   { CmdRIO,     "rio",   "Raw memory IO"            ,"Address"            ,MemIo      },

   { CmdCTRT,    "ctrt",  "ctr hardware test prog"   ,""                   ,CtrTest  },
   { CmdTIMT,    "timt",  "timtest timlib test prog" ,""                   ,TimTest  },

    { CmdLSF,     "ls",    "List files in CWD"        ,""                   ,ListFiles   }

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

#define MAX_OP_CODE_STRING_LENGTH 8
#define MAX_REGISTER_STRING_LENGTH 8
