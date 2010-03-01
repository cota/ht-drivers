/**
 * @file nulldrvrTst.h
 *
 * @brief null driver test program's header
 *
 * @author Emilio G. Cota
 *
 * @date Created on 12/02/2009
 *
 * Based on mil1553 Linux driver's test program by Yury Georgievskiy
 *
 * @version
 */
#ifndef _NULL_TST_H_INCLUDE_
#define _NULL_TST_H_INCLUDE_

/*! @name specific test command vectors.
 */
//@{
int hndl_put(struct cmd_desc *cmdd, struct atom *atoms);
//@}

/*! @name specific test commands
 */
//@{
typedef enum _tag_cmd_id {
	CmdPUT = CmdUSR,	//!< first command you want to define
	CmdLAST		//!< last one
} cmd_id_t;
//@}

extern int hndl_rawwrite(struct cmd_desc *cmdd, struct atom *atoms);

#endif /* _NULL_TST_H_INCLUDE_ */
