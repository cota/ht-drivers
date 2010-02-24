/**
 * @file extest.c
 *
 * @brief Extensible Test program
 *
 * This is a user-space program to test drivers. It is extensible
 * in the sense that allows the user to add his own commands.
 * There is a set of built-in commands that will either use 'skel'
 * handlers or 'generic' handlers.
 * The idea is to bring together all the common 'glue' needed to have
 * a working test program -- hence shortening developer's time for this task.
 * The program is able to run either interactively or by being piped
 * a set of commands (this is done normally at the machine's startup).
 *
 * @author Copyright (C) 2009 CERN CO/HT Emilio G. Cota
 * 					<emilio.garcia.cota@cern.ch>
 * @author Copyright (C) 2008 CERN CO/HT Yury Georgievskiy
 *					<yury.georgievskiy@cern.ch>
 * @author Copyright (C) 2002-2007 CERN CO/HT Julian Lewis
 * 					<julian.lewis@cern.ch>
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <sys/ioctl.h>
#include <curses.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <libinst.h>

/* local includes */
#include <err.h>
#include <general_usr.h>

/* declare tcmd_glob_list and tst_glob_d (which are in cmd_shell.h) */
#define VAR_DECLS
#include <extest.h>
#include <cmd_handlers.h>


//!< user-defined module-specific test commands.
extern int  use_builtin_cmds;
extern char xmlfile[128];
extern struct cmd_desc user_cmds[];

//!< test program global variables
static char __drivername[64];
static char *__progname;

//!< functions to be used only by standard handlers
static struct cmd_desc *get_cmdd(enum def_cmd_id cmd_id);
static int do_cmd(int idx, struct atom *atoms);
static struct cmd_desc* cmd_is_valid(char*);
static int param_amount(struct atom *atoms);
static void enable_builtin_cmds(void);
static void extest_cleanup();

//!< functions to be used by specific handlers
int get_free_user_handle(int);
void __getchar(char *c);
void print_modules();

//!< these declarations here are required by Lynx
extern int snprintf(char *s, size_t n, const char *format, ...);
extern int tputs(const char *str, int affcnt, int (*putc)(int));
extern char *tgetstr(char *id, char **area);

//!< Description of each operator.
struct operator oprs[OprOPRS] = {
	{
		.id   = OprNOOP,
		.name = "?",
		.help = "???     Not an operator"
	},
	{ OprNE,	"#" ,	"Test:   Not equal"	},
	{ OprEQ,	"=" ,	"Test:   Equal"		},
	{ OprGT,	">" ,	"Test:   Greater than"	},
	{ OprGE,	">=",	"Test:   Greater than or equal"	},
	{ OprLT,	"<" ,	"Test:   Less than"	},
	{ OprLE,	"<=",	"Test:   Less than or equal"	},
	{ OprAS,	":=",	"Assign: Becomes equal"	},
	{ OprPL,	"+" ,	"Arith:  Add"		},
	{ OprMI,	"-" ,	"Arith:  Subtract"	},
	{ OprTI,	"*" ,	"Arith:  Multiply"	},
	{ OprDI,	"/" ,	"Arith:  Divide"	},
	{ OprAND,	"&" ,	"Bits:   AND"		},
	{ OprOR,	"!" ,	"Bits:   OR"		},
	{ OprXOR,	"!!",	"Bits:   XOR"		},
	{ OprNOT,	"##",	"Bits:   One's Complement"	},
	{ OprNEG,	"#-",	"Bits:	 Two's complement"	},
	{ OprLSH,	"<<",	"Bits:   Left shift"	},
	{ OprRSH,	">>",	"Bits:   Right shift"	},
	{ OprINC,	"++",	"Arith:  Increment"	},
	{ OprDECR,	"--",	"Arith:  Decrement"	},
	{ OprPOP,	";" ,	"Stack:  POP"		},
	{ OprSTM,	"->",	"Stack:  PUSH"		}
};

//! We divide the extended ASCII table in subsets of atom types (atom_t)
static char atomhash[256] = {
	10,9,9,9,9,9,9,9,9,0,0,9,9,0,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	0,1,12,1,9,4,1,9,2,3,1,1,0,1,11,1,5,5,5,5,5,5,5,5,5,5,1,1,1,1,1,1,
	10,6,6,6,6,6,6,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6,6,6,6,6,7,9,8,9,6,
	9 ,6,6,6,6,6,6,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6,6,6,6,6,9,9,9,9,9,
	9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9
};

