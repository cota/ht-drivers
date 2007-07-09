/* $Id: cdcmInst.h,v 1.1 2007/07/09 09:26:29 ygeorgie Exp $ */
/*
; Module Name:	cdcmInst.h
; Module Descr:	All concerning driver installation is located here.
; Date:         June, 2007.
; Author:       Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
;
;
; -----------------------------------------------------------------------------
; Revisions of cdcmInst.h: (latest revision on top)
;
; #.#   Name       Date       Description
; ---   --------   --------   -------------------------------------------------
; 2.0   ygeorgie   09/07/07   Production release, CVS controlled.
; 1.0	ygeorgie   28/06/07   Initial version.
*/
#ifndef _CDCM_INST_H_INCLUDE_
#define _CDCM_INST_H_INCLUDE_

/* predefined option characters that will be parsed by default. They should
   not be used by the module specific part to avoid collision */
typedef enum _tagDefaultOptionsChars {
  P_T_GRP     = 'G',	/* group designator */
  P_T_LUN     = 'U',	/* Logical Unit Number */
  P_T_ADDR    = 'O',	/* first base address */
  P_T_N_ADDR  = 'M',	/* second base address */
  P_T_ILEV    = 'L',	/* interrupt level */
  P_T_IVECT   = 'V',	/* interrupt vector */
  P_T_CHAN    = 'C',	/* channel amount for the module */
  P_T_FLG     = 'I',	/* driver flag */
  P_T_TRANSP  = 'T',	/* driver transparent parameter */
  P_T_HELP    = 'h',	/* help */
  P_T_SEPAR   = ',',	/* separator between module entries */

  P_ST_ADDR   = ' ',	/* slave first base address */
  P_ST_N_ADDR =	' '	/* slave second base address */
} def_opt_t;

/* vectors that will be called (if not NULL) by the module-specific install
   programm to handle specific part of the module installation procedure */
struct module_spec_ops {
  void  (*mso_educate)();       /* for user education */
  char* (*mso_get_optstr)();	/* module-spesific options in 'getopt' form */
  int   (*mso_parse_opt)(char, char*); /* parse module-spesific option */
  char* (*mso_get_mod_name)(); /* module-specific name. COMPULSORY vector */
};

extern struct module_spec_ops cdcm_inst_ops;

#ifdef __linux__
#define BLOCKDRIVER	1
#define CHARDRIVER	0
int dr_install(char*, int);
int cdv_install(char*, int, int);
#else  /* __Lynx__ */
static inline int makedev(int major, int minor)
{
  int dev_num;
  dev_num = (major << 8) + minor;
  return(dev_num);
}
#endif /* __Lynx__ */

struct list_head cdcm_vme_arg_parser(int argc, char *argv[], char *envp[]);

#endif /* _CDCM_INST_H_INCLUDE_ */
