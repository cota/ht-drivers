/**
 * @file vd80Tst.c
 *
 * @brief
 *
 * @author
 *
 * @date Created on 02/03/2009
 *
 * @version
 */
#include <extest.h>
#include "vd80Tst.h"

int use_builtin_cmds = 1;
char xmlfile[128] = "../VD80.xml";

/*! @name specific test commands and their description
 */
//@{
struct cmd_desc user_cmds[] = {

	{ 1, CmdCLK,   "clk",   "Get/Set clock",             "clock",  1, GetSetClk },
	{ 1, CmdCLKDM, "clkdm", "Get/Set clock divide mode", "mode",   1, GetSetClkDivMod },
	{ 1, CmdCLKDV, "clkdv", "Get/Set clock diviser",     "value",  1, GetSetClkDiv },
	{ 1, CmdCLKEG, "clkeg", "Get/Set clock edge",        "edge",   1, GetSetClkEge },
	{ 1, CmdCLKTM, "clktm", "Get/Set clock termination", "term",   1, GetSetClkTerm },

	{ 1, CmdTRG,   "trg",   "Get/Set trig",              "trig",   1, GetSetTrg },
	{ 1, CmdTRGEG, "trgeg", "Get/Set trig edge",         "edge",   1, GetSetTrgEge },
	{ 1, CmdTRGTM, "trgtm", "Get/Set trig termination",  "term",   1, GetSetTrgTerm },

	{ 1, CmdSTATE, "state", "Get state",                 "",       0, GetState },
	{ 1, CmdCMD,   "cmd",   "Set Command",               "cmnd",   1, SetCommand },
	{ 1, CmdADC,   "adc",   "Get ADC value",             "",       0, GetAdc },
	{ 1, CmdSAMP,  "smp",   "Get Sample",                "",       0, GetAdc },

	{ 1, CmdCH,    "ch",    "Get/Set channel",           "chan",   1, GetSetChan },
	{ 1, CmdNCH,   "nch",   "Next channel",              "",       0, NextChan },

	{ 1, CmdRF,    "rf",    "Read file",                 "file",   1, ReadFile },
	{ 1, CmdWF,    "wf",    "Write file",                "file",   1, WriteFile },
	{ 1, CmdPLOT,  "plt",   "Plot",                      "",       1, PlotSamp },

	{ 1, CmdRSAMP, "rsmp",  "Read sample",               "",       0, ReadSamp },
	{ 1, CmdPSAMP, "psmp",  "Print sample",              "",       0, PrintSamp },

	{ 1, CmdPOST,  "post",  "Get/Set post samples",      "samples", 1, GetSetPostSamples },

	{ 0, } /* list termination */
};
//@}

int main(int argc, char *argv[], char *envp[])
{
	return extest_init(argc, argv);
}