//!< Standard commands definitions
struct cmd_desc def_cmd[CmdUSR] = {
	[CmdNOCM]
	{
		.valid  = 0,
		.id     = CmdNOCM,
		.name   = "???",
		.help   = "Illegal command",
		.opt    = "",
		.comp   = 0,
		.handle = hndl_illegal,
		.list   = LIST_HEAD_INIT(def_cmd[CmdNOCM].list)
	},
	[_CmdSRVDBG]
	{
		0, _CmdSRVDBG, "_dbg_", "debugging", "", 0,
		_hndl_dbg, .list = LIST_HEAD_INIT(def_cmd[_CmdSRVDBG].list)
	},
	[CmdQUIT]
	{
		1, CmdQUIT, "q", "Quit test program", "", 0, hndl_quit,
		.list = LIST_HEAD_INIT(def_cmd[CmdQUIT].list)
	},
	[CmdHELP]
	{
		1, CmdHELP, "h", "Help on commands", "o c", 0, hndl_help,
		.list = LIST_HEAD_INIT(def_cmd[CmdHELP].list)
	},
	[CmdHIST]
	{
		1, CmdHIST, "his", "History", "", 0, hndl_history,
		.list = LIST_HEAD_INIT(def_cmd[CmdHIST].list)
	},
	[CmdATOMS]
	{
		1, CmdATOMS, "a", "Atom list commands", "", 0, hndl_atoms,
		.list = LIST_HEAD_INIT(def_cmd[CmdATOMS].list)
	},
	[CmdSHELL]
	{
		1, CmdSHELL, "sh", "Shell command", "Unix Cmd", 1, hndl_shell,
		.list = LIST_HEAD_INIT(def_cmd[CmdSHELL].list)
	},
	[CmdVER]
	{
		0, CmdVER, "ver", "Get versions", "", 0, hndl_getversion,
		.list = LIST_HEAD_INIT(def_cmd[CmdVER].list)
	},
	[CmdDEBUG]
	{
		0, CmdDEBUG, "deb", "Get/Set driver debug mode", "1|2|4/0", 0,
		hndl_swdeb, .list = LIST_HEAD_INIT(def_cmd[CmdDEBUG].list)
	},
	[CmdRAWIO]
	{
		0, CmdRAWIO, "rio", "Raw memory IO", "Address", 3, hndl_rawio,
		.list = LIST_HEAD_INIT(def_cmd[CmdRAWIO].list)
	},
	[CmdMODULE]
	{
		0, CmdMODULE, "mo", "Select/Show controlled modules", "Module",
		0, hndl_module, .list = LIST_HEAD_INIT(def_cmd[CmdMODULE].list)
	},
	[CmdNEXT]
	{
		0, CmdNEXT, "nm", "Select next module", "", 0,
		hndl_nextmodule, .list = LIST_HEAD_INIT(def_cmd[CmdNEXT].list)
	},
	[CmdMAPS]
	{
		0, CmdMAPS, "maps", "Get mapped addresses information", "", 0,
		hndl_maps, .list = LIST_HEAD_INIT(def_cmd[CmdMAPS].list)
	},
	[CmdTIMEOUT]
	{
		0, CmdTIMEOUT, "tmo", "Get/Set timeout", "timeout [10ms]", 0,
		hndl_timeout, .list = LIST_HEAD_INIT(def_cmd[CmdTIMEOUT].list)
	},
	[CmdQUEUE]
	{
		0, CmdQUEUE, "qf", "Get/Set queue flag", "1/0 (On/Off)", 0,
		hndl_queue, .list = LIST_HEAD_INIT(def_cmd[CmdQUEUE].list)
	},
	[CmdCLIENTS]
	{
		0, CmdCLIENTS, "cl", "List clients", "", 0,
		hndl_clients, .list = LIST_HEAD_INIT(def_cmd[CmdCLIENTS].list)
	},
	[CmdCONNECT]
	{
		0, CmdCONNECT, "con", "Connect", "[Mask [Module]]", 0,
		hndl_connect, .list = LIST_HEAD_INIT(def_cmd[CmdCLIENTS].list)
	},
	[CmdENABLE]
	{
		0, CmdENABLE, "en", "Enable/Disable", "", 1,
		hndl_enable, .list = LIST_HEAD_INIT(def_cmd[CmdENABLE].list)
	},
	[CmdRESET]
	{
		0, CmdRESET, "rst", "Reset module", "", 0, hndl_reset,
		.list = LIST_HEAD_INIT(def_cmd[CmdRESET].list)
	},
	[CmdSTATUS]
	{
		0, CmdSTATUS, "stat", "Get status", "", 0, hndl_getstatus,
		.list = LIST_HEAD_INIT(def_cmd[CmdSTATUS].list)
	},
	[CmdWINTR]
	{
		0, CmdWINTR, "wi", "Wait for interrupt", "", 0, hndl_waitintr,
		.list = LIST_HEAD_INIT(def_cmd[CmdWINTR].list)
	},
	[CmdSINTR]
	{
		0, CmdSINTR, "si", "Simulate interrupt", "", 0, hndl_simintr,
		.list = LIST_HEAD_INIT(def_cmd[CmdSINTR].list)
	},
	[CmdSLEEP]
	{
		0, CmdSLEEP, "s", "Sleep", "seconds", 0, hndl_sleep,
		.list = LIST_HEAD_INIT(def_cmd[CmdSLEEP].list)
	}
#if 0
	[CmdJTAG]
	{
		0, CmdJTAG, "jtag", "VHDL Reload", "*.xsvf file", 1,
		hndl_jtag_vhdl, .list = LIST_HEAD_INIT(def_cmd[CmdJTAG].list)
	}
#endif
};

