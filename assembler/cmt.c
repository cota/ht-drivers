/**************************************************************************/
/* Compile an event table into an mtt object file                         */
/* Julian Lewis AB/CO/HT                                                  */
/* Version Tuesday 19th October 2006                                      */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include <asm.h>
#include <asmP.h>
#include <opcodes.c>

#include <tgv/tgv.h>
#include <tgm/tgm.h>

#define OUT "task_events.ass"
#define ECP "evt.cpp"
#define LN 128

/* ======================================= */
/* String to upper case                    */
/* ======================================= */

void StrToUpper(inp,out)
char *inp;
char *out; {

long        i;
AtomType   at;
char        c;

   for (i=0; i<strlen(inp); i++) {
      c = inp[i];
      at = atomhash[(int) (c)];
      if ((at == Alpha) && (c >= 'a')) c -= ' ';
      out[i] = c;
   }
   out[i] = 0;
}

/* ======================================= */
/* Tab                                     */
/* ======================================= */

char *TabTo(char *cp, int pos) {
unsigned int i, j;
size_t l;
static char tab[128];

   if (cp) {
      l = strlen(cp);
      for (i=l,j=0; i<pos; i++,j++) tab[j]=' ';
      tab[j]=0;
   }
   return tab;
}

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

/* ======================================= */
/* Get file name extension                 */
/* ======================================= */

typedef enum {CPP,ASM,OBJ,TBL,INVALID} FileType;

static char *extNames[] = {".cpp",".asm",".obj",".tbl"};

FileType GetFileType(char *path) {

int i, j;
char c, *cp, ext[5];

   for (i=strlen(path); i>=0; i--) {
      cp = &(path[i]); c = *cp;
      if (c == '.') {
	 strncpy(ext,cp,4); ext[4] = 0;
	 for (j=0; j<INVALID; j++) {
	    if (strcmp(ext,extNames[j]) == 0) break;
	 }
	 return (FileType) j;
      }
   }
   return INVALID;
}

/* ======================================= */
/* Get directory name                      */
/* ======================================= */

char *GetDirName(char *path) {

int i;
static char res[LN];

   if ((path != NULL)
   &&  (strlen(path) > 0)) {
      strcpy(res,path);
      for (i=strlen(res)-1; i>=0; i--) {
	 if (res[i] == '/') {
	    res[i+1] = 0;
	    return res;
	 }
      }
   }
   strcpy(res,"./");
   return res;
}

/* ======================================= */
/* Remove filename extension               */
/* ======================================= */

char *RemoveExtension(char *path) {

int i;
static char res[LN];

   if ((path != NULL)
   &&  (strlen(path) > 0)) {
      strcpy(res,path);
      for (i=strlen(res)-1; i>=0; i--) {
	 if (res[i] == '.') {
	    res[i] = 0;
	    return res;
	 }
      }
   }
   return res;
}

/* ======================================= */
/* Get root filename                       */
/* ======================================= */

char *GetRootName(char *path) {

int i;
char *cp;

   cp = NULL;
   if ((path != NULL)
   &&  (strlen(path) > 0)) {
      for (i=strlen(path)-1; i>=0; i--) {
	 if (path[i] == '/') {
	    cp = &(path[i+1]);
	    break;
	 }
      }
   }
   if (cp == NULL) cp = path;
   return RemoveExtension(cp);
}

/* ======================================= */
/* Compile table                           */
/* ======================================= */

FILE *tblFile = NULL;
FILE *assFile = NULL;

void CompileTable() {

char ln[LN], enm[LN];
unsigned long ul,  rmsk, wmsk;
unsigned int sec, msc,  pay, rltm;

   rmsk = 0;
   wmsk = 0;
   rltm = 0;

   if (tblFile == NULL) {
      fprintf(stderr,"cmt: No table file supplied, Fatal Error\n");
      exit(1);
   }
   while (fgets(ln,LN,tblFile)) {

      if (isdigit((int) ln[0])) {
	 sscanf(ln,"%u %u %s %u",&sec,&msc,enm,&pay);
	 ul = TgvGetFrameForName((TgvName *) enm);

	 rltm = sec*1000 + msc;

	 if (rltm) fprintf(assFile,"   wrlv %d MSFR   # Wait relative\n",(int) rltm);
	 if (ul) {
	    ul &= 0xFFFF0000;
	    ul |= (pay & 0xFFFF);
	    fprintf(assFile,"   movv 0x%08X EVOUT   # Send event\n",(int) ul);
	 } else {
	    fprintf(stderr,"cmt: ERROR: Don't understand: %s (Bad Tgm network)\n",enm);
	    exit(1);
	 }
      }
   }
}

