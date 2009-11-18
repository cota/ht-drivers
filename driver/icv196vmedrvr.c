/*
  _________________________________________________________________
 |                                                                 |
 |   file name :          icv196vmedrvr.c                          |
 |                                                                 |
 |   created:     21-mar-1992 Alain Gagnaire, F. Berlin            |
 |   last update: 09-sep-1997 A. gagnaire                          |
 |                                                                 |
 |   Driver for managing of icv196vme module in the DSC providing: |
 |             - 16 interrupting lines                             |
 |                                                                 |
 |_________________________________________________________________|

  updated:
  =======
 21-mar-1992 A.G., F.B.: version integrated with the event handler common to
                         sdvme, and icv196vme driver
 02-dec-1992 A.G. - correct disconnect: unlink cumul event  in the ring
                  - manage propely disconnection of cumulative connection
		    including set up of cumulative event pending
    standard entries for driver installation:
                   icv196vmeopen icv196vmeclose,
                   icv196vmeread, icv196vmewrite,
                   icv196vmeioctl, icv196vmeselect
                   icv196vmeinstall,icv196vmeuninstall,
		   icv196vmeirq
 16-mar-1994 A.G. : reanable the line supporting connections after a reset
 10-may-1994 A.G.: update to stand 8 modules
 09-sep-1997 A:G:  common source for several platform: motorola, PowerPC
*/
#include <cdcm/cdcm.h>
#include <vme_am.h>
#include <skeldrvr.h>
#include <skeldrvrP.h>
#include <skeluser_ioctl.h>
#include <icv196vme.h>
#include <icv196vmeP.h>


#define USER_VBASE 64
#define PURGE_CPUPIPELINE asm("eieio") /* purge cpu pipe line */
#define ISYNC_CPUPIPELINE asm("sync")  /* synchronise instruction  */
#define IOINTSET(c, v,i,s) c = vme_intset ( v, i, s, 0)
#define IOINTCLR(v)  vme_intclr(v, 0)


#define X_NO_enable_Module  1
#define X_NO_disable_Module 1
#define X_NO_CheckBooking   1

#ifndef NODEBUG
#define DBG(a) cprintf a
#else
#define DBG(a)
#endif


#ifndef NODBX
#define DBX(a) cprintf a
#else
#define DBX(a)
#endif

#define NOCHK
#ifndef NOCHK
#define CHK(a) cprintf a
#else
#define CHK(a)
#endif

#define CPRNT(a) cprintf a
#define ERR_KK(a) kkprintf a

#define ICVDBG_first    0x1
#define ICVDBG_errtr    0x2
#define ICVDBG_trace    0x4
#define ICVDBG_ctrl     0x8
#define ICVDBG_Get      0x10
#define ICVDBG_Put      0x20
#define ICVDBG_ok       0x30
#define ICVDBG_ioctl    0x40
#define ICVDBG_passive  0x80
#define ICVDBG_install  0x100
#define ICVDBG_cprintf  0x200
#define ICVDBG_timeout  0x400
#define ICVDBG_kkprintf 0x800

#define ICVDBG_field  0x0000ffff

#define MILMEAS_trace 0x080000
#define MILTRACK_TO   0x010000

#define DBG_kkprintf(a) {if (G_dbgflag & ICVDBG_kkprintf) {kkprintf a;}}
#define DBG_cprintf(a)  {if (G_dbgflag & ICVDBG_cprintf)  {cprintf  a;}}
#define DBG_ERRTR(a)    {if (G_dbgflag & ICVDBG_errtr)    {cprintf  a;}}
#define DBG_KKFIRST(a)  {if (G_dbgflag & ICVDBG_first)    {cprintf  a;}}
#define DBG_KKTR(a)     {if (G_dbgflag & ICVDBG_trace)    {cprintf  a;}}
#define DBG_CTRL(a)     {if (G_dbgflag & ICVDBG_ctrl)     {cprintf  a;}}
#define DBG_TROK(a)     {if (G_dbgflag & ICVDBG_ok)       {cprintf  a;}}
#define DBG_GETKK(a)    {if (G_dbgflag & ICVDBG_Get)      {cprintf  a;}}
#define DBG_PUTKK(a)    {if (G_dbgflag & ICVDBG_Put)      {cprintf  a;}}
#define DBG_IOCTL(a)    {if (G_dbgflag & ICVDBG_ioctl)    {cprintf  a;}}
#define DBG_PASSIVE(a)  {if (G_dbgflag & ICVDBG_passive)  {cprintf  a;}}
#define DBG_TIMEOUT(a)  {if (G_dbgflag & ICVDBG_timeout)  {cprintf  a;}}
#define DBG_INSTAL(a)   {if (G_dbgflag & ICVDBG_install)  {cprintf  a;}}

/*
                Local define
                ------------
*/
#define IGNORE_SIG 0

#define ICV_tout 1000

static int icv196vmeisr (void *);

static char Version[]="$Revision: 3.1 $";
static char compile_date[] = __DATE__;
static char compile_time[] = __TIME__;
static int G_dbgflag = 0;


/*
                     ______________________________
                    |                              |
                    |        configuration         |
                    |                              |
                    |  for the icv196vme module    |
                    |______________________________|

 It shows how the device lines are mapped on the logical unit index.

 The space of logical line is continuous, is different of the space of
 the physical lines.
 Special routines are dedicated to compute correspondance beetween user
 event source address: (source, line) and the physical line associated
 by this configuration.
 This make code less dependent of hardware configuration and type of module
 used as trigger source !
*/

		/*  structure to store for one module the  logical lines
		    corresponding to the physical lines */
struct T_ModuleLogLine {
                 int Module_Index;
		 short LogLineIndex[icv_LineNb];
	       };
		/*  sizing of module */
short LinePerModule= icv_LineNb;


/* isr entry point table  */
int (*ModuleIsr[icv_ModuleNb])(void *) = {
	icv196vmeisr,
	icv196vmeisr,
	icv196vmeisr,
	icv196vmeisr
};

/*                 table to link for each possible module:
		   actual phys line and logical unit index
*/
 struct T_ModuleLogLine Module1 = {0, {ICV_L101, ICV_L102, ICV_L103, ICV_L104,
				   ICV_L105,ICV_L106, ICV_L107, ICV_L108,
				   ICV_L109,ICV_L110, ICV_L111, ICV_L112,
				   ICV_L113,ICV_L114, ICV_L115, ICV_L116,
				  }};

 struct T_ModuleLogLine Module2 = {1, {ICV_L201, ICV_L202, ICV_L203, ICV_L204,
				   ICV_L205,ICV_L206, ICV_L207, ICV_L208,
				   ICV_L209,ICV_L210, ICV_L211, ICV_L212,
				   ICV_L213,ICV_L214, ICV_L215, ICV_L216,
				  }};

 struct T_ModuleLogLine Module3 = {2, {ICV_L301, ICV_L302, ICV_L303, ICV_L304,
				   ICV_L305,ICV_L306, ICV_L307, ICV_L308,
				   ICV_L309,ICV_L310, ICV_L311, ICV_L312,
				   ICV_L313,ICV_L314, ICV_L315, ICV_L316,
				  }};

 struct T_ModuleLogLine Module4 = {3, {ICV_L401, ICV_L402, ICV_L403, ICV_L404,
				   ICV_L405,ICV_L406, ICV_L407, ICV_L408,
				   ICV_L409,ICV_L410, ICV_L411, ICV_L412,
				   ICV_L413,ICV_L414, ICV_L415, ICV_L416,
				  }};

 struct T_ModuleLogLine Module5 = {4, {ICV_L501, ICV_L502, ICV_L503, ICV_L504,
				   ICV_L505,ICV_L506, ICV_L507, ICV_L508,
				   ICV_L509,ICV_L510, ICV_L511, ICV_L512,
				   ICV_L513,ICV_L514, ICV_L515, ICV_L516,
				  }};
 struct T_ModuleLogLine Module6 = {5, {ICV_L601, ICV_L602, ICV_L603, ICV_L604,
				   ICV_L605,ICV_L606, ICV_L607, ICV_L608,
				   ICV_L609,ICV_L610, ICV_L611, ICV_L612,
				   ICV_L613,ICV_L614, ICV_L615, ICV_L616,
				  }};
 struct T_ModuleLogLine Module7 = {6, {ICV_L701, ICV_L702, ICV_L703, ICV_L704,
				   ICV_L705,ICV_L706, ICV_L707, ICV_L708,
				   ICV_L709,ICV_L710, ICV_L711, ICV_L712,
				   ICV_L713,ICV_L714, ICV_L715, ICV_L716,
				  }};
 struct T_ModuleLogLine Module8 = {7, {ICV_L801, ICV_L802, ICV_L803, ICV_L804,
				   ICV_L805,ICV_L806, ICV_L807, ICV_L808,
				   ICV_L809,ICV_L810, ICV_L811, ICV_L812,
				   ICV_L813,ICV_L814, ICV_L815, ICV_L816,
				  }};


		/* table of the maximum configuration : 4 modules */

 struct T_ModuleLogLine *MConfig[icv_ModuleNb]= {&Module1, &Module2, &Module3, &Module4, &Module5, &Module6, &Module7, &Module8 };

/*		table to link logical index and user line address
*/
struct icv196T_UserLine UserLineAdd[ICV_LogLineNb]={

       {ICV_Group0,ICV_Index00},{ICV_Group0,ICV_Index01},{ICV_Group0,ICV_Index02},{ICV_Group0,ICV_Index03},
       {ICV_Group0,ICV_Index04},{ICV_Group0,ICV_Index05},{ICV_Group0,ICV_Index06},{ICV_Group0,ICV_Index07},
       {ICV_Group0,ICV_Index08},{ICV_Group0,ICV_Index09},{ICV_Group0,ICV_Index10},{ICV_Group0,ICV_Index11},
       {ICV_Group0,ICV_Index13},{ICV_Group0,ICV_Index14},{ICV_Group0,ICV_Index15},{ICV_Group0,ICV_Index16},

       {ICV_Group1,ICV_Index00},{ICV_Group1,ICV_Index01},{ICV_Group1,ICV_Index02},{ICV_Group1,ICV_Index03},
       {ICV_Group1,ICV_Index04},{ICV_Group1,ICV_Index05},{ICV_Group1,ICV_Index06},{ICV_Group1,ICV_Index07},
       {ICV_Group1,ICV_Index08},{ICV_Group1,ICV_Index09},{ICV_Group1,ICV_Index10},{ICV_Group1,ICV_Index11},
       {ICV_Group1,ICV_Index13},{ICV_Group1,ICV_Index14},{ICV_Group1,ICV_Index15},{ICV_Group1,ICV_Index16},

       {ICV_Group2,ICV_Index00},{ICV_Group2,ICV_Index01},{ICV_Group2,ICV_Index02},{ICV_Group2,ICV_Index03},
       {ICV_Group2,ICV_Index04},{ICV_Group2,ICV_Index05},{ICV_Group2,ICV_Index06},{ICV_Group2,ICV_Index07},
       {ICV_Group2,ICV_Index08},{ICV_Group2,ICV_Index09},{ICV_Group2,ICV_Index10},{ICV_Group2,ICV_Index11},
       {ICV_Group2,ICV_Index13},{ICV_Group2,ICV_Index14},{ICV_Group2,ICV_Index15},{ICV_Group2,ICV_Index16},

       {ICV_Group3,ICV_Index00},{ICV_Group3,ICV_Index01},{ICV_Group3,ICV_Index02},{ICV_Group3,ICV_Index03},
       {ICV_Group3,ICV_Index04},{ICV_Group3,ICV_Index05},{ICV_Group3,ICV_Index06},{ICV_Group3,ICV_Index07},
       {ICV_Group3,ICV_Index08},{ICV_Group3,ICV_Index09},{ICV_Group3,ICV_Index10},{ICV_Group3,ICV_Index11},
       {ICV_Group3,ICV_Index13},{ICV_Group3,ICV_Index14},{ICV_Group3,ICV_Index15},{ICV_Group3,ICV_Index16},

       {ICV_Group4,ICV_Index00},{ICV_Group4,ICV_Index01},{ICV_Group4,ICV_Index02},{ICV_Group4,ICV_Index03},
       {ICV_Group4,ICV_Index04},{ICV_Group4,ICV_Index05},{ICV_Group4,ICV_Index06},{ICV_Group4,ICV_Index07},
       {ICV_Group4,ICV_Index08},{ICV_Group4,ICV_Index09},{ICV_Group4,ICV_Index10},{ICV_Group4,ICV_Index11},
       {ICV_Group4,ICV_Index13},{ICV_Group4,ICV_Index14},{ICV_Group4,ICV_Index15},{ICV_Group4,ICV_Index16},

