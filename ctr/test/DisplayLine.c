/***************************************************************************/
/* Display a line of text in a window                                      */
/* Julian Lewis                                                            */
/***************************************************************************/

static Display      * ds;              /* Display pointer */
static int            ds_width;        /* Display width */
static int            ds_height;       /* Display height */

/* Main window */

static Window         mw;              /* Main window */
static int            mw_x;            /* Main window X position */
static int            mw_y;            /* Main window Y position */
static int            mw_width;        /* Main window width */
static int            mw_height;       /* Main window heigth */
static int            mw_bo_width;     /* Main window border width */
static unsigned int   mw_bo_color;     /* Main window border color */
static unsigned int   mw_bk_color;     /* Main window background color */
static int            mw_depth;        /* Main window depth */
static Pixmap         mw_bs;           /* Main window backing store */

/* Fonts and graphics contexts */

static GC             gc_nohl, gc_fill, gc_hl; /* No high light, Fill, High light */
static XFontStruct  * font;
static unsigned int   value_mask;
static XGCValues      value_struct;
static XColor         green_color;
static XColor         grey_color;
static XColor         exact_color;

static int            row_inc;

/***************************************************************************/

void DisplayInit(char *titl, int rows, int cols) {

   XSetErrorHandler((XErrorHandler) exit);
   XSetIOErrorHandler((XIOErrorHandler) exit);

   /* Open the remote display */

   ds = XOpenDisplay(NULL);
   if (ds == NULL) {
      printf("DisplayInit: Can not open display.\n");
      perror("XLib");
      exit(1);
   };

   /* Get display size characteristics */

   ds_width  = DisplayWidth(ds,0);
   ds_height = DisplayHeight(ds,0);

   /* Set up main window characteristics */

   mw_width    = ds_width/2;
   mw_height   = ds_height;
   mw_x        = 0;
   mw_y        = 0;
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

   /* Give main window title */

   XStoreName(ds,mw,titl);

   /* Allocate colors if we are on a coulour display, else use black */
   /* and white pixels. */

   if (mw_depth > 1) {
      XAllocNamedColor(ds,DefaultColormap(ds,0),
		       "green",&green_color,&exact_color);
      XAllocNamedColor(ds,DefaultColormap(ds,0),
		       "brown",&grey_color,&exact_color);
   } else {
      green_color.pixel  = WhitePixel(ds,0);
      grey_color.pixel   = WhitePixel(ds,0);
   };

   /* Get info about and load the font for displaying member name texts */

   font = XLoadQueryFont(ds,"-adobe-courier-bold-r-normal-*-*-140-*-*-*-*-iso8859-1");
   if (font == NULL) {
      printf("DisplayInit: -adobe-courier-bold-r-normal-*-*-140-*-*-*-*-iso8859-1: No such font.\n");
      perror("XLib");
      exit(1);
   };
   row_inc = font->ascent + font->descent + 2;
   if (rows) mw_height = row_inc * (rows + 2);
   if (cols) mw_width  = XTextWidth(font,"W",1) * cols;

   /* Set up a graphics contexts. */

   value_mask = GCFont |
		GCForeground |
		GCBackground;

   /* Texts */

   value_struct.font       = font->fid;
   value_struct.foreground = BlackPixel(ds,0);
   value_struct.background = WhitePixel(ds,0);
   gc_nohl = XCreateGC(ds,mw,value_mask,&value_struct);

   /* Filling */

   value_struct.font       = font->fid;
   value_struct.foreground = grey_color.pixel;
   value_struct.background = grey_color.pixel;
   gc_fill = XCreateGC(ds,mw,value_mask,&value_struct);

   /* Colours */

   value_struct.font       = font->fid;
   value_struct.foreground = green_color.pixel;
   value_struct.background = BlackPixel(ds,0);
   gc_hl = XCreateGC(ds,mw,value_mask,&value_struct);

   /* Map the main window so that it will appear on the screen once the */
   /* X output queue has been flushed. */

   XMapWindow(ds,mw);

   /* Create the main window backing store pixmap. */

   mw_bs = XCreatePixmap(ds,mw,mw_width,mw_height,mw_depth);
   XFillRectangle(ds,mw_bs,gc_fill,0,0,mw_width,mw_height);
   XMoveResizeWindow(ds,mw,mw_x,mw_y,mw_width,mw_height);

   /* Copy the backing store to the main window, and make it appear */
   /* on the screen by flushing the output queue. This is done so that */
   /* if there are no Tg8 interrupts at least something meaningfull */
   /* appears for the user. */

   XCopyArea(ds,mw_bs,mw,gc_fill,0,0,mw_width,mw_height,0,0);
   XFlush(ds);
   XSync(ds,False);
}

/***************************************************************************/

void DisplayLine(char *txt, int row) {

static char otxt[256];
static int  orow = -1;

int y;

   if (orow != -1) {
      y = row_inc * (orow + 1);
      XDrawImageString(ds,mw_bs,gc_nohl,0,y,otxt,strlen(otxt));
   }
   strcpy(otxt,txt);
   orow = row;

   y = row_inc * (row + 1);
   XDrawImageString(ds,mw_bs,gc_hl,0,y,txt,strlen(txt));

   XBell(ds,10);
   XCopyArea(ds,mw_bs,mw,gc_fill,0,0,mw_width,mw_height,0,0);
   XFlush(ds);
   XSync(ds,True);
}
