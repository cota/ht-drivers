/**
 * @file cmd_skel.c
 *
 * @brief extest's command handlers for skel-based drivers
 *
 * @author Copyright (C) 2009 CERN CO/HT Emilio G. Cota
 * 					<emilio.garcia.cota@cern.ch>
 * @author Copyright (C) 2008 CERN CO/HT Yury Georgievskiy
 *					<yury.georgievskiy@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include <fpga_loader/lenval.h>
#include <fpga_loader/micro.h>
#include <fpga_loader/ports.h>

/* local includes */
#include <err.h>
#include <general_usr.h>  /* for handy definitions (mperr etc..) */

#include <extest.h>

extern int get_free_user_handle(int);
extern void __getchar(char *c);
extern void print_modules();

/**
 * hndl_swdeb - handler for software debugging
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_swdeb(struct cmd_desc* cmdd, struct atom *atoms)
{
	SkelDrvrDebug db;
	unsigned int flag;

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s - print current software debugging state\n"
			"%s flag [pid] - set software debugging\n"
			"flag:\n"
			"\t0x00: disable debugging\n"
			"\t0x01: assertion violations\n"
			"\t0x02: trace all IOCTLS\n"
			"\t0x04: warning messages\n"
			"\t0x08: module specific debug warnings\n"
			"\t0x10: verbose debugging information\n"
			"\t0x100: driver emulation on NO hardware\n"
			"pid is optional (current pid is the default)\n",
			cmdd->name, cmdd->name);
		goto out;
	}

	if (is_last_atom(atoms)) { /* no arguments --> GET debug */
		if (ioctl(_DNFD, SkelDrvrIoctlGET_DEBUG, &db) < 0) {
			mperr("%s ioctl failed\n", "GET_DEBUG");
			return -TST_ERR_IOCTL;
		}
		flag = db.DebugFlag;
		printf("Software debug mask: 0x%x\n", flag);
		if (flag & SkelDrvrDebugFlagASSERTION)
			printf("Flag set: assertion violations\n");
		if (flag & SkelDrvrDebugFlagTRACE)
			printf("Flag set: trace all IOCTL calls\n");
		if (flag & SkelDrvrDebugFlagWARNING)
			printf("Flag set: warning messages\n");
		if (flag & SkelDrvrDebugFlagMODULE)
			printf("Flag set: module specific debug warnings\n");
		if (flag & SkelDrvrDebugFlagINFORMATION)
			printf("Flag set: all debug information\n");
		if (flag & SkelDrvrDebugFlagEMULATION)
			printf("Flag set: emulation on NO hardware\n");
		goto out;
	}
	/* SET debug */
	atoms++;
	if (atoms->type != Numeric) {
		printf("invalid argument\n");
		return -TST_ERR_WRONG_ARG;
	}
	db.DebugFlag = atoms->val;
	printf("flag: %x\n", db.DebugFlag);
	atoms++;
	if (is_last_atom(atoms) || !atoms->val)
		db.ClientPid = getpid();
	else {
		if (atoms->type == Numeric)
			db.ClientPid = atoms->val;
		else
			return -TST_ERR_WRONG_ARG;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlSET_DEBUG, &db) < 0) {
		mperr("%s ioctl failed\n", "SET_DEBUG");
		return -TST_ERR_IOCTL;
	}
	printf("%s ioctl: successfully set flag 0x%x for pid %d\n",
		"SET_DEBUG", db.DebugFlag, db.ClientPid);
out:
	return 1;
}

/**
 * hndl_getversion - handler for get driver/module version
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_getversion(struct cmd_desc* cmdd, struct atom *atoms)
{
	SkelDrvrVersion vers;

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("No further help available\n");
		goto out;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlGET_VERSION, &vers) < 0) {
		mperr("GET_VERSION ioctl failed\n");
		return -TST_ERR_IOCTL;
	}
	printf("Version:\n"
		"\tDriver compile date (human): %s"
		"\tDriver compile date (EPOCH): %d\n"
		"\tModule version: %s\n",
		ctime((time_t *)&vers.DriverVersion), vers.DriverVersion,
		vers.ModuleVersion);
out:
	return 1;
}

/**
 * select_module - select a particular module
 *
 * @param modnr module number to select
 *
 * Note that we consider the operation successful even if the module
 * is out of range -- we just print it out and return success
 *
 * @return 0 - on success
 * @return (negative) error code - on failure
 */
