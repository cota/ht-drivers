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
#include "vmod12e16.h"

int use_builtin_cmds = 1;
char xmlfile[128] = "../VD80.xml";

/*! @name specific test commands and their description
 */
//@{
struct cmd_desc user_cmds[] = {

/* cobas: no specific commands at this moment, null test
	{ 1, CmdCLK,   "clk",   "Get/Set clock",             "clock",  1, GetSetClk },
	{ 1, CmdBURST, "burst", "burst-mode Write reg",      "reg  am", 0, hndl_burst_mode },
 */

	{ 0, } /* list termination */
};
//@}

int main(int argc, char *argv[], char *envp[])
{
	return extest_init(argc, argv);
}


