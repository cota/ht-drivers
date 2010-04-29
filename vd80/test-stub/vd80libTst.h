/**
 * @file vd80libTst.h
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
#ifndef _VD80_LIB_TST_H_INCLUDE_
#define _VD80_LIB_TST_H_INCLUDE_

int OpenHandle(struct cmd_desc *cmddint, struct atom *atoms);
int CloseHandle(struct cmd_desc *cmddint, struct atom *atoms);
int ShowHandle(struct cmd_desc *cmddint, struct atom *atoms);
int NextHandle(struct cmd_desc *cmddint, struct atom *atoms);
int GetSymbol(struct cmd_desc *cmddint, struct atom *atoms);
int SetDebug(struct cmd_desc *cmddint, struct atom *atoms);
int ResetMod(struct cmd_desc *cmddint, struct atom *atoms);
int GetAdcValue(struct cmd_desc *cmddint, struct atom *atoms);
int CalibrateChn(struct cmd_desc *cmddint, struct atom *atoms);
int GetState(struct cmd_desc *cmddint, struct atom *atoms);
int GetStatus(struct cmd_desc *cmddint, struct atom *atoms);
int GetDatumSize(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetMod(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetChn(struct cmd_desc *cmddint, struct atom *atoms);
int NextChn(struct cmd_desc *cmddint, struct atom *atoms);
int NextMod(struct cmd_desc *cmddint, struct atom *atoms);
int GetVersion(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetClk(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetClkDiv(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetTrg(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetStop(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetBufferSize(struct cmd_desc *cmddint, struct atom *atoms);
int StrtAcquisition(struct cmd_desc *cmddint, struct atom *atoms);
int TrigAcquisition(struct cmd_desc *cmddint, struct atom *atoms);
int StopAcquisition(struct cmd_desc *cmddint, struct atom *atoms);
int Connect(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetTimeout(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetQueueFlag(struct cmd_desc *cmddint, struct atom *atoms);
int WaitInterrupt(struct cmd_desc *cmddint, struct atom *atoms);
int SimulateInterrupt(struct cmd_desc *cmddint, struct atom *atoms);
int ReadFile(struct cmd_desc *cmddint, struct atom *atoms);
int WriteFile(struct cmd_desc *cmddint, struct atom *atoms);
int PlotSamp(struct cmd_desc *cmddint, struct atom *atoms);
int ReadSamp(struct cmd_desc *cmddint, struct atom *atoms);
int PrintSamp(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetAnalogTrg(struct cmd_desc *cmddint, struct atom *atoms);
int GetSetTrgConfig(struct cmd_desc *cmddint, struct atom *atoms);
int MsSleep(struct cmd_desc *cmddint, struct atom *atoms);

/*! @name specific test commands
 */
//@{

typedef enum _tag_cmd_id {

	LibOPEN    = CmdUSR, /* OpenHandle        Open library handle */
	LibCLOSE,            /* CloseHandle       Close library handle */
	LibSHOWH,            /* ShowHandle        Show open handles */
	LibNXTHAN,           /* NextHandle        Use the next open handle */
	LibSYMB,             /* GetSymbol         Get symbol from handle */
	LibDEBUG,            /* SetDebug          Set debug mask bits */
	LibRESET,            /* ResetMod          Reset module */
	LibADC,              /* GetAdcValue       Get the ADC value */
	LibCALIB,            /* CalibrateChn      Calibrate channel */
	LibSTATE,            /* GetState          Get channel state */
	LibSTATUS,           /* GetStatus         Get module status */
	LibDATSZE,           /* GetDatumSize      Get datum size */
	LibMOD,              /* GetSetMod         Get/Set current module */
	LibCHN,              /* GetSetChn         Get/Set current channel */
	LibNXTMOD,           /* NextMod           Select next module*/
	LibNXTCHN,           /* NextChn           Select next channel */
	LibVER,              /* GetVersion        Get versions */
	LibCCLK,             /* GetSetClk         Get/Set clock source */
	LibCLKDV,            /* GetSetClkDiv      Get/Set clock divisor */
	LibCTRG,             /* GetSetTrg         Get/Set trigger source */
	LibATRG,             /* GetSetAnalogTrg   Get/Set analog trigger */
	LibCNRG,             /* GetSetTrgConfig   Get/Set trigger config */
	LibCSTP,             /* GetSetStop        Get/Set stop source */
	LibABUF,             /* GetSetBufferSize  Configure sample buffer */
	LibSTART,            /* StrtAcquisition   Start acquisition */
	LibTRIG,             /* TrigAcquisition   Trigger acquisition */
	LibSTOP,             /* StopAcquisition   Stop acquisition */
	LibCON,              /* Connect           Connect to interrupt */
	LibTMO,              /* GetSetTimeout     Get/Set timeout */
	LibQFL,              /* GetSetQueueFlag   Get/Set queueflag */
	LibWI,               /* WaitInterrupt     Wait interrupt */
	LibSI,               /* SimulateInterrupt Simulate interrupt */
	LibRF,               /* ReadFile          Read sample file */
	LibWF,               /* WriteFile         Write sample file */
	LibPLOT,             /* PlotSamp          Plot sample file */
	LibPRINT,            /* PrintSamp         Print sample file */
	LibREAD,             /* ReadSamp          Read sample buffer */
	LibMSLEEP,           /* Sleep             Milli Second sleep */

	CmdLAST
} cmd_id_t;

//@}

#endif /* _VD80_LIB_TST_H_INCLUDE_ */