/**
 * get_atoms - Split the command and its arguments into atoms
 *
 * @param input - command to split
 * @param atoms - buffer to store atoms. [max: @ref MAX_ARG_COUNT elements]
 *
 * @return number of atoms processed
 */
static int get_atoms(char *input, struct atom *atoms)
{
	struct cmd_desc *cmdptr;  /* command description pointer */
	char 		*buf;	  /* goes through input */
	char 		*endptr;  /* marks the end within input of an atom */
	atom_t 	atype;	 /* atom type */
	int 	cnt = 0; /* keeps track of @atoms entries */
	int 	i, j;

	if ( input == NULL ||  *input  == '.')
		return cnt; /* repeat last command */

	bzero((void*)atoms, MAX_ARG_COUNT * sizeof(struct atom));
	buf = input;
	while (1) {
		if (cnt >= MAX_ARG_COUNT - 1) { /* avoid overflow */
			atoms[cnt].type = Terminator;
			break;
		}
		atype = atomhash[(int)*buf];
		switch (atype) {
		case Numeric:
			atoms[cnt].val  = (int)strtoul(buf, &endptr, 0);
			snprintf(atoms[cnt].text, MAX_ARG_LENGTH, "%d",
				 atoms[cnt].val);
			atoms[cnt].type = Numeric;
			buf = endptr;
			cnt++;
			break;
		case Alpha:
			atoms[cnt].pos = (unsigned int)(buf - input);
			j = 0;
			while (atype == Alpha) {
				atoms[cnt].text[j++] = *buf++;
				if (j >= MAX_ARG_LENGTH)
					break;
				atype = atomhash[(int)*buf];
			}
			atoms[cnt].text[j] = '\0';
			atoms[cnt].type    = Alpha;
			atoms[cnt].cmd_id  = CmdNOCM;
			list_for_each_entry(cmdptr, &tcmd_glob_list, list) {
				if (!strcmp(cmdptr->name, atoms[cnt].text))
					atoms[cnt].cmd_id = cmdptr->id;
			}
			cnt++;
			break;
		case Quotes:
			/* Strings are "enclosed between quotes" */
			atoms[cnt].pos = (unsigned int)(buf - input);
			j = 0;
			atype = atomhash[(int)(*(++buf))];
			while (atype != Quotes && atype != Terminator) {
				atoms[cnt].text[j++] = *buf++;
				if (j >= MAX_ARG_LENGTH)
					break;
				atype = atomhash[(int)*buf];
			}
			atoms[cnt].text[j] = '\0';
			atoms[cnt].type    = String;
			buf++; /* discard ending quotes */
			cnt++;
			break;
		case Separator:
			while (atype == Separator)
				atype = atomhash[(int)(*(++buf))];
			break;
		case Comment:
			/* Comments are %enclosed between percent marks% */
			atype = atomhash[(int)(*(++buf))];
			while (atype != Comment) {
				atype = atomhash[(int)(*(buf++))];
				if (atype == Terminator) /* no ending % */
					return cnt;
			}
			break;
		case Operator:
			atoms[cnt].pos = (unsigned int)(buf - input);
			j = 0;
			while (atype == Operator) {
				atoms[cnt].text[j++] = *buf++;
				if (j >= MAX_ARG_LENGTH)
					break;
				atype = atomhash[(int)*buf];
			}
			atoms[cnt].text[j] = '\0';
			atoms[cnt].type = Operator;
			for (i = 0; i < OprOPRS; i++) {
				if (!strcmp(oprs[i].name, atoms[cnt].text)) {
					atoms[cnt].oid = oprs[i].id;
					break;
				}
			}
			cnt++;
			break;
		case Open:
		case Close:
		case Open_index:
		case Close_index:
		case Bit:
			atoms[cnt].type = atype;
			buf++;
			cnt++;
			break;
		case Illegal_char:
		case Terminator:
		default:
			atoms[cnt].type = atype;
			return cnt;
		}
	}
	return cnt;
}

/**
 * clone_args - clones @ref nr arguments from a group of atoms w/ termination
 *
 * @param to - address to copy to
 * @param from - address to copy from
 * @param nr - number of atoms to copy
 */
void clone_args(struct atom *to, struct atom *from, int nr)
{
	int i = 0;

	for (i = 0; i <= nr; i++) /* #0 is the cmd itself */
		memcpy(to + i, from + i, sizeof(struct atom));
	(to + i)->type = Terminator;
}