       {ICV_Group5,ICV_Index00},{ICV_Group5,ICV_Index01},{ICV_Group5,ICV_Index02},{ICV_Group5,ICV_Index03},
       {ICV_Group5,ICV_Index04},{ICV_Group5,ICV_Index05},{ICV_Group5,ICV_Index06},{ICV_Group5,ICV_Index07},
       {ICV_Group5,ICV_Index08},{ICV_Group5,ICV_Index09},{ICV_Group5,ICV_Index10},{ICV_Group5,ICV_Index11},
       {ICV_Group5,ICV_Index13},{ICV_Group5,ICV_Index14},{ICV_Group5,ICV_Index15},{ICV_Group5,ICV_Index16},

       {ICV_Group6,ICV_Index00},{ICV_Group6,ICV_Index01},{ICV_Group6,ICV_Index02},{ICV_Group6,ICV_Index03},
       {ICV_Group6,ICV_Index04},{ICV_Group6,ICV_Index05},{ICV_Group6,ICV_Index06},{ICV_Group6,ICV_Index07},
       {ICV_Group6,ICV_Index08},{ICV_Group6,ICV_Index09},{ICV_Group6,ICV_Index10},{ICV_Group6,ICV_Index11},
       {ICV_Group6,ICV_Index13},{ICV_Group6,ICV_Index14},{ICV_Group6,ICV_Index15},{ICV_Group6,ICV_Index16},

       {ICV_Group7,ICV_Index00},{ICV_Group7,ICV_Index01},{ICV_Group7,ICV_Index02},{ICV_Group7,ICV_Index03},
       {ICV_Group7,ICV_Index04},{ICV_Group7,ICV_Index05},{ICV_Group7,ICV_Index06},{ICV_Group7,ICV_Index07},
       {ICV_Group7,ICV_Index08},{ICV_Group7,ICV_Index09},{ICV_Group7,ICV_Index10},{ICV_Group7,ICV_Index11},
       {ICV_Group7,ICV_Index13},{ICV_Group7,ICV_Index14},{ICV_Group7,ICV_Index15},{ICV_Group7,ICV_Index16},

};

/*
                     ______________________________
                    |                              |
                    |  icv196vmes:                 |
                    |  structure of Static table   |
		    |       for the driver         |
                    |                              |
                    |______________________________|
*/
/*
                  structure of the static table
	          ----------------------------

 This shows the structure of the static table of the driver dynamically
 allocated  by the install subroutine of the driver.

*/
struct icv196T_s {
  int   install;
  int   usercounter;			/* User counter */
  int   sem_drvr;			/* semaphore for exclusive access to
					   static table */
  int   dum1;
  int   timid;				/* Interval timer Identification */
  int   interval;			/* Timerinterval */
/*	     % extention to stand interrupt processing
*/
  int   UserTO;				/* User time out, by waiting for ICV */
  short   UserMode;			/* ? */
		/* Sizing : depend  on needs of user interface */
  short SubscriberMxNb;

		/*  User handle: as many as device created at install */

  struct T_UserHdl ServiceHdl;         /* sharable user service handle */
  int    UserHdlICV_Size;
  struct T_UserHdl ICVHdl[ICVVME_MaxChan]; /* user handle for synchro */

		/*  ring buffer to serialise event coming from the modules */

  struct T_RingBuffer RingGlobal;
  long Buffer_Evt[GlobEvt_nb]; /* To put event in a sequence */

				/* Hardware table */

		/* configuration table  */
  int    ModuleCtxt_Size;
  struct T_ModuleCtxt *ModuleCtxtDir[icv_ModuleNb]; /*  module directory */

  struct T_ModuleCtxt ModuleCtxt[icv_ModuleNb]; /* Modules contexts */

				/* Logical line tables */

  int LogLine_Size;
  struct T_LogLineHdl *LineHdlDir[ICV_LogLineNb];    /* Logical line directory */

  struct T_LogLineHdl LineHdl[ICV_LogLineNb];     /* Logical line handles */

  int icv_ends;
}; /* end struct icv196T_s  */

/*
                  _______________________________________
                 |                                       |
                 |   Driver's subroutine                 |
                 |_______________________________________|
*/

/*                        subroutines to manage user wake up
			  ==================================
*/
/*  UserWakeup:
    ++++++++++
    to come out of waiting event
*/
static void UserWakeup(struct T_UserHdl *UHdl)
{
  int ps;

  ssignal (&UHdl -> Ring.Evtsem);
  disable(ps);
  if (UHdl -> timid >= 0){
    cancel_timeout (UHdl -> timid);
  }
  UHdl -> timid = (-1);
  restore(ps);
}

/*	       Management of logical/physical Line mapping
	       ===========================================
*/

/*  CnvrtUserLine:
    +++++++++++++
    subroutines to convert a user line address in a logical line index
*/
static int CnvrtUserLine(char grp, char index)
{
  int val;

  val = (int) ( (grp * ICV_IndexNb) +  index );
  return (val);
}

/*  CnvrtModuleLine:
    +++++++++++++++
    subroutines to convert a module line address in a logical index
*/
static int CnvrtModuleLine(int module, int line)
{
   return ((int)(( *(MConfig[module])).LogLineIndex[line]));
}

/* subroutines to get a user line address from a logical line index */
static void GetUserLine(int logindex, struct icv196T_UserLine *add)
{
  add -> group = UserLineAdd[logindex].group;
  add -> index = UserLineAdd[logindex].index;
}
/*  Init_Dir:
    ++++++++
    subroutines to initialise the directory tables
*/

static void Init_Dir(struct icv196T_s *s)
{
  int i;

  for (i = 0; i < icv_ModuleNb ; i++)
    s -> ModuleCtxtDir[i] = NULL;     /* Default value*/
  for (i = 0; i < ICV_LogLineNb ; i++)
    s -> LineHdlDir[i]    = NULL;     /* Default value */

}

/*			subroutines to manage ring buffer:
			=================================
*/
/*  Init_Ring:
    ++++++++++
    subroutine to  Initialise Ring buffer descriptor
*/
static void Init_Ring(struct T_UserHdl *UHdl, struct T_RingBuffer *Ring,
		      struct icvT_RingAtom *Buf, short mask)
{
  short   i;
  struct icvT_RingAtom *L_ptr;

  Ring->UHdl = UHdl;
  Ring->Buffer = Buf;
  Ring->Evtsem =0;
  Ring->mask = mask;
  Ring->reader = Ring->writer = 0;
  L_ptr = &Buf[mask];
  for (i = mask; i >= 0; i--, L_ptr--) {
    L_ptr -> Subscriber = NULL;
    L_ptr -> Evt.All = 0;
  }
} /* Init_Ring */

/*   PushTo_Ring:
     ++++++++++++
     Subroutine - to push value in ring and signal semaphore,
                -  callable from isr only: no disable done !!
*/
static struct icvT_RingAtom *PushTo_Ring(struct T_RingBuffer *Ring,
					 struct icvT_RingAtom *atom)
{
  short  I;
  struct icvT_RingAtom *L_Atom;
  struct T_UserHdl *L_UHdl;

  if (Ring -> Evtsem >= Evt_nb){	/* Ring buffer full */
    sreset( &(Ring -> Evtsem));	/* Clear semaphore, awake all process waiting on it */
    Ring -> reader = Ring -> writer = 0;
			/* push an event to signal purge of buffer */
    L_Atom = &(Ring -> Buffer[0]);
    L_Atom -> Subscriber = NULL;
    L_Atom -> Evt.All= 0;
    L_Atom -> Evt.Word.w1 = (-1);
    Ring -> writer++;
    ssignal( &(Ring -> Evtsem));
    L_UHdl = Ring -> UHdl;
/*    DBG (("icv196:isr:purge Ring for Chanel=%d UHdl= $%lx\n", L_UHdl -> chanel, L_UHdl ));*/
  }
		/* now push the event in the ring  */
  I = Ring -> writer;
  L_Atom = &(Ring -> Buffer[I]); /* Atom to set up */
  L_Atom -> Evt.All =     atom -> Evt.All;
  L_Atom -> Subscriber =  atom -> Subscriber;
  Ring -> writer = ((++I) & (Ring -> mask));
  ssignal(&(Ring -> Evtsem));		/* manage ring  */
/*  DBG (("icvp:Push Evtsem %lx Writer = %lx Evt= %lx, Subs=$%lx \n",Ring -> Evtsem, Ring -> writer, L_Atom -> Evt.All, L_Atom -> Subscriber ));*/

  return (L_Atom);
} /* PushTo_Ring  */

/*  PullFrom_Ring:
    +++++++++++++
    Subroutine to pull out an event from the Ring
*/
static unsigned long PullFrom_Ring(struct T_RingBuffer *Ring)
{
  int   ps;
  register struct icvT_RingAtom *L_Atom;
  register struct T_Subscriber *L_Subs;
  short  I, ix;
  union icvU_Evt L_Evt;
  /*  */
  /*DBG(("icv:PullFrom: *Ring=$%lx Evtsem=%d \n", Ring, Ring -> Evtsem ));*/
  disable (ps);				/* Protect seq. from isr access */
    ix= I = Ring -> reader;
  if ((Ring -> Evtsem) != 0){	/* Ring non empty */
    swait( &(Ring -> Evtsem), (-1));	/* to manage sem counter */
    L_Atom = &(Ring -> Buffer[I]); /* point current atom to pull from ring */
	/* extract the event to be read */
    L_Subs =    L_Atom -> Subscriber;
    L_Evt.All = L_Atom -> Evt.All;
    Ring -> reader = ((++I) & (Ring -> mask));	/* manage ring buffer */
		/* build the value to return according to mode */
    if ((L_Subs != NULL)) {	/* Case cumulative mode */
      /* DBG(("icv:PullFrom: Subs*=$%lx LHdl=%lx \n", L_Subs, L_Subs -> LHdl));*/
      L_Evt.Word.w1 = L_Subs -> EvtCounter; /* counter from subscriber*/
		/* Clear event flag in subscriber context */
      L_Subs -> CumulEvt = NULL;        /*no more event in ring*/
      L_Subs -> EvtCounter = (-1);	/*clear event present in the ring */
    }
/*    DBG (("icv196:PullRing: buff $%lx Reader= %d Evt= $%lx\n", Ring -> Buffer, ix, L_Evt.All));*/
  }
  else {	/* ring empty  */
    L_Evt.All = (-1);
    /* DBG (("icv196:Empty ring: buff * %lx \n", Ring -> Buffer)); */
  }
  restore (ps);
  return (L_Evt.All);
} /* PullFromRing */

/*                        subroutines to initialise structures
			  ====================================
*/
/*  Init_UserHdl:
     ++++++++++++
    Subroutine to initialise a user Handle
*/
static void Init_UserHdl(struct T_UserHdl *UHdl, int chanel,
			 struct icv196T_s *s)
{
  int i;
  char *L_cptr;

  UHdl -> s = s;
  UHdl -> chanel = chanel;
  UHdl -> pid = 0;
  UHdl -> usercount = 0;       /* clear current user number */
  UHdl -> sel_sem = NULL;		/* clear the semaphore for the select */
	/* clear up  bit map of established connection */

  for ( (i = 0, L_cptr = &(UHdl -> Cmap[0])); i < ICV_mapByteSz ; i++)
    *L_cptr++ = 0;

  Init_Ring (UHdl, &(UHdl -> Ring), &(UHdl -> Atom[0]), Evt_msk);
} /* Init_UserHdl */

/* Init_SubscriberHdl:
    ++++++++++++++++++++
    subroutines Initialise a subscriber Handle
*/

static void Init_SubscriberHdl(struct T_Subscriber *Subs, int mode)
{
  struct icvT_RingAtom *L_Evt;

			/* clean up  Event in Ring if element active  */
  if ((L_Evt = Subs -> CumulEvt) != NULL){
    L_Evt -> Evt.Word.w1 = Subs -> EvtCounter; /* unlink cumulative event */
    L_Evt -> Subscriber = NULL;
  }
  Subs -> CumulEvt = NULL;	/* no event in the ring */
  Subs -> Ring = NULL;	/* Clear the subscribing link */
  Subs -> mode = mode;
  Subs -> EvtCounter = 0;

/*    CHK (("icv:Subs: Subs Hdl add = $%lx\n", Subs ));*/

}

