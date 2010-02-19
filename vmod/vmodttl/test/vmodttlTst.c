/**
 * @file nullTst.c
 *
 * @brief null Driver test program
 *
 * @author Emilio G. Cota
 *
 * @date Created on 12/02/2009
 *
 * Based on mil1553 Linux driver's test program by Yury Georgievskiy
 *
 * @version
 */
#include <extest.h>
#include "vmodttlTst.h"

int  use_builtin_cmds = 1;
char xmlfile[128] = "../object_NULL/config.xml";

/*! @name specific test commands and their description
 */
//@{
struct cmd_desc user_cmds[] = {
/*	{ 1, CmdMYFEATURE, "myfeat", "My feature", "option (1-10)", 1,
	  						hndl_myfeat },
 */
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
int hndl_myfeat(struct cmd_desc *cmdd, struct atom *atoms)
{
	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("This is a dummy feature\n");
		return(cmdd->pa + 1);
	}
	if (!compulsory_ok(cmdd)) {
		printf("Wrong parameter amount\n");
		return -TST_ERR_ARG_O_S;
	}
	/* whatever(); */
	printf("MyFeature: do whatever here. Returning...\n");
	return cmdd->pa + 1; /* Note: always return this on success */
}

int main(int argc, char *argv[], char *envp[])
{
	return extest_init(argc, argv);
}