/**
 * get_cmdd - get command description
 *
 * @param cmd_id - command id (from the command enum list)
 *
 * @return pointer to cmd descriptor on success
 * @return NULL if there is no command with id @ref cmd_id
 */
struct cmd_desc *get_cmdd(enum def_cmd_id cmd_id)
{
	struct cmd_desc *cmd;

	list_for_each_entry(cmd, &tcmd_glob_list, list)
		if (cmd->id == cmd_id)
			return cmd;
	return NULL;
}

static void atoms_init(struct atom *atoms, unsigned int elems)
{
	int i;

	bzero((void *)atoms, elems * sizeof(struct atom));

	for (i = 0; i < elems; i++) {
		atoms[i].type = Terminator;
	}
}

/**
 * do_cmd - Execute command described in @atoms
 *
 * @param idx   - index in command atom list to start execution from
 * @param atoms - command atoms list
 *
 * @return end arg index             - if OK
 * @return one of @ref tst_prg_err_t - in case of error
 */
static int do_cmd(int idx, struct atom *atoms)
{
	struct atom 	*ap;	/* atom pointer */
	struct cmd_desc *cdp;	/* command's description */
	int ret;		/* stores what the handler returns */
	int count = 1;		/* stores the Numeric before brackets (Open) */
	int rec;		/* for recursive do_cmd() in Open */
	int i;
	struct atom args_copy[MAX_ARG_COUNT + 1]; /* +1 for the terminator */

	/*
	 * @todo
	 * test what happens when inside a (loop), there is an error.
	 * Would it catch it or not? Would it break badly?
	 */
	atoms_init(args_copy, MAX_ARG_COUNT + 1);

	while (1) {
		ap = &atoms[idx];
		if (ap->type != Numeric && ap->type != Open)
			count = 1; /* set count to default */
		switch (ap->type) {
		case Numeric:
			count = ap->val;
			idx++;
			break;
		case String:
			idx++;
			break;
		case Open:
			idx++;
			for (i = 0; i < count; i++)
				rec = do_cmd(idx, atoms);
			idx = rec;
			count = 1;
			break;
		case Alpha:
			cdp = get_cmdd(ap->cmd_id);
			cdp->pa = param_amount(ap);
			clone_args(args_copy, ap, cdp->pa);
			/*
			 * We don't do anything (yet) with ret.
			 * It could be used to implement a richer interface,
			 * but for the time being we'll leave it like that.
			 */
			ret = (*cdp->handle)(cdp, args_copy);
			idx += cdp->pa + 1;
			if (IS_ERR_VALUE(idx)) {
				return idx;
			}
			break;
		case Close:
			idx++;
			return idx;
		case Terminator:
			return idx;
		default:
			idx++;
			return idx;
		}
	}
}

/**
 * param_amount - catch the amount of parameters for this command
 *
 * @param atoms - stream of atoms (user's command-line input, split into atoms)
 *
 * This function takes a stream of atoms, starting with a particular command.
 * It identifies the number of subsequent atoms that this first atom should
 * be fed with.
 *
 * @return number of parameters @b excluding the command itself
 */
static int param_amount(struct atom *atoms)
{
	int i = 1; /* don't count the command itself */

	/* "help" command is special */
	if (atoms[0].cmd_id == CmdHELP &&
		atoms[1].type == Alpha && atoms[1].cmd_id != CmdNOCM) {
		i++;
		goto out;
	}

	/* "atom list" command is special */
	if (atoms[0].cmd_id == CmdATOMS) {
		while (atoms[i].type != Terminator &&
			atoms[i].type != Illegal_char)
			i++;
		goto out;
	}

	while (atoms[i].type == Numeric ||
		atoms[i].type == Operator ||
		atoms[i].type == String) {
		if (atoms[i].type == Numeric &&	atoms[i + 1].type == Open)
			break;
		i++;
	}
out:
	return i - 1;
}

/**
 * build_cmd_list - Initialises the (global) command list
 *
 * @param usrcmds - NULL-terminated array of specific commands\n
 *                  Can be NULL if none are provided
 * @param use_std - 1 - use standard test commands (from enum @ref def_id) \n
 *                  0 - don't use defaults. Use only the ones provided in
 *                      @ref usrcmds parameter
 */
static void build_cmd_list(struct cmd_desc *usrcmds, int use_std)
{
	enable_builtin_cmds();
	/* add illegal command to make sure the list won't be empty */
	list_add(&def_cmd[0].list, &tcmd_glob_list);
	if (use_std) {
		int i;
		for (i = 1; i < CmdUSR; i++)
			if (def_cmd[i].valid)
				list_add_tail(&def_cmd[i].list,
					&tcmd_glob_list);
	}
	if (usrcmds)
		while (usrcmds->id) {
			if (usrcmds->valid)
				list_add_tail(&usrcmds->list, &tcmd_glob_list);
			usrcmds++;
		}
}