/* Init_LineSubscribers:
   ++++++++++++++++++++
    subroutines Initialise the subscriber directory
  of a logical line handle
*/
static void Init_LineSubscribers(struct T_LogLineHdl *LHdl)
{
  struct T_Subscriber *L_Subs;
  int i, n, mode;

  if ( ((LHdl -> LineCtxt) -> Type) == icv_FpiLine)
    mode  = icv_cumulative;  /*  default for pls line cumulative mode */
  else
    mode  = icv_queuleuleu ; /*default for fpi line a la queue leuleu */

  L_Subs = &(LHdl -> Subscriber[0]);
/*  CHK (("icv:LSubs: LSubs add =$%lx   \n",L_Subs ));*/

  n = LHdl -> SubscriberMxNb;
  for (i=0; i < n ; i++, L_Subs ++ ){
    L_Subs -> LHdl = LHdl;
    L_Subs -> CumulEvt = NULL;	/* to prevent active event processing */

    Init_SubscriberHdl(L_Subs, mode);
  }
}

/*  Init_LineHdl:
    ++++++++++++
    subroutines to initialise a Line handle
    associated to a physical line
*/
static struct T_LogLineHdl *Init_LineHdl(int lli, struct T_LineCtxt *LCtxt)
{
  struct icv196T_s *s;
  struct T_LogLineHdl *L_LHdl;
  union  icv196U_UserLine L_UserLine;

  s = LCtxt -> s;
  L_LHdl = s -> LineHdlDir[lli] = &(s -> LineHdl[lli]);
  L_LHdl -> s = s;
  L_LHdl -> LogIndex = lli;
  L_LHdl -> LineCtxt = LCtxt;	/* Link Line handle and Line context */
  GetUserLine(lli, (struct icv196T_UserLine *) &L_UserLine); /* Get the user line address pattern */
  /* build the event pattern for that line */
  L_LHdl -> Event_Pattern.Word.w2 =  L_UserLine.All;
  L_LHdl -> Event_Pattern.Word.w1 = 0;

/*  CHK (("icv:LHdl: s = $%lxLogIndex %d LHdl add= $%lx  \n",L_LHdl -> s, lli, L_LHdl ));*/

  L_LHdl -> SubscriberCurNb =0;	        /* Current Subscriber nb set to 0  */
  if (LCtxt -> Type == icv_FpiLine)	/* Fpi line only one subscriber allowed */
    L_LHdl -> SubscriberMxNb =1;
  else				        /* Icv line multi user to wait  */
    L_LHdl -> SubscriberMxNb = icv_SubscriberNb;
  Init_LineSubscribers(L_LHdl);
  return (L_LHdl);
}

/*			subroutines to manage logical connection
			========================================
*/
/*  LineBooking:
    ++++++++++++
    Establish connection for a channel with a given logical line
*/
static struct T_Subscriber *LineBooking(struct T_UserHdl *UHdl,
					struct T_LogLineHdl *LHdl,
					int mode)
{
  int ps, i, ns;
  struct T_Subscriber *L_Subs, *L_val ;
  struct T_LineCtxt   *L_LCtxt;

  L_val = NULL;
  L_LCtxt = LHdl -> LineCtxt;
  ns = LHdl -> SubscriberMxNb;
  L_Subs = &(LHdl -> Subscriber[0]);
  disable(ps);

  for (i=0; i < ns ; i++, L_Subs++){

    if ( L_Subs -> Ring != NULL )    /* subscriber occupied, scan on */
      continue;

    /* allocated this element */
    LHdl -> SubscriberCurNb ++;	/* update current subscriber number  */
    L_val =   L_Subs;		/* found a place */
    L_Subs -> Ring = &(UHdl -> Ring); /* link to Ring buffer */
    L_Subs -> CumulEvt = NULL;
		/*  flag booking of the line in the User handle */
    SETBIT(UHdl -> Cmap,LHdl -> LogIndex);
    L_Subs -> mode = icv_queuleuleu; /* default mode */
    L_Subs -> EvtCounter = 0;	/* counter for connected line */

    if (mode == 0){		/* no user requirement */
      if (L_LCtxt -> Type == icv_FpiLine){
	L_Subs -> mode = icv_cumulative; /*def. cumulative  for Pls Line */
	L_Subs -> EvtCounter = (-1);
      }
    }
    else if (mode == icv_cumulative){
      L_Subs -> mode = icv_cumulative;
      L_Subs -> EvtCounter = (-1);
    }
    break;
   }/*end for  */

  restore(ps);
  return(L_val);
} /* LineBooking  */

/*  LineUnbooking:
    ++++++++++++++
    Rub out  connection of a chanel from a given logical line
*/
static struct T_Subscriber *LineUnBooking(struct T_UserHdl *UHdl,
					  struct T_LogLineHdl *LHdl)
{
  int ps, i, ns;
  struct T_Subscriber *L_Subs;
  struct T_Subscriber *L_val;
  register struct T_RingBuffer *L_UHdlRing;

  L_val = NULL;
  L_UHdlRing = &(UHdl -> Ring);
  disable(ps);
  ns = LHdl -> SubscriberMxNb;
  L_Subs = &(LHdl -> Subscriber[0]);
  for (i = 0; i < ns ; i++, L_Subs++){
    if (   (L_Subs -> Ring == NULL)
	|| (L_Subs -> Ring != L_UHdlRing))
      continue;			/* ==> scan on */

    /* subscriber found: now get rid of connection */
    LHdl -> SubscriberCurNb --;	/* update current subscriber number  */
    Init_SubscriberHdl(L_Subs, 0); /* init subscriber with default mode */

    L_val = L_Subs;
		/*  clear flag of booking of the line in the User handle */
    CLRBIT(UHdl -> Cmap,LHdl -> LogIndex);
    break;
  }
  restore(ps);
  return(L_val);
}

#ifndef X_NO_CheckBooking

/*  CheckBooking:
    ++++++++++++++
    Check if a given UHdl got connected to the LHdl
*/
static struct T_Subscriber *CheckBooking(struct T_UserHdl *UHdl,
					 struct T_LogLineHdl *LHdl)
{
  int ps, i, ns;
  struct T_Subscriber *L_Subs;
  struct T_Subscriber *L_val;
  register struct T_RingBuffer *L_UHdlRing;

  L_val = NULL;
  L_UHdlRing = &(UHdl -> Ring);
  disable(ps);
  ns = LHdl -> SubscriberMxNb;
  L_Subs = &(LHdl -> Subscriber[0]);
  for (i = 0; i < ns ; i++, L_Subs++){
    if (   (L_Subs -> Ring == NULL)
	|| (L_Subs -> Ring != L_UHdlRing))
      continue;			/* ==> scan on */

    L_val = L_Subs;
    break;
  }
  restore(ps);
  return(L_val);
} /* CheckBooking  */
#endif


/*  enable_Line:
    +++++++++++
    to enable interrupt from a line called from service level
*/
static void enable_Line(struct T_LineCtxt *LCtxt)
{
  int ps, locdev;
  unsigned char status, dummy, *L_CtrStat;
  unsigned short mask;

  L_CtrStat = LCtxt->MCtxt->VME_StatusCtrl;
  locdev = LCtxt->Line;
  /*DBX (("enable line: locdev = %d\n", locdev));*/

  if (locdev >= PortA_nln) {             /*if locdev is on portB:  */
					     /*enable interrupt on   */
					     /*line nr. locdev on portB*/
    disable(ps);
    mask = 1 << locdev;                /* set interrupt enable */
    LCtxt->MCtxt->int_en_mask |= mask;      /* mask          */
    /*DBX (("enable line: int_en_mask = 0x%x\n", LCtxt->MCtxt->int_en_mask));*/

    mask = 1 << (locdev - PortA_nln);

    *L_CtrStat = CSt_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrPo_Breg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status | mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrPo_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrTr_Breg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status | mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrTr_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrMsk_Breg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status | mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrMsk_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    /* TEST  !! */
    *L_CtrStat = PtrMsk_Breg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("enable line: PtrMsk_Breg = 0x%x\n", dummy));*/

    /* TEST  !! */
    *L_CtrStat = PtrPo_Breg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("enable line: PtrPo_Breg = 0x%x\n", dummy));*/

    /* TEST  !! */
    *L_CtrStat = PtrTr_Breg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("enable line: PtrTr_Breg = 0x%x\n", dummy));*/


    *L_CtrStat = CSt_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    restore(ps);
  }

  else {			/* Case port A */
    disable(ps);
    mask = 1 << locdev;                /* set interrupt enable */
    LCtxt->MCtxt->int_en_mask |= mask;      /* mask          */
    /*DBX (("enable line: int_en_mask = 0x%x\n", LCtxt->MCtxt->int_en_mask));*/

    *L_CtrStat = CSt_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrPo_Areg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status | mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrPo_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrTr_Areg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status | mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrTr_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrMsk_Areg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status | mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrMsk_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    /* TEST  !! */
    *L_CtrStat = PtrMsk_Areg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("enable line: PtrMsk_Areg = 0x%x\n", dummy));*/

    /* TEST  !! */
    *L_CtrStat = PtrPo_Areg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("enable line: PtrPo_Areg = 0x%x\n", dummy));*/

    /* TEST  !! */
    *L_CtrStat = PtrTr_Areg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("enable line: PtrTr_Areg = 0x%x\n", dummy));*/


    *L_CtrStat = CSt_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    restore(ps);
  }

  LCtxt -> status = icv_Enabled;
} /* enable_Line */

/*  disable_Line:
    ++++++++++++
    to stop interrupt from this line this is done when user  request it
*/
static void disable_Line(struct T_LineCtxt *LCtxt)
{
  int ps, locdev;
  unsigned char status, dummy, *L_CtrStat;
  unsigned short mask;

  L_CtrStat = LCtxt->MCtxt->VME_StatusCtrl;
  locdev = LCtxt->Line;
  /*DBX (("disable line: locdev = %d\n", locdev));*/
  if (locdev >= PortA_nln) {             /*if locdev is on portB:  */
					     /*enable interrupt on   */
					     /*line nr. locdev on portB*/
    disable(ps);
    mask = ~(1 << locdev);              /* clear interrupt enable */
    LCtxt->MCtxt->int_en_mask &= mask;  /* mask                   */
    /*DBX (("disable line: int_en_mask = 0x%x\n", LCtxt->MCtxt->int_en_mask))*/;

    mask = ~(1 << (locdev - PortA_nln));

    *L_CtrStat = CSt_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrPo_Breg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status & mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrPo_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrTr_Breg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status & mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrTr_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrMsk_Breg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status & mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrMsk_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    /* TEST  !! */
    *L_CtrStat = PtrMsk_Breg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("disable line: PtrMsk_Breg = 0x%x\n", dummy));*/

    /* TEST  !! */
    *L_CtrStat = PtrPo_Breg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("disable line: PtrPo_Breg = 0x%x\n", dummy));*/

    /* TEST  !! */
    *L_CtrStat = PtrTr_Breg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("disable line: PtrTr_Breg = 0x%x\n", dummy));*/

    *L_CtrStat = CSt_Breg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    restore(ps);
  }
  else {                               /*else:                 */
					   /*disable interrupt on  */
					   /*line nr. locdev on portA*/
    disable(ps);
    mask = ~(1 << locdev);
    /* clear interrupt enable*/
    LCtxt->MCtxt->int_en_mask &= mask;      /* mask          */
    /*DBX (("disable line: int_en_mask = 0x%x\n", LCtxt->MCtxt->int_en_mask))*/;

    *L_CtrStat = CSt_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrPo_Areg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status & mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrPo_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrTr_Areg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status & mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrTr_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    *L_CtrStat = PtrMsk_Areg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;
      PURGE_CPUPIPELINE;
    status = status & mask;
      PURGE_CPUPIPELINE;
    *L_CtrStat = PtrMsk_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = status;
      PURGE_CPUPIPELINE;

    /* TEST  !! */
    *L_CtrStat = PtrMsk_Areg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("disable line: PtrMsk_Areg = 0x%x\n", dummy));*/

    /* TEST  !! */
    *L_CtrStat = PtrPo_Areg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("disable line: PtrPo_Areg = 0x%x\n", dummy));*/

    /* TEST  !! */
    *L_CtrStat = PtrTr_Areg;
      PURGE_CPUPIPELINE;
    dummy = *L_CtrStat;
      PURGE_CPUPIPELINE;
    /*DBX(("disable line: PtrTr_Areg = 0x%x\n", dummy));*/

    *L_CtrStat = CSt_Areg;
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    restore(ps);
  }
  LCtxt -> status = icv_Disabled;
}

