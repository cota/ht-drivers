/**
 * @file extest_common.h
 *
 * @brief Common header file for extest's programs
 *
 * @author Copyright (C) 2009 CERN CO/HT Emilio G. Cota
 * 					<emilio.garcia.cota@cern.ch>
 * @author Copyright (C) 2008 CERN CO/HT Yury Georgievskiy
 *					<yury.georgievskiy@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _EXTEST_COMMON_H_
#define _EXTEST_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <types.h>

#include <general_both.h>
#include <list.h>
#include <data_tables.h>
#include <config_data.h>

/* we just want to check if __SKEL_EXTEST is defined */
#if IS_SKEL == y
#define __SKEL_EXTEST
#else
#undef __SKEL_EXTEST
#endif

#ifdef __SKEL_EXTEST
#include <skeluser.h>
#include <skel.h>
#endif /* __SKEL_EXTEST */

#define MAX_ARG_COUNT   32	//!< maximum command line arguments
#define MAX_ARG_LENGTH  128	//!< max characters per argument
#define F_CLOSED        (-1)	//!< file closed

#define WHITE_ON_BLACK	"\033[40m\033[1;37m"
#define DEFAULT_COLOR         "\033[m"

//! Operator ID's
typedef enum {
	OprNOOP,
	OprNE,
	OprEQ,
	OprGT,
	OprGE,
	OprLT,
	OprLE,
	OprAS,
	OprPL,
	OprMI,
	OprTI,
	OprDI,
	OprAND,
	OprOR,
	OprXOR,
	OprNOT,
	OprNEG,
	OprLSH,
	OprRSH,
	OprINC,
	OprDECR,
	OprPOP,
	OprSTM,
	OprOPRS
} oprid_t;

struct operator {
	oprid_t id;	  //!< operator ID (one of @ref oprid_t)
	char	name[16]; //!< Human form
	char    help[32]; //!< help string
};

//!< atom types
typedef enum {
	Separator    = 0,  //!< [\t\n\r ,]
	Operator     = 1,  //!< [!#&*+-/:;<=>?]
	Open         = 2,  //!< [(]
	Close        = 3,  //!< [)]
	Comment      = 4,  //!< [%]
	Numeric      = 5,  //!< [0-9]
	Alpha        = 6,  //!< [a-zA-Z_]
	Open_index   = 7,  //!< [\[]
	Close_index  = 8,  //!< [\]]
	Illegal_char = 9,  //!< all the rest in the ASCII table
	Terminator   = 10, //!< [@\0]
	Bit          = 11, //!< [.]
	String	     = 12  //!< ["]
} atom_t;
#define Quotes String

/*! atom container
 * Example: 'oprd min 5 0x400' is formed by 4 atoms
 */
struct atom {
	unsigned int 	pos;  //!< position if @av_type is @Alpha or @Operator
	int      	val;  //!< value if @av_type is @Numeric
	atom_t   	type; //!< atom type
	char     	text[MAX_ARG_LENGTH]; //!< string representation
	unsigned int cmd_id; //!< command id (might be built-in or user-defined)
	oprid_t  	oid; //!< operator id (if any)
};

/*! @name Default commands for every test program
 *
 * Some default commands use ioctl calls to access the driver.
 * The user should provide these ioctl numbers to use these commands.
 * If a particular ioctl number is not provided, its command won't be issued.
 * IOCTL numbers can be obtained using @b debugfs
 */
enum def_cmd_id {
	CmdNOCM = 0,	//!< llegal command
	_CmdSRVDBG,	//!< Service entry (for debugging purposes)
	CmdQUIT,	//!< Quit test program
	CmdHELP,	//!< Help on commands
	CmdATOMS,	//!< Atom list commands
	CmdHIST,	//!< History
	CmdSLEEP,	//!< Sleep
	CmdSHELL,	//!< Shell command

	CmdCONNECT,	//!< Connect/show connections	<=> ioctl
	CmdCLIENTS,	//!< Show list of clients	<=> ioctl
	CmdDEBUG,	//!< Get Set debug mode		<=> ioctl
	CmdENABLE,	//!< Enable/Disable module	<=> ioctl
	CmdMAPS,	//!< Print module mappings	<=> ioctl
	CmdMODULE,	//!< Module selection		<=> ioctl
	CmdNEXT,	//!< Select next module		<=> ioctl
	CmdQUEUE,	//!< Get/Set queue flag		<=> ioctl
	CmdRAWIO,	//!< Raw memory IO		<=> ioctl
	CmdRESET,	//!< Reset module		<=> ioctl
	CmdSINTR,	//!< Simulate interrupt		<=> ioctl
	CmdSTATUS,	//!< Get status			<=> ioctl
	CmdTIMEOUT,	//!< Get/Set timeout		<=> ioctl
	CmdVER,		//!< Get versions		<=> ioctl
	CmdWINTR,	//!< Wait for interrupt		<=> ioctl
	CmdJTAG,	//!< JTAG VHDL files into FPGA	<=> 4 ioctls
	CmdUSR		//!< first available command for the user
};

//!< Command description
struct cmd_desc {
	int  valid;	//!< show command to the user? (1 - yes, 0 - no)
	int  id;	//!< id (user-defined && @def_cmd_id)
	char *name;	//!< spelling
	char *help;	//!< short help string
	char *opt;	//!< options (if any)
	int  comp;	//!< amount of compulsory options
	int  (*handle)(struct cmd_desc *, struct atom *); //!< handler
	int  pa;	//!< number of arguments to be passed to the handler
	struct list_head list; //!< linked list
};

DECLARE_GLOB_LIST_HEAD(tcmd_glob_list); //!< test command list

//!< Global test data */
struct tst_data {
	int mod; //!< current active module (starting from one)
	int ma;  //!< amount of controlled modules
	int fd;  //!< driver node descriptor (F_CLOSED if not opened)
	InsLibModlDesc descr[MAX_DEV_SUPP]; //!< module description
};

//!< General Test data
_DECL struct tst_data tst_glob_d _INIT({ .fd = F_CLOSED });

#endif /* _EXTEST_COMMON_H_ */
