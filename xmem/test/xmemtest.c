/**************************************************************************/
/* Ctr test                                                              */
/* Julian Lewis 15th Nov 2002                                             */
/**************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>        /* Error numbers */
#include <sys/file.h>
#include <a.out.h>
#include <ctype.h>
#include <tgm/tgm.h>
#include <malloc.h>
#include <limits.h>

#include <xmemDrvr.h>

/**************************************************************************/
/* Code section from here on                                              */
/**************************************************************************/

/* Print news on startup if not zero */

#define NEWS 1

#define HISTORIES 24
#define CMD_BUF_SIZE 128

static char history[HISTORIES][CMD_BUF_SIZE];
static char *cmdbuf = &(history[0][0]);
static int  cmdindx = 0;
static char prompt[32];
static char *pname = NULL;
static int  xmem;

char		*configfiles;
extern char	*optarg;

static const char usage_string[] =
	"Xmem test program\n"
	"  xmemtest [-h] [-c<path to conf directory>]";
static const char commands_string[] =
	"options:\n"
	"  -c = path to configuration directory\n"
	"         (node and segment descriptions are taken from there)\n"
	"  -h = show this help text\n";

#include "Cmds.h"
#include "GetAtoms.h"
#include "PrintAtoms.h"
#include "DoCmd.h"
#include "Cmds.inc.h"
#include "XmemOpen.h"
#include "XmemCmds.h"

char *build_fullpath(const char *confpath, const char *filename)
{
	static char fullpath[PATH_MAX + 32];
	int i, j;

	strncpy(fullpath, confpath, PATH_MAX);

	i = strlen(fullpath);
	j = 0;
	while (filename[j] != '\0' && i < PATH_MAX + 32)
		fullpath[i++] = filename[j++];
	fullpath[i] = '\0';

	return fullpath;
}

static int check_file_exists(const char *confpath, const char *filename)
{
	FILE *file;
	char *fullpath;

	fullpath = build_fullpath(confpath, filename);
	file = fopen(fullpath, "r");
	if (!file)
		return -1;
	fclose(file);

	return 0;
}

static int set_conf_path(const char *path)
{
	static char confpath[PATH_MAX];
	int i;

	if (path == NULL)
		return -1;
	if (strlen(path) >= PATH_MAX)
		return -1;

	strncpy(confpath, path, PATH_MAX);

	i = strlen(confpath);
	if (confpath[i] != '/')
		confpath[i] = '/';

	if (check_file_exists(confpath, SEG_TABLE_NAME))
		return -1;
	if (check_file_exists(confpath, NODE_TABLE_NAME))
		return -1;

	configfiles = confpath;
	return 0;
}

static void usage_complete(void)
{
	printf("%s\n", usage_string);
	printf("%s\n", commands_string);
}

/**************************************************************************/
/* Prompt and do commands in a loop                                       */
/**************************************************************************/

int main(int argc,char *argv[]) {

char *cp;
char host[49];
char tmpb[CMD_BUF_SIZE];
int  c;

#if NEWS
   printf("xmemtest: See <news> command\n");
   printf("xmemtest: Type h for help\n");
#endif

   pname = argv[0];
   printf("%s: Compiled %s %s\n",pname,__DATE__,__TIME__);

   for (;;) {
	   c = getopt(argc, argv, "c:h");
	   if (c < 0)
		   break;
	   switch (c) {
	   case 'c':
		   if (set_conf_path(optarg))
			   printf("\nWarning: path invalid or too long\n");
		   break;
	   case 'h':
		   usage_complete();
		   exit(0);
	   }
   }

   xmem = XmemOpen();
   if (xmem == 0) {
      printf("\nWARNING: Could not open driver");
      printf("\n\n");
   } else {
      printf("Driver opened OK: Using XMEM module: 1\n\n");
   }
   ReadSegTable(0);
   ReadNodeTableFile(0);
   printf("\n");

   bzero((void *) host,49);
   gethostname(host,48);

   while (True) {

      if (xmem) sprintf(prompt,"%s:Xmem[%02d]",host,cmdindx+1);
      else     sprintf(prompt,"%s:NoDriver:Xmem[%02d]",host,cmdindx+1);

      cmdbuf = &(history[cmdindx][0]);
      if (strlen(cmdbuf)) printf("{%s} ",cmdbuf);
      printf("%s",prompt);

      bzero((void *) tmpb,CMD_BUF_SIZE);
      if (gets(tmpb)==NULL) exit(1);

      cp = &(tmpb[0]);
      if (*cp == '!') {
	 cmdindx = strtoul(++cp,&cp,0) -1;
	 if (cmdindx >= HISTORIES) cmdindx = 0;
	 cmdbuf = &(history[cmdindx][0]);
	 continue;
      } else if (*cp == '.') {
	 printf("Execute:%s\n",cmdbuf); fflush(stdout);
      } else if ((*cp == '\n') || (*cp == '\0')) {
	 cmdindx++;
	 if (cmdindx >= HISTORIES) { printf("\n"); cmdindx = 0; }
	 cmdbuf = &(history[cmdindx][0]);
	 continue;
      } else if (*cp == '?') {
	 printf("History:\n");
	 printf("\t!<1..24> Goto command\n");
	 printf("\tCR       Goto next command\n");
	 printf("\t.        Execute current command\n");
	 printf("\this      Show command history\n");
	 continue;
      } else {
	 cmdindx++; if (cmdindx >= HISTORIES) { printf("\n"); cmdindx = 0; }
	 strcpy(cmdbuf,tmpb);
      }
      bzero((void *) val_bufs,sizeof(val_bufs));
      GetAtoms(cmdbuf);
      DoCmd(0);
   }
}
