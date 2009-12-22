/**
 * @file cmd_handlers.h
 *
 * @brief extest's handlers' function declarations
 *
 * @author Copyright (C) 2009 CERN CO/HT
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _EXTEST_HANDLERS_H_
#define _EXTEST_HANDLERS_H_

#include <extest.h>

/*! @name Test Default command vectors.
 */
//@{
int hndl_illegal	(struct cmd_desc*, struct atom*);
int _hndl_dbg		(struct cmd_desc*, struct atom*);
int hndl_quit		(struct cmd_desc*, struct atom*);
int hndl_help		(struct cmd_desc*, struct atom*);
int hndl_history	(struct cmd_desc*, struct atom*);
int hndl_atoms		(struct cmd_desc*, struct atom*);
int hndl_sleep		(struct cmd_desc*, struct atom*);
int hndl_shell		(struct cmd_desc*, struct atom*);

int hndl_module		(struct cmd_desc*, struct atom*);
int hndl_nextmodule	(struct cmd_desc*, struct atom*);
int hndl_getversion	(struct cmd_desc*, struct atom*);
int hndl_swdeb		(struct cmd_desc*, struct atom*);
int hndl_rawio		(struct cmd_desc*, struct atom*);
int hndl_maps		(struct cmd_desc*, struct atom*);
int hndl_timeout	(struct cmd_desc*, struct atom*);
int hndl_queue		(struct cmd_desc*, struct atom*);
int hndl_clients	(struct cmd_desc*, struct atom*);
int hndl_connect	(struct cmd_desc*, struct atom*);
int hndl_enable		(struct cmd_desc*, struct atom*);
int hndl_reset		(struct cmd_desc*, struct atom*);
int hndl_getstatus	(struct cmd_desc*, struct atom*);
int hndl_waitintr	(struct cmd_desc*, struct atom*);
int hndl_simintr	(struct cmd_desc*, struct atom*);
#if 0
int hndl_jtag_vhdl	(struct cmd_desc*, struct atom*);
#endif
//@}

#endif /* _EXTEST_HANDLERS_H_ */
