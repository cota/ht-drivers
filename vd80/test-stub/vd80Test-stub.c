/**
 * @file vd80libTst.c
 *
 * @brief
 *
 * <long description>
 *
 * @author Copyright (C) 2009 CERN CO/HT Julian Lewis
 * 					<julian.lewis@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sched.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>

#include <extest.h>
#include <stub/StubLib.h>
#include <stub/StubApi.h>

#include "vd80libTst.h"
#include "vd80libCmds.h"

/*
 * Note that the library's header needs to be stored in /mymodule/lib to
 * be found by the makefile.
 */

int use_builtin_cmds = 0; /* no built-in skel's commands */
char xmlfile[128] = "../driver/vd80.xml";

/*! @name specific test commands and their description
 */
//@{
struct cmd_desc user_cmds[] = {

   { 1, LibOPEN,   "oph",    "Open library handle",      "device,name",      2, OpenHandle        },
   { 1, LibCLOSE,  "clh",    "Close library handle",     "",                 0, CloseHandle       },
   { 1, LibSHOWH,  "swh",    "Show open handles",        "",                 0, ShowHandle        },
   { 1, LibNXTHAN, "nxh",    "Use the next open handle", "",                 0, NextHandle        },
   { 1, LibSYMB,   "sym",    "Get symbol from handle",   "func,param",       2, GetSymbol         },
   { 1, LibRESET,  "reset",  "Reset module",             "",                 0, ResetMod          },
   { 1, LibDEBUG,  "deb",    "Set debug mask bits",      "debug",            1, SetDebug          },
   { 1, LibADC,    "adc",    "Get the ADC value",        "",                 0, GetAdcValue       },
   { 1, LibCALIB,  "cal",    "Calibrate channel",        "low,hig",          2, CalibrateChn      },
   { 1, LibSTATE,  "state",  "Get channel state",        "",                 0, GetState          },
   { 1, LibSTATUS, "status", "Get module status",        "",                 0, GetStatus         },
   { 1, LibDATSZE, "dsz",    "Get datum size",           "",                 0, GetDatumSize      },
   { 1, LibMOD,    "mod",    "Get/Set current module",   "module",           1, GetSetMod         },
   { 1, LibCHN,    "chn",    "Get/Set current channel",  "channel",          1, GetSetChn         },
   { 1, LibNXTMOD, "nxmod",  "Select next module",       "",                 0, NextMod           },
   { 1, LibNXTCHN, "nxchn",  "Select next channel",      "",                 0, NextChn           },
   { 1, LibVER,    "ver",    "Get versions",             "",                 0, GetVersion        },
   { 1, LibCCLK,   "cclk",   "Get/Set clock source",     "source,edge,term", 3, GetSetClk         },
   { 1, LibCLKDV,  "clkdv",  "Get/Set clock divisor",    "divisor",          1, GetSetClkDiv      },
   { 1, LibCTRG,   "ctrg",   "Get/Set trigger source",   "source,edge,term", 3, GetSetTrg         },
   { 1, LibATRG,   "atrg",   "Get/Set analog trigger",   "control,level",    2, GetSetAnalogTrg   },
   { 1, LibCNRG,   "cnrg",   "Get/Set trigger config",   "delay,min pre trg",2, GetSetTrgConfig   },
   { 1, LibCSTP,   "cstp",   "Get/Set stop source",      "source,edge,term", 3, GetSetStop        },
   { 1, LibABUF,   "buf",    "Configure sample buffer",  "size,post",        2, GetSetBufferSize  },
   { 1, LibSTART,  "str",    "Start acquisition",        "",                 0, StrtAcquisition   },
   { 1, LibTRIG,   "trg",    "Trigger acquisition",      "",                 0, TrigAcquisition   },
   { 1, LibSTOP,   "stp",    "Stop acquisition",         "",                 0, StopAcquisition   },
   { 1, LibCON,    "con",    "Connect to interrupt",     "mask",             1, Connect           },
   { 1, LibTMO,    "tmo",    "Get/Set timeout",          "timeout",          1, GetSetTimeout     },
   { 1, LibQFL,    "qfl",    "Get/Set queueflag",        "flag",             1, GetSetQueueFlag   },
   { 1, LibWI,     "wi",     "Wait interrupt",           "mask",             1, WaitInterrupt     },
   { 1, LibSI,     "si",     "Simulate interrupt",       "mask",             1, SimulateInterrupt },
   { 1, LibRF,     "rf",     "Read sample file",         "file",             1, ReadFile          },
   { 1, LibWF,     "wf",     "Write sample file",        "file",             1, WriteFile         },
   { 1, LibPLOT,   "plt",    "Plot sample file",         "",                 0, PlotSamp          },
   { 1, LibPRINT,  "prt",    "Print sample file",        "",                 0, PrintSamp         },
   { 1, LibREAD,   "rdb",    "Read sample buffer",       "",                 0, ReadSamp          },

   { 0, }
};
//@}

int main(int argc, char *argv[], char *envp[]) {
   return extest_init(argc, argv);
}