/*  ClrSynchro:
    +++++++++++
  to get rid of all connections established on a channel
  called at close of channel
*/
static void ClrSynchro(struct T_UserHdl *UHdl)
{
  int ps;
  int i,j;
  struct icv196T_s   *s;
  struct T_LineCtxt   *L_LCtxt;
  register struct T_LogLineHdl  *L_LHdl;
  register struct T_RingBuffer *L_UHdlRing;
  register struct T_Subscriber *L_Subs;
  unsigned char w;
  char *L_cptr;
	/*  */
  s =  UHdl -> s;
  L_UHdlRing = &(UHdl -> Ring);
  if ( UHdl -> timid >= 0)
    cancel_timeout(UHdl -> timid);

  for ( (i = 0, L_cptr = &(UHdl -> Cmap[0]) ); i < ICV_mapByteSz; i++, L_cptr++ ){ /* scan map byte array  */
    if ((w = (*L_cptr) & 255) != 0){
      *L_cptr = 0;	/* Clear bits */
      for (j=7; j >=0; j--, w <<= 1 ){ /* scan each bit of the current byte */
	if (w == 0)
	  break;
	if ( ((char) w) < 0 ){	/* line connected */
	  if ( (L_LHdl  = s -> LineHdlDir[(i*8 + j)]) == NULL){
/*	    DBG(("icv196:Clrsynchro: log line not in directory \n"));*/
	    continue;
	  }
	  if ( (L_Subs = LineUnBooking(UHdl, L_LHdl)) == NULL ){
/*	    DBG(("icv196:Clrsynchro: inconsistency at unbooking \n"));*/
	  }
	  L_LCtxt = L_LHdl -> LineCtxt;
	  disable(ps);
	  if (L_LHdl -> SubscriberCurNb == 0 )
	      disable_Line(L_LCtxt);

	  restore(ps);
	}/* if line connected*/
      } /* end for/ byte bit scanning */
    }
  } /* end for/ Cmap byte scanning  */
  sreset(&(UHdl -> Ring.Evtsem)); /* clear semaphore */

  for ( (i = 0, L_cptr = &(UHdl -> Cmap[0]) ) ; i < ICV_mapByteSz; i++)
    *L_cptr++ = 0;

}

/*			Device dependent routines
			=========================
*/

/*  Init_LineCtxt:
    ++++++++++++++
    subroutines to initialise a line context table of a icv196 module
    table to manage the hardware of a physical line of a module
*/
static void Init_LineCtxt(int line, int type, struct T_ModuleCtxt *MCtxt)
{
  struct T_LineCtxt *L_LCtxt;
  int LogIndex,m;

  L_LCtxt = &(MCtxt -> LineCtxt[line]);
  m = MCtxt -> Module;
  LogIndex =  CnvrtModuleLine(m,line);
  L_LCtxt -> s = MCtxt -> s;
  L_LCtxt -> MCtxt = MCtxt;
  L_LCtxt -> Line = line;
  L_LCtxt -> Type = type;
  L_LCtxt -> intmod = icv_ReenableOn;
  L_LCtxt -> status = icv_Disabled;
  L_LCtxt -> loc_count = -1;

/*  CHK (("icv:LCtxt:s= $%lx Module %d Line %d Line ctxt add = $%lx, LogIndex %d\n",L_LCtxt -> s ,m, line, L_LCtxt, LogIndex ));*/

		/* Link physical line to logical line and vise versa */

  L_LCtxt -> LHdl  = Init_LineHdl(LogIndex, L_LCtxt);

		/* Set up device dependent info */

}



/*  Init_ModuleCtxt:
    ++++++++++++++++
    Subroutine to initialise a module context for a icv196 module
    table to manage the hardware of a physical module
*/
static struct T_ModuleCtxt *Init_ModuleCtxt(struct icv196T_s *s,
					    short module,
					    struct icv196T_ModuleParam *vmeinfo)
{
  int   i;
  short n;
  unsigned long L_base, L_offset, L_sysBase, L_moduleSysBase;
  struct T_ModuleCtxt *L_MCtxt;
  int L_type;
  int L_am;
  struct pdparam_master param; /* CES: structure containing VME access parameters */

			/* declare module in the Module directory */
  L_MCtxt = &(s -> ModuleCtxt[module]);

			/* init context table */
  L_MCtxt -> s = s;
  L_MCtxt -> length = sizeof(struct T_ModuleCtxt);
  L_MCtxt -> sem_module = 1;	/* semaphore for exclusive access */
  L_MCtxt -> Module = module;
  L_MCtxt -> dflag = 0;
  L_MCtxt -> LineMxNb = LinePerModule;

/*  CHK (("icv:MCtxt: s= $%lx Module %d ctxt add = $%lx\n", L_MCtxt ->s, module, L_MCtxt ));*/

		/*  set up device dependent info */

/* Set up base add of module as seen from cpu mapping.
   This information is available to user through ioctl function
   it provides the Module base add required by  smemcreate to create a virtual
   window on the vme space of the module
   this is for accessing the module directly from user program access
   through a window created by smem_create.
*/

  L_base = vmeinfo -> base;

  L_MCtxt -> VME_offset =  L_base;
  L_MCtxt -> VME_size =     vmeinfo -> size;

  L_offset = L_base & (unsigned long)0x00ffffffL;
  L_am = AM_A24_UDA;

		/* Compute address in system space */
  bzero((char *)&param, sizeof(struct pdparam_master));
  /* CES: build an address window (64 kbyte) for VME A16D16 accesses */
  param.iack   = 1;	/* no iack */
  param.rdpref = 0;	/* no VME read prefetch option */
  param.wrpost = 0;	/* no VME write posting option */
  param.swap   = 1;	/* VME auto swap option */
  param.dum[0]   = 0;	/* window is sharable */
  L_sysBase = (unsigned long)find_controller( 0x0, 0x10000, L_am, 0, 0, &param);
  if (L_sysBase == (unsigned long)(-1)){
    CPRNT(("icv196vme_drvr:find_controller:cannot map device address space, INSTALLATION IMPOSSIBLE\n"));
    return ((struct T_ModuleCtxt *) (-1));
  }

  L_moduleSysBase = L_sysBase + L_offset;
  DBG_INSTAL(("icv196vme:install: am 0x%x Standard,  0x%lx base + 0x%lx"
	      "offset => 0x%lx module sys base\n",  L_am, L_sysBase, L_offset, L_moduleSysBase));
/*
                     Set up permanent pointers to access the module
		     from system mapping to be used from the driver
*/
  L_MCtxt -> SYSVME_Add = (short *) (L_moduleSysBase);

  L_MCtxt -> VME_StatusCtrl =   (unsigned char *)(L_moduleSysBase + CoReg_Z8536);
  L_MCtxt -> VME_IntLvl =       (unsigned char *)(L_moduleSysBase + CSNIT_ICV);
  L_MCtxt -> VME_CsDir =        (short *)(L_moduleSysBase + CSDIR_ICV);

		/* Initialise the associated line contexts */
  L_type = 0; /* to select default line type  */
  n = L_MCtxt -> LineMxNb;
  for (i = 0 ; i < n; i++) {
    L_MCtxt -> Vect[i] = vmeinfo -> vector[0];
    L_MCtxt -> Lvl[i]  = vmeinfo -> level[0];
    Init_LineCtxt(i,L_type, L_MCtxt);
  }
  L_MCtxt -> isr[0] = ModuleIsr[module];
  return (L_MCtxt);

}


/*  icvModule_Init_HW:
    ++++++++++++++++++
    Initializes the hardware of a icv196 module.  Upon the initialization,
    all interrupt lines are in the disabled state.
*/

int icvModule_Init_HW(struct T_ModuleCtxt *MCtxt)
{
  unsigned char L_v, L_l, dummy, *L_CtrStat;
  int m, ps;

  m =         MCtxt -> Module;
  L_v =       MCtxt -> Vect[0];
  L_l =       MCtxt -> Lvl[0];
  L_CtrStat = MCtxt -> VME_StatusCtrl;


		  /* To catch bus error if module does not answer */
  if (recoset()){    /* To prevent system crash in case of bus error*/
    cprintf  ("icv196 install: on Module %d VME offset$%lx,"
	      " CTrStat address = $%x bus error occured !!!...\n", m, MCtxt -> VME_offset, (uint)L_CtrStat );
    cprintf("icv196:install: Module %d INSTALLATION IMPOSSIBLE\n", m);
    return (-1);
  }

		/* reset hardware of icv196vme module */
  disable(ps);

  dummy = *L_CtrStat;                   /* force the board to state 0     */
  PURGE_CPUPIPELINE;
  ISYNC_CPUPIPELINE;

  *L_CtrStat = MIC_reg;
  PURGE_CPUPIPELINE;
  dummy = *L_CtrStat;
  PURGE_CPUPIPELINE;
  *L_CtrStat = MIC_reg;
  PURGE_CPUPIPELINE;
  *L_CtrStat = b_RESET;                 /* reset the board                */
  PURGE_CPUPIPELINE;
  *L_CtrStat = 0;                    /*go to state 0 (normal operation)*/
  PURGE_CPUPIPELINE;

  *(MCtxt -> VME_IntLvl)  = ~(L_l);/* set interrupt level for this module */
  PURGE_CPUPIPELINE;
  *(MCtxt -> VME_CsDir)  =  0;     /* set all I/O ports to input          */
  PURGE_CPUPIPELINE;

  MCtxt -> old_CsDir = 0;
  MCtxt -> startflag = 0;
  MCtxt -> int_en_mask = 0;             /* clear interrupt enable mask    */

	/*  set up hardware for portA and portB on the icv module  */

  *L_CtrStat = MCC_reg;
  PURGE_CPUPIPELINE;
  *L_CtrStat = (PortA_Enable | PortB_Enable);
  PURGE_CPUPIPELINE;
  /*(portA and portB operate independently)*/

  *L_CtrStat = MSpec_Areg;             /*portA: bitport */
  PURGE_CPUPIPELINE;
  *L_CtrStat = (PtrM_Or | Latch_On_Pat_Match);
  PURGE_CPUPIPELINE;

  *L_CtrStat = MSpec_Breg;             /*portB: bitport */
  PURGE_CPUPIPELINE;
  *L_CtrStat = (PtrM_Or | Latch_On_Pat_Match);
  PURGE_CPUPIPELINE;

  *L_CtrStat = CSt_Areg;               /*portA: no interrupt on error */
  PURGE_CPUPIPELINE;
  *L_CtrStat = CoSt_SeIe;              /*interrupts enabled           */
  PURGE_CPUPIPELINE;

  *L_CtrStat = CSt_Breg;               /*portB: no interrupt on error */
  PURGE_CPUPIPELINE;
  *L_CtrStat = CoSt_SeIe;              /*interrupts enabled           */
  PURGE_CPUPIPELINE;

  *L_CtrStat = DPPol_Areg;             /*portA: non inverting*/
  PURGE_CPUPIPELINE;
  *L_CtrStat = Non_Invert;
  PURGE_CPUPIPELINE;

  *L_CtrStat = DPPol_Breg;             /*portB: non inverting*/
  PURGE_CPUPIPELINE;
  *L_CtrStat = Non_Invert;
  PURGE_CPUPIPELINE;

  *L_CtrStat = DDir_Areg;              /*portA: input port*/
  PURGE_CPUPIPELINE;
  *L_CtrStat = All_Input;
  PURGE_CPUPIPELINE;

  *L_CtrStat = DDir_Breg;              /*portB: input port*/
  PURGE_CPUPIPELINE;
  *L_CtrStat = All_Input;
  PURGE_CPUPIPELINE;

  *L_CtrStat = SIO_Areg;               /*portA: normal input*/
  PURGE_CPUPIPELINE;
  *L_CtrStat = Norm_Inp;
  PURGE_CPUPIPELINE;

  *L_CtrStat = SIO_Breg;               /*portB: normal input*/
  PURGE_CPUPIPELINE;
  *L_CtrStat = Norm_Inp;
  PURGE_CPUPIPELINE;

  *L_CtrStat = PtrMsk_Areg;            /*portA: masked off*/
  PURGE_CPUPIPELINE;
  *L_CtrStat = All_Masked;
  PURGE_CPUPIPELINE;

  *L_CtrStat = PtrTr_Areg;
  PURGE_CPUPIPELINE;
  *L_CtrStat = 0;
  PURGE_CPUPIPELINE;

  *L_CtrStat = PtrPo_Areg;
  PURGE_CPUPIPELINE;
  *L_CtrStat = 0;
  PURGE_CPUPIPELINE;

  *L_CtrStat = PtrMsk_Breg;            /*portB: masked off*/
  PURGE_CPUPIPELINE;
  *L_CtrStat = All_Masked;
  PURGE_CPUPIPELINE;

  *L_CtrStat = PtrTr_Breg;
  PURGE_CPUPIPELINE;
  *L_CtrStat = 0;
  PURGE_CPUPIPELINE;

  *L_CtrStat = PtrPo_Breg;
  PURGE_CPUPIPELINE;
  *L_CtrStat = 0;
  PURGE_CPUPIPELINE;

  /* write interrupt vectors for portA and portB */

  *L_CtrStat = ItVct_Areg;
  PURGE_CPUPIPELINE;
  *L_CtrStat = L_v;
  PURGE_CPUPIPELINE;
  *L_CtrStat = ItVct_Breg;
  PURGE_CPUPIPELINE;
  *L_CtrStat = L_v;
  PURGE_CPUPIPELINE;


  *L_CtrStat = MIC_reg; /* no status in interrupt vector */
  PURGE_CPUPIPELINE;
  *L_CtrStat = b_MIE;          /*master interrupt enable */
  PURGE_CPUPIPELINE;

  restore(ps);

  /* DBG (("icv196vmedrvr: install: hardware set up finished \n"));*/
  /* DBG (("icv196vme installation terminated, driver ready to work \n"));*/
  noreco();                     /* remove local bus trap */
  return (0);
}

