/**
 * @file extest.h
 *
 * @brief Extensible Test program's public API
 *
 * This header declares the API for the user that intends to add more
 * commands on top of the built-in ones.
 *
 * @author Copyright (C) 2009 CERN CO/HT Emilio G. Cota
 * 					<emilio.garcia.cota@cern.ch>
 * @author Copyright (C) 2008 CERN CO/HT Yury Georgievskiy
 *					<yury.georgievskiy@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _EXTEST_H_
#define _EXTEST_H_

#include <extest_common.h>

/*! @name extest's public API
 */
//@{
//!< Device Node open File Descriptor
#define _DNFD (tst_glob_d.fd)

//!< User wants verbose command help
#define VERBOSE_HELP (-1)

//! Test program Error return codes
typedef enum _tag_tst_prog_error {
	TST_NO_ERR,		//!< cool
	TST_ERR_NOT_IMPL,	//!< function not implemented
	TST_ERR_NO_PARAM,	//!< compulsory parameter is not provided
	TST_ERR_WRONG_ARG,	//!< wrong command argument
	TST_ERR_ARG_O_S,	//!< argument overflow/shortcoming
	TST_ERR_NO_MODULE,	//!< active module not set
	TST_ERR_IOCTL,		//!< ioctl call fails
	TST_ERR_SYSCALL,	//!< system call fails
	TST_ERR_NO_VECTOR,	//!< vector for this call is not provided
	TST_ERR_LAST		//!< error idx
} tst_prg_err_t;

int  	do_yes_no	(char *question, char *extra);
int 	compulsory_ok	(struct cmd_desc *cmdd);
int 	extest_init	(int argc, char **argv);
int	is_last_atom	(struct atom *atom);
//@}

#endif /* _EXTEST_H_ */
