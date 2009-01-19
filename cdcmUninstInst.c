/**
 * @file cdcmUninstInst.c
 *
 * @brief Linux/Lynx driver installation/uninstallation.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 28/06/2007
 *
 * Helps to unify driver install procedure between two systems. All standart
 * options are parsed here and CDCM group description is build-up.
 * Module-specific options are parsed using defined module-specific vectors.
 * Inspired by module-init-tools-3.2/insmod.c and rmmod,
 * written by Rusty Russell. Thxs!
 *
 * @version $Id: cdcmUninstInst.c,v 1.6 2009/01/09 10:26:03 ygeorgie Exp $
 */
#ifdef __linux__
#define _GNU_SOURCE /* asprintf rocks */
#include <sys/sysmacros.h>	/* for major, minor, makedev */
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stropts.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <include/general_both.h>
#include <include/list.h>
#include <cdcm/cdcmInfoT.h>
#include <cdcm/cdcmUninstInst.h>
#include <cdcm/cdcmUsrKern.h>

#ifdef __linux__

#define streq(a,b) (strcmp((a),(b)) == 0)
static char *linux_do_cdcm_info_header_file(int);
static void *linux_grab_file(const char*, unsigned long*);
static const char *linux_moderror(int);
/* sys_init_module system call */
extern long init_module(void *, unsigned long, const char *);

#else  /* __Lynx__ */

extern char *optarg; /* for getopt() calls */
extern int   getopt _AP((int, char * const *, const char *));

#endif

static void init_lists(void);
static char *get_optstring(void);
static opt_char_cap_t* get_char_cap(int);
static void educate_user(void);
static void check_mod_compulsory_params(cdcm_md_t*);
static void assert_all_mod_descr(void);
static void assert_compulsory_vectors(void);
static int  check_and_set_option_chars(char*);

/* module description */
static cdcm_md_t mod_desc[MAX_MODULE_AM] = {
  [0 ... MAX_MODULE_AM-1] {
    .md_lun      = _NOT_DEF_,
    .md_mpd      =  1,
    .md_ivec     = -1,
    .md_ilev     = -1,
    .md_vme1addr = _NOT_DEF_,
    .md_vme2addr = _NOT_DEF_,
    .md_pciVenId = -1,
    .md_pciDevId = -1,
    .md_opaque   =  NULL,
    .md_owner    =  NULL
  }
};

/* !WARNING! Keep next variable in concistency with def_opt_t definition! */
/* default option characters capability */
static opt_char_cap_t cdcm_def_opt_char_cap[] = {
  [9] {             /* G */
    .opt_val      = P_T_GRP,
    .opt_redef    = 0, /* not (re)defined by the user */
    .opt_with_arg = 1, /* requires an arg. */
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[9].opt_list)
  },
  [8] {             /* U */
    .opt_val      = P_T_LUN,
    .opt_redef    = 0,
    .opt_with_arg = 1,
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[8].opt_list)
  },
  [7] {             /* O */
    .opt_val      = P_T_ADDR,
    .opt_redef    = 0,
    .opt_with_arg = 1,
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[7].opt_list)
  },
  [6] {             /* M */
    .opt_val      = P_T_N_ADDR,
    .opt_redef    = 0,
    .opt_with_arg = 1,
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[6].opt_list)
  },
  [5] {             /* L */
    .opt_val      = P_T_ILEV,
    .opt_redef    = 0,
    .opt_with_arg = 1,
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[5].opt_list)
  },
  [4] {             /* V */
    .opt_val      = P_T_IVECT,
    .opt_redef    = 0,
    .opt_with_arg = 1,
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[4].opt_list)
  },
  [3] {             /* C */
    .opt_val      = P_T_CHAN,
    .opt_redef    = 0,
    .opt_with_arg = 1,
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[3].opt_list)
  },
  [2] {             /* T */
    .opt_val      = P_T_TRANSP,
    .opt_redef    = 0,
    .opt_with_arg = 1,
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[2].opt_list)
  },
  [1] {             /* h */
    .opt_val      = P_T_HELP,
    .opt_redef    = 0,
    .opt_with_arg = 0, /* no arg. needed */
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[1].opt_list)
  },
  [0] {             /* , */
    .opt_val      = P_T_SEPAR,
    .opt_redef    = 0,
    .opt_with_arg = 0, /* argument is compulsory */
    .opt_list     = LIST_HEAD_INIT(cdcm_def_opt_char_cap[0].opt_list)
  }
};

