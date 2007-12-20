/* $Id: cdcmUninstInst.h,v 1.7 2007/12/20 08:43:12 ygeorgie Exp $ */
/**
 * @file cdcmUninstInst.h
 *
 * @brief All concerning driver installation and uninstallation is
 *        located here.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date June, 2007
 *
 * @version 3.0  ygeorgie  01/08/2007  Full Lynx-like installation behaviour.
 * @version 2.0  ygeorgie  09/07/2007  Production release, CVS controlled.
 * @version 1.0  ygeorgie  28/06/2007  Initial version.
 */
#ifndef _CDCM_UNINST_INST_H_INCLUDE_
#define _CDCM_UNINST_INST_H_INCLUDE_

#include <elf.h> /* for endianity business */
#include <cdcm/cdcmInfoT.h>
#include <cdcm/cdcmVars.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef  __Lynx__
extern int vsnprintf(char*, size_t,const char*, va_list);
#endif

/* swap bytes */
static inline void __endian(const void *src, void *dest, unsigned int size)
{
  unsigned int i;
  for (i = 0; i < size; i++)
    ((unsigned char*)dest)[i] = ((unsigned char*)src)[size - i - 1];
}

/* find out current endianity */
static inline int __my_endian()
{
  static int my_endian = ELFDATANONE;
  union { short s; char c[2]; } endian_test;
  
  if (my_endian != ELFDATANONE)	/* already defined */
    return(my_endian);
  
  endian_test.s = 1;
  if (endian_test.c[1] == 1) my_endian = ELFDATA2MSB;
  else if (endian_test.c[0] == 1) my_endian = ELFDATA2LSB;
  else abort();

  return(my_endian);
}

/* assert data to Big Endian */
#define ASSERT_MSB(x)				\
({						\
  typeof(x) __x;				\
  typeof(x) __val = (x);			\
  if (__my_endian() == ELFDATA2LSB)		\
    __endian(&(__val), &(__x), sizeof(__x));	\
  else						\
    __x = x;					\
  __x;						\
})

static inline void mperr(char *token, ...)
{
  char errstr[256];
  va_list ap;
  va_start(ap, token);
  vsnprintf(errstr, sizeof(errstr),  token, ap);
  va_end(ap);
  perror(errstr);
}

/* 
   Possible format of VME module installation string examples:
   -----------------------------------------------------------

   1. -> inst_prog.a -A1 -p2 -G9 -U2 -O0x3200 -L2 -V250 -, -U1 -O0x8359 -L2 -V234 -, -U5 -O0x5542 -L1 -V134 -, -G3 -U15 -O0x39837 -L5 -V185 -M0x98375 -, -U10 -O0x1875 -L3 -V180 -, -G5 -U12 -O0x23865 -S"transparent module param as a string"

   Here we declare two global parameters (-A1 and -p2), that are common for ALL
   groups and modules.
   We declare three groups (-G9, -G3 and -G5)
   Group1 (G9) has thee modules with LUN's (-U2, -U1 and -U5)
   Group2 (G3) has two modules (-U15 and -U10)
   Group3 (G5) has only one module (-U12)

   2. -> inst_prog.a -A1 -p2 -U2 -O0x3200 -L2 -V250 -, -U1 -O0x8359 -L2 -V234 -, -U5 -O0x5542 -L1 -V134

   Here we declare two global parameters (-A1 and -p2), that are common for ALL
   groups and modules.
   We declare no groups, means that user doesn't want any logical group
   coupling. In this case all modules are belonging to one logical group only,
   which is of no importanse for the user.
   We declare three modules (-U2, -U1 and -U5)
*/

/* Predefined option characters that will be parsed by default. Note, that
   almost all of them <b>(exept @e -, and @e -G)</b> could be re-defined by
   the module-specific installation part, which separates module and group
   descriptions respectivelly from each other. User-redefined options will
   not be parsed automatically and it's completely up to user to handle this
   options! If not redefined, then they've got the following meaning: */
typedef enum _tagDefaultOptionsChars {
  P_T_GRP     = 'G',	/* group designator !CAN'T BE REDEFINED! */
  P_T_LUN     = 'U',	/* Logical Unit Number */
  P_T_ADDR    = 'O',	/* first base address */
  P_T_N_ADDR  = 'M',	/* second base address */
  P_T_ILEV    = 'L',	/* interrupt level */
  P_T_IVECT   = 'V',	/* interrupt vector */
  P_T_CHAN    = 'C',	/* channel amount for the module */
  P_T_TRANSP  = 'T',	/* driver transparent String parameter */
  P_T_HELP    = 'h',	/* help */
  P_T_SEPAR   = ',',	/* separator between module entries
			   !CAN'T BE REDEFINED! */

  P_T_LAST    = 0xff,	/* indicates last valid option character */

  /* TO_ADD_IF_NECESSARY list */
  P_ST_ADDR   = ' ',	/* slave first base address */
  P_ST_N_ADDR =	' '	/* slave second base address */
} def_opt_t;

/* option characters and their capabilities */
typedef struct _tagOptCharCap {
  int opt_val;               /* wchich character */
  int opt_redef;             /* if (re)defined by the user? */
  int opt_with_arg;          /* if option requires an argument? */
  struct list_head opt_list; /* linked list */
} opt_char_cap_t;


/* vectors and data that will be called (if not NULL) by the module-specific
   install programm to handle specific part of the module installation
   procedure */
struct module_spec_inst_ops {
  void  (*mso_educate)();       /* for user education */
  char* (*mso_get_optstr)();	/* module-spesific options in 'getopt' form */
  int   (*mso_parse_opt)(void*, int, char, char*); /* parse module-spesific option */
  char mso_module_name[64]; /* COMPULSORY module-specific name */
};

/* user-defined installation vector table */
extern struct module_spec_inst_ops cdcm_inst_ops;

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

struct list_head* cdcm_vme_arg_parser(int, char *[], char *[]);

#endif /* _CDCM_UNINST_INST_H_INCLUDE_ */
