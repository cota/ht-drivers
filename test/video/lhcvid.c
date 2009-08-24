
OLD version not valid, use genvid instaed


/********************************************************************/
/* LHC Video displays the LHC telegram in a non generic form        */
/* Julian Lewis 2/Feb/2007                                          */
/********************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <math.h>

#ifdef NOSTDHDRS                /* SUN */
#undef NOSTDHDRS
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <err/err.h>
#include <tgm/tgm.h>

#ifdef Status
#undef Status
#endif

#include <mtthard.h>
#include <mttdrvr.h>
#include <libmtt.h>

#define DISPLAY_TIME 100
#define SCREEN_NUMBER DefaultScreen(ds)
#define START_VISTAR_ROW 16
#define LAST_ROW 20
#define LAST_COL 9

extern int errno;

/*******************************************/
/* Error handler                           */
/* Usual crap for error logging            */
/*******************************************/

static Boolean warnings = True;

static int error_handler (ErrClass class, char *text) {

   if ((warnings) && (class == ErrWARNING)) return (False);
   fprintf (stderr, "%s\n", text);
   perror("lhcvid");
   if (class == ErrFATAL) exit(1);
   return True;
}

/*******************************************************************/
/* Main:                                                           */
/*******************************************************************/

int main (int argc, char *argv[])
{

/* General purpose crap */

double ts;                  /* Last time stamp as double */
unsigned long uvt, mst;     /* Universal time and millisecond */
char dstr[80];              /* Display string */
TgmNetworkId nid;           /* Network of machine */
TgmLineNameTable ltsbf;
TgmLineNameTable ltmode;
TgmLineNameTable ltbtc;
TgmLineNameTable ltrngi;

unsigned int geng, gthrs, gitn1, gitn2, gsbf1, gsbf2, gfiln, gbpnm, gmode, gbtc1, gbtc2, gbtni, gbkni, grngi;
unsigned int  eng,  thrs,  itn1,  itn2,  sbf1,  sbf2,  filn,  bpnm,  mode,  btc1,  btc2,  btni,  bkni,  rngi;
unsigned int secs, mins, hors;
double feng, fdam1, fdam2;

unsigned int i, msk;
char *cp;

/* Display */

char *displayName = NULL;   /* Display name */
Display *ds;                /* Display pointer */
Cardinal ds_width;          /* Display width */
Cardinal ds_height;         /* Display height */

/* Main window */

Window mw;                  /* Main window */
char mw_title[81];          /* Main window title */
int mw_x;                   /* Main window X position */
int mw_y;                   /* Main window Y position */
Cardinal mw_width;          /* Main window width */
Cardinal mw_height;         /* Main window heigth */
Cardinal mw_bo_width;       /* Main window border width */
unsigned long mw_bo_color;  /* Main window border color */
unsigned long mw_bk_color;  /* Main window background color */
int mw_depth;               /* Main window depth */
Pixmap mw_bs;               /* Main window backing store */

/* Fonts and graphics contexts */

GC gc_big, gc_wee, gc_fill, gc_stat;
XFontStruct *font_big, *font_wee;
unsigned long value_mask;
XGCValues value_struct;
Cardinal col_inc;           /* Font dependent width of display item */
Cardinal row_inc;           /* Font dependent height of display item */
XColor blue_color;
XColor green_color;
XColor grey_color;
XColor exact_color;

   printf("LHC parameters display. Ver: %s \n",__DATE__);
   ErrSetHandler(error_handler);

   ds = XOpenDisplay(displayName);
   if (ds == NULL) {
      printf ("%s: Can not open display.\n", argv[0]);
      exit(1);
   }
   ds_width = DisplayWidth(ds, SCREEN_NUMBER);
   ds_height = DisplayHeight(ds, SCREEN_NUMBER);

   /* Set up main window characteristics */

   mw_width = (ds_width * 65) / 100;   /* Percentage screen width */
   mw_height = (ds_height * 42) / 100; /* Percentage screen height */
   mw_x = mw_width/3;
   mw_y = mw_height/3;

   mw_bo_width = 4;
   mw_bo_color = BlackPixel(ds, SCREEN_NUMBER);
   mw_bk_color = WhitePixel(ds, SCREEN_NUMBER);
   mw_depth = DefaultDepth(ds, SCREEN_NUMBER);

   mw = XCreateSimpleWindow (ds,
			     RootWindow(ds,SCREEN_NUMBER),
			     mw_x,
			     mw_y,
			     mw_width,
			     mw_height,
			     mw_bo_width,
			     mw_bo_color,
			     mw_bk_color);

   nid = TgmMachineToNetworkId(TgmLHC);
   gethostname (dstr, sizeof(dstr));
   sprintf(mw_title,
	   "LHC Parameters Display: %s %s",
	   dstr,
	   (char *) TgmNetworkIdToName(nid));
   XStoreName(ds,mw,mw_title);

   XAllocNamedColor(ds,DefaultColormap(ds, SCREEN_NUMBER),"blue",&blue_color,&exact_color);
   XAllocNamedColor(ds,DefaultColormap(ds, SCREEN_NUMBER),"green",&green_color,&exact_color);
   XAllocNamedColor(ds,DefaultColormap(ds, SCREEN_NUMBER),"grey90",&grey_color,&exact_color);
   XAllocNamedColor(ds,DefaultColormap(ds, SCREEN_NUMBER),"grey",&grey_color,&exact_color);

   font_big = XLoadQueryFont(ds, "-adobe-courier-bold-r-normal-*-*-140-*-*-*-*-iso8859-1");
   font_wee = XLoadQueryFont(ds, "-adobe-courier-bold-r-normal-*-*-120-*-*-*-*-iso8859-1");

   col_inc = XTextWidth (font_big, "WWWWWWWWW", 9);
   row_inc = font_big->ascent + font_big->descent;

   value_mask = GCFont | GCForeground | GCBackground | GCLineStyle | GCLineWidth;

   value_struct.font = font_big->fid;
   value_struct.foreground = blue_color.pixel;
   value_struct.background = grey_color.pixel;
   value_struct.line_width = 4;
   value_struct.line_style = LineSolid;
   gc_big = XCreateGC(ds,mw,value_mask,&value_struct);

   value_struct.foreground = grey_color.pixel;
   value_struct.background = BlackPixel (ds, SCREEN_NUMBER);
   gc_stat = XCreateGC(ds,mw,value_mask,&value_struct);

   value_struct.foreground = grey_color.pixel;
   value_struct.background = BlackPixel(ds,SCREEN_NUMBER);
   gc_fill = XCreateGC(ds,mw,value_mask,&value_struct);

   value_struct.font = font_wee->fid;
   value_struct.foreground = green_color.pixel;
   value_struct.background = BlackPixel(ds,SCREEN_NUMBER);
   gc_wee = XCreateGC(ds,mw,value_mask,&value_struct);

   XMapWindow(ds,mw);

   mw_height = LAST_ROW * row_inc;
   mw_width = LAST_COL * col_inc;

   mw_bs = XCreatePixmap(ds,mw,mw_width,mw_height,mw_depth);
   XFillRectangle(ds,mw_bs,gc_fill,0,0,mw_width,mw_height);
   XMoveResizeWindow(ds,mw,mw_x,mw_y,mw_width,mw_height);

   if(TgmAttach(TgmLHC,TgmTELEGRAM | TgmLINE_NAMES) == TgmFAILURE) exit(1);

   /******************************/
   /* Main event processing loop */
   /******************************/

   geng = gthrs = gitn1 = gitn2 = 0;
   TgmGetLineNameTable(TgmLHC,"SBF1",&ltsbf);
   TgmGetLineNameTable(TgmLHC,"MODE",&ltmode);
   TgmGetLineNameTable(TgmLHC,"BTC1",&ltbtc);
   TgmGetLineNameTable(TgmLHC,"RNGI",&ltrngi);

   XSelectInput(ds,mw,KeyPressMask | ButtonPressMask);
   while(True) {

      XFillRectangle(ds,mw_bs,gc_fill,0,0,mw_width,mw_height);

      sprintf(dstr,"TgmTimeStamp     ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*1,dstr,strlen(dstr));

      sprintf(dstr,"Energy/Intensity ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*3,dstr,strlen(dstr));
      sprintf(dstr,"PotentialDamage  ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*4,dstr,strlen(dstr));

      sprintf(dstr,"FlagsRing-1      ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*6,dstr,strlen(dstr));
      sprintf(dstr,"FlagsRing-2      ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*7,dstr,strlen(dstr));

      sprintf(dstr,"FillNum/Period   ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*9,dstr,strlen(dstr));
      sprintf(dstr,"MachineMode      ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*10,dstr,strlen(dstr));

      sprintf(dstr,"BeamInRing-1     ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*12,dstr,strlen(dstr));
      sprintf(dstr,"BeamInRing-2     ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*13,dstr,strlen(dstr));

      sprintf(dstr,"NextInjection    ");
      XDrawImageString(ds,mw_bs,gc_big,2,row_inc*15,dstr,strlen(dstr));

      if(TgmWaitForNextTelegram(TgmLHC) == TgmFAILURE) exit(1);

      if((ts = TgmGetLastTelegramTimeStamp(TgmLHC))!=0) {
	 TgmDoubleToTimeStamp(ts,&uvt,&mst);
	 sprintf(dstr,"%s ",TgmTimeStampToStr(uvt,mst));
	 XDrawImageString(ds,mw_bs,gc_wee,col_inc*2,row_inc,dstr,strlen(dstr));
      }

      if (geng==0) geng = TgmGetGroupNumber(TgmLHC,"ENG");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,geng,&eng);

      if (gthrs==0) gthrs = TgmGetGroupNumber(TgmLHC,"THRS");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gthrs,&thrs);

      if (gitn1==0) gitn1 = TgmGetGroupNumber(TgmLHC,"INT1");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gitn1,&itn1);

      if (gitn2==0) gitn2 = TgmGetGroupNumber(TgmLHC,"INT2");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gitn2,&itn2);

      feng = (double) eng * 0.12;
      sprintf(dstr,"Energy:[0x%04X]/%1.2f GeV Intensity:R1:%04d-e10 R2:%04d-e10 ",eng,feng,itn1,itn2);
      XDrawImageString(ds,mw_bs,gc_wee,col_inc*2,row_inc*3,dstr,strlen(dstr));

      MttLibGetSafeBeamFlag(eng,itn1,thrs);
      fdam1 = MttLibGetDamage();

      MttLibGetSafeBeamFlag(eng,itn2,thrs);
      fdam2 = MttLibGetDamage();

      sprintf(dstr,"Damage:R1:%5.2f R2:%5.2f Threshold:%05d ",fdam1,fdam2,thrs);
      XDrawImageString(ds,mw_bs,gc_wee,col_inc*2,row_inc*4,dstr,strlen(dstr));

      if (gsbf1==0) gsbf1 = TgmGetGroupNumber(TgmLHC,"SBF1");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gsbf1,&sbf1);

      if (gsbf2==0) gsbf2 = TgmGetGroupNumber(TgmLHC,"SBF2");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gsbf2,&sbf2);

      for (i=0; i<4; i++) {
	 msk = 1<<i;
	 if (sbf1 & msk) {
	    cp = (char *) &(ltsbf.Table[i].Name);
	    XDrawImageString(ds,mw_bs,gc_wee,col_inc*(i+2),row_inc*6,cp,strlen(cp));
	 }
      }

      for (i=0; i<4; i++) {
	 msk = 1<<i;
	 if (sbf2 & msk) {
	    cp = (char *) &(ltsbf.Table[i].Name);
	    XDrawImageString(ds,mw_bs,gc_wee,col_inc*(i+2),row_inc*7,cp,strlen(cp));
	 }
      }

      if (gfiln==0) gfiln = TgmGetGroupNumber(TgmLHC,"FILLN");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gfiln,&filn);

      if (gbpnm==0) gbpnm = TgmGetGroupNumber(TgmLHC,"BPNM");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gbpnm,&bpnm);

      secs =  bpnm%60;
      mins = (bpnm/60)%60;
      hors =  bpnm/3600;

      sprintf(dstr,"Id:%05d Period:%05d Seconds since SETUP [%02d:%02d:%02d] ",filn,bpnm,hors,mins,secs);
      XDrawImageString(ds,mw_bs,gc_wee,col_inc*2,row_inc*9,dstr,strlen(dstr));

      if (gmode==0) gmode = TgmGetGroupNumber(TgmLHC,"MODE");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gmode,&mode);
      sprintf(dstr,"Value:[%02d]/%s %s ",mode,ltmode.Table[mode-1].Name,ltmode.Table[mode-1].Comment);
      XDrawImageString(ds,mw_bs,gc_wee,col_inc*2,row_inc*10,dstr,strlen(dstr));

      if (gbtc1==0) gbtc1 = TgmGetGroupNumber(TgmLHC,"BTC1");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gbtc1,&btc1);
      sprintf(dstr,"Value:[%02d]/%s %s ",btc1,ltbtc.Table[btc1-1].Name,ltbtc.Table[btc1-1].Comment);
      XDrawImageString(ds,mw_bs,gc_wee,col_inc*2,row_inc*12,dstr,strlen(dstr));

      if (gbtc2==0) gbtc2 = TgmGetGroupNumber(TgmLHC,"BTC2");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gbtc2,&btc2);
      sprintf(dstr,"Value:[%02d]/%s %s ",btc2,ltbtc.Table[btc2-1].Name,ltbtc.Table[btc2-1].Comment);
      XDrawImageString(ds,mw_bs,gc_wee,col_inc*2,row_inc*13,dstr,strlen(dstr));

      if (gbtni==0) gbtni = TgmGetGroupNumber(TgmLHC,"BTNI");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gbtni,&btni);

      if (gbkni==0) gbkni = TgmGetGroupNumber(TgmLHC,"BKNI");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,gbkni,&bkni);

      if (grngi==0) grngi = TgmGetGroupNumber(TgmLHC,"RNGI");
      TgmGetGroupValue(TgmLHC,TgmCURRENT,0,grngi,&rngi);

      sprintf(dstr,"Ring:%s Bucket:%04d Beam:%8s ",ltrngi.Table[rngi-1].Name,bkni,ltbtc.Table[btni-1].Name);
      XDrawImageString(ds,mw_bs,gc_wee,col_inc*2,row_inc*15,dstr,strlen(dstr));

      XCopyArea(ds,mw_bs,mw,gc_big,0,0,mw_width,mw_height,0,0);
      XFlush(ds);
      XSync(ds,True);       /* Garbage collect queue */
   }
}