/* ======================================= */
/* Main program                            */
/* ======================================= */

static char MachineNames[TgmMACHINES][TgmNAME_SIZE];

int main(int argc,char *argv[]) {

int i;

char *cp;

char tbl[LN];
char ass[LN];
char xld[LN];
char cpp[LN];
char cmd[LN];
char out[LN];

FileType ft = INVALID;
struct stat tblStat;

TgmMachine eMch=TgmMACHINE_NONE;
TgmMachine *mch=&eMch;
TgmNetworkId nid;

   umask(0);

   bzero(tbl,LN);

   printf("\ncmt: Version: %s %s Event Table Compiler\n",__DATE__,__TIME__);

   nid = TgmGetDefaultNetworkId();
   
   /* Quick fix */
   if ( nid==TgmNETWORK_CTF ) eMch=TgmSCT;
   if ( nid==TgmNETWORK_LHC ) eMch=TgmLHC;
   
   if ( *mch == TgmMACHINE_NONE ) {
   	printf("Invalid network\n");
   	exit(1);
   }
#if 0
   mch = TgmGetAllMachinesInNetwork(nid);
   if (mch != NULL) {
      printf("On this network:");
      for (i=0; i<TgmMACHINES; i++) {
	 if (mch[i] == TgmMACHINE_NONE) break;
	 strcpy(MachineNames[TgvTgmToTgvMachine(mch[i])],TgmGetMachineName(mch[i]));
	 printf("%s ",TgmGetMachineName(mch[i]));
      }
      printf("\n");
   }
#endif
   for (i=1; i<argc; i++) {
      ft = GetFileType(argv[i]);
      switch (ft) {

	 case TBL:
	    strcpy(tbl,argv[i]);
	    if (stat(tbl,&tblStat) == -1) {
	       fprintf(stderr,"cmt: Can't get file statistics: %s for read\n",tbl);
	       perror("cmt");
	       exit(1);
	    }
	    if (tblStat.st_size < 2) {
	       fprintf(stderr,"cmt: File: %s is empty\n",tbl);
	       exit(1);
	    }
	    tblFile = fopen(tbl,"r");
	    if (tblFile == NULL) {
	       fprintf(stderr,"cmt: Can't open: %s for read\n",tbl);
	       perror("cmt");
	       exit(1);
	    }
	 break;

	 default:
	    fprintf(stderr,"cmt: Invalid file extension: %s\n",argv[i]);
	    exit(1);
      }
   }

   strcpy(out,GetFile(OUT));
   assFile = fopen(out,"w");
   if (assFile == NULL) {
      fprintf(stderr,"cmt: Can't open: %s for write\n",out);
      perror("cmt");
      exit(1);
   }

   CompileTable();
   fclose(tblFile);
   fclose(assFile);

   sprintf(cmd,"%s Tgm%s",GetFile("gtm"),TgmGetMachineName(*mch));
   printf("%s\n",cmd);
   system(cmd);

   strcpy(cpp,GetFile("cpp"));
   sprintf(cmd,"%s -I%s %s -o %s%s.ass",
		cpp,
		GetDirName(out),
		GetFile(ECP),
		GetDirName(out),
		GetRootName(tbl));
   printf("%s\n",cmd);
   system(cmd);

   strcpy(ass,GetFile("asm"));
   sprintf(cmd,"%s %s%s.ass",
	       ass,
	       GetDirName(out),
	       GetRootName(tbl));
   printf("%s\n",cmd);
   system(cmd);

   cp = GetFile("xldr");
   if (cp) {
      strcpy(xld,cp);
      sprintf(cmd,"%s %s.obj",
		  xld,
		  GetRootName(tbl));
      printf("%s\n",cmd);
      system(cmd);
   }
   exit(0);
}