/* option characters list */
LIST_HEAD(glob_opt_list);

/* Claimed groups list. It's incremently ordered by module index [0-20] */
LIST_HEAD(glob_grp_list);

/* global parameters, common for all the groups */
static cdcm_glob_t glob_params = {
  .glob_opaque = NULL
};

/* group description */
static cdcm_grp_t grp_desc[MAX_GROUP_AM] = {
  [0 ... MAX_GROUP_AM-1] {
    .mod_amount   = 0,
    .grp_opaque   = NULL,
    .grp_glob     = &glob_params
  }
};


/**
 * @brief Miscellaneous list initialization.
 *
 * @param none
 *
 * @return void
 */
static void init_lists(void)
{
  int i;

  /* for modules */
  for (i = 0; i < MAX_MODULE_AM; i++)
    INIT_LIST_HEAD(&mod_desc[i].md_list); /* init module list */

  /* for groups */
  for (i = 0; i < MAX_GROUP_AM; i++) {
    /* init group lists */
    INIT_LIST_HEAD(&grp_desc[i].grp_list); 
    INIT_LIST_HEAD(&grp_desc[i].mod_list);

    /* set group number */
    grp_desc[i].grp_num = i+1;
  }

  /* add pre-defined option characters */
  for (i = 0; i < sizeof(cdcm_def_opt_char_cap)/sizeof(*cdcm_def_opt_char_cap); i++)
    list_add(&cdcm_def_opt_char_cap[i].opt_list/*new*/, &glob_opt_list/*head*/);
}


/**
 * @brief 
 *
 * @param optch - option character in question.
 *
 * @return option character capabilities pointer - if OK.
 * @return NULL - if FAILS.
 */
static inline opt_char_cap_t* get_char_cap(int optch)
{
  opt_char_cap_t *capP;
  list_for_each_entry(capP, &glob_opt_list, opt_list) {
    if (capP->opt_val == optch)
      return(capP);
  }
  return(NULL);
}


/**
 * @brief Build-up command line options string, including module-specific
 *        options, if any.
 *
 * @param none
 *
 * @return command line option string pointer - if OK.
 * @return NULL pointer                       - if FAILS.
 */
#define MAX_OPT_STR_LENGTH 128	/* max allowed option string length */
static char* get_optstring(void)
{
  char *usr_optstr = NULL;
  // option string will look something like this: "-G:U:O:M:L:V:C:T:h,"
  static char cdcm_optstr[MAX_OPT_STR_LENGTH] = { "-" };
  char *ptr = cdcm_optstr + strlen(cdcm_optstr);
  opt_char_cap_t *lptr;

  /* hook module-specific options, if any */
  if (cdcm_inst_ops.mso_get_optstr)
    usr_optstr = (*cdcm_inst_ops.mso_get_optstr)();
  
  if (check_and_set_option_chars(usr_optstr) < 0)
    return(NULL);
  
  /* now we've got all options in glob_opt_list list.
     Can build option string now for getopt() */
  list_for_each_entry(lptr, &glob_opt_list, opt_list) {
    sprintf(ptr, "%c%s", lptr->opt_val, (lptr->opt_with_arg) ? ":" : "");
    if (lptr->opt_with_arg)
      ptr +=2;
    else
      ptr++;
  }

  return(cdcm_optstr);
}


/**
 * @brief User education. !TODO!
 *
 * @param none
 *
 * @return void
 */
static void educate_user(void)
{

}


/**
 * @brief Cheks if all compulsory parameters are provided for the module.
 *
 * @param chk - let's checkit
 *
 * Compulsory parameters are First Base Address and LUN.
 * If error detected - programm will terminate.
 *
 * @return void
 */
static void check_mod_compulsory_params(cdcm_md_t *chk)
{
  
  if (chk->md_lun == _NOT_DEF_) { /* LUN not provided */
    fprintf(stderr, "LUN is NOT provided!\n");
    goto do_barf;
  }

  if (chk->md_vme1addr == _NOT_DEF_) { /* base address is not here. too bad! */
    fprintf(stderr, "Base address is not provided!\n");
    goto do_barf;
  }

  return; /* we cool */

 do_barf:
  fprintf(stderr, "Wrong params detected!\nAborting...\n");
  if (cdcm_inst_ops.mso_cleanup)
    (*cdcm_inst_ops.mso_cleanup)();
  exit(EXIT_FAILURE);		/* 1 */
}


