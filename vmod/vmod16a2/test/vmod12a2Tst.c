/**
 * @file vmodTst.c
 *
 * @brief null Driver test program
 *
 * @author Juan David Gonzalez Cobas
 *
 * @date Created on 05/10/2009
 *
 * Based on mil1553 Linux driver's test program by Yury Georgievskiy
 *
 * @version
 */
#include <extest.h>
#include <general_usr.h>
#include <sys/ioctl.h>
#include <skel.h>
#include "../driver/skeluser_ioctl.h"
#include "vmod12a2.h"
#include "vmod12a2Tst.h"

int  use_builtin_cmds = 1;
char xmlfile[128] = "../object_NULL/config.xml";

/*! @name specific test commands and their description
 */
//@{
struct cmd_desc user_cmds[] = {
	/* 
	   { 1, CmdMYFEATURE, "myfeat", "My feature", "option (1-10)", 1,
	  						hndl_myfeat },
	 */
	{ 1, CmdPUT, "put", "put value to DAC", "slot channel value", 3, hndl_put },
	{ 0, } /* list termination */
};
//@}

/**
 * @brief handler for a pretty useless feature
 *
 * @param cmdd:  command description
 * @param atoms: command atoms list
 *
 * @return index of the next atom to parse in the current command
 * @return -TST_ERR_ARG_O_S - wrong parameters passed
 */
/**
 * hndl_rawwrite - 'RawWrite' handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_put(struct cmd_desc* cmdd, struct atom *atoms)
{
	int slot, channel, value, arg;
	int err;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("put\n"
			"%s slot channel value\n"
			"\tslot: physical slot in carrier (0-1)\n"
			"\tchannel: output channel (0-1)\n"
			"\tvalue: 12-bit digital value\n"
			"\t\t(0-10V or -10V-10V depending on hw config)\n" ,
			cmdd->name);
		goto out;
	}
	if (!compulsory_ok(cmdd)) {
		printf("Wrong parameter amount\n");
		return -TST_ERR_ARG_O_S;
	}
	if (((atoms + 1)->type | (atoms + 2)->type | (atoms + 3)->type)
		!= Numeric) {
		printf("Invalid argument (parameters have to be numeric.)\n");
		return -TST_ERR_WRONG_ARG;
	}
	slot    = (atoms+1)->val;
	channel = (atoms+2)->val;
	value   = (atoms+3)->val;
	arg	= VMODA12A2_WRITECHANNEL_ARG(slot, channel, value);

	err = ioctl(_DNFD, WriteChannel, &arg);
	return err;
out:
	return 1;
}

int main(int argc, char *argv[], char *envp[])
{
	return extest_init(argc, argv);
}


