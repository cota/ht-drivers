/**
 * @file vd80Tst.h
 *
 * @brief
 *
 * @author
 *
 * @date Created on 02/03/2009
 *
 * @version
 */
#ifndef _VD80_TST_H_INCLUDE_
#define _VD80_TST_H_INCLUDE_

/*! @name specific test command vectors.
 */
//@{

int GetSetClk(struct cmd_desc *cmddint, struct atom *atoms);
int hndl_burst_mode(struct cmd_desc *, struct atom *);

//@}

/*! @name specific test commands
 */
//@{

typedef enum _tag_cmd_id {

	CmdCLK    = CmdUSR,

	CmdBURST,

	CmdLAST		//!< last one
} cmd_id_t;

//@}

#endif /* _VD80_TST_H_INCLUDE_ */