/**
 * @brief How many declared modules
 *
 * @param none
 *
 * @return declared module amount
 */
static inline int __module_am(void)
{
  int mc = 0;

  while (mod_desc[mc].md_lun != _NOT_DEF_)
    mc++;

  return(mc);
}


/**
 * @brief Check already prepared module descriptions for any errors and
 *        declaration collisions.
 *
 * @param none
 *
 * @return void
 */
static void assert_all_mod_descr(void)
{
  int mc = 0, cntr1, cntr2;

  mc = __module_am();	/* claimed module amount */

  /* pass through all module list */
  for (cntr1 = 0; cntr1 < mc; cntr1++) {
    for (cntr2 = cntr1+1; cntr2 < mc; cntr2++) {

      /* LUN checking */
      if (mod_desc[cntr1].md_lun == mod_desc[cntr2].md_lun) {
	fprintf(stderr, "Identical LUNs (%d) detected!\nAborting...\n", mod_desc[cntr1].md_lun);
	if (cdcm_inst_ops.mso_cleanup)
	  (*cdcm_inst_ops.mso_cleanup)();
	exit(EXIT_FAILURE);
      }

      /* first base address checking */
      if (mod_desc[cntr1].md_vme1addr == mod_desc[cntr2].md_vme1addr) {
	fprintf(stderr, "Identical Base VME addresses (0x%x) detected!\nAborting...\n", mod_desc[cntr1].md_vme1addr);
	if (cdcm_inst_ops.mso_cleanup)
	  (*cdcm_inst_ops.mso_cleanup)();
	exit(EXIT_FAILURE);
      }

      /* second base address checking (if defined) */
      if (mod_desc[cntr1].md_vme2addr != _NOT_DEF_) {
	if (mod_desc[cntr1].md_vme2addr == mod_desc[cntr2].md_vme2addr) {
	  fprintf(stderr, "Identical Second VME addresses (0x%x) detected!\nAborting...\n", mod_desc[cntr1].md_vme2addr);
	  if (cdcm_inst_ops.mso_cleanup)
	    (*cdcm_inst_ops.mso_cleanup)();
	  exit(EXIT_FAILURE);
	}
      }
      
    }
  }
  
}


/**
 * @brief Check if all compulsory vectors are provided in @b cdcm_inst_ops
 *        vector table.
 *
 * @param none
 *
 * @return void
 */
static void assert_compulsory_vectors(void)
{
  if (!strlen(cdcm_inst_ops.mso_module_name)) {
    fprintf(stderr, "Compulsory member 'mso_module_name' is not provided. Can't get module name!\n");
    goto no_compulsory_vectors;
  }
  return;	/* we cool */
  
 no_compulsory_vectors:
  fprintf(stderr, "Aborting...\n");
  if (cdcm_inst_ops.mso_cleanup)
    (*cdcm_inst_ops.mso_cleanup)();
  exit(EXIT_FAILURE);
}


/**
 * @brief Check, if pre-defined option characters were re-defined by the user.
 *
 * @param usroptstr - user-defined option string
 *
 * If they were, then redefinition flag is set to ON.
 *
 * @return how many new option characters added - if SUCCESS.
 * @return -1  - if FAILS. Printout error message on stderr.
 */