int select_module(int modnr)
{
	if (modnr == tst_glob_d.mod)
		return 0;
	if (!WITHIN_RANGE(1, modnr, tst_glob_d.ma)) {
		printf("Requested module out of range\n");
		return 0;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlSET_MODULE, &modnr) < 0) {
		mperr("%s ioctl fails\n", "SET_MODULE");
		return -TST_ERR_IOCTL;
	}
	tst_glob_d.mod = modnr;
	printf("Controlling module #%d\n", tst_glob_d.mod);
	return 0;
}

/**
 * hndl_module - handle for 'module' -- used to manage devices
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_module(struct cmd_desc *cmdd, struct atom *atoms)
{
	int sel; /* keep ret. code from select_module */

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("select or show modules\n"
			"%s - shows currently selected module\n"
			"%s ? - prints all managed modules by the driver\n"
			"%s x - select module x (where x ranges from 1 to n)\n"
			"%s ++ - select next module\n"
			"%s -- - select next module\n",
			cmdd->name, cmdd->name, cmdd->name, cmdd->name,
			cmdd->name);
		goto out;
	}
	if (_DNFD == F_CLOSED) {
		if ((_DNFD = get_free_user_handle(-1)) == -1) {
			mperr("Can't open driver node\n");
			goto out;
		}
	}
	if (ioctl(_DNFD, SkelDrvrIoctlGET_MODULE_COUNT, &tst_glob_d.ma) < 0) {
		mperr("Can't get module count. %s ioctl fails\n",
			"GET_MODULE_COUNT");
		goto out;
	}
	if (!tst_glob_d.ma) {
		printf("The driver does not control any modules\n");
		goto out;
	}
	++atoms;

	if (atoms->type != Numeric) {
		if (atoms->type == Operator)
			switch (atoms->oid) {
			case OprNOOP:
				print_modules();
				break;
			case OprINC:
				sel = select_module(tst_glob_d.mod + 1);
				if (sel < 0)
					return sel;
				goto out;
			case OprDECR:
				sel = select_module(tst_glob_d.mod - 1);
				if (sel < 0)
					return sel;
				goto out;
			default:
				break;
			}
		if (tst_glob_d.mod)
			printf("Controlling module #%d (out of %d)\n",
				tst_glob_d.mod, tst_glob_d.ma);
		else
		printf("The driver manages %d device%s\n",
			tst_glob_d.ma, tst_glob_d.ma > 1 ? "s" : "");
		goto out;
	}
	sel = select_module(atoms->val);
	if (sel < 0)
		return sel;
out:
	return 1;
}

/**
 * hndl_nextmodule - select next module handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_nextmodule(struct cmd_desc *cmdd, struct atom *atoms)
{
	int sel; /* keep ret. code from select_module */

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - select next module\n"
			"This command takes no arguments.\n", cmdd->name);
		goto out;
	}
	sel = select_module(tst_glob_d.mod + 1);
	if (sel < 0)
		return sel;
out:
	return 1;
}

/**
 * print_rawio - print the 'RawIO' beginning of line
 *
 * @param rio - RawIo block
 * @param radix - hex (16) or decimal
 */
void print_rawio(SkelDrvrRawIoBlock *rio, int radix)
{
	printf("\n%08x@space#%02x: ", rio->Offset, rio->SpaceNumber);
	if (radix == 16)
		printf("0x%-*x ", rio->DataWidth >> 2, rio->Data);
	else
		printf("%5d ", rio->Data);

}

/**
 * rawio_input - manage 'RawIO' command input
 *
 * @param ch - initial input character
 * @param rio - RawIo block
 * @param radix - hex (16) or decimal
 *
 * @return 0 - on success
 * @return tst_prg_err_t - on failure
 */
