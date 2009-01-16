/***************************************************************************/
/* Xmem installation program.                                              */
/* Oct 2004                                                                */
/* Julian Lewis                                                            */
/***************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <sys/file.h>
#include <a.out.h>
#include <errno.h>

extern int dr_install(char *path, int type);
extern int cdv_install(char *path, int driver_ID, char *extra);

#include "xmemDrvr.h"
#include "drm.h"
#include <cdcm/cdcmBoth.h>
#include <cdcm/cdcmIo.h>


static XmemDrvrInfoTable info;

/***************************************************************************/
/* Main                                                                    */
/***************************************************************************/

int main(int argc, char **argv)
{

char                   * object     = "xmemPciDrvr.ppc4.o";
char                   * progname   = "xmemLynxInst";
char                   * info_file  = "/tmp/xmemdrvr.info";
char                   * extra      = "xmemPciDrvr";
char                     node_name[128];
int                      driver_id, major, minor;
int                      uid, fd, i;

   for (i=1; i<argc; i++) {
      if (strcmp(argv[i],"-object"  ) == 0) object = argv[i+1];
   }
   printf("xmeminstall: Installing driver from: %s\n",object);

   uid = geteuid();
   if (uid != 0) {
      printf("%s: User ID is not ROOT.\n",progname);
      exit(1);
   }

   /***********************************/
   /* Build the info table in memory. */
   /***********************************/

   bzero((void *) &info,sizeof(XmemDrvrInfoTable)); /* Clean table */

   /* Open the info file and write the table to it. */

   if ((fd = open(info_file,(O_WRONLY | O_CREAT),0644)) < 0) {
      printf("%s: Can't create Xmem driver information file: %s\n",progname,info_file);
      perror(progname);
      exit(1);
   }

   if (write(fd,&info,sizeof(info)) != sizeof(XmemDrvrInfoTable)) {
      printf("%s: Can't write to Xmem driver information file: %s\n",progname,info_file);
      perror(progname);
      exit(1);
   }
   close(fd);

   /* Install the driver object code in the system and call its install */
   /* procedure to initialize memory and hardware. */

   if ((driver_id = dr_install(object,CHARDRIVER)) < 0) {
      printf("%s: Error calling dr_install with: %s\n",progname,object);
      perror(progname);
      exit(1);
   }
   if ((major = cdv_install(info_file, driver_id, extra)) < 0) {
      printf("%s: Error calling cdv_install with: %s\n",progname,info_file);
      perror(progname);
      exit(1);
   }

   /* Create the device nodes. */

   umask(0);
   for (minor=1; minor<=XmemDrvrCLIENT_CONTEXTS; minor++) {
      sprintf(node_name,"/dev/xmem.%1d",minor);
      unlink (node_name);
      if (mknod(node_name,(S_IFCHR | 0666),((major << 8) | minor)) < 0) {
	 printf("%s: Error making node: %s\n",progname,node_name);
	 perror(progname);
	 exit(1);
      }
   }

   exit(0);
}