/* how many extra option characters user can add */
#define MAX_USR_DEF_OPT_CHARS 64
static int check_and_set_option_chars(char *usroptstr)
{
  int uocnt = 0, cntr; /* counters */
  int new_opt = 0;
  opt_char_cap_t *lptr;		/* list pointer */
  static opt_char_cap_t usr_opt[MAX_USR_DEF_OPT_CHARS] = { { 0 } };
  char *uoP = usroptstr;

  if (usroptstr) {
    /* check, if user is trying to redefine '-,' or '-G' options */
    if (strchr(usroptstr, ',') || strchr(usroptstr, 'G')) {
      fprintf(stderr, "ERROR! Redefinition of '-,' and '-G' option characters is NOT allowed!\n");
      return(-1);
    }
    
    while (*uoP) {
      if (!uocnt && *uoP == ':') {
	fprintf(stderr, "ERROR! Wrong option character string format!\n");
	return(-1);
      }
      if (uocnt == MAX_USR_DEF_OPT_CHARS) {
	fprintf(stderr, "ERROR! Too many user-defined option characters!\n");
	return(-1);
      }

      usr_opt[uocnt].opt_val      = *uoP; /* opt value */
      usr_opt[uocnt].opt_redef    = 1; /* user (re)defined opt */
      usr_opt[uocnt].opt_with_arg = 0; /* require arg? */
      INIT_LIST_HEAD(&usr_opt[uocnt].opt_list);

      /* first one */
      if (!uocnt) {
	uoP++;
	uocnt++;
	continue;
      }

      if (*uoP == ':')      
	usr_opt[uocnt-1].opt_with_arg = 1; /* option requires parameter */

      uoP++;
      if (usr_opt[uocnt].opt_val != ':')
	uocnt++;
    }
  }

  /* check, if user override pre-defined option characters */
  list_for_each_entry(lptr, &glob_opt_list, opt_list) {
    for (cntr = 0; cntr < uocnt; cntr++)
      if (lptr->opt_val == usr_opt[cntr].opt_val) {
	/* this is user-redefined option character */
	lptr->opt_redef = 1;
	lptr->opt_with_arg = usr_opt[cntr].opt_with_arg;
	usr_opt[cntr].opt_val = 0; /* no need to add it in the opt char list */
      }
  }

  /* add new option characters to the list */
  for (cntr = 0; cntr < uocnt; cntr++)
    if (usr_opt[cntr].opt_val) {
      ++new_opt;
      list_add(&usr_opt[cntr].opt_list/*new*/, &glob_opt_list/*head*/);
    }
  
  return(new_opt); /* we cool */
}



/**
 * @brief 
 *
 * @param gp - group
 * @param mp - module
 *
 * @return  0 - Ok
 * @return -1 - NOT Ok
 */
static void set_mod_descr(cdcm_grp_t *gp, cdcm_md_t *mp)
{
  gp->mod_amount++;

  /* add new module to the group */
  list_add(&mp->md_list/*new*/, &gp->mod_list/*head*/);
}


#define ASSERT_GRP(mlun)												\
do {															\
  if (cur_grp == -1) {													\
    fprintf(stderr, "Compulsory Current Group (G param) is not set for module [LUN #%d]!\nAborting...\n", mlun);	\
    if (cdcm_inst_ops.mso_cleanup)											\
      (*cdcm_inst_ops.mso_cleanup)();											\
    exit(EXIT_FAILURE);		/* 1 */											\
  }															\
} while (0)
/**
 * @brief Main function that @b SHOULD be called by the module-specific
 *        installation procedure command line args.
 *
 * @param argc - argument count
 * @param argv - argument vector
 * @param envp - environment variables
 *
 * @return @e cdcm_grp_t group linked list
 */
struct list_head* cdcm_inst_vme_arg_parser(int argc, char *argv[], char *envp[])
{
  int cur_lun, cur_grp = -1, cmdOpt, cntr;
  int grp_cntr = 0, comma_cntr = 0;  /* for grouping and module management */
  int isSet; /* module description is saved already */
  int seq_insp = 0;	/* parameter sequence inspector */
  opt_char_cap_t *cap_ptr;
  int coco = 0; /* completion code */
  const char *optstring; /* default + module-specific command line options */

  assert_compulsory_vectors();	/* will barf if something missing */
  init_lists();			/* init crap */

  if (!(optstring = get_optstring())) {
    fprintf(stderr, "Can't get command line options.\nAborting...\n");
    if (cdcm_inst_ops.mso_cleanup)
      (*cdcm_inst_ops.mso_cleanup)();
    exit(EXIT_FAILURE);
  }