int rawio_input(char ch, SkelDrvrRawIoBlock *rio, int *radix)
{
	char str[128];
	char *endptr;
	int input;
	int i = 0;
	char c = ch;

	bzero((void *)str, 128);
	switch (c) {
	case '/':
		while (c != '\n' && i < 128) {
			__getchar(&c);
			str[i++] = c;
		}
		input = strtoul(str, &endptr, *radix);
		if (endptr != str) { /* catched something */
			rio->Offset = input;
			printf("\n");
		}
		break;
	case 'x':
		*radix = 16;
		break;
	case 'd':
		*radix = 10;
		break;
	case '\n':
		rio->Offset += rio->DataWidth / 8;
		break;
	case '.':
		c = getchar();
		printf("\n");
		break;
	default: /* write */
		str[i++] = c;
		while (c != '\n' && i < 128) {
			__getchar(&c);
			str[i++] = c;
		}
		input = strtoul(str, &endptr, *radix);
		if (endptr == str)
			break; /* nothing catched */
		rio->Data = input;
		if (ioctl(_DNFD, SkelDrvrIoctlRAW_WRITE, rio) < 0) {
			mperr("RAW_WRITE ioctl fails");
			return -TST_ERR_IOCTL;
		}
		break;
	}
	return 0;
}

/**
 * hndl_rawio - 'RawIO' handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_rawio(struct cmd_desc* cmdd, struct atom *atoms)
{
	SkelDrvrRawIoBlock rio;
	int radix = 16;
	int err;
	char c = '\n';

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("raw I/O\n"
			"%s spacenumber datawidth offset\n"
			"\tspacenumber: VME->AddressModifier, PCI->BAR\n"
			"\tdatawidth: 8, 16, 32.\n"
			"\toffset: offset within the address space\n",
			cmdd->name);
		goto out;
	}
	if (!compulsory_ok(cmdd)) {
		printf("Wrong parameter amount\n");
		return -TST_ERR_ARG_O_S;
	}
	if (((atoms + 1)->type | (atoms + 2)->type | (atoms + 3)->type)
		!= Numeric) {
		printf("Invalid argument (parameters have to be numeric.)\n");
		return -TST_ERR_WRONG_ARG;
	}
	rio.SpaceNumber = (++atoms)->val;
	rio.DataWidth 	= (++atoms)->val;
	rio.DataWidth 	&= (1 << 3) | (1 << 4) | (1 << 5);
	if (!rio.DataWidth) {
		printf("Invalid argument (DataWidth must be 8, 16 or 32.)\n");
		return -TST_ERR_WRONG_ARG;
	}
	rio.Offset = (++atoms)->val;

	printf("RawIO: [/]: Open; [CR]: Next; [.]: Exit; [x]: Hex; [d]: Dec\n");
	do {
		if (ioctl(_DNFD, SkelDrvrIoctlRAW_READ, &rio) < 0) {
			mperr("%s ioctl fails:\n"
				"space #: %d; data width:%d, offset: %d\n",
				"RAW_READ", rio.SpaceNumber, rio.DataWidth,
				rio.Offset);
			return -TST_ERR_IOCTL;
		}
		print_rawio(&rio, radix);
		__getchar(&c);
		if ((err = rawio_input(c, &rio, &radix)))
			return err;
     	} while (c != '.');
out:
	return 1;
}

/**
 * hndl_maps - handler for showing the address mappings of the current device
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_maps(struct cmd_desc *cmdd, struct atom *atoms)
{
	int i;
	SkelDrvrMaps maps;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s takes no parameters\n"
			"\tjust select a module before issuing %s\n",
			cmdd->name, cmdd->name);
		goto out;
	}
	if (!tst_glob_d.mod) {
		printf("Not working on a particular module\n");
		goto out;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlGET_MODULE_MAPS, &maps) < 0) {
		mperr("%s ioctl fails\n", "GET_MODULE_MAPS");
		return -TST_ERR_IOCTL;
	}
	if (!maps.Mappings) {
		printf("No mappings for module #%d\n", tst_glob_d.mod);
		goto out;
	}
	printf("Space#\t\tMapped\n"
		"------\t\t------\n");

	for (i = 0; i < maps.Mappings; i++) {
		printf("0x%x\t\t0x%lx\n", maps.Maps[i].SpaceNumber,
			maps.Maps[i].Mapped);
	}
out:
	return 1;
}

/**
 * hndl_timeout - 'timeout' handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_timeout(struct cmd_desc *cmdd, struct atom *atoms)
{
	int timeout;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("get/set timeout\n"
			"%s - shows current timeout [in 10ms chunks]\n"
			"%s x - set timeout x [in 10ms chunks]\n",
			cmdd->name, cmdd->name);
		goto out;
	}
	if (is_last_atom(atoms)) {
		if (ioctl(_DNFD, SkelDrvrIoctlGET_TIMEOUT, &timeout) < 0) {
			mperr("%s ioctl fails\n", "GET_TIMEOUT");
			return -TST_ERR_IOCTL;
		}
		printf("timeout: %d [10*ms]\n", timeout);
		goto out;
	}
	atoms++;
	if (atoms->type != Numeric) {
		printf("Invalid argument (must be numeric.)\n");
		return -TST_ERR_WRONG_ARG;
	}
	timeout = atoms->val;
	if (ioctl(_DNFD, SkelDrvrIoctlSET_TIMEOUT, &timeout) < 0) {
		mperr("%s ioctl fails\n", "SET_TIMEOUT");
		return -TST_ERR_IOCTL;
	}
	printf("timeout set to %d [10*ms] == %d ms\n", timeout, timeout * 10);
out:
	return 1;

}

/**
 * print_queue - display current device's queue information
 */