/*  icvModule_Startup:
    ++++++++++++++++++
    subroutines to startup a icv196 module
    This routine performs the hardware set up of the device and connects it to
    the system interrupt processing, this actually start up the driver for this
    device.
    The devices are served by the same Lynx OS driver, the isr recognizes the
    line by inspecting the Control register if the associated module.
    Each Module has its own context, the different isr if there are several
    different level are working in the local stack, no  jam
    should occurr !!!.
*/
int icvModule_Startup(struct T_ModuleCtxt *MCtxt)
{
    unsigned char        L_v;
    struct icv196T_s     *s;
    int                  m,
			 cc;

    s =         MCtxt -> s;
    m =         MCtxt -> Module;
    L_v =       MCtxt -> Vect[0];

/*    CHK (("icv:Setup: s =$%lx, Module %d ctxt add = $%lx\n", s, m, MCtxt ));*/

    /* Now connect interrupt to system */
/*    DBG (("icv:Setup: module %d vector %d level %d \n",m,L_v, L_l ));*/

    /* Connect interrupt from module  */
    IOINTSET(cc, L_v, (int (*)(void *))MCtxt->isr[0], (char *)MCtxt);
    if (cc <0){
	cprintf("icv196:install: iointset for  vector=%d error%d\n", L_v,cc);
	pseterr(EFAULT);
	return (SYSERR);
    }
    DBG_INSTAL(("icv196vme_drvr:install: iointset for module %d, vector %d\n", m, L_v));
    cc = icvModule_Init_HW(MCtxt);
    if (cc == 0)
       return (0);
    else
       return (-1);
}

#ifndef X_NO_disable_Module
/*  disable_Module:
    ++++++++++++++
    protection routine
    must be called from service level before accessing hardware, in order
    to be protected from isr processing
    the enable must be done by calling enable_Module
*/
static void disable_Module(struct T_ModuleCtxt *MCtxt)
{
  int ps;
  unsigned int L_w1;
  unsigned char L_msk, status, *L_CtrStat;

  L_CtrStat = MCtxt -> VME_StatusCtrl;
  L_w1 = ~(b_MIE);    /* mask excludes the MIE bit (Master Interrupt Enable) */
  L_msk = (unsigned char) (L_w1 & 0xff);
  disable(ps);
  *L_CtrStat = MIC_reg;
    PURGE_CPUPIPELINE;
  status = *L_CtrStat;
    PURGE_CPUPIPELINE;
  status = status & L_msk;
    PURGE_CPUPIPELINE;
  *L_CtrStat = MIC_reg;
    PURGE_CPUPIPELINE;
  *L_CtrStat = status;
    PURGE_CPUPIPELINE;
  restore(ps);
}
#endif
#ifndef X_NO_enable_Module

/*  enable_Module:
    +++++++++++++
    to enable interrupt again after disable,
    called from service level
*/
static void enable_Module(struct T_ModuleCtxt *MCtxt)
{
  int ps;
  unsigned char L_msk, status, *L_CtrStat;

  L_CtrStat = MCtxt -> VME_StatusCtrl;
  L_msk = b_MIE;    /* mask includes the MIE bit (Master Interrupt Enable) */
  disable(ps);
  *L_CtrStat = MIC_reg;
    PURGE_CPUPIPELINE;
  status = *L_CtrStat;
    PURGE_CPUPIPELINE;
  status = status | L_msk;
    PURGE_CPUPIPELINE;
  *L_CtrStat = MIC_reg;
    PURGE_CPUPIPELINE;
  *L_CtrStat = status;
    PURGE_CPUPIPELINE;
  restore(ps);
}
#endif

/*  icvModule_Reinit:
    ++++++++++++++++
    Check if module lost its status, reinitialise it if yes
    to face a  reset without system crash
*/

int icvModule_Reinit(struct T_ModuleCtxt *MCtxt, int line)
{
  struct icv196T_s     *s;
  struct T_LineCtxt *L_LCtxt;
  unsigned char L_v, L_l;
  int  i, m, lx, cc;
  unsigned char L_bw1, *L_CtrStat;

  s = MCtxt -> s;
  L_LCtxt = &(MCtxt -> LineCtxt[line]);

  m = MCtxt -> Module;
  L_v = MCtxt -> Vect[line];

  L_l = MCtxt -> Lvl[line];
  L_CtrStat = MCtxt -> VME_StatusCtrl;

  *L_CtrStat = MIC_reg;             /* master interrupt disable, permits    */
    PURGE_CPUPIPELINE;
  *L_CtrStat = 0;                   /* reading interrupt vector             */
    PURGE_CPUPIPELINE;

  *L_CtrStat = ItVct_Areg;
    PURGE_CPUPIPELINE;
  L_bw1 = (unsigned char)*L_CtrStat;/* read interrupt vector for this module*/
    PURGE_CPUPIPELINE;

  *L_CtrStat = MIC_reg;
    PURGE_CPUPIPELINE;
  *L_CtrStat = b_MIE;               /*master interrupt enable */
    PURGE_CPUPIPELINE;

		/* Check if setting still well recorded  */
  if (L_bw1 != L_v)
  {
    /* The module lost its initialisation: redo it */

/*  DBG (("icv: Reinit Module %d line %d !!! corrupted vect= %d instead of %d\n", m, line, L_bw1, L_v )); */

    lx = MCtxt -> LineMxNb;
    for (i=0; i < lx ; i++){
      L_LCtxt = &(MCtxt -> LineCtxt[i]);
      L_LCtxt -> Reset = 1;	/* To manage reenable of line at connect */
    }
    /* reset hardware of icv196vme module */
    cc = icvModule_Init_HW(MCtxt);

				/* %%%(94/03/15 A.G. */
    for (i=0; i < lx ; i++){
      L_LCtxt = &(MCtxt -> LineCtxt[i]);
      if (L_LCtxt -> LHdl -> SubscriberCurNb){
	enable_Line(L_LCtxt);
    cprintf ("icv196vmedrvr: Reinit: in module %d, reanable the active line %d\n", m, L_LCtxt -> Line);
      }
      L_LCtxt -> Reset = 1;	/* To manage reenable of line at connect */
    }
				/* )%%%  */
    if (cc == 0)
       return (0);
    else
       return (-1);
  }
  return(0);
}


/*
 This routine is called at driver install
*/
char *icv196install(struct icv196T_ConfigInfo *info)
{
  register struct icv196T_s  *s; /* static table pointer */
  struct icv196T_ModuleParam *L_MInfo;
  struct T_ModuleCtxt      *L_MCtxt;
  unsigned char L_l;
  ushort L_v;
  long L_base;
  int   i, m;

  G_dbgflag = 0;
  compile_date[11] = 0;		/* overwrite LF by 0 = end of string */
  compile_time [9] = 0;
  Version[strlen(Version)-1]=0;

  cprintf (":V%s %s%s",&Version[11], compile_date, compile_time);
		/* allocate static table */
  s = (struct icv196T_s *) sysbrk ((long) sizeof (struct icv196T_s));
  if (!s) {
    cprintf("\nicv:install: no room for static table, INSTALLATION IMPOSSIBLE\n");
      return ((char *) SYSERR);
  }

/*			 Set up the static table of driver
			 =================================
*/
  s -> sem_drvr = 1;	/* to protect global ressources management sequences*/
  s -> UserTO = 6000;   /* default T.O. for waiting Trigger */
  s -> UserMode = 0;    /* Flag no wait on read empty Evt Ring */

  s -> UserMode |= ((short)icv_bitwait); /* set flag wait for read = wait */

			/* Initialise user'shandle  */
		/* handles to get synchronized with the icv */
  for (i = ICVVME_IcvChan01; i < ICVVME_MaxChan; i++){
    Init_UserHdl(&(s -> ICVHdl[i]), i, s);
  }
			/* Initialize management tables  */
  Init_Dir(s);			/* Initialize Directories */

/*
		Set up of table associated to devices:
		-------------------------------------
  this depends on the info table which tell which modules are declared
*/

  /* for each module declared in info table: Set up tables */

  for ( m=0 ; m < icv_ModuleNb; m++) {

 		/* Set up pointer to current device table  */

    if (info -> ModuleFlag[m] == 0) /* this module non present; */
      continue;

    L_MInfo = &(info -> ModuleInfo[m]); /* set up the current Module info */

/*	Set up of the current module tables
	-----------------------------------
*/
		/*  Check info parameters */

    L_base = L_MInfo -> base;
		/* check if base address conform to hardware strapping */
    if (   ((L_base & 0xff000000) != 0)
	|| ((L_base & 0x000000ff) != 0)
       ){
      cprintf ("\n");
      cprintf ("icv196:install: base address out of range !.. %lx \n",L_base);
      cprintf("icv196:install: INSTALLATION IMPOSSIBLE\n");
      pseterr(EFAULT);
      return ((char *) SYSERR);
    };
	/*  Only one vector and one level for the 8 lines of the module */
    L_v = L_MInfo -> vector[0];

    if ((L_v < USER_VBASE) || (L_v > 255)) { /* USER_VBASE from interrupt.h */
      cprintf ("\n");
      cprintf ("icv196vmeinstall: vector value out of range !.. %d \n",L_v);
      cprintf("icv196:install: INSTALLATION IMPOSSIBLE\n");
      pseterr(EFAULT);
      return ((char *) SYSERR);
    }
    L_l = L_MInfo -> level[0];

    if ((L_l >5) || (L_l <1)){
      cprintf ("\n");
      cprintf ("icv196vmeinstall: Int. level out of range must be in [2..5] !.. %d \n",L_l);
      cprintf("icv196:install: INSTALLATION IMPOSSIBLE\n");
      pseterr(EFAULT);
      return ((char *) SYSERR);
    }
				/*  */
		/* Set up the tables of the  current Module */

    L_MCtxt = Init_ModuleCtxt(s, m, L_MInfo);

		/*  Update Module directory */
    s -> ModuleCtxtDir[m] = L_MCtxt;

  }

/*
		Now setup devices
		+++++++++++++++++++
   driven by the device directory  which tell which modules have been declared
*/

  for ( m=0 ; m < icv_ModuleNb; m++) {

    if ( (L_MCtxt = s -> ModuleCtxtDir[m]) == NULL){
      continue;
    }
		/* Startup the hardware for that module context */

    if (icvModule_Startup(L_MCtxt) < 0)
      L_MCtxt = s -> ModuleCtxtDir[m] = 0;
  }
  cprintf("\rDrivOK:\n");
  return ((char *) s);
}

/*
  ______________________________
 |                              |
 |  icv196uninstall()           |
 |                              |
 |  install service subroutine  |
 |  of icv196 driver            |
 |______________________________|

 This routine is called to uninstall the driver
*/

int icv196uninstall(struct icv196T_s *s)
{
  struct T_ModuleCtxt      *L_MCtxt;
  unsigned char L_v;
  int m;

  for ( m=0 ; m < icv_ModuleNb; m++) {

    if ( (L_MCtxt = s -> ModuleCtxtDir[m]) == NULL){
      continue;
    }
    L_v = L_MCtxt -> Vect[0];

    IOINTCLR(L_v);
  }
  sysfree ((char *)s, (long) sizeof (s));
  return (OK);
}