  /* Scan params of the command line */
  while ( (coco != 2) && ((cmdOpt = getopt(argc, argv, optstring)) != EOF) ) {
    seq_insp++;
    isSet = 0;
    cap_ptr = get_char_cap(cmdOpt);
    if (cap_ptr && cap_ptr->opt_redef)
      goto user_def_option; /* redefined by user! */

    switch (cmdOpt) {
    case P_T_HELP:	/* 'h' Help information */
      educate_user();
      if (cdcm_inst_ops.mso_educate)
	(*cdcm_inst_ops.mso_educate)();
      exit(EXIT_SUCCESS);	/* 0 */
      break;
    case P_T_LUN:	/* 'U' Logical Unit Number */
      cur_lun = abs(atoi(optarg));
      if (!WITHIN_RANGE(MIN_MOD_NUM, cur_lun, MAX_MOD_NUM)) {
	fprintf(stderr, "Wrong LUN (U param). Allowed absolute range is [%d - %d]\nAborting...\n", MIN_MOD_NUM, MAX_MOD_NUM);
	if (cdcm_inst_ops.mso_cleanup)
	  (*cdcm_inst_ops.mso_cleanup)();
	exit(EXIT_FAILURE);
      }
      mod_desc[comma_cntr].md_lun = cur_lun;
      break;
    case P_T_GRP:	/* 'G' Logical Group Coupling */
      if (seq_insp != 1 && cur_grp == -1) {
	/* should come first among module parameters */
	fprintf(stderr, "Group numbering error (G param). Should come FIRST among module parameters!\nAborting...\n");
	if (cdcm_inst_ops.mso_cleanup)
	  (*cdcm_inst_ops.mso_cleanup)();
	exit(EXIT_FAILURE);
      }

      if (comma_cntr && !grp_cntr) {
	/* should exist from the very beginning i.e. before _any_ separator */
	fprintf(stderr, "Group numbering error (G param). Should come BEFORE any module description and AFTER any global parameters!\nAborting...\n");
	if (cdcm_inst_ops.mso_cleanup)
	  (*cdcm_inst_ops.mso_cleanup)();
	exit(EXIT_FAILURE);
      }

      cur_grp = atoi(optarg);

      if (!WITHIN_RANGE(MIN_GRP_NUM, cur_grp, MAX_GRP_NUM)) {
	fprintf(stderr, "Wrong group number [#%d] (G param). Allowed range is [%d - %d]\nAborting...\n", cur_grp, MIN_GRP_NUM, MAX_GRP_NUM);
	if (cdcm_inst_ops.mso_cleanup)
	  (*cdcm_inst_ops.mso_cleanup)();
	exit(EXIT_FAILURE);	/* 1 */
      }
      cur_grp -= 1; /* set correct table index */
      mod_desc[comma_cntr].md_owner = &grp_desc[cur_grp]; /* set owner */
      grp_cntr++;
      break;
    case P_T_ADDR:	/* 'O' First base address */
      mod_desc[comma_cntr].md_vme1addr = strtol(optarg, NULL, 16);
      break;
    case P_T_N_ADDR:	/* 'M' Second base address */
      mod_desc[comma_cntr].md_vme2addr = strtol(optarg, NULL, 16);
      break;
    case P_T_IVECT:	/* 'V' Interrupt vector */
      mod_desc[comma_cntr].md_ivec = atoi(optarg);
      break;
    case P_T_ILEV:	/* 'L' Interrupt level */
      mod_desc[comma_cntr].md_ilev = atoi(optarg);
      break;
    case P_T_TRANSP:	/* 'T' Transparent String driver parameter */
      break;
    case P_T_CHAN: /* 'C' Channel amount (amount of minor devices to create) */
      mod_desc[comma_cntr].md_mpd = atoi(optarg);
      break;
    case P_T_SEPAR:	/* ',' - End of module device definition */
      if (comma_cntr == MAX_MODULE_AM) {
	fprintf(stderr, "Too many modules declared (max is %d)\nAborting...\n", MAX_MODULE_AM);
	if (cdcm_inst_ops.mso_cleanup)
	  (*cdcm_inst_ops.mso_cleanup)();
	exit(EXIT_FAILURE);
      }
      
      /* set default LUN if not provided */
      if (mod_desc[comma_cntr].md_lun == _NOT_DEF_)
	mod_desc[comma_cntr].md_lun = comma_cntr;

      ASSERT_GRP(mod_desc[comma_cntr].md_lun);

      /* will barf if error */
      check_mod_compulsory_params(&mod_desc[comma_cntr]);
      set_mod_descr(&grp_desc[cur_grp], &mod_desc[comma_cntr]);

      comma_cntr++;
      isSet = 1;
      seq_insp = 0;	/* reset inspector */
      break;
    case '?':	/* error */
      fprintf(stderr, "Wrong/unknown argument!\n");
      if (cdcm_inst_ops.mso_educate)
	(*cdcm_inst_ops.mso_educate)();
      if (cdcm_inst_ops.mso_cleanup)
	(*cdcm_inst_ops.mso_cleanup)();
      exit(EXIT_FAILURE);	/* 1 */
      break;
      /* all the rest is not interpreted by the CDCM arg parser */
#ifdef __linux__
    case 1: /* this is non-option args */
#else /* __Lynx__ */
    case 0:
#endif
    default:
    user_def_option:
      seq_insp--; /* we know nothing about module-specific options */
      /* this _is_ module specific argument */
      if (cdcm_inst_ops.mso_parse_opt) {
	/* and user provide us with parsing vector */
	coco = (*cdcm_inst_ops.mso_parse_opt)((cur_grp == -1)?((void*)&glob_params):((void*)&mod_desc[comma_cntr]), cur_grp, cmdOpt, optarg);
	if (coco) {
	  switch (coco) {
	  case 1:
	    fprintf(stderr, "Wrong params detected!\nAborting...\n");
	    if (cdcm_inst_ops.mso_cleanup)
	      (*cdcm_inst_ops.mso_cleanup)();
	    exit(EXIT_FAILURE); /* 1 */
	    break;
	  case 2:
	    fprintf(stderr, "Param scanning finished due to user request\n");
	    break;
	  }
	}
      }	else {
	fprintf(stderr, "Wrong/Unknown option '%c'!\nAborting...\n", cmdOpt);
	if (cdcm_inst_ops.mso_cleanup)
	  (*cdcm_inst_ops.mso_cleanup)();
	exit(EXIT_FAILURE);
      }
      break;
    } /* end of switch */
  } /* end of while */