int print_queue()
{
	unsigned int qflag, qsize, qover;

	if (ioctl(_DNFD, SkelDrvrIoctlGET_QUEUE_FLAG, &qflag) < 0) {
		mperr("%s ioctl fails\n", "GET_QUEUE_FLAG");
		return -TST_ERR_IOCTL;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlGET_QUEUE_SIZE, &qsize) < 0) {
		mperr("%s ioctl fails\n", "GET_QUEUE_SIZE");
		return -TST_ERR_IOCTL;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlGET_QUEUE_FLAG, &qover) < 0) {
		mperr("%s ioctl fails\n", "GET_QUEUE_FLAG");
		return -TST_ERR_IOCTL;
	}
	if (qflag)
		printf("Queueing is OFF\n");
	else {
		printf("Queueing is ON\n"
			"\tQueue size: %d\n"
			"\tQueue overflow: %d\n", qsize, qover);
	}
	return 0;
}

/**
 * hndl_queue - 'queue' handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_queue(struct cmd_desc *cmdd, struct atom *atoms)
{
	int qflag;
	int ret;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("get/set queue flag\n"
			"%s - shows current state of the queue\n"
			"%s 1 - Turn queueing OFF\n"
			"%s 0 - Turn queueing ON\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}
	if ((++atoms)->type == Numeric) {
		qflag = atoms->val;
		if (ioctl(_DNFD, SkelDrvrIoctlSET_QUEUE_FLAG, &qflag) < 0) {
			mperr("%s ioctl fails\n", "SET_QUEUE_FLAG");
			return -TST_ERR_IOCTL;
		}
		printf("Queue switched %s successfully\n",
			!!qflag ? "OFF" : "ON");
		goto out;
	}
	ret = print_queue();
	if (ret < 0)
		return ret;
out:
	return 1;

}

/**
 * print_client_connections - show a client's list of connections
 *
 * @param pid - pid of the client
 *
 * @return number of connections of the client
 */
