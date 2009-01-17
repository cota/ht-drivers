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

int RegisterCallback();
int GetAllNodeIds();
int GetNodeName();
int GetNodeId();
int GetAllTableIds();
int GetTableName();
int GetTableId();
int GetTableDesc();
int Wait();
int Poll();
int SendTable();
int RecvTable();
int SendMessage();
int CheckTables();
int LoadTable();
int SaveTable();
int EditSharedMemory();
int Init();

int ShowDaemonEventLog();
int ShowDaemonEventHistory();
int ClearDaemonEventLog();
int KillDaemon();

typedef enum {

   CmdNOCM,    /* llegal command */

   CmdQUIT,    /* Quit test program  */
   CmdHELP,    /* Help on commands   */
   CmdHIST,    /* History            */
   CmdSHELL,   /* Shell command      */
   CmdSLEEP,   /* Sleep seconds      */
   CmdPAUSE,   /* Pause keyboard     */
   CmdATOMS,   /* Atom list commands */

   CmdRGCB,    /* Register/Unregister callback */

   CmdNDS,     /* Show all nodes */
   CmdTDS,

   CmdWAIT,
   CmdPOLL,

   CmdSNDT,
   CmdRCVT,

   CmdSMS,

   CmdCHKT,

   CmdLDT,     /* Load table from disc */
   CmdSVT,     /* Save table to dsic */

   CmdESHM,    /* Edit table in shared memory */

   CmdSDEL,
   CmdSDEH,
   CmdCDEL,
   CmdKILD,
   CmdINIT,    /* Execute initialize protocol */

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

   { CmdRGCB,"rgcb","Register callback again"   ,"?|<EvtMask>"            ,RegisterCallback },
   { CmdNDS, "nds" ,"Get all node Ids"          ,""                       ,GetAllNodeIds    },
   { CmdTDS, "tds" ,"Get table description"     ,"<TbIdMsk>"              ,GetTableDesc     },
   { CmdWAIT,"wait","Wait for callback"         ,""                       ,Wait             },
   { CmdPOLL,"poll","Poll for callback"         ,""                       ,Poll             },
   { CmdSNDT,"wsh" ,"Write shmem to xmem"       ,"<TbId>"                 ,SendTable        },
   { CmdRCVT,"rsh" ,"Read shmem from xmem"      ,"<TbId>"                 ,RecvTable        },
   { CmdSMS, "sms" ,"Send message"              ,"?|<Type><NdIdMsk><Data>",SendMessage      },
   { CmdCHKT,"chkt","Check for table updates"   ,""                       ,CheckTables      },
   { CmdLDT, "ldsh","Load shmem table from disc","<TbId>"                 ,LoadTable        },
   { CmdSVT, "svsh","Save shmem table to disc"  ,"<TbId>"                 ,SaveTable        },
   { CmdESHM,"esh" ,"Edit table shared memory"  ,"<TbId>"                 ,EditSharedMemory },
   { CmdSDEL,"sdel","Show daemon event log"     ,""                       ,ShowDaemonEventLog     },
   { CmdSDEH,"sdeh","Show daemon event history" ,"<NdId>"                 ,ShowDaemonEventHistory },
   { CmdCDEL,"cdel","Clear daemon event log"    ,""                       ,ClearDaemonEventLog    },
   { CmdKILD,"kd"  ,"Kill daemons"              ,"<NdIdMsk>"              ,KillDaemon             },
   { CmdINIT,"init","Start Initialize protocol" ,""                       ,Init                   }
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

#define True 1
#define False 0