  if (!isSet) { /* we neet to set the last module config */
    /* set default LUN if not provided */
    if (mod_desc[comma_cntr].md_lun == _NOT_DEF_)
      mod_desc[comma_cntr].md_lun = comma_cntr;

    ASSERT_GRP(mod_desc[comma_cntr].md_lun);

    /* will barf if error */
    check_mod_compulsory_params(&mod_desc[comma_cntr]); 
    set_mod_descr(&grp_desc[cur_grp], &mod_desc[comma_cntr]);
  }

  /* final check for conflicts in module declaration */
  assert_all_mod_descr();

  /* set sorted group list */
  for (cntr = 0; cntr < MAX_GROUP_AM; cntr++)
    if (grp_desc[cntr].mod_amount) /* group is claimed, add it to the list */
      list_add(&grp_desc[cntr].grp_list/*new*/, &glob_grp_list/*head*/);

  return(&glob_grp_list);
}


/**
 * @brief Get all currently defined groups.
 *
 * @param none
 *
 * @return @e cdcm_grp_t group linked list
 */
struct list_head* cdcm_inst_get_groups(void)
{
  return(&glob_grp_list);
}


#ifdef __linux__
/**
 * @brief CDCM service node name container. Looks like @e /dev/cdcm_<mod_name>
 *
 * @param none
 *
 * @return service node name
 */
static inline char* __linux_srv_node(void)
{
  static char cdcm_s_n_nm[64] = { 0 };

  if (cdcm_s_n_nm[0])		/* already defined */
    return(cdcm_s_n_nm);
  
  snprintf(cdcm_s_n_nm, sizeof(cdcm_s_n_nm), "/dev/%s", __srv_dev_name(NULL));
  
  return(cdcm_s_n_nm);
}


/**
 * @brief Driver installation Lynx call wrapper.
 *
 * @param drvr_fn - driver path
 * @param type    - for now @b ONLY char devices
 *
 * Will install the driver in the system. Supposed to be called @b ONLY once
 * during installation procedure. If more - then it will terminate
 * installation.
 *
 * @return driver id - if successful
 * @return -1        - if not.
 */