int print_client_connections(int pid)
{
	int i;
	int mypid = getpid();
	SkelDrvrConnection *conn;
	SkelDrvrClientConnections conns;

	bzero((void *)&conns, sizeof(SkelDrvrClientConnections));
	conns.Pid = pid;
	if (ioctl(_DNFD, SkelDrvrIoctlGET_CLIENT_CONNECTIONS, &conns) < 0) {
		mperr("%s ioctl fails\n", "GET_CLIENT_CONNECTIONS");
		return -TST_ERR_IOCTL;
	}
	printf("Pid: %d%s\n", pid, pid == mypid ? " (test program)" : "");
	if (!conns.Size)
		return 0;
	for (i = 0; i < conns.Size; i++) {
		conn = &conns.Connections[i];
		printf("\tModule: %d, Mask: 0x%08x\n", conn->Module,
			conn->ConMask);
	}
	return conns.Size;
}

/**
 * hndl_clients - 'list of clients' handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_clients(struct cmd_desc *cmdd, struct atom *atoms)
{
	int i;
	int ccret;
	SkelDrvrClientList clist;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - shows connected clients\n", cmdd->name);
		goto out;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlGET_CLIENT_LIST, &clist) < 0) {
		mperr("%s ioctl fails\n", "GET_CLIENT_LIST");
		return -TST_ERR_IOCTL;
	}
	for (i = 0; i < clist.Size; i++)
		if ((ccret = print_client_connections(clist.Pid[i])) < 0) {
			printf("Can't get client's connections\n");
			return ccret;
		}
out:
	return 1;
}

/**
 * hndl_connect - 'connect' handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_connect(struct cmd_desc *cmdd, struct atom *atoms)
{
	int ccret;
	SkelDrvrConnection conn;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s - shows connections of current pid\n"
			"%s x - connect to mask on current module\n"
			"%s x y - connect to mask x on module y\n",
			cmdd->name, cmdd->name, cmdd->name);
		goto out;
	}
	if ((++atoms)->type != Numeric) {
		if ((ccret = print_client_connections(getpid())) < 0) {
			printf("Could not get client's connections\n");
			return ccret;
		}
		goto out;
	}
	conn.ConMask = atoms->val;
	conn.Module = 0;
	if (!is_last_atom(atoms)) {
		if ((++atoms)->type != Numeric) {
			printf("Invalid module number\n");
			return -TST_ERR_WRONG_ARG;
		}
		conn.Module = atoms->val;
	}
	/* 0 == current module */
	if (!WITHIN_RANGE(0, conn.Module, tst_glob_d.ma)) {
		printf("Invalid module number\n");
		return -TST_ERR_WRONG_ARG;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlCONNECT, &conn) < 0) {
		mperr("%s ioctl fails\n", "CONNECT");
		return -TST_ERR_IOCTL;
	}
	if (conn.ConMask)
		printf("Connected to 0x%08x on module %d\n", conn.ConMask,
			!conn.Module ? tst_glob_d.mod : conn.Module);
	else {
		printf("Disconnected from all the interrupts on %s module%s",
			!conn.Module ? "all the" : "", !conn.Module ? "s" : "");
		if (conn.Module)
			printf(" %d", conn.Module);
		printf("\n");
	}
out:
	return 1;
}

/**
 * hndl_enable - 'enable' handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_enable(struct cmd_desc *cmdd, struct atom *atoms)
{
	int enable;

	if (atoms == (struct atom *)VERBOSE_HELP) {
		printf("%s [1]- enable current module ('1' is not required)\n"
			"%s 0 - disable current module\n",
			cmdd->name, cmdd->name);
		goto out;
	}
	if (is_last_atom(atoms)) {
		enable = 1;
	}
	else {
		if ((++atoms)->type != Numeric) {
			printf("Invalid argument (must be an integer)\n");
			return -TST_ERR_WRONG_ARG;
		}
		enable = !!atoms->val;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlENABLE, &enable) < 0) {
		mperr("%s ioctl fails\n", "ENABLE");
		return -TST_ERR_IOCTL;
	}
	printf("Module %d %s\n", tst_glob_d.mod,
		enable ? "enabled" : "disabled");
out:
	return 1;
}

/**
 * hndl_reset - 'reset' handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_reset(struct cmd_desc* cmdd, struct atom *atoms)
{
	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s - reset current module\n", cmdd->name);
		goto out;
	}
	if (!tst_glob_d.mod) {
		printf("Not controlling any module\n");
		return -TST_ERR_NO_MODULE;
	}
	if (!do_yes_no("Reset the current module. Are you sure", NULL))
		goto out;
	if (ioctl(_DNFD, SkelDrvrIoctlRESET, NULL) < 0) {
		mperr("%s ioctl fails\n", "RESET");
		return -TST_ERR_IOCTL;
	}
	printf("Module %d reset correctly\n", tst_glob_d.mod);
out:
	return 1;
}

/**
 * hndl_getstatus - 'status' handler
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return >= 0 - on success
 * @return tst_prg_err_t - on failure
 */