/**
 * cmd_lnm - command's longest name
 *
 * @return strlen of the longest command
 */
static int cmd_lnm(void)
{
	struct cmd_desc *cmdd;
	static int len = 0;

	if (len)
		return len;
	list_for_each_entry(cmdd, &tcmd_glob_list, list)
		if (strlen(cmdd->name) > len)
			len = strlen(cmdd->name);
	return len;
}

/**
 * cmd_lopt - command's longest option
 *
 * @return strlen of the longest command option
 */
static int cmd_lopt(void)
{
	struct cmd_desc *cmdd;
	static int len = 0;

	if (len)
		return len;
	list_for_each_entry(cmdd, &tcmd_glob_list, list)
		if (strlen(cmdd->opt) > len)
			len = strlen(cmdd->opt);
	return len;
}

/**
 * enable_builtin_cmds - enable built-in commands
 *
 * Note that via @ref use_builtin_cmds the user can control this function
 */
static void enable_builtin_cmds(void)
{
	if (!use_builtin_cmds)
		return ;

	def_cmd[CmdDEBUG].valid		= 1;
	def_cmd[CmdVER].valid		= 1;
	def_cmd[CmdRAWIO].valid		= 1;
	def_cmd[CmdMODULE].valid	= 1;
	def_cmd[CmdNEXT].valid		= 1;
	def_cmd[CmdMAPS].valid		= 1;
	def_cmd[CmdMAPS].valid		= 1;
	def_cmd[CmdTIMEOUT].valid	= 1;
	def_cmd[CmdQUEUE].valid		= 1;
	def_cmd[CmdCLIENTS].valid	= 1;
	def_cmd[CmdCONNECT].valid	= 1;
	def_cmd[CmdENABLE].valid	= 1;
	def_cmd[CmdRESET].valid		= 1;
	def_cmd[CmdWINTR].valid		= 1;
	def_cmd[CmdSINTR].valid		= 1;
	def_cmd[CmdSLEEP].valid		= 1;
//	def_cmd[CmdJTAG].valid		= 1;
}

/**
 * devnode - Device node name builder and container.
 *
 * @param basename -  if 1 - return @b only filename component\n
 *                    if 0 - return @b full pathname string, including leading
 *                          directory components (normally just "/dev/")
 * @param idx - user context index. Normally (0 - MAX_UCA] range\n
 *              0 - is a service entry point, so will return service node name
 *
 * If the user wants the service device handle, the name will contain .0 suffix
 * In other words, any of the following will be returned:\n
 * <mod_name>.0\n
 * <mod_name>.1 ... <mod_name>.(MAX_UCA - 1)
 *
 * @return full pathname or just device filename
 */
static char* devnode(int basename, int idx)
{
	static char node_name[64];

	memset(node_name, 0, sizeof(node_name));

	if (basename)
		snprintf(node_name, sizeof(node_name), "%s.%d",
			 __drivername, idx);
	else
		snprintf(node_name, sizeof(node_name), "/dev/%s.%d",
			 __drivername, idx);

	return node_name;
}

/**
 * extest_cleanup - clean up the session
 *
 * This is just called before exiting.
 * It closes any device driver files that were opened
 */
static void extest_cleanup()
{
	if (_DNFD != F_CLOSED)
		close(_DNFD);
}

/**
 * sighandler - SIGNAL handler
 *
 * @param sig: SIGNAL number
 *
 * We catch signals so that we can properly clean-up the session and exit
 */
static void sighandler(int sig)
{
	extest_cleanup();
	/*
	 * we use sys_siglist[] instead of strsignal() to stay compatible
	 * with old versions of glibc
	 */
	printf("\nEXIT: Signal %s received\n", sys_siglist[sig]);
	exit(1);
}

/**
 * sighandler_init - register and configure the SIGNAL handler
 */
static void sighandler_init()
{
	struct sigaction act;

	act.sa_handler = sighandler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGFPE,  &act, 0);
	sigaction(SIGINT,  &act, 0);
	sigaction(SIGILL,  &act, 0);
	sigaction(SIGKILL, &act, 0);
	sigaction(SIGQUIT, &act, 0);
	sigaction(SIGSEGV, &act, 0);
	sigaction(SIGTERM, &act, 0);
}

static void usage_short()
{
	printf("Usage: %s [-x xmlfile] [-n driver_name]\n", __progname);
}

static void usage_error()
{
	usage_short();
	printf("Try '%s -h' for more information.\n", __progname);
}

static void usage()
{
	usage_short();
	printf("Optional arguments:\n"
		" -x xmlfile:      path to the driver's xml description file"
		" (default: '%s')\n", xmlfile);
	printf( " -n driver_name:  string used to open /dev devices"
		" (default: '" DRIVER_NAME "')\n");
	printf( " -h               display this help message\n\n");
	printf( "[ %s: Compiled on %s %s ]\n\n", __progname, __DATE__, __TIME__);
}