int dr_install(char *drvr_fn, int type)
{
  static int c_cntr = 0; /* how many times i was already called */
  void *drvrfile = NULL;
  FILE *procfd   = NULL;
  unsigned long len;
  char *options;		/* driver option */
  long int ret;
  char buff[32], *bufPtr; /* for string parsing */
  int major_num = 0;
  int devn; /* major/minor device number */
  char *itf;	/* info table filepath */
  int grp_bits = 0;

  if (c_cntr > 1) {	/* oooops... */
    fprintf(stderr, "%s() was called more then once. Should NOT happen!\nAborting...\n", __FUNCTION__);
    if (cdcm_inst_ops.mso_cleanup)
      (*cdcm_inst_ops.mso_cleanup)();
    exit(EXIT_FAILURE);
  } else {
    cdcm_grp_t *grpp;
    list_for_each_entry(grpp, &glob_grp_list, grp_list)
      grp_bits |= 1<<((grpp->grp_num)-1); /* set group bit mask */
  }

  /* put .ko in the local buffer */
  if (!(drvrfile = linux_grab_file(drvr_fn, &len))) {
    fprintf(stderr, "insmod: can't read '%s': %s\n", drvr_fn, strerror(errno));
    if (cdcm_inst_ops.mso_cleanup)
      (*cdcm_inst_ops.mso_cleanup)();
    exit(EXIT_FAILURE);
  }

  /* create info file */
  if ( (itf = linux_do_cdcm_info_header_file(grp_bits)) == NULL) {
    free(drvrfile);
    if (cdcm_inst_ops.mso_cleanup)
      (*cdcm_inst_ops.mso_cleanup)();
    exit(EXIT_FAILURE);
  }
    
  asprintf(&options, "ipath=%s",  itf);	/* set parameter */
  
  /* insert module in the kernel */
  if ( (ret = init_module(drvrfile, len, options)) != 0 ) {
    fprintf(stderr, "insmod: error inserting '%s': %li %s\n", drvr_fn, ret, linux_moderror(errno));
    free(drvrfile);
    free(options);
    if (cdcm_inst_ops.mso_cleanup)
      (*cdcm_inst_ops.mso_cleanup)();
    exit(EXIT_FAILURE);
  }

  free(drvrfile);
  free(options);

  /* check if he made it */
  if ( (procfd = fopen("/proc/devices", "r")) == NULL ) {
    mperr("Can't open /proc/devices");
    if (cdcm_inst_ops.mso_cleanup)
      (*cdcm_inst_ops.mso_cleanup)();
    exit(EXIT_FAILURE);
  }
  
  /* search for the device driver entry */
  bufPtr = buff;
  while (fgets(buff, sizeof(buff), procfd))
    if (strstr(buff, __srv_dev_name(NULL))) { /* bingo! */
      major_num = atoi(strsep(&bufPtr, " "));
      break;
    }
  
  /* check if we cool */
  if (!major_num) {
    fprintf(stderr, "'%s' device NOT found in proc fs.\n", __srv_dev_name(NULL));
    if (cdcm_inst_ops.mso_cleanup)
      (*cdcm_inst_ops.mso_cleanup)();
    exit(EXIT_FAILURE);
  } else 
    printf("\n'%s' device driver installed. Major number %d.\n", __srv_dev_name(NULL), major_num);

  /* create service entry point */
  unlink(__linux_srv_node()); /* if already exist delete it */
  devn = makedev(major_num, 0);
  if (mknod(__linux_srv_node(), S_IFCHR | 0666, devn)) {
    mperr("Can't create %s service node", __linux_srv_node());
    if (cdcm_inst_ops.mso_cleanup)
      (*cdcm_inst_ops.mso_cleanup)();
    exit(EXIT_FAILURE);
  }
  chmod(__linux_srv_node(), 0666);

  c_cntr++;
  return(major_num); /* 'fake' driver id will be used by the cdv_install */
}


/**
 * @brief Lynx call wrapper. Will uninstall the driver in the system. !TODO!
 *
 * @param id - driver ID to uninstall
 *
 * Supposed to be called @b ONLY once during uninstallation.
 *
 * @return 
 */
int dr_uninstall(int id)
{
  return(-1);
}


/**
 * @brief Lynx call wrapper for character device installation.
 *
 * @param path      - info file
 * @param driver_id - service entry point major number
 * @param extra     - 
 *
 * Can be called several times from the driver installation procedure.
 * Depends on how many major devices user wants to install. If installation
 * programm is written correctly, then it should be called as many times as
 * groups declared. i.e. each group represents exactly one character device.
 * By Lynx driver strategy - each time @e cdv_install() (or @e bdv_install)
 * is called by the user-space installation programm, driver install vector
 * from @b dldd structure is activated. It returns statics table, that in turn
 * is used in every entry point of 'dldd' vector table.
 *
 * @return major device ID - if successful.
 * @return -1              - if not.
 */
