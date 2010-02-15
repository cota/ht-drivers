
#include "libmtt.h"

/* Describe the MTT CTG module instruction Set */

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

   { "MOVIR" ,"Move Index register"     ,MOVIR   ,AtRegister ,AtNothing  ,AtRegister },
   { "MOVRI" ,"Move register Index"     ,MOVRI   ,AtRegister ,AtRegister ,AtNothing  },

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

RegisterDsc Regs[REGNAMES] = {

   { "GR"      ,"Global"                     ,GAdrGR01    ,LastGAdrGR  ,  1, GLOBAL_REG},
   { "OUTPUTS" ,"Control of outputs"         ,GAdrOUTPUTS ,GAdrOUTPUTS ,  0, GLOBAL_REG},
   { "EVIN"    ,"Event input"                ,GAdrEVIN    ,GAdrEVIN    ,  0, GLOBAL_REG},
   { "EVINCNT" ,"Event input counter"        ,GAdrEVINCNT ,GAdrEVINCNT ,  0, GLOBAL_REG},
   { "EVOUTCNT","Event output counter"       ,GAdrEVOUTCNT,GAdrEVOUTCNT,  0, GLOBAL_REG},
   { "EVOUTFB" ,"Event output feed back"     ,GAdrEVOUTFB ,GAdrEVOUTFB ,  0, GLOBAL_REG},
   { "EVOUTL"  ,"Event output low priority"  ,GAdrEVOUTL  ,GAdrEVOUTL  ,  0, GLOBAL_REG},
   { "MSMR"    ,"MilliSecond modulo"         ,GAdrMSMR    ,GAdrMSMR    ,  0, GLOBAL_REG},
   { "TSYNC"   ,"Time from sync"             ,GAdrTSYNC   ,GAdrTSYNC   ,  0, GLOBAL_REG},
   { "VMEP2"   ,"VME P2 input"               ,GAdrVMEP2   ,GAdrVMEP2   ,  0, GLOBAL_REG},
   { "EVOUTH"  ,"Event output high priority" ,GAdrEVOUTH  ,GAdrEVOUTH  ,  0, GLOBAL_REG},
   { "MSFR"    ,"Free running milliSecond"   ,GAdrMSFR    ,GAdrMSFR    ,  0, GLOBAL_REG},
   { "SYNCFR"  ,"Free running Sync time"     ,GAdrSYNCFR  ,GAdrSYNCFR  ,  0, GLOBAL_REG},
   { "INDEX"   ,"Index address"              ,LAdrINDEX   ,LAdrINDEX   ,  0, LOCAL_REG },
   { "LR"      ,"Local"                      ,LAdrLR01    ,LastLAdrLR  ,  1, LOCAL_REG }
   };
