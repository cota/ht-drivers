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
int GetSetClkDivMod(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetClkDiv(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetClkEge(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetClkTerm(struct cmd_desc *cmddint, struct atom *atoms);

int GetSetTrg(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetTrgEge(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetTrgTerm(struct cmd_desc *cmddint, struct atom *atoms);

int GetState(struct cmd_desc *cmddint, struct atom *atoms);
int SetCommand(struct cmd_desc *cmddint, struct atom *atoms);
int GetAdc(struct cmd_desc *cmddint, struct atom *atoms);

int GetSetChan(struct cmd_desc *cmddint, struct atom *atoms);
int NextChan(struct cmd_desc *cmddint, struct atom *atoms);

int ReadFile(struct cmd_desc *cmddint, struct atom *atoms);
int WriteFile(struct cmd_desc *cmddint, struct atom *atoms);
int PlotSamp(struct cmd_desc *cmddint, struct atom *atoms);

int ReadSamp(struct cmd_desc *cmddint, struct atom *atoms);
int PrintSamp(struct cmd_desc *cmddint, struct atom *atoms);

int GetSetPostSamples(struct cmd_desc *cmddint, struct atom *atoms);

//@}

/*! @name specific test commands
 */
//@{

typedef enum _tag_cmd_id {

	CmdCLK    = CmdUSR,
	CmdCLKDM,
	CmdCLKDV,
	CmdCLKEG,
	CmdCLKTM,

	CmdTRG,
	CmdTRGEG,
	CmdTRGTM,

	CmdSTATE,
	CmdCMD,
	CmdADC,
	CmdSAMP,

	CmdCH,
	CmdNCH,

	CmdRF,
	CmdWF,
	CmdPLOT,

	CmdRSAMP,
	CmdPSAMP,

	CmdPOST,
	CmdBURST,

	CmdLAST		//!< last one
} cmd_id_t;

//@}

#endif /* _VD80_TST_H_INCLUDE_ */