int cdv_install(char *path, int driver_id, int extra)
{
  static int c_cntr = 0; /* how many times i was already called */
  int cdcmfd  = -1;
  int maj_num = -1;

  c_cntr++;

  if (c_cntr > list_capacity(&glob_grp_list)) {	/* oooops... */
    fprintf(stderr, "%s() was called more times, then groups declared. Should NOT happen!\nAborting...\n", __FUNCTION__);
    return(-1);
  }

  if ((cdcmfd = open(__linux_srv_node(), O_RDWR)) == -1) {
    mperr("Can't open %s service node", __linux_srv_node());
    return(-1);
  }

  if ( (maj_num = ioctl(cdcmfd, CDCM_CDV_INSTALL, path)) < 0) {
    mperr("CDCM_CDV_INSTALL ioctl fails. Can't get major number");
    close(cdcmfd);
    return(-1);
  }

  close(cdcmfd);
  return(maj_num);
}


/**
 * @brief Lynx call wrapper. Can be called several times from the driver
 *        uninstallation procedure.
 *
 * @param cdevice_ID
 *
 * @return 
 */
int cdv_uninstall(int cdevice_ID)
{
  return(-1);
}


/**
 * @brief Setup CDCM header and create info file that will passed to
 *        the driver druring installation.
 *
 * @param grp_bit_mask - claimed groups mask
 *
 * @return info file name - if we are cool
 * @return NULL           - if we are not
 */
static char* linux_do_cdcm_info_header_file(int grp_bit_mask)
{
  static char cdcm_if_nm[PATH_MAX] = { 0 };
  int ifd; /* info file descriptor */

  /* buidup CDCM header */
  cdcm_hdr_t cdcm_hdr = {
    .cih_signature = ASSERT_MSB(CDCM_MAGIC_IDENT),
    .cih_grp_bits  = grp_bit_mask
    //.cih_grp_bits  = ASSERT_MSB(list_capacity(&glob_grp_list))
  };

  /* write specific module name */
  strncpy(cdcm_hdr.cih_dnm, cdcm_inst_ops.mso_module_name, sizeof(cdcm_hdr.cih_dnm));

  snprintf(cdcm_if_nm, sizeof(cdcm_if_nm), "/tmp/%s.info", __srv_dev_name(cdcm_inst_ops.mso_module_name));

  if ( (ifd = open(cdcm_if_nm, O_CREAT | O_RDWR, 0664)) == -1) {
    mperr("Can't open %s info file", cdcm_if_nm);
    return(NULL);
  }

  /* write CDCM header */
  if( write(ifd, &cdcm_hdr, sizeof(cdcm_hdr)) == -1) {
    mperr("Can't write into %s info file", cdcm_if_nm);
    close(ifd);
    return(NULL);
  }

  close(ifd);
  return(cdcm_if_nm);
}


/**
 * @brief 
 *
 * @param filename -  .ko file name
 * @param size     - its size will be placed here
 *
 * @return void
 */
static void* linux_grab_file(const char *filename, unsigned long *size)
{
  unsigned int max = 16384;
  int ret, fd;
  void *buffer = malloc(max);
  if (!buffer)
    return NULL;
  
  if (streq(filename, "-"))
    fd = dup(STDIN_FILENO);
  else
    fd = open(filename, O_RDONLY, 0);
  
  if (fd < 0)
    return NULL;
  
  *size = 0;
  while ((ret = read(fd, buffer + *size, max - *size)) > 0) {
    *size += ret;
    if (*size == max)
      buffer = realloc(buffer, max *= 2);
  }
  if (ret < 0) {
    free(buffer);
    buffer = NULL;
  }
  close(fd);
  return(buffer);
}


/**
 * @brief We use error numbers in a loose translation...
 *
 * @param err
 *
 * @return 
 */
static const char* linux_moderror(int err)
{
  switch (err) {
  case ENOEXEC:
    return "Invalid module format";
  case ENOENT:
    return "Unknown symbol in module";
  case ESRCH:
    return "Module has wrong symbol version";
  case EINVAL:
    return "Invalid parameters";
  default:
    return strerror(err);
  }
}
#endif /* __linux__ */
