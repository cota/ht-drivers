/***************************************************************************/
/* Ctr clock V2 version                                                    */
/* Julian Lewis.                                                           */
/***************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sched.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef Status
#undef Status
#endif

#ifdef CTR_PCI
#include <drm.h>
#endif

#include <ctrdrvr.h>

#include <errno.h>
extern int errno;

#define BOLD_FONT "-adobe-courier-bold-r-normal-*-*-140-*-*-*-*-iso8859-1"

#define PI 3.1415926535897932384626433
#define UBCD(bcd) ((bcd&0xF)+10*(bcd>>4))

static int ctr = 0;
#include "CtrOpen.c"

/**************************************************************************/
/* Convert a CtrDrvr time in milliseconds to a string routine.            */
/* Result is a pointer to a static string representing the time.          */
/*    the format is: Thu-18/Jan/2001 08:25:14.967                         */
/*                   day-dd/mon/yyyy hh:mm:ss.ddd                         */

static char *TimeToStr (CtrDrvrTime *t, int hpflag) {

static char buf[32];

char tmp[32];
char *yr, *ti, *md, *mn, *dy;
int ms;
double fms;

    if (t->Second) {

	ctime_r ((time_t *) &t->Second, tmp);                  /* Day Mon DD HH:MM:SS YYYY\n\0 */

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

	if ((t->TicksHPTDC) && (hpflag)) {
	   fms  = ((double) t->TicksHPTDC) / 2560.00;   /* I multiply by 10^6 for 6 sig figs */
	   ms = fms;
	   sprintf (buf, "%s-%s/%s/%s %s.%06d", dy, md, mn, yr, ti, ms); /* 6 figs */
	} else
	   sprintf (buf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);

    }
    else {
        sprintf (buf, "--- Zero ---");
    }
    return (buf);
}

/*****************************************************************/

int ConnectPPS(int module) {
CtrDrvrConnection cnx;

   cnx.Module = module;
   cnx.EqpClass = CtrDrvrConnectionClassHARD;
   cnx.EqpNum = CtrDrvrInterruptMaskPPS;
   if (ioctl(ctr,CtrDrvrCONNECT,&cnx) < 0) {
      return 0;
   }
   return 1;
}

/*****************************************************************/

int WaitPPS(CtrDrvrTime *t) {
int cc;
CtrDrvrReadBuf rbf;
CtrDrvrCTime *tim;

   tim = &(rbf.OnZeroTime);
   cc = read(ctr,&rbf,sizeof(CtrDrvrReadBuf));
   if (cc <= 0) {
      fprintf(stderr,"CtrLib: Read: TimeOut\n");
      return 0;
   }
   *t = tim->Time;
   return 1;
}

/***********************************************/
/* Signal handler to do gracefull termination. */
/***********************************************/

void terminate(ar)
int ar; {

   perror("XLib");
   printf("End (%1d) ctrclock.\n",ar);
   exit(ar);
}

/***************************************************************************/
/* Main                                                                    */
/***************************************************************************/