/**
 * process_options - parse the program's options when it is invoked
 *
 * @param argc: argument counter
 * @param  char *argv[]: argument values array
 */
static void process_options(int argc, char *argv[])
{
	/*
	 * before even parsing anything, we assign default values for these
	 * two global variables
	 * Note that xmlfile is set by default when it's declared
	 */
	__progname = argv[0];
	snprintf(__drivername, 64, DRIVER_NAME);

	argv++;
	argc--;
	if (!argc) {
		return ;
	}
	while (argc) {
		char opt;

		if (**argv != '-') {
			printf("Unknown option prefix %s, skipping\n", *argv);
			argv++;
			argc--;
			if (!argc) {
				printf("No more arguments\n\n");
				usage_error();
				exit(1);
			}
		}
		opt = argv[0][1];
		argv++;
		argc--;
		switch (opt) {
		case 'x':
			if (sscanf(*argv, "%s", xmlfile) != 1) {
				printf("Failed to parse xml file\n");
				usage();
				exit(1);
			}
			argv++;
			argc--;
			break;
		case 'n':
			if (sscanf(*argv, "%s", __drivername) != 1) {
				printf("Failed to driver name\n");
				usage();
				exit(1);
			}
			argv++;
			argc--;
			break;
		case 'h':
			usage();
			exit(1);
			break;
		default:
			printf("Invalid option -- %c\n", opt);
			usage_error();
			exit(1);
			break;
		}
	}
}

/**
 * cmd_is_valid - Check if the command is valid
 *
 * @param cmd command to check
 *
 * @return command description pointer @ref struct cmd_desc - if found
 * @return NULL - if command not found
 */
static struct cmd_desc* cmd_is_valid(char *cmd)
{
	struct cmd_desc *cmdd; /* command description pointer */

	list_for_each_entry(cmdd, &tcmd_glob_list, list) {
		if (!strcmp(cmdd->name, cmd))
			return cmdd;
	}
	return NULL;
}


/*
 * Handler helpers
 * may be used by helpers and therefore are non-static
 */

/**
 * get_free_user_handle - Open free device handle (if any)
 *
 * @param handle handle number to open\n
 *                   0 - for service handle\n
 *                  -1 - for any free handle, except the service one\n
 *     [1 - MAX_UCA-1] - for specific handle idx to open
 *
 * Each device has up to @ref MAX_UCA user contexts. (one of them is 'service').
 * The test program tries to open [1 - MAX_UCA-1] handles and returns the first
 * free handle it finds
 *
 * @return open device node file handle - on success
 * @return -1                           - on failure
 */
int get_free_user_handle(int handle)
{
	int dfd = -1; /* driver node file descriptor */
	int i;

	if (handle == -1) /* any free handle, except the service one */
		for (i = 1; i < MAX_UCA; i++) {
			dfd = open(devnode(0, i), O_RDWR);
			if (dfd != -1)
				break;
		}
	else {
		dfd = open(devnode(0, handle), O_RDWR);
		if (dfd == -1)
			mperr("Can't open '%s' node", devnode(0, handle));
	}
	return dfd;
}

/**
 * print_modules - print information about controlled modules
 *
 * This information is taken from "xmlfile" using InsLib
 */
void print_modules(void)
{
	InsLibHostDesc *hostd = NULL;
	InsLibDrvrDesc *drvrd = NULL;

	if (access(xmlfile, F_OK) < 0) {
		printf("Error: xmlfile not found (requested: \"%s\")\n",
			xmlfile);
		return ;
	}
	hostd = InsLibParseInstallFile(xmlfile, 1);
	if (InsLibErrorCount)
		printf("\nXML parser: %d Errors detected\n", InsLibErrorCount);
	else
		printf("\nXML parser: no errors detected\n");
	drvrd = InsLibGetDriver(hostd, __drivername);
	printf("Requested driver: %s ", __drivername);
	if (drvrd)
		printf("OK\n");
	else
		printf("failed\n");
	InsLibPrintDriver(drvrd);
	InsLibFreeDriver(drvrd);
}

/**
 * __getchar - get a character from stdin
 *
 * @param c - pointer to the address where the character will be put
 */
void __getchar(char *c)
{
	char *line;

	do {
		line = readline("");
	} while (line == NULL);

	*c = line[0];
	free(line);
}

/*
 * Basic Handlers
 * handlers for the commands that are always part of extest
 */

/**
 * hndl_illegal - Illegal command handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_illegal(struct cmd_desc* cmdd, struct atom *atoms)
{
	printf("%s --> %s\n", atoms->text, cmdd->help);
	return 1;
}

/**
 * _hndl_dbg - Service entry point. For debugging purposes.
 *
 * @param cmdd  command description
 * @param atoms command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int _hndl_dbg(struct cmd_desc* cmdd, struct atom *atoms)
{
	return 1;
}

/**
 * hndl_quit - Quit test program
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return nothing; it just cleans up and exits
 */
