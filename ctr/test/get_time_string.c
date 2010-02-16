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
#include <time.h>         /* ctime */

/******************************/
/* Convert UTC time to string */

volatile char *TimeToStr(unsigned long utc) {

static char tbuf[128];

char tmp[128];
char *yr, *ti, *md, *mn, *dy;
double ms;
double fms;

    bzero((void *) tbuf, 128);
    bzero((void *) tmp, 128);

    if (utc) {

	ctime_r (&utc,tmp ); /* Day Mon DD HH:MM:SS YYYY\n\0 */

        tmp[3] = 0;
        dy = &(tmp[0]);
        tmp[7] = 0;
        mn = &(tmp[4]);
        tmp[10] = 0;
        md = &(tmp[8]);
        if (md[0] == ' ')
            md[0] = '0';
        tmp[19] = 0;
        ti = &(tmp[11]);
        tmp[24] = 0;
        yr = &(tmp[20]);

	sprintf (tbuf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);

    } else sprintf (tbuf, "--- Zero ---");
    return (tbuf);
}

/* Read time and print it out */

int main(int argc,char *argv[]) {
char *ep;

   if (argc > 1)
      printf("%s\n",TimeToStr(strtoul(argv[1],&ep,0)));
   else
      printf("%s <UTC>\n");
   exit(0);
}