/*
  ______________________________
 |                              |
 |  icv196open()                |
 |                              |
 |  open    service subroutine  |
 |  of icv196 driver            |
 |______________________________|

 This routine is called to open a service access point for the user

*/
int icv196open(struct icv196T_s *s, int dev, struct file *f)
{
  int   L_chan;
  short   L_DevType;
  register struct T_UserHdl *L_UHdl;
  /*  */

/*  DBG (("icvvme: Open on device= %lx \n", (long) dev)); */
  L_chan = minor (dev);
/*  DBG (("icv196:Open: on minor device= %d \n", (int) L_chan)); */

  if ( (s == (struct icv196T_s *) NULL) || ( s == (struct icv196T_s *)(-1))){
/*    DBG (("icvvme:Install wrong: static pointer is = %lx \n", (int) s));*/
    pseterr (EACCES);
    return (SYSERR);
  }

  L_UHdl = NULL;
  L_DevType = 0;
  if (L_chan == ICVVME_ServiceChan) {
    /* DBG (("icvvme:Open:Handle for icv services \n"));*/
    L_UHdl = &(s -> ServiceHdl);
    L_DevType = 1;
  }
  else{
    if ((L_chan >= ICVVME_IcvChan01) && (L_chan <= ICVVME_MaxChan)) {
      /* DBG (("icvvme:Open:Handle for synchro with ICV lines \n"));*/
      if (f -> access_mode & FWRITE) {
	pseterr (EACCES);
	return (SYSERR);
      }
      L_UHdl = &s -> ICVHdl[L_chan];
    }
    else {
      pseterr (EACCES);
      return (SYSERR);
    }
  }
  if ((L_DevType == 0) && (L_UHdl -> usercount)) { /* synchro channel already open */
    pseterr (EACCES);
    return (SYSERR);
  }
		/* Perform the open */

  swait(&(s -> sem_drvr), IGNORE_SIG);
  if (L_DevType == 0) {			/* Case synchro */
    Init_UserHdl (L_UHdl, L_chan, s);
    L_UHdl -> UserMode = s -> UserMode;
    L_UHdl -> WaitingTO = s -> UserTO;
    L_UHdl -> pid = getpid ();

    /* DBG (("icvvme:open: Chanel =%d UHdl=$%lx\n", L_chan, L_UHdl ));*/
  }
  ++(s -> usercounter);			/* Update user counter value */
  L_UHdl -> usercount++;
  ssignal( &(s -> sem_drvr) );
  return (OK);
}

/*
 This routine is called to close the devices access point
*/
int icv196close(struct icv196T_s *s, struct file *f)
{
  int   L_chan;
  short   L_DevType;
  register struct T_UserHdl *L_UHdl;

  L_chan = minor (f -> dev);
  L_UHdl = NULL;
  L_DevType = 0;
  if (L_chan == ICVVME_ServiceChan) {
/*   DBG (("icvvme:Close:Handle for icv services\n"));*/
    L_UHdl = &(s -> ServiceHdl);
    L_DevType = 1;
  }
  else {
    if ((L_chan >= ICVVME_IcvChan01) && (L_chan <= ICVVME_MaxChan)) {
    /* DBG (("icvvme:Close:Handle for synchro with Icv \n"));*/
      L_UHdl = &(s -> ICVHdl[L_chan]);
    }
    else {
      pseterr (EACCES);
      return (SYSERR);
    }
  }
		/* Perform the close */

  swait(&(s -> sem_drvr), IGNORE_SIG);
  if (L_DevType==0){		/* case of Synchro handle */
/*    DBG (("icv196:close: Chanel =%d UHdl=%lx \n",L_chan, L_UHdl));*/
    ClrSynchro(L_UHdl);		/* Clear connection established*/
  }
  --(s -> usercounter);			/* Update user counter value */
  --L_UHdl -> usercount;
  ssignal( &(s -> sem_drvr) );

  return (OK);
}


int icv196read(struct icv196T_s *s, struct file *f, char *buff, int bcount)
{
	int   L_count, L_Chan;
  struct T_UserHdl  *L_UHdl;
  long   *L_buffEvt;
  struct T_ModuleCtxt *L_MCtxt;
  int   i,m;
  long  L_Evt;
  int L_bn;
  int ps;
  int L_Line;

  /* Check parameters and Set up channel environnement */
  if (wbounds ((int)buff) == EFAULT) {
    /* DBG (("icv:read wbound error \n" ));*/
    pseterr (EFAULT);
    return (SYSERR);
  }
  L_Chan = minordev (f -> dev);
  if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
    L_UHdl = &(s -> ICVHdl[L_Chan]);	/* ICV event handle */
  }
  else {
    pseterr (ENODEV);
    return (SYSERR);
  }
  if (L_UHdl == NULL) {	   /* Abnormal status system corruption	   !!!! */

    pseterr (EWOULDBLOCK);
    return (SYSERR);
  }

	/* processing of the read  */

/*			Case of  Reading the event buffer
			================================
*/

  /* Event are long and so count should be multiple of 4 bytes */
    if ((L_count = (bcount >> 2)) <= 0) {
      pseterr (EINVAL);
      return (SYSERR);
    }

/*           BEWARE !!!!
     alignment constraint for the buffer
      should be satisfied by caller  !!!!!
*/
    L_buffEvt = (long *)buff;
    L_bn = 0;
    i = L_count;			/* Long word count */
    /* DBG (("icv196:read:Entry Evtsem value= %d\n", scount( &L_UHdl -> Ring.Evtsem) )); */
    do {
      if ((L_Evt= PullFrom_Ring ( &(L_UHdl -> Ring))) != (-1)){

	*L_buffEvt = L_Evt;
	L_buffEvt++; i--; L_bn +=1;
      }
      else {			/* no event in ring, wait if requested */
	if ((L_bn==0) && ((L_UHdl -> UserMode & icv_bitwait ))) { /* Wait for Lam */
	/* Contro waiting by t.o. */
	  L_UHdl -> timid = 0;

	  L_UHdl -> timid = timeout((int (*)())UserWakeup, (char *)L_UHdl, (int)L_UHdl -> WaitingTO);
	  swait (&L_UHdl -> Ring.Evtsem, -1);	/* Wait Lam or Time Out */

	  disable(ps);
	  if (L_UHdl -> timid < 0 ) {	/* time Out occured */
	    restore(ps);

		/* check if any module setting got reset */
	    for ( m=0 ; m < icv_ModuleNb; m++) {
		/*  Update Module directory */
	      L_MCtxt = s -> ModuleCtxtDir[m];
	      if ( L_MCtxt != NULL) {
		L_Line=0;
		icvModule_Reinit(L_MCtxt, L_Line);
	      }
	    }

	    pseterr(ETIMEDOUT);
	    return(SYSERR);		             /* RETURN ==>  */
	  }
	  cancel_timeout(L_UHdl->timid);
	  L_UHdl -> Ring.Evtsem += 1;	/* to remind this event for swait by reading */

	  restore(ps);
	}
	else {
	  break;
	}
      }
    } while (i > 0);		/* while  room in buffer */
    return ((L_bn) << 2);

 /* end of reading event*/
}

/*
  ______________________________
 |                              |
 |  icv196write()               |
 |                              |
 |                              |
 |  No such facility available  |
 |______________________________|

*/

int icv196write(struct icv196T_s *s, struct file *f, char *buff,int bcount)
{
/*   DBG (("icvdrvr: write: no such facility available \n"));*/
  return (bcount = s -> usercounter);
}


