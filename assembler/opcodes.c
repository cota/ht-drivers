
/* Describe the MTT CTG module instruction Set */


typedef enum {

   HALT = 0x00,
   NOOP = 0x4d,
   INT  = 0xC1,

   WEQV = 0x91, WEQR = 0x11,
   WRLV = 0x92, WRLR = 0x12,
   WORV = 0x93, WORR = 0x13,

   MOVV = 0xe2, MOVR = 0x62,

   MOVIR= 0x64,
   MOVRI= 0x65,

   ADDV = 0xe6, ADDR = 0x66,
   SUBV = 0xe7, SUBR = 0x67,
   LORV = 0xe8, LORR = 0x68,
   ANDV = 0xe9, ANDR = 0x69,
   XORV = 0xea, XORR = 0x6a,
   LSV  = 0xeb, LSR  = 0x6b,
   RSV  = 0xec, RSR  = 0x6c,

   WDOG = 0x34,
   JMP  = 0x85,
   BEQ  = 0x86,
   BNE  = 0x87,
   BLT  = 0x88,
   BGT  = 0x89,
   BLE  = 0x8a,
   BGE  = 0x8b,
   BCR  = 0x8c,

   OPCODES = 36

 } IdNumber;

OpCode InstructionSet[OPCODES] = {

/* { Name    ,Comment                   ,Number  ,Src1       ,Src2       ,Dest       } */

   { "HALT"  ,"Stop program"            ,HALT    ,AtNothing  ,AtNothing  ,AtNothing  },
   { "NOOP"  ,"No operation"            ,NOOP    ,AtNothing  ,AtNothing  ,AtNothing  },

   { "INT"   ,"Interrupt[1..16]"        ,INT     ,AtLiteral  ,AtNothing  ,AtNothing  },
   { "WEQV"  ,"Wait value equal"        ,WEQV    ,AtLiteral  ,AtRegister ,AtNothing  },
   { "WRLV"  ,"Wait value relative"     ,WRLV    ,AtLiteral  ,AtRegister ,AtNothing  },
   { "WORV"  ,"Wait value bit mask"     ,WORV    ,AtLiteral  ,AtRegister ,AtNothing  },

   { "WEQR"  ,"Wait register equal"     ,WEQR    ,AtRegister ,AtRegister ,AtNothing  },
   { "WRLR"  ,"Wait register relative"  ,WRLR    ,AtRegister ,AtRegister ,AtNothing  },
   { "WORR"  ,"Wait register bit mask"  ,WORR    ,AtRegister ,AtRegister ,AtNothing  },

   { "MOVV"  ,"Load register"           ,MOVV    ,AtLiteral  ,AtNothing  ,AtRegister },

   { "MOVR"  ,"Move register"           ,MOVR    ,AtRegister ,AtNothing  ,AtRegister },

   { "MOVIR" ,"Move Index register"     ,MOVIR   ,AtNothing  ,AtNothing  ,AtRegister },
   { "MOVRI" ,"Move register Index"     ,MOVRI   ,AtRegister ,AtNothing  ,AtNothing  },

   { "ADDR"  ,"Add register"            ,ADDR    ,AtRegister ,AtRegister ,AtRegister },
   { "SUBR"  ,"Subtract register"       ,SUBR    ,AtRegister ,AtRegister ,AtRegister },
   { "LORR"  ,"Or register"             ,LORR    ,AtRegister ,AtRegister ,AtRegister },
   { "ANDR"  ,"And register"            ,ANDR    ,AtRegister ,AtRegister ,AtRegister },
   { "XORR"  ,"ExclusiveOr register"    ,XORR    ,AtRegister ,AtRegister ,AtRegister },

   { "LSR"   ,"LeftShift register"      ,LSR     ,AtRegister ,AtRegister ,AtRegister },
   { "RSR"   ,"RightShift register"     ,RSR     ,AtRegister ,AtRegister ,AtRegister },

   { "ADDV"  ,"Add value"               ,ADDV    ,AtLiteral  ,AtRegister ,AtRegister },
   { "SUBV"  ,"Subtract value"          ,SUBV    ,AtLiteral  ,AtRegister ,AtRegister },
   { "LORV"  ,"Or value"                ,LORV    ,AtLiteral  ,AtRegister ,AtRegister },
   { "ANDV"  ,"And value"               ,ANDV    ,AtLiteral  ,AtRegister ,AtRegister },
   { "XORV"  ,"ExclusiveOr value"       ,XORV    ,AtLiteral  ,AtRegister ,AtRegister },
   { "LSV"   ,"LeftShift value"         ,LSV     ,AtLiteral  ,AtRegister ,AtRegister },
   { "RSV"   ,"RightShift value"        ,RSV     ,AtLiteral  ,AtRegister ,AtRegister },

   { "WDOG"  ,"Watch Dod"               ,WDOG    ,AtAddress  ,AtNothing  ,AtNothing  },
   { "JMP"   ,"Jump"                    ,JMP     ,AtAddress  ,AtNothing  ,AtNothing  },
   { "BEQ"   ,"Branch Equal"            ,BEQ     ,AtAddress  ,AtNothing  ,AtNothing  },
   { "BNE"   ,"Branch NotEqual"         ,BNE     ,AtAddress  ,AtNothing  ,AtNothing  },
   { "BLT"   ,"Branch LessThan"         ,BLT     ,AtAddress  ,AtNothing  ,AtNothing  },
   { "BGT"   ,"Branch GreaterThan"      ,BGT     ,AtAddress  ,AtNothing  ,AtNothing  },
   { "BLE"   ,"Branch LessThanEqual"    ,BLE     ,AtAddress  ,AtNothing  ,AtNothing  },
   { "BGE"   ,"Branch GreaterThanEqual" ,BGE     ,AtAddress  ,AtNothing  ,AtNothing  },
   { "BCR"   ,"Branch Carry"            ,BCR     ,AtAddress  ,AtNothing  ,AtNothing  } };

/* Describe the MTT CTG module register Set */

#define REGISTERS 256
#define REGNAMES 9
#define GLOBALS 224
#define LOCALS 32
#define GLOBAL_REG 1
#define LOCAL_REG 2

typedef struct {
   char *Name;
   char *Comment;
   int   Start;
   int   End;
   int   Offset;
   int   LorG;      /* 1=>Global 2=>Local */
 } RegisterDsc;

RegisterDsc Regs[REGNAMES] = {

   { "GR"    ,"Global"                  ,0   ,217 ,  1, 1},
   { "MSMR"  ,"MilliSecond modulo"      ,218 ,218 ,  0, 1},
   { "TSYNC" ,"Time from sync"          ,219 ,219 ,  0, 1},
   { "VMEP2" ,"VME P2 input"            ,220 ,220 ,  0, 1},
   { "EVOUT" ,"Event output"            ,221 ,221 ,  0, 1},
   { "MSFR"  ,"Free running milliSecond",222 ,222 ,  0, 1},
   { "SYNCFR","Free running Sync time"  ,223 ,223 ,  0, 1},
   { "INDEX" ,"Index address"           ,224 ,224 ,  0, 2},
   { "LR"    ,"Local"                   ,225 ,255 ,  1, 2} };