int hndl_quit(struct cmd_desc* cmdd, struct atom *atoms)
{
	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("\nexit the test program\n");
		return 1;
	}
	extest_cleanup();
	exit(EXIT_SUCCESS);
}

/**
 * hndl_help - User help
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_help(struct cmd_desc* cmdd, struct atom *atoms)
{
	struct cmd_desc *cdp = NULL; /* command description ptr */
	int i = 0;

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s - display full command list\n"
			"%s o - help on the operators\n"
			"%s cmd - help on the command 'cmd'\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}

	if (atoms[1].type == Alpha && !strcmp(atoms[1].text, "o")) {
		printf("\nOPERATORS:\n\n");
		for (i = 1; i < OprOPRS; i++)
			printf("Id: %2d Opr: %s \t--> %s\n",
				oprs[i].id, oprs[i].name, oprs[i].help);
	} else if (atoms[1].type == Alpha &&
		(cdp = cmd_is_valid(atoms[1].text))) {
		(*cdp->handle)(cdp, (struct atom *)VERBOSE_HELP);
	} else {
		printf("Valid COMMANDS:\n%-*s %-*s %-*s %-s\n",
			(int)strlen("#xx:"), "Idx", cmd_lnm(), "Name",
			(int)strlen("[  ] ->") + cmd_lopt(), "Params",
			"Description");
		list_for_each_entry(cdp, &tcmd_glob_list, list)
			if (cdp->valid)
				printf("#%2d: %-*s [ %-*s ] -> %s\n", ++i,
					cmd_lnm(), cdp->name, cmd_lopt(),
					cdp->opt, cdp->help);
		printf("\nType \"h name\" to get complete command help\n");
	}
out:
	return 1;
}

/**
 * hndl_history - Command history print-out
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_history(struct cmd_desc* cmdd, struct atom *atoms)
{
	HIST_ENTRY **list;
	int i;

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s - display command history\n", cmdd->name);
		goto out;
	}
	list = history_list();
	if (list) {
		for (i = 0; list[i]; i++)
			printf("Cmd[%02d] %s\n", i, list[i]->line);
	}
out:
	return 1;
}

/**
 * hndl_atoms - Print out the command line split into atoms
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * Useful for debugging
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_atoms(struct cmd_desc* cmdd, struct atom *atoms)
{
	struct cmd_desc *dp;
	struct atom 	*ap;
	int i = 0;

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s - split the command line input into atoms\n",
			cmdd->name);
		goto out;
	}

	printf("Arg list: Consists of %d atoms.\n", cmdd->pa);
	for (i = 0; i < cmdd->pa; i++) {
		ap = &atoms[i + 1]; /* skip command itself */
		switch (ap->type) {
		case Numeric:
			printf("Arg[%02d] Num: %d\n", i + 1, ap->val);
			break;
		case Alpha:
			if (ap->cmd_id) {
				dp = get_cmdd(ap->cmd_id);
				printf("Arg[%02d] Cmd: %d Name: %s Help: %s\n",
					i + 1, ap->cmd_id, dp->name, dp->help);
			} else /* illegal command, i.e. 'CmdNOCM' case */
				printf("Arg[%02d] Unknown Command: %s\n",
					i + 1, ap->text);
			break;
		case Operator:
			if (ap->oid) {
				int id = ap->oid;
				printf("Arg[%02d] Operator: %s Help: %s\n",
					i + 1, oprs[id].name, oprs[id].help);
			}
			else
				printf("Arg[%02d] Unknown Operator: %s\n",
					i + 1, ap->text);
			break;
		case String:
			printf("Arg[%02d] String: \"%s\"\n", i + 1, ap->text);
			break;
		case Open:
			printf("Arg[%02d] Opn: (\n", i + 1);
			break;
		case Close:
			printf("Arg[%02d] Cls: )\n", i + 1);
			break;
		case Open_index:
			printf("Arg[%02d] Opn: [\n", i + 1);
			break;
		case Close_index:
			printf("Arg[%02d] Cls: ]\n", i + 1);
			break;
		case Bit:
			printf("Arg[%02d] Bit: .\n", i + 1);
			break;
		case Terminator:
			printf("Arg[%02d] End: @\n", i + 1);
			break;
		default:
			printf("Arg[%02d] ???\n", i + 1);
		}
	}
out:
	return 1;
}