/*
  ______________________________
 |                              |
 |  icv196ioctl()               |
 |                              |
 |  ioctl   service subroutine  |
 |  of icv196 driver            |
 |______________________________|

 This routine is called to perform ioctl function

*/
int icv196ioctl(struct icv196T_s *s, struct file *f, int fct, char *arg)
{
  int   L_err;
  struct icv196T_ModuleInfo *Infop;
  register int  Timeout;
  register int  i,j,l;
  int   L_Chan;
  short L_group;
  short L_index;
  int   L_mode;
  int   L_SubsMode;
  struct T_UserHdl  *L_UHdl;
  struct T_ModuleCtxt *L_MCtxt;
  struct T_LineCtxt   *L_LCtxt;
  struct T_LogLineHdl *L_LHdl;
  struct T_Subscriber *L_Subs;
  struct icv196T_HandleInfo *L_HanInfo;
  struct icv196T_HandleLines *L_HanLin;
  struct icv196T_UserLine L_ULine;
  long   L_Module;
  short  L_Line;
  int    L_Type;
  int    L_LogIx;
  int    L_grp;
  int    L_dir;
  int    L_group_mask;
  unsigned long *L_Data;
  int Iw1;
  int L_Flag;

  L_err = 0;
  L_UHdl = NULL;
  DBG_IOCTL(("icv196:ioctl: function code = %lx \n", fct));


  if ( (rbounds((int)arg) == EFAULT) || (wbounds((int)arg) == EFAULT)) {
    pseterr (EFAULT);
    return (SYSERR);
  }
  L_Chan = minordev (f -> dev);

  switch (fct) {

  /* get device information */

  case ICVVME_getmoduleinfo:

    Infop = (struct icv196T_ModuleInfo *)arg;
    for ( i = 0; i < icv_ModuleNb; i++, Infop++) {
	if (s -> ModuleCtxtDir[i] == NULL) {
	    Infop -> ModuleFlag = 0;
	    continue;
	}
	else {
	    L_MCtxt = &(s -> ModuleCtxt[i]);
	    Infop -> ModuleFlag = 1;
	    Infop -> ModuleInfo.base = (unsigned long)L_MCtxt -> SYSVME_Add;
	    Infop -> ModuleInfo.size = L_MCtxt -> VME_size;
	    for ( j = 0; j < icv_LineNb; j++) {
		Infop -> ModuleInfo.vector[j] = L_MCtxt -> Vect[j];
		Infop -> ModuleInfo.level[j] = L_MCtxt -> Lvl[j];
	    }
	}
    };
    break;



  case ICVVME_gethandleinfo:

    L_HanInfo = (struct icv196T_HandleInfo *)arg;
    L_UHdl = s -> ICVHdl;           /* point to first user handle */
    for ( i = 0; i < ICVVME_MaxChan; i++, L_UHdl++) {
	L_HanLin = &(L_HanInfo -> handle[i]);
	l = 0;
	if (L_UHdl -> usercount == 0)
	    continue;
	else {
	    L_HanLin -> pid = L_UHdl -> pid;
	    for ( j = 0; j < ICV_LogLineNb; j++) {
		if (TESTBIT(L_UHdl -> Cmap, j)) {
		    GetUserLine(j, &L_ULine);
		    L_HanLin -> lines[l].group = L_ULine.group;
		    L_HanLin -> lines[l].index = L_ULine.index;
		    l++;
		}
	    }
	}
    };
    break;


/*
				ioctl reset function
				====================
   perform hardware reset of icv196 module
*/
  case ICVVME_reset:

      break;

/*                              ioctl icvvme dynamic debug flag control
                                =======================================
    Toggle function on debug flag
*/
  case ICVVME_setDbgFlag:
    L_Flag = G_dbgflag;
    if ( *( (int *) arg) != (-1)){ /* update the flag if value  not = (-1) */
      G_dbgflag = *( (int *) arg);
    }
    *( (int *) arg) = L_Flag;	/* give back old value */
    break;


/*                              ioctl icvvme set reenable flag
				==============================
    Set reenable flag to allow continuous interrupts
*/
  case ICVVME_setreenable:

      /* Check channel number and Set up Handle pointer */
    if (L_Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
      L_UHdl = &(s -> ICVHdl[L_Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv:clearreenable: Entry:UHdl= %lx \n", (long) L_UHdl));*/

    if (L_UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    L_group = ((struct icv196T_UserLine *) arg) -> group;
    L_index = ((struct icv196T_UserLine *) arg) -> index;

    if (s -> ModuleCtxtDir[L_group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((L_group < 0) || (L_group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",L_group, L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((L_index < 0) || (L_index > (icv_LineNb -1) )) {
/*      DBG (("icv:setreenable: line index= %d out of range on channel %d\n",L_index,  L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    L_LogIx = CnvrtUserLine( (char) L_group,(char) L_index );

    /* DBG (("icv:setreenable: request on group %d index %d, associated  log line %d\n",L_group, L_index, L_LogIx )); */

    if (L_LogIx < 0 || L_LogIx > ICV_LogLineNb){
/*      DBG (("icv:setreenable: LogIndex outside [0..%d] for channel %d\n", ICV_LogLineNb, L_Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    if ((L_LHdl = s -> LineHdlDir[L_LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv:setreenable: %d no such log line in config, for channel %d \n", L_LogIx, L_Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    L_LCtxt = L_LHdl -> LineCtxt;
    if ( (L_LCtxt -> LHdl) != L_LHdl){
/*      DBG (("icv:setreenable: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", L_LHdl, (L_LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    L_LCtxt -> intmod = icv_ReenableOn;
    break;



/*                              ioctl icvvme clear reenable flag
				==================================
    Clear reenable: after an interrupt, further interrupts are inhibited
		    until the line is enabled again
*/
  case ICVVME_clearreenable:

      /* Check channel number and Set up Handle pointer */
    if (L_Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
      L_UHdl = &(s -> ICVHdl[L_Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv:clearreenable: Entry:UHdl= %lx \n", (long) L_UHdl));*/

    if (L_UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    L_group = ((struct icv196T_UserLine *) arg) -> group;
    L_index = ((struct icv196T_UserLine *) arg) -> index;

    if (s -> ModuleCtxtDir[L_group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((L_group < 0) || (L_group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",L_group, L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((L_index < 0) || (L_index > (icv_LineNb -1) )) {
/*      DBG (("icv:clearreenable: line index= %d out of range on channel %d\n",L_index,  L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    L_LogIx = CnvrtUserLine( (char) L_group,(char) L_index );

    /* DBG (("icv:clearreenable: request on group %d index %d, associated  log line %d\n",L_group, L_index, L_LogIx )); */

    if (L_LogIx < 0 || L_LogIx > ICV_LogLineNb){
/*      DBG (("icv:clearreenable: LogIndex outside [0..%d] for chanel %d\n", ICV_LogLineNb, L_Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    if ((L_LHdl = s -> LineHdlDir[L_LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv:clearreenable: %d no such log line in config, for chanel %d \n", L_LogIx, L_Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    L_LCtxt = L_LHdl -> LineCtxt;
    if ( (L_LCtxt -> LHdl) != L_LHdl){
/*      DBG (("icv:clearreenable: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", L_LHdl, (L_LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    L_LCtxt -> intmod = icv_ReenableOff;
    break;


/*                              ioctl icvvme disable interrupt
				==============================
    interrupts on the given logical line are disabled
*/
  case ICVVME_disable:

      /* Check channel number and Set up Handle pointer */
    if (L_Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
      L_UHdl = &(s -> ICVHdl[L_Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv:disableint: Entry:UHdl= %lx \n", (long) L_UHdl));*/

    if (L_UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    L_group = ((struct icv196T_UserLine *) arg) -> group;
    L_index = ((struct icv196T_UserLine *) arg) -> index;

    if (s -> ModuleCtxtDir[L_group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((L_group < 0) || (L_group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",L_group, L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((L_index < 0) || (L_index > (icv_LineNb -1) )) {
/*      DBG (("icv:disableint: line index= %d out of range on channel %d\n",L_index,  L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    L_LogIx = CnvrtUserLine( (char) L_group,(char) L_index );

    /* DBG (("icv:disableint: request on group %d index %d, associated  log line %d\n",L_group, L_index, L_LogIx )); */

    if (L_LogIx < 0 || L_LogIx > ICV_LogLineNb){
/*      DBG (("icv:disableint: LogIndex outside [0..%d] for chanel %d\n", ICV_LogLineNb, L_Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    if ((L_LHdl = s -> LineHdlDir[L_LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv:disableint: %d no such log line in config, for chanel %d \n", L_LogIx, L_Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    L_LCtxt = L_LHdl -> LineCtxt;
    if ( (L_LCtxt -> LHdl) != L_LHdl){
/*      DBG (("icv:disableint: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", L_LHdl, (L_LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    disable_Line(L_LCtxt);
    break;


/*                              ioctl icvvme enable interrupt
				=============================
    interrupts on the given logical line are enabled
*/
  case ICVVME_enable:

      /* Check channel number and Set up Handle pointer */
    if (L_Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
      L_UHdl = &(s -> ICVHdl[L_Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv:disableint: Entry:UHdl= %lx \n", (long) L_UHdl));*/

    if (L_UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    L_group = ((struct icv196T_UserLine *) arg) -> group;
    L_index = ((struct icv196T_UserLine *) arg) -> index;

    if (s -> ModuleCtxtDir[L_group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((L_group < 0) || (L_group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",L_group, L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((L_index < 0) || (L_index > (icv_LineNb -1) )) {
/*      DBG (("icv:disableint: line index= %d out of range on channel %d\n",L_index,  L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    L_LogIx = CnvrtUserLine( (char) L_group,(char) L_index );

    /* DBG (("icv:disableint: request on group %d index %d, associated  log line %d\n",L_group, L_index, L_LogIx )); */

    if (L_LogIx < 0 || L_LogIx > ICV_LogLineNb){
/*      DBG (("icv:disableint: LogIndex outside [0..%d] for chanel %d\n", ICV_LogLineNb, L_Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    if ((L_LHdl = s -> LineHdlDir[L_LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv:disableint: %d no such log line in config, for chanel %d \n", L_LogIx, L_Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    L_LCtxt = L_LHdl -> LineCtxt;
    if ( (L_LCtxt -> LHdl) != L_LHdl){
/*      DBG (("icv:disableint: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", L_LHdl, (L_LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    enable_Line(L_LCtxt);
    break;



/*                              ioctl icvvme read interrupt counters
				====================================
    Read interrupt counters for all lines in the given module
*/
  case ICVVME_intcount:

    L_Module = (long)((struct icv196T_Service *) arg) -> module;
    L_Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[L_Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    L_MCtxt =  &(s -> ModuleCtxt[L_Module]);
    L_LCtxt =  L_MCtxt -> LineCtxt;
    for (i = 0; i < icv_LineNb; i++, L_LCtxt++)
      *L_Data++ = L_LCtxt -> loc_count;
    break;



/*                              ioctl icvvme read reenable flags
				================================
    Read reenable flags for all lines in the given module
*/
  case ICVVME_reenflags:

    L_Module = (long)((struct icv196T_Service *) arg) -> module;
    L_Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[L_Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    L_MCtxt =  &(s -> ModuleCtxt[L_Module]);
    L_LCtxt =  L_MCtxt -> LineCtxt;
    for (i = 0; i < icv_LineNb; i++, L_LCtxt++)
      *L_Data++ = L_LCtxt -> intmod;
    break;




/*                              ioctl icvvme read interrupt enable mask
				=======================================
    Read interrupt enable mask for the given module
*/
  case ICVVME_intenmask:

    L_Module = (long)((struct icv196T_Service *) arg) -> module;
    L_Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[L_Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    L_MCtxt =  &(s -> ModuleCtxt[L_Module]);
    *L_Data = L_MCtxt -> int_en_mask;
    break;




/*                              ioctl icvvme read io semaphores
				===============================
    Read io semaphores for all lines in the given module
*/
  case ICVVME_iosem:

    L_Module = (long)((struct icv196T_Service *) arg) -> module;
    L_Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[L_Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    L_MCtxt =  &(s -> ModuleCtxt[L_Module]);
    L_LCtxt =  &(L_MCtxt -> LineCtxt[0]);
    for (i = 0; i < icv_LineNb; i++, L_LCtxt++) {
	L_LHdl = L_LCtxt -> LHdl;
	L_Subs = &(L_LHdl -> Subscriber[0]);
	for (j = 0; j < L_LHdl -> SubscriberCurNb; j++, L_Subs++) {
	    if (L_Subs -> Ring != NULL) {
		if ((j = 0))
		    L_Data++;
		continue;
	    }
	    *L_Data++ = L_Subs -> Ring -> Evtsem;
	}
    }
    break;

/*                              ioctl icvvme set io direction
				===============================
	     ICVVME_setio: set direction of input/output ports.
	     (when using the I/O part of the ICV196 board)
*/
  case ICVVME_setio:

    L_Module = (long)((struct icv196T_Service *) arg) -> module;
    L_Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[L_Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    L_MCtxt =  &(s -> ModuleCtxt[L_Module]);
    L_grp = *L_Data++;
    L_dir = *L_Data;

		if ( L_grp < 0 || L_grp > 11)
		    return(SYSERR);

		L_group_mask = 1 << L_grp;
		if (L_dir) {            /* dir >< 0 : output */
		    *(L_MCtxt -> VME_CsDir) =
		    L_MCtxt -> old_CsDir | L_group_mask;
		    L_MCtxt -> old_CsDir =
		    L_MCtxt -> old_CsDir | L_group_mask;
		}
		else {                     /* dir = 0: input  */
		    *(L_MCtxt -> VME_CsDir) =
		    L_MCtxt -> old_CsDir & ~L_group_mask;
		    L_MCtxt -> old_CsDir =
		    L_MCtxt -> old_CsDir & ~L_group_mask;
		}
  break;


/*                              ioctl icvvme read io direction
				===============================
	     ICVVME_setio: read direction of input/output ports.
	     (when using the I/O part of the ICV196 board)
*/
  case ICVVME_readio:

    L_Module = (long)((struct icv196T_Service *) arg) -> module;
    L_Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[L_Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    L_MCtxt =  &(s -> ModuleCtxt[L_Module]);
    *L_Data = L_MCtxt -> old_CsDir;
  break;




/*                              ioctl wait/nowait flag control:  no wait
                                =========================================
   to set nowait mode on the read function
*/
  case ICVVME_nowait:

      /* Check channel number and Set up Handle pointer */
    if (L_Chan == 0) {		/* no such function on channel 0 */
      pseterr (EACCES);
      return (SYSERR);
    };
    if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
      L_UHdl = &(s -> ICVHdl[L_Chan]);
    }
    else {
      pseterr (ENODEV);
      return (SYSERR);
    }
    if (L_UHdl == NULL) {		/* system corruption */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }
    L_UHdl->UserMode &= (~((short) icv_bitwait)); /* reset flag wait */

/*     DBG (("icvdrvr:ioctl:reset wait flag UserMode now= %lx \n", L_UHdl -> UserMode ));*/

    break;
/*
                                ioctl wai/nowait flag control:     wait
                                ===========================================
   to set nowait mode on the read function
*/
  case ICVVME_wait:

      /* Check channel number and Set up Handle pointer */
    if (L_Chan == 0) {		/* no such function on channel 0 */
      pseterr (EACCES);
      return (SYSERR);
    };
    if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
      L_UHdl = &(s -> ICVHdl[L_Chan]);/* LAM handle */
    }
    else {
      pseterr (ENODEV);
      return (SYSERR);
    }
    if (L_UHdl == NULL) {		/* system corruption !!! ...*/
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }
    L_UHdl->UserMode |= ((short)icv_bitwait); /* set flag wait */

/*    DBG (("icv196:ioctl:set wait flag UserMode now= %lx \n", L_UHdl -> UserMode ));*/

    break;


/*
                        ioctl function: to set Time out for waiting Event
                        ================================================
   to monitor duration of wait on interrupt event before time out  declared
*/
  case ICVVME_setupTO:


      /* Check channel number and Set up Handle pointer */
      if (L_Chan == 0) {		/* no such function on channel 0 */
	pseterr (EACCES);
	return (SYSERR);
      };
    if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
      L_UHdl = &(s -> ICVHdl[L_Chan]);/* LAM handle */
    }
    else {
      pseterr (ENODEV);
      return (SYSERR);
    }
    if (L_UHdl == NULL) {		/* system corruption */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }
    if ( ( *( (int *) arg)) < 0 ){
      pseterr (EINVAL); return (SYSERR);
    }
    Timeout = L_UHdl -> WaitingTO; /* current value     */
    Iw1 = *( (int *) arg);	    /* new value         */
    L_UHdl -> WaitingTO =  Iw1;    /* set T.O.          */
    *( (int *) arg) = Timeout;    /* return old value  */

/*      DBG (("icv196:setTO: for chanel= %d oldTO= %d newTO= %d in 1/100 s\n",L_Chan, Timeout, Iw1 ));*/

    break;
/* ICVVME_setTO */

/*				connect function
				================
*/

  case ICVVME_connect:

    L_err = 0;
      /* Check channel number and Set up Handle pointer */
    if (L_Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
      L_UHdl = &(s -> ICVHdl[L_Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv196:connect: Entry:UHdl= %lx \n", (long) L_UHdl));*/

    if (L_UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    L_group = ((struct icv196T_connect *) arg) -> source.field.group;
    L_index = ((struct icv196T_connect *) arg) -> source.field.index;
    L_mode = ((struct icv196T_connect *) arg) -> mode;
   /* DBG (("icv196:connect: param = group = %d Line %d Mode %d \n", L_group, L_index, L_mode));   */
    if (L_mode & (~(icv_cumul | icv_disable))){
/*      DBG (("icv:connect: mode out of range on channel %d\n", L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }

    if (s -> ModuleCtxtDir[L_group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((L_group < 0) || (L_group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",L_group, L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((L_index < 0) || (L_index > (icv_LineNb -1) )) {
/*      DBG (("icv:connect: line index= %d out of range on channel %d\n",L_index,  L_Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    L_LogIx = CnvrtUserLine( (char) L_group,(char) L_index );

    /* DBG (("icv196:connect: request on group %d index %d, associated  log line %d\n",L_group, L_index, L_LogIx )); */

    if (L_LogIx < 0 || L_LogIx > ICV_LogLineNb){
/*      DBG (("icv196:connect: LogIndex outside [0..%d] for chanel %d\n", ICV_LogLineNb, L_Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }
    if ((L_LHdl = s -> LineHdlDir[L_LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv196:connect: %d no such log line in config, for chanel %d \n", L_LogIx, L_Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    L_LCtxt = L_LHdl -> LineCtxt;
    if ( (L_LCtxt -> LHdl) != L_LHdl){
/*      DBG (("icv196:connect: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", L_LHdl, (L_LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }
/*  DBG (("icv196:connect: checking if connection already exists\n")); */
    if  ( TESTBIT(L_UHdl -> Cmap, L_LogIx) ) {
      if (L_LCtxt -> Reset) {
	enable_Line(L_LCtxt);
/*      DBG (("icv196:connect: line %d enabled\n", L_Chan));            */
      }
      L_err = 0;
/*      DBG (( "icv196:connect: already connected group = %d index = %d Logline =%d\n", L_group, L_index, L_LogIx));*/
      break;			/* already connected ok --->  exit */
    }
    L_SubsMode = 0;		/* default mode requested */
    if (L_mode & icv_cumul)
      L_SubsMode = icv_cumulative;

    if (L_mode & icv_disable)
      L_LCtxt -> intmod = icv_ReenableOff;

    L_MCtxt = L_LCtxt -> MCtxt;

    L_Type = L_LCtxt -> Type;
    L_Line = L_LCtxt -> Line;

       /* Reinit module if setting lost !!!!
	  beware !,  all lines loose their current state */

    icvModule_Reinit(L_MCtxt, L_Line);

    if ((L_Subs = LineBooking(L_UHdl, L_LHdl, L_SubsMode)) == NULL){
/*      DBG (("icv196:connect: booking unsuccessfull UHdl $%lx LHdl $%lx\n", L_UHdl, L_LHdl ));*/
      pseterr (EUSERS); return (SYSERR);
    }

/*    DBG(("icv196 connect chanel= %d on group = %d, index = %d, mode %d \n", L_Chan, L_group, L_index, L_mode));*/


      /*  enable line interrupt */
    if (L_LCtxt -> status == icv_Disabled)
	enable_Line(L_LCtxt);
    L_LCtxt -> loc_count = 0;

    break;

/*  ICVVME_connect */

/*
				disconnect function
				===================
*/

  case ICVVME_disconnect:

    L_err = 0;
      /* Check channel number and Set up Handle pointer */
    if (L_Chan == 0) {		/* no disconnect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
      L_UHdl = &(s -> ICVHdl[L_Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    if (L_UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    };
                 /* Check parameters and Set up environnement */

    L_group = ((struct icv196T_connect *) arg) -> source.field.group;
    L_index = ((struct icv196T_connect *) arg) -> source.field.index;

    if (s -> ModuleCtxtDir[L_group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }


/*      DBG (("icv196:disconnect:chanel= %d from group = %lx, index = %lx \n", L_Chan,(long) L_group, (long) L_index));*/

    if ((L_group < 0) || (L_group >= icv_ModuleNb)) {
      pseterr (EINVAL); return (SYSERR);
    }
    if ((L_index < 0) || (L_index >= icv_LineNb)) {
      pseterr (EINVAL); return (SYSERR);
    }
    L_LogIx = CnvrtUserLine( (char) L_group, (char) L_index);
           /* set up Logical line handle pointer */
    if ((L_LHdl = s -> LineHdlDir[L_LogIx]) == NULL){ /*log line not affected*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ( !(TESTBIT(L_UHdl -> Cmap, L_LogIx)) ){ /* non connected */
      pseterr (ENOTCONN); return (SYSERR);
    }
    if ((L_Subs = LineUnBooking(L_UHdl, L_LHdl)) == NULL){
      pseterr (ENOTCONN); return (SYSERR);
    }
			/* Set up line context */
    L_LCtxt = L_LHdl -> LineCtxt;

    /* disable the corresponding line */
    if (L_LHdl -> SubscriberCurNb == 0){
      disable_Line(L_LCtxt);
      L_LCtxt -> loc_count = -1;
    }

    break;

/* ICVVME_disconnect */



/*			Ioctl function code out of range
			================================
*/
  default:
    pseterr (EINVAL);
    return (SYSERR);

  }
  return (L_err);
}


int icv196select(struct icv196T_s *s, struct file *f, int which, struct sel *se)
{
  int   L_Chan;
  register struct T_UserHdl *L_UHdl;

  L_Chan = minordev (f -> dev);
  if (L_Chan == 0) {			/* no select on channel 0 */
    pseterr (EACCES);
    return (SYSERR);
  };
  if (L_Chan >= ICVVME_IcvChan01 && L_Chan <= ICVVME_MaxChan) {
    L_UHdl = &s -> ICVHdl[L_Chan];	/* general handle */
  }
/*  DBG (("icv196:select:Entry: on channel %lx which = %lx \n", L_Chan, which ));*/
  switch (which) {
    case SREAD:
      se -> iosem = &(L_UHdl -> Ring.Evtsem);
      se -> sel_sem = &(L_UHdl -> sel_sem);
      break;
    case SWRITE:
      pseterr (EACCES);
      return (SYSERR);
      break;
    case SEXCEPT:
      pseterr (EACCES);
      return (SYSERR);
      break;
    }
  return (OK);
}

/*
  There is one entry for each of the module present in the actual configuration
*/
/*
  interrupt service routine
  of the icv196 module driver
  work on the module context which start the corresponding isr
*/
static int icv196vmeisr(void *arg)
{
    struct icv196T_s *s;
    int m;
    struct T_UserHdl    *L_UHdl;
    struct T_LineCtxt   *L_LCtxt;
    struct T_LogLineHdl *L_LHdl;
    struct T_Subscriber *L_Subs;
    struct icvT_RingAtom L_Atom;
    struct icvT_RingAtom *L_RingAtom;
    int i,j, ns, cs;
    short L_count;
    unsigned short L_Sw1;
    unsigned char     *L_CtrStat;
    unsigned char   mdev, mask, status;
    static unsigned short input[16];
    struct T_ModuleCtxt *MCtxt = (struct T_ModuleCtxt *)arg;

    s = MCtxt -> s;                       /* common static area */
    m = MCtxt -> Module;
    L_CtrStat = MCtxt -> VME_StatusCtrl;
    /*DBX (("icv196:isr: s=$%lx MCtxt= $%lx MCtxt.Module= =$%d \n", s, (long)MCtxt,(long)m ));*/

    if ((m <0) || ( m > icv_ModuleNb)){
/*	DBG(("icv:isr: Module context pointer mismatch  \n"));*/
	return;
    }

    if ( !(s -> ModuleCtxtDir[m]) ){
/*	DBG (( "icv196:isr: Interrupt from a non declared module \n"));*/
	return;
    }

    if (MCtxt -> startflag == 0) {
	for (i = 0; i <= 15; i++) input[i] = 0;
	MCtxt -> startflag = 1;/*reset the input array the first */
    }                              /*time the routine is entered     */

    status = *L_CtrStat;        /*force board to defined state    */
      PURGE_CPUPIPELINE;
    *L_CtrStat = CSt_Areg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;        /*read portA's status register    */
      PURGE_CPUPIPELINE;
    /*DBX(("CSt_Areg = 0x%x\n", status));*/

    if (status & CoSt_Ius) {     /*did portA cause the interrupt ? */

	mask = 1;
	*L_CtrStat = Data_Areg;
          PURGE_CPUPIPELINE;
	mdev = *L_CtrStat;      /*read portA's data register      */
          PURGE_CPUPIPELINE;
	/*DBG(("Icv196isr:Data_Areg = 0x%x\n", mdev));*/

	for (i = 0; i <= 7; i++) {

	    if (mdev & mask & MCtxt -> int_en_mask)
		input[i] = 1;   /* mark port A's active interrupt  */
	    mask <<= 1;         /* lines                           */
	}
    }

    *L_CtrStat = CSt_Breg;
      PURGE_CPUPIPELINE;
    status = *L_CtrStat;        /*read portB's status register    */
      PURGE_CPUPIPELINE;
    /*DBX(("CSt_Breg = 0x%x\n", status));*/

    if (status & CoSt_Ius) {     /*did portB cause the interrupt ? */

	mask = 1;
	*L_CtrStat = Data_Breg;
          PURGE_CPUPIPELINE;
	mdev = *L_CtrStat;      /*read portB's data register      */
          PURGE_CPUPIPELINE;
	/*DBX(("Data_Breg = 0x%x\n", mdev));*/

	for (i = 8; i <= 15; i++) {
	    if (mdev & mask & (MCtxt -> int_en_mask >> 8))
		input[i] = 1;           /* mark port B's active interrupt  */
	    mask <<= 1;                 /* lines                           */
	}
    }
				/* loop on active line */
    for (i = 0; i <= 15; i++) {
	if (input[i]) {
			/* active line */
	    /*DBX(("icv:isr:mdev = 0x%x\n", i + 1));*/
	    input[i] = 0;
	    L_LCtxt = &(MCtxt -> LineCtxt[i]);
	    L_LCtxt -> loc_count++;

	    L_LHdl = L_LCtxt -> LHdl;
		      /* Build the Event */
	    L_Atom.Evt.All = L_LHdl -> Event_Pattern.All;
			     /*  scan the subscriber table to send them the event */
	    ns = L_LHdl -> SubscriberMxNb;
	    cs = L_LHdl -> SubscriberCurNb;
	    L_Subs= &(L_LHdl -> Subscriber[0]);
	    for (j= 0 ; ((j < ns) && (cs >0) ) ; j++, L_Subs++){

	      if (L_Subs -> Ring == NULL) /* subscriber passive */
		continue;
	      cs--;   /* nb of subbscribers to find out in Subs table*/
	      L_UHdl = (L_Subs -> Ring) -> UHdl;
	      /*DBX(("icv:isr:UHdl=$%x j= %d  Subs=$%x\n", L_UHdl, j, L_Subs));*/
	      /*DBX(("*Ring$%x Subs->Counter$%x\n",L_Subs->Ring,L_Subs->EvtCounter));*/

	      if (( L_count = L_Subs -> EvtCounter) != (-1)){
		L_Sw1 = (unsigned short) L_count;
		L_Sw1++;
		if ( ( (short)L_Sw1 ) <0 )
		  L_Sw1=1;

		L_Subs -> EvtCounter = L_Sw1; /* keep counter positive */
	      }
		      /* give the event according to subscriber status  */
	      if (L_Subs -> mode  == icv_queuleuleu){ /* mode a la queueleuleu*/

		L_Atom.Subscriber = NULL;
		L_Atom.Evt.Word.w1  = L_Sw1;     /* set up event counter */
		L_RingAtom = PushTo_Ring(L_Subs -> Ring, &L_Atom);
	      }
	      else{                                     /* cumulative mode */
		if (L_count <0){      /* no event in Ring */

		  L_Subs -> EvtCounter = 1; /* init counter of Subscriber */
		  L_Atom.Subscriber = L_Subs; /* Link event to subscriber */
		  L_Atom.Evt.Word.w1  = 1;     /* set up event counter */
		  L_RingAtom = PushTo_Ring(L_Subs -> Ring, &L_Atom);
		  L_Subs -> CumulEvt = L_RingAtom; /* link to cumulative event, used to disconnect*/
		}
	      }
	      /* process case user stuck on select semaphore  */
	      if (L_UHdl -> sel_sem)
		ssignal(L_UHdl -> sel_sem);

	    } /* for/subscriber */
	    /* Enable the line before leaving */

	    if (L_LCtxt -> intmod == icv_ReenableOff )
	      disable_Line(L_LCtxt);
	    /*break;  */
	} /* if line active  */

    } /*  for / line */
    *L_CtrStat = CSt_Breg;       /*clear IP bit on portB  */
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;
    *L_CtrStat = CSt_Areg;       /*clear IP bit on portA  */
      PURGE_CPUPIPELINE;
    *L_CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    /*DBX (("icv:isr: end\n"));*/
}

struct dldd  entry_points = {
	icv196open, icv196close,
  icv196read, icv196write,
  icv196select, icv196ioctl,
  icv196install, icv196uninstall
};
