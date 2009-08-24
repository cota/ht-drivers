/************************************************************************** */
/* Build an include file naming telegram groups and registers for including */
/* in the mtt assembler and in C-programs.                                  */
/* Julian Lewis AB/CO/HT                                                    */
/* Version Tuesday 19th October 2006                                        */
/************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include <tgv/tgv.h>
#include <tgm/tgm.h>

#define TARGET_FILE "mttregs.h"
#define LN 128
#define OFFSET 0

#include <asm.h>
#include "opcodes.c"

/* ============================== */
/* Get a file configuration path  */
/* ============================== */

#ifdef __linux__
static char *defaultconfigpath = "/dsrc/drivers/mtt/test/mttconfig.linux";
#else
static char *defaultconfigpath = "/dsc/data/mtt/Mtt.conf";
#endif

static char *configpath = NULL;

char *GetFile(char *name) {
FILE *gpath = NULL;
char txt[LN];
int i, j;

static char path[LN];

   if (configpath) {
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = NULL;
      }
   }

   if (configpath == NULL) {
      configpath = defaultconfigpath;
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = NULL;
	 sprintf(path,"./%s",name);
	 return path;
      }
   }

   bzero((void *) path,LN);

   while (1) {
      if (fgets(txt,LN,gpath) == NULL) break;
      if (strncmp(name,txt,strlen(name)) == 0) {
	 for (i=strlen(name); i<strlen(txt); i++) {
	    if (txt[i] != ' ') break;
	 }
	 j= 0;
	 while ((txt[i] != ' ') && (txt[i] != 0) && (txt[i] != '\n')) {
	    path[j] = txt[i];
	    j++; i++;
	 }
	 strcat(path,name);
	 fclose(gpath);
	 return path;
      }
   }
   fclose(gpath);
   return NULL;
}

/* =============================== */
/* Build Tgm task                  */
/* =============================== */

static TgmMachine tmch = TgmMACHINE_NONE;

int BuildTgmInc(FILE *fp) {

int gn;
TgmGroupDescriptor desc;

   if (tmch != TgmMACHINE_NONE) {
      if (TgmAttach(tmch,TgmGROUPS) != TgmSUCCESS) return 0;

      for (gn=1; gn<=TgmLastGroupNumber(tmch); gn++) {
	 TgmGetGroupDescriptor(tmch,gn,&desc);
	 fprintf(fp,"#define GRegTgm%s gr%d\n",desc.Name,gn+OFFSET);
      }
      gn--;
      fprintf(fp,"#define ConsTgmGROUPS %d\n",gn);
      return gn;
   }
   return 0;
}

/* ============================= */
/* Generate register definitions */
/* ============================= */

int GenerateRegDefs(FILE *fp) {

int i, j, k, cnt, ofs;
RegisterDsc *reg;

   fprintf(fp,"\n");

   ofs = 0; cnt = 0;
   for (i=0; i<REGNAMES; i++) {
      reg = &(Regs[i]);
      if (reg->Start == reg->End) {
	 if (reg->LorG == 1) fprintf(fp,"#define GAdr%s %d\n",reg->Name,i+ofs);
	 else                fprintf(fp,"#define LAdr%s %d\n",reg->Name,i+ofs);
	 cnt++;
	 continue;
      }
      for (j=reg->Start, k=reg->Offset; j<=reg->End; j++,k++) {
	 if (reg->LorG == 1) fprintf(fp,"#define GAdr%s%02d %d\n",reg->Name,k,j);
	 else                fprintf(fp,"#define LAdr%s%02d %d\n",reg->Name,k,j);
	 ofs = k -1;
	 cnt++;
      }
   }
   return cnt;
}

/* ===================================== */
/* Generate global register enumerations */
/* ===================================== */

int GenerateGlobalEnums(FILE *fp) {

int i, j, k, cnt;
RegisterDsc *reg;

   fprintf(fp,"\ntypedef enum {\n");
   cnt = 0;
   for (i=0; i<REGNAMES; i++) {
      reg = &(Regs[i]);
      if (reg->LorG == 1) {
	 if (reg->Start == reg->End) {
	    fprintf(fp,"   Mtt%s,\n",reg->Name);
	    cnt++;
	    continue;
	 }
	 for (j=reg->Start, k=reg->Offset; j<=reg->End; j++,k++) {
	    fprintf(fp,"   Mtt%s%d,\n",reg->Name,k);
	    cnt++;
	 }
      }
   }
   fprintf(fp," } MttLibGlobalRegister;\n");
   return cnt;
}

/* ==================================== */
/* Generate local register enumerations */
/* ==================================== */

int GenerateLocalEnums(FILE *fp) {

int i, j, k, cnt;
RegisterDsc *reg;

   fprintf(fp,"\ntypedef enum {\n");
   cnt = 0;
   for (i=0; i<REGNAMES; i++) {
      reg = &(Regs[i]);
      if (reg->LorG == 2) {
	 if (reg->Start == reg->End) {
	    fprintf(fp,"   Mtt%s,\n",reg->Name);
	    cnt++;
	    continue;
	 }
	 for (j=reg->Start, k=reg->Offset; j<=reg->End; j++,k++) {
	    fprintf(fp,"   Mtt%s%d,\n",reg->Name,k);
	    cnt++;
	 }
      }
   }
   fprintf(fp," } MttLibLocalRegister;\n");
   return cnt;
}

