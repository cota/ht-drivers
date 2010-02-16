/* ================================================================================= */
/* CERN High Prescision Time to Digital Convertor (HPTDC) chip, definitions file.    */
/* On the CTR the HPTDC is accessed accross a JTAG interface implemented as a        */
/* register in the CTR memory map.                                                   */
/* Julian Lewis AB/CO/HT CERN Julian.Lewis@cern.ch 4th/Feb/2004                      */
/* ================================================================================= */

/* Refere to HPTDC Version 2.1 Prelimanary July 2002 manual by J.Christansen CERN/EP */
/* For the HPTDC chip version 1.2                                                    */
/* See http://micdigital.web.cern.ch/micdigital/hptdc/hptdc_manual_ver2.1.pdf        */
/* ================================================================================= */

#ifndef HPTDC
#define HPTDC

/* ID of HPTDC for version 1.1 and 1.2 of the HPTDC chip */

#define HptdcVERSION 0x8470DACE

/* JTAG interface for HPTDC */

#define HptdcJTAG_TRST 0x01  /* Reset line */
#define HptdcJTAG_TCK  0x02  /* Clock */
#define HptdcJTAG_TDI  0x04  /* Data In */
#define HptdcJTAG_TDO  0x08  /* Data Out */
#define HptdcJTAG_TMS  0x10  /* Mode State */

typedef enum {
   HptdcCmdEXTEST,  /* 0000: Boundary scan for test of inter-chip connections */
   HptdcCmdIDCODE,  /* 0001: Scan out the HPTDC chips identification code */
   HptdcCmdSAMPLE,  /* 0010: Sample of all chip pins via boundary scan registers */
   HptdcCmdINTEST,  /* 0011: Using boundary scan registers ti test chip itself */
   HptdcCmdBIST,    /* 0100: Memory self test (production testing) */
   HptdcCmdSCAN,    /* 0101: Scan of all internal flip-flops (production testing) */
   HptdcCmd0110,    /* 0110: Not used */
   HptdcCmdREADOUT, /* 0111: Read of HPTDC output data */
   HptdcCmdSETUP,   /* 1000: Load & Read of setup data */
   HptdcCmdCONTROL, /* 1001: Load & Read of control information */
   HptdcCmdSTATUS,  /* 1010: Read of status information */
   HptdcCmdCORETEST,/* 1011: Access to internal coretest scan registers (production testing) */
   HptdcCmd1100,    /* 1100: Not used */
   HptdcCmd1101,    /* 1101: Not used */
   HptdcCmd1110,    /* 1110: Not used */
   HptdcCmdBYPASS,  /* 1111: Put the HPTDC into bypass state */
   HptdcCmdCOMMANDS /* Number of HPTDC commands */
 } HptdcCmd;

#define HptdcCmdMASK 0x0000000F
#define HptdcCmdBITS 4

#define HptdcSetupREGISTERS 112
#define HptdcControlREGISTERS 5
#define HptdcStatusREGISTERS 14

/* Each HPTDC register is a series of no more than 32-bits, the entire setup is */
/* then composed of 113 of these registers. */

typedef struct {
   unsigned int   Value;
   unsigned short EndBit;
   unsigned short StartBit;
 } HptdcRegisterVlaue;

#endif