int hndl_getstatus(struct cmd_desc* cmdd, struct atom *atoms)
{
	SkelDrvrStatus status;

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s - print current status\n"
			"Standard status can be:\n"
			"\t0x001: NO_ISR\n"
			"\t0x002: BUS_ERROR\n"
			"\t0x004: DISABLED\n"
			"\t0x008: HARDWARE_FAIL\n"
			"\t0x010: WATCH_DOG\n"
			"\t0x020: BUS_FAULT\n"
			"\t0x040: FLASH_OPEN\n"
			"\t0x080: EMULATION\n"
			"\t0x100: NO_HARDWARE\n"
			"\t0x200: IDLE\n"
			"\t0x400: BUSY\n"
			"\t0x800: READY\n"
			"\t0x1000: HARDWARE_DBUG\n",
			cmdd->name);
		goto out;
	}
	if (ioctl(_DNFD, SkelDrvrIoctlGET_STATUS, &status) < 0) {
		mperr("%s ioctl failed\n", "GET_STATUS");
		return -TST_ERR_IOCTL;
	}
	printf("Hardware status: %d\n", status.HardwareStatus);
	printf("Standard status flag: 0x%04x\n", status.StandardStatus);
out:
	return 1;
}

/**
 * hndl_waitintr - connect to an interrupt via read()
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return index of the next atom to parse in the current command
 * @return -TST_ERR_IOCTL - IOCTL fails
 */
int hndl_waitintr(struct cmd_desc* cmdd, struct atom *atoms)
{
	SkelDrvrReadBuf rbuf;
	int 		ret;
	int 		nc;	/* number of connections */

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s - wait for any interrupts subscribed to\n",
			cmdd->name);
		goto out;
	}
	nc = print_client_connections(getpid());
	if (!nc) {
		printf("No connections for this client\n");
		goto out;
	}
	printf("Waiting...\n");
	ret = read(_DNFD, &rbuf, sizeof(SkelDrvrReadBuf));
	if (ret <= 0) {
		printf("Timeout or Interrupted call\n");
		goto out;
	}
	printf("\nWoken up: Module[%d] Mask[0x%08x]\n"
		" @@ %s"
		" @@ %dns (EPOCH: %d)\n\n",
		rbuf.Connection.Module, rbuf.Connection.ConMask,
		ctime((time_t *)&rbuf.Time.Second), rbuf.Time.NanoSecond,
		rbuf.Time.Second);
	ret = print_queue();
	if (ret < 0)
		return ret;
out:
	return 1;
}

/**
 * hndl_simintr - simulate an interrupt via write()
 *
 * @param cmdd  - command description
 * @param atoms - command atoms list
 *
 * @return index of the next atom to parse in the current command
 * @return -TST_ERR_IOCTL - IOCTL fails
 */