/* ======================================= */
/* Main program                            */
/* ======================================= */

static char MachineNames[TgmMACHINES][TgmNAME_SIZE];

int main(int argc,char *argv[]) {

int i, gn;
FILE *fp;
TgmMachine *mch;
TgmNetworkId nid;
char *fn, *tnm;

   printf("\ngtm: Version: %s %s Telegram task builder\n",__DATE__,__TIME__);

   tnm = NULL;
   for (i=1; i<argc; i++) {
      if (strncmp(argv[i],"Tgm",3) == 0) {
	 tnm = argv[i];
	 break;
      }
   }

   nid = TgmGetDefaultNetworkId();
   mch = TgmGetAllMachinesInNetwork(nid);
   if (mch != NULL) {
      printf("On this network:\n");
      for (i=0; i<TgmMACHINES; i++) {
	 if (mch[i] == TgmMACHINE_NONE) break;
	 strcpy(MachineNames[TgvTgmToTgvMachine(mch[i])],TgmGetMachineName(mch[i]));
	 if ((tnm != NULL) && (strcmp(TgmGetMachineName(mch[i]),&(tnm[3])) == 0)) {
	    tmch = TgmGetMachineId(&(tnm[3]));
	    printf("%s <===\n",tnm);
	 } else printf("Tgm%s\n",TgmGetMachineName(mch[i]));
      }
   }
   if (tmch == TgmMACHINE_NONE) {
      if (tnm != NULL)
	 printf("Please define a Tgm machine from the above list. (%s Not in list)\n",tnm);
      else
	 printf("Please define a Tgm machine from the above list. (No machine specified)\n");

      exit(0);
   }

   umask(0);
   fn = GetFile(TARGET_FILE);
   fp = fopen(fn,"w");
   if (fp == NULL) {
      fprintf(stderr,"Can't open: %s for write\n",fn);
      exit(1);
   }

   fprintf(fp,"#ifdef MTTLIB\n");
   fprintf(fp,"/* ===================================================== */\n");
   fprintf(fp,"/* This include file is automatically generated by gtm.c */\n");
   fprintf(fp,"/* All definitions are determined from opcodes.c source  */\n");
   fprintf(fp,"/* ===================================================== */\n");
   fprintf(fp,"#endif\n");
   fprintf(fp,"\n");

   fprintf(fp,"#ifndef MTTREGS\n");
   fprintf(fp,"#define MTTREGS\n");
   fprintf(fp,"\n");

   gn = BuildTgmInc(fp);
   printf("Processed: %d telegram groups to: %s\n",gn,fn);

   i = GenerateRegDefs(fp);
   printf("Processed: %d register definitons to: %s\n",i,fn);

   fprintf(fp,"\n");
   fprintf(fp,"#define ConsNOT_SET 0x80000000\n");
   fprintf(fp,"#define ConsALL_SET 0xFFFFFFFF\n");
   fprintf(fp,"#define GRegTELEGRAM gr1\n");
   fprintf(fp,"#define GAdrTELEGRAM GAdrGR01\n");
   fprintf(fp,"#define GRegWAIT_MASK gr33\n");
   fprintf(fp,"#define GRegBASIC_PERIOD gr34\n");
   fprintf(fp,"#define GRegHOST_ID gr35\n");

   fprintf(fp,"#define LRegParRUN_COUNT lr1\n");    /* Run count          */
   fprintf(fp,"#define LRegParRUN_MASK lr2\n");     /* Run mask           */
   fprintf(fp,"#define LRegParWAIT_MASK lr3\n");    /* Wait mask          */
   fprintf(fp,"#define LRegParSTART_SEC lr4\n");    /* Start second       */
   fprintf(fp,"#define LRegParSTART_MSC lr5\n");    /* Start milli second */
   fprintf(fp,"#define LRegParWAIT_VMEP2 lr6\n");   /* VME P2 mask        */
   fprintf(fp,"#define LRegParSTART_EVENT lr7\n");  /* Start event        */

   fprintf(fp,"#define TskStsNONE 0\n");            /* Task status NONE               */
   fprintf(fp,"#define TskStsCRASHED 1\n");         /* Task status CRASHED            */
   fprintf(fp,"#define TskStsWAITING_PARS 2\n");    /* Task status WAITING PAMAMETERS */
   fprintf(fp,"#define TskStsWAITING_TRIG 3\n");    /* Task status WAITING TRIGGER    */
   fprintf(fp,"#define TskStsRUNNING 4\n");         /* Task status RUNNING            */

   fprintf(fp,"#define LRegTASK_STATUS lr8\n");     /* Task status register */

   fprintf(fp,"#define LRegTEMP lr31\n");

   fprintf(fp,"\n");
   fprintf(fp,"#ifdef MTTLIB\n");

   i = GenerateGlobalEnums(fp);
   fprintf(fp,"\n#define MttLibGlobalREGISTERS %d\n",i);
   printf("Processed: %d Global enumerations to: %s\n",i,fn);

   i = GenerateLocalEnums(fp);
   fprintf(fp,"\n#define MttLibLocalREGISTERS %d\n",i);
   printf("Processed: %d Local enumerations to: %s\n",i,fn);

   fprintf(fp,"\n");
   fprintf(fp,"#endif\n");
   fprintf(fp,"#endif\n");

   fclose(fp);
   exit(0);
}
