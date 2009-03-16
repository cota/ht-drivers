/**
 * @file cmd_generic.c
 *
 * @brief Generic handlers for extest's built-in commands
 *
 * This hasn't been done yet.
 * It would be wise to use for these handler the 'general IOCTL' concept;
 * that is, we should have a separate--and generic--set of IOCTLs
 * (or a user-space library) to be able to handle these frequently used
 * commands.
 * The GIOCTL set used in mil1553's Linux' driver might be a good start.
 *
 * @author Copyright (C) 2009 CERN CO/HT
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <cmd_handlers.h>

extern int snprintf(char *s, size_t n, const char *format, ...);

int hndl_swdeb(struct cmd_desc* cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_getversion(struct cmd_desc* cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_module(struct cmd_desc *cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_nextmodule(struct cmd_desc *cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_rawio(struct cmd_desc* cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_maps(struct cmd_desc *cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_timeout(struct cmd_desc *cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_queue(struct cmd_desc *cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_clients(struct cmd_desc *cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_connect(struct cmd_desc *cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_enable(struct cmd_desc *cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_reset(struct cmd_desc* cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_getstatus(struct cmd_desc* cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_waitintr(struct cmd_desc* cmdd, struct atom *atoms)
{
	return 1;
}

int hndl_simintr(struct cmd_desc* cmdd, struct atom *atoms)
{
	return 1;
}

#if 0 /* JTAG -- to be implemented */
int hndl_jtag_vhdl(struct cmd_desc* cmdd, struct atom *atoms)
{
	return 1;
}
#endif /* JTAG */