int main (argc, argv)
int     argc;
char  **argv; {

/* Display */

Display      * ds;         /* Display pointer */
int       ds_width;        /* Display width */
int       ds_height;       /* Display height */

/* Main window */

Window         mw;              /* Main window */
char           mw_title[32];    /* Main window title */
int            mw_x;            /* Main window X position */
int            mw_y;            /* Main window Y position */
int            mw_width;        /* Main window width */
int            mw_height;       /* Main window heigth */
int            mw_bo_width;     /* Main window border width */
unsigned int   mw_bo_color;     /* Main window border color */
unsigned int   mw_bk_color;     /* Main window background color */
int            mw_depth;        /* Main window depth */
Pixmap         mw_bs;           /* Main window backing store */

/* Fonts and graphics contexts */

GC             gc_hour, gc_minute, gc_second, gc_tenths, gc_fill, gc_date;
XFontStruct  * font;
unsigned int   value_mask;
XGCValues      value_struct;
XColor         red_color;
XColor         blue_color;
XColor         green_color;
XColor         orange_color;
XColor         grey_color;
XColor         exact_color;

/* General purpose crap */

struct sigaction action, old;
int i, j, module;
char ymd[81], *cp, *ep;
int xo, yo, xh, yh, xm, ym, xs, ys;
double d30, hl, ml, sl, tl;
int Hour, Minute, Second, secs;
struct tm *tod;
CtrDrvrTime t;

   module = 0;
   if (argc >= 2) {
      cp = argv[1];
      module = strtoul(cp,&ep,0);
      if (cp == ep) module = 0;
   }

   /* Catch all termination signals. */

   action.sa_handler = terminate;
   action.sa_mask = (sigset_t) {{0,0}};
   action.sa_flags = 0;
   for (i=1; i<NSIG; i++) sigaction(i,&action,&old);

   /* Open the remote display */

   ds = XOpenDisplay(NULL);
   if (ds == NULL) {
      fprintf(stderr,"ctrclock: Can not open display.\n");
      exit(1);
   };

   /* Get display size characteristics */

   ds_width  = DisplayWidth(ds,0);
   ds_height = DisplayHeight(ds,0);

   /* Set up main window characteristics */

   mw_width    = (ds_width*25)/100;     /* Percentage screen width */
   mw_height   = (ds_height*25)/100;    /* Percentage screen height */
   mw_x        = ds_width/2;
   mw_y        = ds_height/2;
   mw_bo_width = 4;
   mw_bo_color = BlackPixel(ds,0);
   mw_bk_color = WhitePixel(ds,0);
   mw_depth    = DefaultDepth(ds,0);

   mw = XCreateSimpleWindow(
		 ds,
		 RootWindow(ds,0),
		 mw_x,
		 mw_y,
		 mw_width,
		 mw_height,
		 mw_bo_width,
		 mw_bo_color,
		 mw_bk_color);

   /* Allocate colors */

   if (mw_depth > 1) {
      XAllocNamedColor(ds,DefaultColormap(ds,0),
		       "red",&red_color,&exact_color);
      XAllocNamedColor(ds,DefaultColormap(ds,0),
		       "blue",&blue_color,&exact_color);
      XAllocNamedColor(ds,DefaultColormap(ds,0),
		       "green",&green_color,&exact_color);
      XAllocNamedColor(ds,DefaultColormap(ds,0),
		       "orange",&orange_color,&exact_color);
      XAllocNamedColor(ds,DefaultColormap(ds,0),
		       "black",&grey_color,&exact_color);
   } else {
      red_color.pixel    = BlackPixel(ds,0);
      blue_color.pixel   = BlackPixel(ds,0);
      green_color.pixel  = BlackPixel(ds,0);
      orange_color.pixel = BlackPixel(ds,0);
      grey_color.pixel   = WhitePixel(ds,0);
   };

   /* Get info about and load chosen fonts */

   font = XLoadQueryFont(ds,BOLD_FONT);
   if (font == NULL) {
      printf("No font: %s: BAD system installation\n",BOLD_FONT);
      terminate(1);
   };

   /* Set up a graphics contexts */

   value_mask = /* GCFont | */
		GCCapStyle |
		GCForeground |
		GCBackground |
		GCLineStyle |
		GCLineWidth;

   /* Hour hand */

   value_struct.font       = font->fid;
   value_struct.foreground = orange_color.pixel;
   value_struct.background = orange_color.pixel;
   value_struct.line_width = 8;
   value_struct.line_style = LineSolid;
   value_struct.cap_style  = CapRound;
   gc_hour = XCreateGC(ds,mw,value_mask,&value_struct);

   /* Minute hand */

   value_struct.font       = font->fid;
   value_struct.foreground = green_color.pixel;
   value_struct.background = green_color.pixel;
   value_struct.line_width = 4;
   value_struct.line_style = LineSolid;
   gc_minute = XCreateGC(ds,mw,value_mask,&value_struct);

   /* Second hand */

   value_struct.font       = font->fid;
   value_struct.foreground = blue_color.pixel;
   value_struct.background = blue_color.pixel;
   value_struct.line_width = 2;
   value_struct.line_style = LineSolid;
   gc_second = XCreateGC(ds,mw,value_mask,&value_struct);

   /* Tenths of a second */

   value_struct.font       = font->fid;
   value_struct.foreground = red_color.pixel;
   value_struct.background = red_color.pixel;
   value_struct.line_width = 1;
   value_struct.line_style = LineSolid;
   gc_tenths = XCreateGC(ds,mw,value_mask,&value_struct);

   /* Date */

   value_struct.font       = font->fid;
   value_struct.foreground = BlackPixel(ds,0);
   value_struct.background = WhitePixel(ds,0);
   value_struct.line_width = 2;
   value_struct.line_style = LineSolid;
   gc_date = XCreateGC(ds,mw,value_mask,&value_struct);

   /* Filling */

   value_struct.font       = font->fid;
   value_struct.foreground = grey_color.pixel;
   value_struct.background = grey_color.pixel;
   value_struct.line_width = 4;
   value_struct.line_style = LineSolid;
   gc_fill = XCreateGC(ds,mw,value_mask,&value_struct);

   /* Map window */

   XMapWindow(ds,mw);

   /* Create backing store */

   mw_bs = XCreatePixmap(ds,mw,mw_width,mw_height,mw_depth);
   XFillRectangle(ds,mw_bs,gc_fill,0,0,mw_width,mw_height);

   /* Define loop constants */

   xo = mw_width / 2;
   yo = mw_height / 2;
   d30 = PI / 6.00;
   if (mw_width < mw_height) tl = (mw_width *0.50);
   else                      tl = (mw_height*0.50);
   sl = (tl*3.00)/4.00;
   ml = (sl*3.00)/4.00;
   hl = (ml*3.00)/4.00;

   /* Open driver and connect to 1PPS on given module */

   for (j=1; j<argc; j++) {
      for (i=0; i<CtrDrvrDEVICES; i++) {
	 if (strcmp(argv[j],devnames[i]) == 0) {
	    ctr_device = (CtrDrvrDevice) i;
	    printf("Forcing CTR device type:%s\n",devnames[i]);
	    break;
	 }
      }
   }

   ctr = CtrOpen();
   if (module) ioctl(ctr,CtrDrvrSET_MODULE,&module);
   ioctl(ctr,CtrDrvrGET_MODULE,&module);
   ConnectPPS(module);

   /* Give window title */

   sprintf(mw_title,"CtrClock Module: %d",module);
   XStoreName(ds,mw,mw_title);

   /******************************/
   /* Main event processing loop */
   /******************************/

   while(True) {

      WaitPPS(&t);
      secs = t.Second;

      XFillRectangle(ds,mw_bs,gc_fill,0,0,mw_width,mw_height);

      tod = gmtime(&secs);

      Hour   = tod->tm_hour;
      Minute = tod->tm_min;
      Second = tod->tm_sec;

      if (Hour > 12) Hour = Hour - 12;

      xh = (int) (hl * sin(((double) (Hour) + ((double) (Minute) / 60.00)) * d30));
      yh = (int) (hl * cos(((double) (Hour) + ((double) (Minute) / 60.00)) * d30));
      xm = (int) (ml * sin( (double) (Minute) / 5.00 * d30));
      ym = (int) (ml * cos( (double) (Minute) / 5.00 * d30));
      xs = (int) (sl * sin( (double) (Second) / 5.00 * d30));
      ys = (int) (sl * cos( (double) (Second) / 5.00 * d30));

      XDrawLine(ds,mw_bs,gc_hour,  xo,yo,(xo+xh),(yo-yh));
      XDrawLine(ds,mw_bs,gc_minute,xo,yo,(xo+xm),(yo-ym));
      XDrawLine(ds,mw_bs,gc_second,xo,yo,(xo+xs),(yo-ys));

      sprintf(ymd,"%s",TimeToStr(&t,0));
      XDrawImageString(ds,mw_bs,gc_date,0,mw_height-20,ymd,strlen(ymd));

      /* Flush and kill output queue */

      XCopyArea(ds,mw_bs,mw,gc_fill,0,0,mw_width,mw_height,0,0);
      XFlush(ds);
      XSync(ds,True);                /* Garbage collect queue */
   };
}

/* eof */