/**
 * hndl_sleep - sleep for a few seconds
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_sleep(struct cmd_desc* cmdd, struct atom *atoms)
{
	int seconds;

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s [n] - put the process to sleep for 'n' seconds\n"
			"\tNote that n=1s is the default value.\n",
			cmdd->name);
		goto out;
	}
	atoms++;
	if (atoms->type == Numeric)
		seconds = atoms->val;
	else
		seconds = 1;
	sleep(seconds);
out:
	return 1;
}

/**
 * hndl_shell - execute a shell command
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_shell(struct cmd_desc* cmdd, struct atom *atoms)
{
	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s \"shellcmd args\" - execute a shell command.\n"
			"Note that shellcmd and its arguments need to be "
			"enclosed in double quotes. These can't be escaped "
			"so use single quotes (if needed) inside args.\n",
			cmdd->name);
		goto out;
	}

	if ((++atoms)->type != String)
		return -TST_ERR_WRONG_ARG;

	system(atoms->text);
out:
	return 1;
}

/*
 * extest's User's API
 */

/**
 * do_yes_no - Get user answer (y/n)
 *
 * @param question - prompt the user with it
 * @param extra    - extra argument to the question (can be NULL)
 *
 * @return 1 - user replied 'yes'
 * @return 0 - user replied 'no'
 */
int do_yes_no(char *question, char *extra)
{
	char reply[2] = { 0 };

ask:
	printf("%s", question);
	if (extra)
		printf(" %s", extra);
	printf("? (y/n) > ");
	__getchar(reply);
	printf("\n");
	if (reply[0] != 110 && reply[0] != 121 && /* accept y/n & Y/N */
		reply[0] != 78 && reply[0] != 89) {
		printf("answer y/n only\n");
		goto ask;
	}
	return reply[0] == 'y';
}

/**
 * compulsory_ok - checks that compulsory parameters are passed
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return 1 - ok
 * @return 0 - not ok
 */
int compulsory_ok(struct cmd_desc *cmdd)
{
	return cmdd->pa == cmdd->comp;
}

/**
 * is_last_atom - checks if @ref atom is the last in the list to be processed
 *
 * @param atom - command atoms list
 *
 * An atom is considered to be the last one of a list if it is either
 * the terminator or the atom preceding the terminator
 *
 * @return 1 - if it's the last one
 * @return 0 - if it isn't
 */
int is_last_atom(struct atom *atom)
{
	if (atom->type == Terminator)
		return 1;
	if ((atom + 1)->type == Terminator)
		return 1;
	return 0;
}

#ifdef __SKEL_EXTEST
static int extest_open_fd(void)
{
	if (_DNFD == F_CLOSED) {
		if ((_DNFD = get_free_user_handle(-1)) == -1) {
			mperr("Can't open driver node\n");
			return -1;
		}
	}
	if (ioctl(_DNFD, SkelDrvrIoctlGET_MODULE, &tst_glob_d.mod) < 0) {
		mperr("Get module failed\n");
		return -1;
	}
	return 0;
}
#else
static int extest_open_fd(void)
{
	return 0;
}
#endif /* __SKEL_EXTEST */

/* add a line to the history if it's not a duplicate of the previous */
static void extest_add_history(char *line)
{
	HIST_ENTRY *prev;
	int save_it;

	if (!line[0])
		return;

	using_history();
	prev = previous_history();
	/* save it if there's no previous line or if they're different */
	save_it = !prev || strcmp(line, prev->line);
	using_history();

	if (save_it)
		add_history(line);
}

/**
 * extest_init - Initialise extest
 *
 * @param argc - argument counter
 * @param  char *argv[] - argument values
 *
 * This function it is meant to run as long as the test program process runs.
 * It will only return when there is a severe failure -- such as a signal.
 *
 * @return EXIT_FAILURE - on failure
 */
int extest_init(int argc, char *argv[])
{
	struct atom cmd_atoms[MAX_ARG_COUNT];
	char host[32] = { 0 };
	char prompt[128]; /* prompt string */
	char *cmd; /* command pointer */
	int cmdindx = 0;

	sighandler_init();
	process_options(argc, argv);
	build_cmd_list(user_cmds, 1); /* add specific commands */
	gethostname(host, 32);
	using_history();
	if (extest_open_fd())
		goto out_err;
	while(1) {
		bzero(prompt, sizeof(prompt));
		if (tst_glob_d.mod)
			snprintf(prompt, sizeof(prompt),
				WHITE_ON_BLACK "[module#%d] @%s [%02d] > "
				DEFAULT_COLOR, tst_glob_d.mod, host, cmdindx++);
		else
			snprintf(prompt, sizeof(prompt), WHITE_ON_BLACK
				"%s@%s [%02d] > " DEFAULT_COLOR,
				__drivername, host, cmdindx++);
		cmd = readline(prompt);
		if (!cmd)
			exit(1);
		extest_add_history(cmd);
		atoms_init(cmd_atoms, MAX_ARG_COUNT);
		get_atoms(cmd, cmd_atoms); /* split cmd into atoms */
		do_cmd(0, cmd_atoms); /* execute atoms */
		free(cmd);
	}
out_err:
	/* error occurs */
	extest_cleanup();
	fprintf(stderr, "Unexpected error!\n");
	return EXIT_FAILURE;
}
