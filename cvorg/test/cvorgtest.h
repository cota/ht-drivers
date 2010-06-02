/**
 * cvorgtest.h
 *
 * CVORG driver's test program header
 * Copyright (c) 2009 Emilio G. Cota <cota@braap.org>
 *
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _CVORG_TEST_H_
#define _CVORG_TEST_H_

#include <sys/ioctl.h>

#include <cvorg.h>
#include <skeluser_ioctl.h>
#include <ad9516.h>

#define CVORG_STATUS_REG	0x40
#define CVORG_CONFIG_REG	0x44
#define CVORG_SAMPFREQ_REG	0x28

#define CVORG_MAX_NR_SEQUENCES	26

#define TEST_WV_MAXLEN		(1024*1024)

enum _tag_cmd_id {
	CmdPLAY = CmdUSR,
	CmdCH,
	CmdLEN,
	CmdOUTOFF,
	CmdINPOL,
	CmdCHANSTAT,
	CmdTRIGGER,
	CmdPLL,
	CmdSAMPFREQ,
	CmdRSF,
	CmdSRAM,
	CmdLOCK,
	CmdUNLOCK,
	CmdMODE,
	CmdMEMTEST,
	CmdADCREAD,
	CmdDAC,
	CmdOUTGAIN,
	CmdSTARTEN,
	CmdPCB,
	CmdLAST
};

enum cvorg_testwv {
	TWV_SQUARE = 0,
	TWV_SINE,
	TWV_RAMP
};

enum cvorg_memtest_type {
	CVORG_MEMTEST_FIXED,
	CVORG_MEMTEST_INCREMENT,
	CVORG_MEMTEST_READONLY
};

/**
 * struct cvorg_memtest_op - descriptor of a memtest operation
 * @start:	start index
 * @end:	end index
 * @skip:	skip index increment
 * @pattern:	pattern to write
 * @type:	access type
 */
struct cvorg_memtest_op {
	unsigned int		start;
	unsigned int		end;
	unsigned int		skip;
	uint32_t		pattern;
	enum cvorg_memtest_type	type;
};

#define CVORG_MEMTEST_MAX_ERRORS	10

#define CVORG_CHAN2_OFFSET	0x200
#define CVORG_SRAMADDR		0x4c
#define CVORG_SRAMDATA		0x50

#endif /* _CVORG_TEST_H_ */
