/* $Id$ */
/**
 * @file xmemInst.c
 *
 * @brief Installation of the xmem driver.
 *
 * @author Julian Lewis AB/CO/HT CERN
 *
 * @date Created on Oct 2004
 *
 * @version 2.0 Emilio G. Cota 22/10/2008, CDCM-compliant.
 * @version 1.0 Julian Lewis 01/10/2004
 */


#ifdef __linux__
#include <sys/param.h> /* for MAXPATHLEN */
#include <libgen.h> /* for basename */
#else /* __Lynx__ */
#include <proto.h>
#include <io.h>
#include <dir.h>
#endif /* !__linux__ */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xmemDrvr.h"

#include <common/include/list.h>
#include <cdcm/cdcmUninstInst.h>
#include <cdcm/cdcmInfoT.h>


static XmemDrvrInfoTable xmem_infotab = { {0,0} };
static char progNm[32]; /* program name */
/* driver object file name */
static char drvr_fn[] = "xmemPciDrvr."CPU".ko";
/* info file */
static char info_file[] = "/tmp/xmemDrvr.info";



/**
 * @brief Parsing command line args and build-up info table
 *
 * @param argc - argument count
 * @param argv - argument vector
 * @param envp - environment variables
 *
 * @return info table pointer
 */
static XmemDrvrInfoTable* build_info_table_and_parse_args(int argc, 
							  char *argv[],
							  char *envp[])
{
  int mcntr, i;
  cdcm_grp_t *grp_ptr;	/* group description pointer */
  /* module description pointer: */  
  cdcm_md_t  *md_ptr __attribute__((__unused__)); 
  struct list_head *grp_lst = cdcm_inst_vme_arg_parser(argc, argv, envp);

  /* pass through all the groups */
  mcntr=0;
  list_for_each_entry(grp_ptr, grp_lst, grp_list) {
#if 0    /* check if max group module amount is within range */
    if (grp_ptr->mod_amount > XmemDrvrMODULE_CONTEXTS) {
      printf("MAX module amount exeeded for the group!\nAborting...\n");
      exit(EXIT_FAILURE);
    }

    /* pass through all the modules in the group */
    mcntr = 0;
    list_for_each_entry(md_ptr, &(grp_ptr->mod_list), md_list) {
      //      infoModule = &(fpi_infotab.ModuleInfo[mcntr]);
      mcntr++;
      /* first base address */
      if ( ((md_ptr->md_vme1addr & 0x00000fff) == 0) 
	   && ((md_ptr->md_vme1addr & 0xffff0000) == 0) 
	   && (md_ptr->md_vme1addr > 0x00000fff) ) {
	infoModule->base = md_ptr->md_vme1addr;
	infoModule->size = sizeof(struct fpiT_vme);
      }

      /* int vect */
      if (md_ptr->md_ivec > 64 && md_ptr->md_ivec < 256)
	infoModule->vector[0] = (md_ptr->md_ivec & 0xff);
      else {
	printf("Interrupt vector=%d is out of (64-256) range!\nAborting...\n",
	       md_ptr->md_ivec);
	exit(EXIT_FAILURE);
      }

      /* int level */
      if (md_ptr->md_ilev > 1 || md_ptr->md_ilev < 5)
	infoModule->level[0] = md_ptr->md_ilev;
      else {
	printf("Interrupt level is out of range!\nAborting...\n");
	exit(EXIT_FAILURE);
      }
    } /* end of parsing all the modules in the group */
#endif
    printf("\n(TEST)Iteration #%d, parsing through all the groups\n", mcntr);
  } /* end of passing through all the groups */
  /* ----------------------------------------------------------------------- */

  /* set up info table */
  for (i = 0; i < XmemDrvrMODULE_CONTEXTS; i++) {
      //    if ( !InfoSetup(&(xmem_infotab.ModuleInfo[i])) )
      xmem_infotab.ModuleFlag[i] = 1; /* Flag = this module declared */
      //    else
      //      xmem_infotab.ModuleFlag[i] = 0; 
      //      /* Flag = this module non declared */
  }
  //  fpi_infotab.trailer.base = fpi_infotab.trailer.size = -1;
  return(&xmem_infotab);
}

static void xmem_educate(void)
{
  fprintf(stderr, "xmem Driver Install Program\n");
  fprintf(stderr, "Instructions:\n");
  fprintf(stderr, "LynxOS: call the program with no arguments.\n");
  fprintf(stderr, "Linux: no table file in this driver.\n");
  fprintf(stderr, "\t fill the groups, etc. with dummy values:\n");
  fprintf(stderr, "xmemInst -G1 -O0xdeadface -U1.\n");
  return;
}

/***************************************************************************/
/* Main                                                                    */
/***************************************************************************/

int main(int argc, char *argv[], char *envp[])
{
  char fpid[128];
  int  major, minor;
  char cwd[MAXPATHLEN];
  int  fd, did, extra = 0;
  XmemDrvrInfoTable *infotab;

  strncpy(progNm, basename(argv[0]), sizeof(progNm));

  infotab = build_info_table_and_parse_args(argc, argv, envp);

  getcwd(cwd, sizeof(cwd));
  printf("install %s in %s", drvr_fn, cwd);

  /* Create the file to save the info table */
  if ( (fd = open(info_file, O_WRONLY | O_CREAT, 0777)) < 0 ) {
    perror("\nxmemInstall: Cannot create driver informations table");
    exit(EXIT_FAILURE);
  }

  if (write(fd, (char*) infotab, sizeof(*infotab)) != sizeof(*infotab)) {
    perror("\nxmemInstall: Error writing driver informations table");
    close(fd);
    exit(EXIT_FAILURE);
  }

  close(fd);

  /* Install the driver in the system */
  if ((did = dr_install(drvr_fn, CHARDRIVER)) < 0) {
    fprintf(stderr, "\n%s: dr_install fails. Cannot install driver\n", progNm);
    exit(EXIT_FAILURE);
  }

  /* Install Major device associated to driver */
  if ((major = cdv_install(info_file, did, extra)) < 0) {
    fprintf(stderr, "\n%s: cdv_install fails. Cannot install device\n", progNm);
    exit(EXIT_FAILURE);
  }

  for (minor = 1; minor <= XmemDrvrCLIENT_CONTEXTS; minor++) {
    sprintf(fpid,"/dev/xmem.%1d",minor);
    unlink (fpid);
    if (mknod(fpid, (S_IFCHR | 0666), makedev(major, minor)) < 0) {
      printf("%s: Error making node: %s\n",progNm,fpid);
      perror(fpid);
      exit(EXIT_FAILURE);
    }
    /* we chmod explicitly because otherwise it seems not to work */
    chmod(fpid, S_IFCHR | 0666);
  }
  printf("\nAll done. EOF :-)\n");
  exit(EXIT_SUCCESS);
}

/* installation vector table. used by CDCM */
struct module_spec_inst_ops cdcm_inst_ops = {
  .mso_educate       = xmem_educate,
  .mso_get_optstr    = NULL,
  .mso_parse_opt     = NULL,
  .mso_module_name   = "xmem"	/* compulsory */
};