int hndl_simintr(struct cmd_desc* cmdd, struct atom *atoms)
{
	SkelDrvrConnection wbuf;
	int ret;

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s mask - simulate a mask interrupt on current module\n"
			"%s mask n - simulate a mask interrupt on module n\n",
			cmdd->name, cmdd->name);
		goto out;
	}
	atoms++;
	if (atoms->type != Numeric) {
		printf("Mask needed to simulate a certain interrupt\n");
		goto out;
	}
	wbuf.ConMask = atoms->val;
	wbuf.Module = tst_glob_d.mod;
	if ((++atoms)->type == Numeric)
		wbuf.Module = atoms->val;
	ret = write(_DNFD, &wbuf, sizeof(SkelDrvrConnection));
	if (ret <= 0) {
		printf("Interrupt NOT simulated; write() failed (%s)\n",
			strerror(ret));
		goto out;
	}
	printf("Interrupt 0x%08x on module#%d sent\n",
		wbuf.ConMask, wbuf.Module);
out:
	return 1;
}

/*
 * @todo JTAG -- to be implemented
 * The code below comes from old versions from extest; with it
 * it should be relatively straight-forward to make it work
 */
#if 0
/**
 * hndl_jtag_vhdl - JTAG VHDL code
 *
 * @cmdd  command description
 * @atoms command atoms list
 *
 * @return index of the next atom to parse in the current command, or
 *         index of the @ref Terminator atom to finish parsing.
 * @return -TST_ERR_NO_MODULE - active module not set
 * @return -TST_ERR_ARG_O_S   - argument overflow/shortcoming
 * @return -TST_ERR_NO_VECTOR - user doesn't provide ioctl number to be able
 *                              to use this vector.
 * @return -TST_ERR_IOCTL     - ioctl call fails
 * @return -TST_ERR_WRONG_ARG - wrong command argument
 *
 * @return
 */
int hndl_jtag_vhdl(struct cmd_desc* cmdd, struct atom *atoms)
{
	char cwd[MAXPATHLEN] = { 0 };
	DIR *dir;

	if (atoms == (struct atom*)VERBOSE_HELP) {
		printf("%s filename - load VHDL from .xsvf file 'filename'\n",
			cmdd->name);
		goto out;
	}
	getcwd(cwd, sizeof(cwd));
	dir = opendir(cwd);

	if ((atoms++)->type == Alpha) /* user provides its own filename */
		fname = atoms->text;
	else
		fname = ".xsvf";
	while ( (direntry = readdir(dir)) ) {
		snprintf(fname, sizeof(fname), "/dev/%s", direntry->d_name);
		stat(fname, &fstat);
		if (majordev(fstat.st_rdev) == devID) { /* bingo */
			if ( nodeNm && (nodeCntr < elAm) )
				strncpy(nodeNm[nodeCntr], fname,
					sizeof(nodeNm[nodeCntr]));
			nodeCntr++;
		}
	}


	closedir(cwd);


	xsvfExecute();


	return cmdd->pa + 1; /* set index to the last (Terminator atom) */
}

int JtagLoader(char *fname) {
	int cc;
	CtrDrvrVersion version;
	CtrDrvrTime t;

	inp = fopen(fname,"r");
	if (inp) {
		if (ioctl(ctr,CtrDrvrJTAG_OPEN,NULL)) {
			IErr("JTAG_OPEN",NULL);
			return 1;
		}

		cc = xsvfExecute(); /* Play the xsvf file */
		printf("\n");
		if (cc) printf("Jtag: xsvfExecute: ReturnCode: %d Error\n",cc);
		else    printf("Jtag: xsvfExecute: ReturnCode: %d All OK\n",cc);
		fclose(inp);

		if (ioctl(ctr,CtrDrvrJTAG_CLOSE,NULL) < 0) {
			IErr("JTAG_CLOSE",NULL);
			return 1;
		}

		sleep(5); /* Wait for hardware to reconfigure */

		bzero((void *) &version, sizeof(CtrDrvrVersion));
		if (ioctl(ctr,CtrDrvrGET_VERSION,&version) < 0) {
			IErr("GET_VERSION",NULL);
			return 1;
		}
		t.Second = version.VhdlVersion;
		printf("New VHDL bit-stream loaded, Version: [%u] %s\n",
			(int) version.VhdlVersion,
			TimeToStr(&t,0));
	} else {
		perror("fopen");
		printf("Error: Could not open the file: %s for reading\n",fname);
		return 1;
	}
	return cc;
}
#endif /* if 0 */
