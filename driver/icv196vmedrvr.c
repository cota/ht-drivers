/**
 * @file icv196vmedrvr.c
 *
 * @brief Driver for managing icv196vme module providing 16 interrupt lines
 *
 * Created on 21-mar-1992 Alain Gagnaire, F. Berlin
 *
 * @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 18/11/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#include <cdcm/cdcm.h>
#include <vme_am.h>
#include <skeldrvr.h>
#include <skeldrvrP.h>
#include <skeluser_ioctl.h>
#include <icv196vme.h>
#include <icv196vmeP.h>

#define USER_VBASE 64

#ifdef __lynx__
#define PURGE_CPUPIPELINE asm("eieio") /* purge cpu pipe line */
#define ISYNC_CPUPIPELINE asm("sync")  /* synchronise instruction  */
#else /* __linux__ */
#define PURGE_CPUPIPELINE
#define ISYNC_CPUPIPELINE
#endif

#define IOINTSET(c, v, i, s) c = vme_intset(v, i, s, 0)
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

#define IGNORE_SIG 0
#define ICV_tout   1000

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

/* structure to store for one module the  logical lines
   corresponding to the physical lines */
struct T_ModuleLogLine {
	int Module_Index;
	short LogLineIndex[icv_LineNb];
};

/* sizing of module */
short LinePerModule= icv_LineNb;

/* isr entry point table */
int (*ModuleIsr[icv_ModuleNb])(void *) = {
	icv196vmeisr,
	icv196vmeisr,
	icv196vmeisr,
	icv196vmeisr
};

/* table to link for each possible module:
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
 struct T_ModuleLogLine *MConfig[icv_ModuleNb] = {&Module1, &Module2, &Module3,
						 &Module4, &Module5, &Module6,
						 &Module7, &Module8 };

/* table to link logical index and user line address */
struct icv196T_UserLine UserLineAdd[ICV_LogLineNb] = {

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
  static table
  This shows the structure of the static table of the driver dynamically
  allocated  by the install subroutine of the driver.
*/
struct icv196T_s {
	int install;
	int usercounter; /* User counter */
	int sem_drvr;    /* semaphore for exclusive access to static table */
	int timid;       /* Interval timer Identification */
	int interval;    /* Timerinterval */

	/* extention to stand interrupt processing */
	int   UserTO;   /* User time out, by waiting for ICV */
	short UserMode; /* ? */

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
	struct T_ModuleCtxt *ModuleCtxtDir[icv_ModuleNb]; /* module directory */
	struct T_ModuleCtxt ModuleCtxt[icv_ModuleNb]; /* Modules contexts */

	/* Logical line tables */
	int LogLine_Size;
	struct T_LogLineHdl *LineHdlDir[ICV_LogLineNb]; /* Logical line directory */
	struct T_LogLineHdl LineHdl[ICV_LogLineNb]; /* Logical line handles */
};

/* to come out of waiting event */
static int UserWakeup(void *data)
{
	ulong ps;
	struct T_UserHdl *UHdl = (struct T_UserHdl*)data;

	ssignal(&UHdl->Ring.Evtsem);
	disable(ps);
	if (UHdl->timid >= 0)
		cancel_timeout(UHdl -> timid);

	UHdl->timid = -1;
	restore(ps);
	return 0;
}

/*	       Management of logical/physical Line mapping
	       ===========================================
*/

/* subroutines to convert a user line address in a logical line index */
static int CnvrtUserLine(char grp, char index)
{
	int val;

	val = (int) ( (grp * ICV_IndexNb) + index);
	return val;
}

/* subroutines to convert a module line address in a logical index */
static int CnvrtModuleLine(int module, int line)
{
	return ((int)(( *(MConfig[module])).LogLineIndex[line]));
}

/* subroutines to get a user line address from a logical line index */
static void GetUserLine(int logindex, struct icv196T_UserLine *add)
{
	add->group = UserLineAdd[logindex].group;
	add->index = UserLineAdd[logindex].index;
}

/* subroutines to initialise the directory tables */
static void Init_Dir(struct icv196T_s *s)
{
	int i;

	for (i = 0; i < icv_ModuleNb; i++)
		s->ModuleCtxtDir[i] = NULL; /* Default value */

	for (i = 0; i < ICV_LogLineNb; i++)
		s->LineHdlDir[i] = NULL; /* Default value */
}

/* subroutine to  Initialise Ring buffer descriptor */
static void Init_Ring(struct T_UserHdl *UHdl, struct T_RingBuffer *Ring,
		      struct icvT_RingAtom *Buf, short mask)
{
	short   i;
	struct icvT_RingAtom *ptr;

	Ring->UHdl = UHdl;
	Ring->Buffer = Buf;
	Ring->Evtsem =0;
	Ring->mask = mask;
	Ring->reader = Ring->writer = 0;
	ptr = &Buf[mask];

	for (i = mask; i >= 0; i--, ptr--) {
		ptr->Subscriber = NULL;
		ptr->Evt.All = 0;
	}
}

/*
  Subroutine - to push value in ring and signal semaphore,
  -  callable from isr only: no disable done!
*/
static struct icvT_RingAtom *PushTo_Ring(struct T_RingBuffer *Ring,
					 struct icvT_RingAtom *atom)
{
	short I;
	struct icvT_RingAtom *Atom;
	struct T_UserHdl *UHdl;

	if (Ring->Evtsem >= Evt_nb) { /* Ring buffer full */
		sreset(&Ring->Evtsem); /* Clear semaphore, awake all
					  process waiting on it */
		Ring->reader = Ring->writer = 0;

		/* push an event to signal purge of buffer */
		Atom = &(Ring->Buffer[0]);
		Atom->Subscriber = NULL;
		Atom->Evt.All= 0;
		Atom->Evt.Word.w1 = -1;
		Ring->writer++;
		ssignal(&Ring->Evtsem);
		UHdl = Ring->UHdl;
		/* DBG (("icv196:isr:purge Ring for Chanel=%d UHdl= $%lx\n",
		   UHdl -> chanel, UHdl)); */
	}

	/* now push the event in the ring  */
	I = Ring->writer;
	Atom = &(Ring->Buffer[I]); /* Atom to set up */
	Atom->Evt.All    = atom->Evt.All;
	Atom->Subscriber = atom->Subscriber;
	Ring->writer     = ((++I) & (Ring->mask));
	ssignal(&(Ring->Evtsem)); /* manage ring  */
	/*  DBG(("icvp:Push Evtsem %lx Writer = %lx Evt= %lx, Subs=$%lx\n",
	    Ring->Evtsem, Ring->writer, Atom->Evt.All, Atom->Subscriber)); */

  return Atom;
}

/* Subroutine to pull out an event from the Ring */
static unsigned long PullFrom_Ring(struct T_RingBuffer *Ring)
{
	ulong ps;
	register struct icvT_RingAtom *Atom;
	register struct T_Subscriber *Subs;
	short  I, ix;
	union icvU_Evt Evt;

	/* DBG(("icv:PullFrom: Ring=$%lx Evtsem=%d \n",
	   Ring, Ring -> Evtsem )); */

	disable(ps); /* Protect seq. from isr access */
	ix = I = Ring->reader;
	if ((Ring->Evtsem) != 0) { /* Ring non empty */
		swait(&Ring->Evtsem, -1); /* to manage sem counter */
		Atom = &Ring->Buffer[I]; /* point current atom
					    to pull from ring */
		/* extract the event to be read */
		Subs = Atom->Subscriber;
		Evt.All = Atom->Evt.All;
		Ring->reader = ((++I) & (Ring->mask)); /* manage ring buffer */

		/* build the value to return according to mode */
		if (Subs != NULL) { /* Case cumulative mode */
			/* DBG(("icv:PullFrom: Subs*=$%lx LHdl=%lx\n",
			   Subs, Subs -> LHdl));*/
			Evt.Word.w1 = Subs->EvtCounter; /* counter from
							   subscriber */

			/* Clear event flag in subscriber context */
			Subs->CumulEvt = NULL; /*no more event in ring*/
			Subs->EvtCounter = -1; /* clear event present
						  in the ring */
		}
		/* DBG (("icv196:PullRing: buff $%lx Reader= %d Evt= $%lx\n",
		   Ring -> Buffer, ix, Evt.All));*/
	} else { /* ring empty  */
		Evt.All = -1;
		/* DBG (("icv196:Empty ring: buff * %lx \n",
		   Ring -> Buffer)); */
	}

	restore(ps);
	return Evt.All;
}

/* Subroutine to initialise a user Handle */
static void Init_UserHdl(struct T_UserHdl *UHdl, int chanel,
			 struct icv196T_s *s)
{
	int i;
	char *cptr;

	UHdl -> s = s;
	UHdl -> chanel = chanel;
	UHdl -> pid = 0;
	UHdl -> usercount = 0; /* clear current user number */
	UHdl -> sel_sem = NULL; /* clear the semaphore for the select */

	/* clear up  bit map of established connection */
	for (i = 0, cptr = &UHdl->Cmap[0]; i < ICV_mapByteSz; i++)
		*cptr++ = 0;

	Init_Ring(UHdl, &UHdl->Ring, &UHdl->Atom[0], Evt_msk);
}

/* subroutines Initialise a subscriber Handle */
static void Init_SubscriberHdl(struct T_Subscriber *Subs, int mode)
{
	struct icvT_RingAtom *Evt;

	/* clean up  Event in Ring if element active  */
	if ((Evt = Subs->CumulEvt) != NULL) {
		Evt->Evt.Word.w1 = Subs->EvtCounter; /* unlink cumulative
							event */
		Evt->Subscriber = NULL;
  }

	Subs->CumulEvt = NULL;	/* no event in the ring */
	Subs->Ring = NULL;	/* Clear the subscribing link */
	Subs->mode = mode;
	Subs->EvtCounter = 0;

	/* CHK (("icv:Subs: Subs Hdl add = $%lx\n", Subs )); */
}

/* Initialise the subscriber directory of a logical line handle */
static void Init_LineSubscribers(struct T_LogLineHdl *LHdl)
{
	struct T_Subscriber *Subs;
	int i, n, mode;

	if (LHdl->LineCtxt->Type == icv_FpiLine)
		mode = icv_cumulative; /* default for pls line
					  cumulative mode */
	else
		mode = icv_queuleuleu; /*default for fpi line a la
					 queue leuleu */

	Subs = &LHdl->Subscriber[0];
	/* CHK (("icv:LSubs: LSubs add =$%lx   \n",Subs )); */

	n = LHdl->SubscriberMxNb;
	for (i=0; i < n; i++, Subs++) {
		Subs->LHdl = LHdl;
		Subs->CumulEvt = NULL; /* to prevent active event processing */
		Init_SubscriberHdl(Subs, mode);
	}
}

/* initialise a Line handle associated to a physical line */
static struct T_LogLineHdl *Init_LineHdl(int lli, struct T_LineCtxt *LCtxt)
{
	struct icv196T_s *s;
	struct T_LogLineHdl *LHdl;
	union  icv196U_UserLine UserLine;

	s = LCtxt->s;
	LHdl = s->LineHdlDir[lli] = &s->LineHdl[lli];
	LHdl->s = s;
	LHdl->LogIndex = lli;
	LHdl->LineCtxt = LCtxt;	/* Link Line handle and Line context */

	/* Get the user line address pattern */
	GetUserLine(lli, (struct icv196T_UserLine *) &UserLine);

	/* build the event pattern for that line */
	LHdl->Event_Pattern.Word.w2 = UserLine.All;
	LHdl->Event_Pattern.Word.w1 = 0;

	/* CHK (("icv:LHdl: s = $%lxLogIndex %d LHdl add= $%lx\n",
	   LHdl -> s, lli, LHdl)); */

	LHdl->SubscriberCurNb = 0; /* Current Subscriber nb set to 0 */

	if (LCtxt->Type == icv_FpiLine)
		/* Fpi line only one subscriber allowed */
		LHdl->SubscriberMxNb = 1;
	else
		/* Icv line multi user to wait  */
		LHdl->SubscriberMxNb = icv_SubscriberNb;

	Init_LineSubscribers(LHdl);

	return LHdl;
}

/* Establish connection for a channel with a given logical line */
static struct T_Subscriber *LineBooking(struct T_UserHdl *UHdl,
					struct T_LogLineHdl *LHdl,
					int mode)
{
	ulong ps;
	int i, ns;
	struct T_Subscriber *Subs, *val ;
	struct T_LineCtxt   *LCtxt;

	val = NULL;
	LCtxt = LHdl->LineCtxt;
	ns = LHdl->SubscriberMxNb;
	Subs = &LHdl->Subscriber[0];
	disable(ps);

	for (i = 0; i < ns; i++, Subs++) {
		if (Subs->Ring != NULL) /* subscriber occupied, scan on */
			continue;

		/* allocated this element */
		LHdl->SubscriberCurNb++; /* update current subscriber number  */
		val = Subs; /* found a place */
		Subs->Ring = &UHdl->Ring; /* link to Ring buffer */
		Subs->CumulEvt = NULL;

		/*  flag booking of the line in the User handle */
		SETBIT(UHdl->Cmap, LHdl->LogIndex);
		Subs->mode = icv_queuleuleu; /* default mode */
		Subs->EvtCounter = 0;	/* counter for connected line */

    if (!mode) { /* no user requirement */
	    if (LCtxt->Type == icv_FpiLine) {
		    Subs->mode = icv_cumulative; /* def. cumulative
						    for Pls Line */
		    Subs->EvtCounter = -1;
	    }
    } else if (mode == icv_cumulative) {
	    Subs->mode = icv_cumulative;
	    Subs->EvtCounter = -1;
    }
    break;
   }

	restore(ps);
	return val;
}

/* Rub out connection of a chanel from a given logical line */
static struct T_Subscriber *LineUnBooking(struct T_UserHdl *UHdl,
					  struct T_LogLineHdl *LHdl)
{
	ulong ps;
	int i, ns;
	struct T_Subscriber *Subs;
	struct T_Subscriber *val;
	struct T_RingBuffer *UHdlRing;

	val = NULL;
	UHdlRing = &UHdl->Ring;
	disable(ps);
	ns = LHdl->SubscriberMxNb;
	Subs = &LHdl->Subscriber[0];

	for (i = 0; i < ns; i++, Subs++) {
		if ( (Subs->Ring == NULL)
		     || (Subs->Ring != UHdlRing))
			continue;

		/* subscriber found: now get rid of connection */
		LHdl->SubscriberCurNb--; /* update current subscriber number */
		Init_SubscriberHdl(Subs, 0); /* init subscriber with
						default mode */

		val = Subs;
		/*  clear flag of booking of the line in the User handle */
		CLRBIT(UHdl->Cmap, LHdl->LogIndex);
		break;
	}

	restore(ps);
	return val;
}

#ifndef X_NO_CheckBooking
/* Check if a given UHdl got connected to the LHdl */
static struct T_Subscriber *CheckBooking(struct T_UserHdl *UHdl,
					 struct T_LogLineHdl *LHdl)
{
	ulong ps;
	int i, ns;
	struct T_Subscriber *Subs;
	struct T_Subscriber *val;
	register struct T_RingBuffer *UHdlRing;

	val = NULL;
	UHdlRing = &UHdl->Ring;
	disable(ps);
	ns = LHdl->SubscriberMxNb;
	Subs = &LHdl->Subscriber[0];

	for (i = 0; i < ns; i++, Subs++) {
		if ( (Subs->Ring == NULL)
		     || (Subs->Ring != UHdlRing))
			continue;

		val = Subs;
		break;
	}

	restore(ps);
	return val;
}
#endif

/* to enable interrupt from a line called from service level */
static void enable_Line(struct T_LineCtxt *LCtxt)
{
	ulong ps;
	int locdev;
	unsigned char status, dummy, *CtrStat;
	unsigned short mask;

	CtrStat = LCtxt->MCtxt->VME_StatusCtrl;
	locdev = LCtxt->Line;
	/* DBX (("enable line: locdev = %d\n", locdev)); */

	if (locdev >= PortA_nln) { /*if locdev is on portB  */
		/*enable interrupt on line nr. locdev on portB */
		disable(ps);
		mask = 1 << locdev; /* set interrupt enable */
		LCtxt->MCtxt->int_en_mask |= mask; /* mask */
		/* DBX (("enable line: int_en_mask = 0x%x\n",
		  LCtxt->MCtxt->int_en_mask)); */

		mask = 1 << (locdev - PortA_nln);

		*CtrStat = CSt_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = CoSt_ClIpIus;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrPo_Breg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status | mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrPo_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrTr_Breg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status | mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrTr_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrMsk_Breg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status | mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrMsk_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		/* TEST!! */
		*CtrStat = PtrMsk_Breg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("enable line: PtrMsk_Breg = 0x%x\n", dummy)); */

		/* TEST!! */
		*CtrStat = PtrPo_Breg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("enable line: PtrPo_Breg = 0x%x\n", dummy)); */

		/* TEST!! */
		*CtrStat = PtrTr_Breg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("enable line: PtrTr_Breg = 0x%x\n", dummy)); */

		*CtrStat = CSt_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = CoSt_ClIpIus;
		PURGE_CPUPIPELINE;

		restore(ps);
	} else { /* Case port A */
		disable(ps);
		mask = 1 << locdev; /* set interrupt enable */
		LCtxt->MCtxt->int_en_mask |= mask; /* mask */
		/* DBX (("enable line: int_en_mask = 0x%x\n",
		  LCtxt->MCtxt->int_en_mask)); */

		*CtrStat = CSt_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = CoSt_ClIpIus;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrPo_Areg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status | mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrPo_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrTr_Areg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status | mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrTr_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrMsk_Areg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status | mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrMsk_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		/* TEST!! */
		*CtrStat = PtrMsk_Areg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("enable line: PtrMsk_Areg = 0x%x\n", dummy)); */

		/* TEST !! */
		*CtrStat = PtrPo_Areg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("enable line: PtrPo_Areg = 0x%x\n", dummy)); */

		/* TEST !! */
		*CtrStat = PtrTr_Areg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("enable line: PtrTr_Areg = 0x%x\n", dummy)); */

		*CtrStat = CSt_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = CoSt_ClIpIus;
		PURGE_CPUPIPELINE;

		restore(ps);
	}

	LCtxt->status = icv_Enabled;
}

/* to stop interrupt from this line this is done when user request it */
static void disable_Line(struct T_LineCtxt *LCtxt)
{
	ulong ps;
	int locdev;
	unsigned char status, dummy, *CtrStat;
	unsigned short mask;

	CtrStat = LCtxt->MCtxt->VME_StatusCtrl;
	locdev = LCtxt->Line;
	/* DBX (("disable line: locdev = %d\n", locdev)); */
	if (locdev >= PortA_nln) { /* if locdev is on portB */
		/* enable interrupt on line nr. locdev on portB */
		disable(ps);
		mask = ~(1 << locdev); /* clear interrupt enable */
		LCtxt->MCtxt->int_en_mask &= mask;  /* mask */
		/*DBX (("disable line: int_en_mask = 0x%x\n",
		  LCtxt->MCtxt->int_en_mask))*/;

		mask = ~(1 << (locdev - PortA_nln));

		*CtrStat = CSt_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = CoSt_ClIpIus;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrPo_Breg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status & mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrPo_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrTr_Breg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status & mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrTr_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrMsk_Breg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status & mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrMsk_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		/* TEST !! */
		*CtrStat = PtrMsk_Breg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("disable line: PtrMsk_Breg = 0x%x\n", dummy)); */

		/* TEST!! */
		*CtrStat = PtrPo_Breg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("disable line: PtrPo_Breg = 0x%x\n", dummy)); */

		/* TEST!! */
		*CtrStat = PtrTr_Breg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("disable line: PtrTr_Breg = 0x%x\n", dummy)); */

		*CtrStat = CSt_Breg;
		PURGE_CPUPIPELINE;
		*CtrStat = CoSt_ClIpIus;
		PURGE_CPUPIPELINE;

		restore(ps);
	} else { /* else */
		/* disable interrupt on line nr. locdev on portA */
		disable(ps);
		mask = ~(1 << locdev);
		/* clear interrupt enable */
		LCtxt->MCtxt->int_en_mask &= mask; /* mask */
		/*DBX (("disable line: int_en_mask = 0x%x\n",
		  LCtxt->MCtxt->int_en_mask))*/;

		*CtrStat = CSt_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = CoSt_ClIpIus;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrPo_Areg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status & mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrPo_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrTr_Areg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status & mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrTr_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		*CtrStat = PtrMsk_Areg;
		PURGE_CPUPIPELINE;
		status = *CtrStat;
		PURGE_CPUPIPELINE;
		status = status & mask;
		PURGE_CPUPIPELINE;
		*CtrStat = PtrMsk_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = status;
		PURGE_CPUPIPELINE;

		/* TEST!! */
		*CtrStat = PtrMsk_Areg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("disable line: PtrMsk_Areg = 0x%x\n", dummy)); */

		/* TEST!! */
		*CtrStat = PtrPo_Areg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("disable line: PtrPo_Areg = 0x%x\n", dummy)); */

		/* TEST!! */
		*CtrStat = PtrTr_Areg;
		PURGE_CPUPIPELINE;
		dummy = *CtrStat;
		PURGE_CPUPIPELINE;
		/* DBX(("disable line: PtrTr_Areg = 0x%x\n", dummy)); */

		*CtrStat = CSt_Areg;
		PURGE_CPUPIPELINE;
		*CtrStat = CoSt_ClIpIus;
		PURGE_CPUPIPELINE;

		restore(ps);
	}

	LCtxt->status = icv_Disabled;
}

/* to get rid of all connections established on a channel
   called at close of channel */
static void ClrSynchro(struct T_UserHdl *UHdl)
{
	ulong ps;
	int i, j;
	struct icv196T_s   *s;
	struct T_LineCtxt   *LCtxt;
	register struct T_LogLineHdl  *LHdl;
	register struct T_RingBuffer *UHdlRing;
	register struct T_Subscriber *Subs;
	unsigned char w;
	char *cptr;

	s = UHdl->s;
	UHdlRing = &UHdl->Ring;
	if (UHdl->timid >= 0)
		cancel_timeout(UHdl->timid);

	/* scan map byte array  */
	for ( i = 0, cptr = &UHdl->Cmap[0]; i < ICV_mapByteSz; i++, cptr++) {
		if ((w = (*cptr) & 255) != 0) {
			*cptr = 0; /* Clear bits */
			for (j = 7; j >= 0; j--, w <<= 1 ) {
				/* scan each bit of the current byte */
				if (w == 0)
					break;
				if ( ((char) w) < 0 ){	/* line connected */
					if ( (LHdl = s->LineHdlDir[(i*8 + j)])
					     == NULL) {
						/* DBG(("icv196:Clrsynchro: log line not in directory \n")); */
						continue;
					}
					if ( (Subs = LineUnBooking(UHdl, LHdl))
					     == NULL ) {
						/*DBG(("icv196:Clrsynchro: inconsistency at unbooking \n")); */
					}
					LCtxt = LHdl->LineCtxt;
					disable(ps);
					if (LHdl->SubscriberCurNb == 0)
						disable_Line(LCtxt);

					restore(ps);
				} /* if line connected */
			} /* end for/ byte bit scanning */
		}
	} /* end for/ Cmap byte scanning  */

	sreset(&(UHdl->Ring.Evtsem)); /* clear semaphore */

	for (i = 0, cptr = &UHdl->Cmap[0]; i < ICV_mapByteSz; i++)
		*cptr++ = 0;
}

/*
  subroutines to initialise a line context table of a icv196 module
  table to manage the hardware of a physical line of a module
*/
static void Init_LineCtxt(int line, int type, struct T_ModuleCtxt *MCtxt)
{
	struct T_LineCtxt *LCtxt;
	int LogIndex, m;

	LCtxt = &MCtxt->LineCtxt[line];
	m = MCtxt->Module;
	LogIndex = CnvrtModuleLine(m, line);
	LCtxt->s = MCtxt->s;
	LCtxt->MCtxt = MCtxt;
	LCtxt->Line = line;
	LCtxt->Type = type;
	LCtxt->intmod = icv_ReenableOn;
	LCtxt->status = icv_Disabled;
	LCtxt->loc_count = -1;

	/* CHK(("icv:LCtxt:s= $%lx Module %d Line %d Line ctxt add = $%lx, LogIndex %d\n",
	   LCtxt -> s ,m, line, LCtxt, LogIndex )); */

	/* Link physical line to logical line and vise versa */
	LCtxt->LHdl = Init_LineHdl(LogIndex, LCtxt);
}



/*
  Subroutine to initialise a module context for a icv196 module
  table to manage the hardware of a physical module
*/
static struct T_ModuleCtxt *Init_ModuleCtxt(struct icv196T_s *s,
					    short module,
					    struct icv196T_ModuleParam *vmeinfo)
{
	int   i;
	short n;
	unsigned long base, offset, sysBase, moduleSysBase;
	struct T_ModuleCtxt *MCtxt;
	int type;
	int am;
	struct pdparam_master param; /* structure containing VME
					access parameters */

	/* declare module in the Module directory */
	MCtxt = &s->ModuleCtxt[module];

	/* init context table */
	MCtxt -> s = s;
	MCtxt -> length = sizeof(struct T_ModuleCtxt);
	MCtxt -> sem_module = 1;	/* semaphore for exclusive access */
	MCtxt -> Module = module;
	MCtxt -> dflag = 0;
	MCtxt -> LineMxNb = LinePerModule;

	/*  CHK (("icv:MCtxt: s= $%lx Module %d ctxt add = $%lx\n", MCtxt ->s, module, MCtxt ));*/

	/*  set up device dependent info */

	/* Set up base add of module as seen from cpu mapping.
	   This information is available to user through ioctl function
	   it provides the Module base add required by  smemcreate to create
	   a virtual window on the vme space of the module
	   this is for accessing the module directly from user program access
	   through a window created by smem_create.
	*/

	base = vmeinfo -> base;

	MCtxt -> VME_offset =  base;
	MCtxt -> VME_size =     vmeinfo -> size;

	offset = base & (unsigned long)0x00ffffffL;
	am = AM_A24_UDA;

	/* Compute address in system space */
	bzero((char *)&param, sizeof(struct pdparam_master));
	/* CES: build an address window (64 kbyte) for VME A16D16 accesses */
	param.iack   = 1;	/* no iack */
	param.rdpref = 0;	/* no VME read prefetch option */
	param.wrpost = 0;	/* no VME write posting option */
	param.swap   = 1;	/* VME auto swap option */
	param.dum[0]   = 0;	/* window is sharable */
	sysBase = (unsigned long)find_controller( 0x0, 0x10000, am, 0, 0, &param);
	if (sysBase == (unsigned long)(-1)){
		CPRNT(("icv196vme_drvr:find_controller:cannot map device address space, INSTALLATION IMPOSSIBLE\n"));
		return ((struct T_ModuleCtxt *) (-1));
	}

	moduleSysBase = sysBase + offset;
	DBG_INSTAL(("icv196vme:install: am 0x%x Standard,  0x%lx base + 0x%lx"
		    "offset => 0x%lx module sys base\n",  am, sysBase, offset, moduleSysBase));
	/*
	  Set up permanent pointers to access the module
	  from system mapping to be used from the driver
	*/
	MCtxt -> SYSVME_Add = (short *) (moduleSysBase);

	MCtxt -> VME_StatusCtrl =   (unsigned char *)(moduleSysBase + CoReg_Z8536);
	MCtxt -> VME_IntLvl =       (unsigned char *)(moduleSysBase + CSNIT_ICV);
	MCtxt -> VME_CsDir =        (short *)(moduleSysBase + CSDIR_ICV);

	/* Initialise the associated line contexts */
	type = 0; /* to select default line type  */
	n = MCtxt -> LineMxNb;
	for (i = 0 ; i < n; i++) {
		MCtxt -> Vect[i] = vmeinfo -> vector[0];
		MCtxt -> Lvl[i]  = vmeinfo -> level[0];
		Init_LineCtxt(i,type, MCtxt);
	}
	MCtxt -> isr[0] = ModuleIsr[module];
	return MCtxt;
}

/*
  Initializes the hardware of a icv196 module.  Upon the initialization,
  all interrupt lines are in the disabled state.
*/
int icvModule_Init_HW(struct T_ModuleCtxt *MCtxt)
{
	unsigned char v, l, dummy, *CtrStat;
	int m;
	ulong ps;

	m       = MCtxt->Module;
	v       = MCtxt->Vect[0];
	l       = MCtxt->Lvl[0];
	CtrStat = MCtxt->VME_StatusCtrl;

  /* To catch bus error if module does not answer */
	if (recoset()) { /* To prevent system crash in case of bus error */
		cprintf("icv196 install: on Module %d VME offset$%lx,"
			" CTrStat address = $%x bus error occured!\n",
			m, MCtxt->VME_offset, (uint)CtrStat);
		cprintf("icv196:install: Module %d INSTALLATION IMPOSSIBLE\n",
			m);
		return -1;
	}

	/* reset hardware of icv196vme module */
	disable(ps);

	dummy = *CtrStat; /* force the board to state 0 */
	PURGE_CPUPIPELINE;
	ISYNC_CPUPIPELINE;

	*CtrStat = MIC_reg;
	PURGE_CPUPIPELINE;
	dummy = *CtrStat;
	PURGE_CPUPIPELINE;
	*CtrStat = MIC_reg;
	PURGE_CPUPIPELINE;
	*CtrStat = b_RESET; /* reset the board */
	PURGE_CPUPIPELINE;
	*CtrStat = 0; /* go to state 0 (normal operation) */
	PURGE_CPUPIPELINE;

	*(MCtxt->VME_IntLvl)  = ~l; /* set interrupt level for this module */
	PURGE_CPUPIPELINE;
	*(MCtxt->VME_CsDir) = 0; /* set all I/O ports to input */
	PURGE_CPUPIPELINE;

	MCtxt->old_CsDir = 0;
	MCtxt->startflag = 0;
	MCtxt->int_en_mask = 0; /* clear interrupt enable mask */

	/* set up hardware for portA and portB on the icv module */
	*CtrStat = MCC_reg;
	PURGE_CPUPIPELINE;
	*CtrStat = (PortA_Enable | PortB_Enable);
	PURGE_CPUPIPELINE;

	/* (portA and portB operate independently) */
	*CtrStat = MSpec_Areg; /* portA: bitport */
	PURGE_CPUPIPELINE;
	*CtrStat = (PtrM_Or | Latch_On_Pat_Match);
	PURGE_CPUPIPELINE;

	*CtrStat = MSpec_Breg; /* portB: bitport */
	PURGE_CPUPIPELINE;
	*CtrStat = (PtrM_Or | Latch_On_Pat_Match);
	PURGE_CPUPIPELINE;

	*CtrStat = CSt_Areg; /* portA: no interrupt on error */
	PURGE_CPUPIPELINE;
	*CtrStat = CoSt_SeIe; /* interrupts enabled */
	PURGE_CPUPIPELINE;

	*CtrStat = CSt_Breg; /* portB: no interrupt on error */
	PURGE_CPUPIPELINE;
	*CtrStat = CoSt_SeIe; /* interrupts enabled */
	PURGE_CPUPIPELINE;

	*CtrStat = DPPol_Areg; /* portA: non inverting */
	PURGE_CPUPIPELINE;
	*CtrStat = Non_Invert;
	PURGE_CPUPIPELINE;

	*CtrStat = DPPol_Breg; /* portB: non inverting */
	PURGE_CPUPIPELINE;
	*CtrStat = Non_Invert;
	PURGE_CPUPIPELINE;

	*CtrStat = DDir_Areg; /* portA: input port */
	PURGE_CPUPIPELINE;
	*CtrStat = All_Input;
	PURGE_CPUPIPELINE;

	*CtrStat = DDir_Breg; /* portB: input port */
	PURGE_CPUPIPELINE;
	*CtrStat = All_Input;
	PURGE_CPUPIPELINE;

	*CtrStat = SIO_Areg; /* portA: normal input */
	PURGE_CPUPIPELINE;
	*CtrStat = Norm_Inp;
	PURGE_CPUPIPELINE;

	*CtrStat = SIO_Breg; /* portB: normal input */
	PURGE_CPUPIPELINE;
	*CtrStat = Norm_Inp;
	PURGE_CPUPIPELINE;

	*CtrStat = PtrMsk_Areg; /* portA: masked off */
	PURGE_CPUPIPELINE;
	*CtrStat = All_Masked;
	PURGE_CPUPIPELINE;

	*CtrStat = PtrTr_Areg;
	PURGE_CPUPIPELINE;
	*CtrStat = 0;
	PURGE_CPUPIPELINE;

	*CtrStat = PtrPo_Areg;
	PURGE_CPUPIPELINE;
	*CtrStat = 0;
	PURGE_CPUPIPELINE;

	*CtrStat = PtrMsk_Breg; /* portB: masked off */
	PURGE_CPUPIPELINE;
	*CtrStat = All_Masked;
	PURGE_CPUPIPELINE;

	*CtrStat = PtrTr_Breg;
	PURGE_CPUPIPELINE;
	*CtrStat = 0;
	PURGE_CPUPIPELINE;

	*CtrStat = PtrPo_Breg;
	PURGE_CPUPIPELINE;
	*CtrStat = 0;
	PURGE_CPUPIPELINE;

	/* write interrupt vectors for portA and portB */
	*CtrStat = ItVct_Areg;
	PURGE_CPUPIPELINE;
	*CtrStat = v;
	PURGE_CPUPIPELINE;
	*CtrStat = ItVct_Breg;
	PURGE_CPUPIPELINE;
	*CtrStat = v;
	PURGE_CPUPIPELINE;

	*CtrStat = MIC_reg; /* no status in interrupt vector */
	PURGE_CPUPIPELINE;
	*CtrStat = b_MIE; /* master interrupt enable */
	PURGE_CPUPIPELINE;

	restore(ps);

	/* DBG (("icv196vmedrvr: install: hardware set up finished \n"));*/
	/* DBG (("icv196vme installation terminated, driver ready to work \n"));*/

	noreco(); /* remove local bus trap */
	return 0;
}

/*
  subroutines to startup a icv196 module
  This routine performs the hardware set up of the device and connects it to
  the system interrupt processing, this actually start up the driver for this
  device.
  The devices are served by the same Lynx OS driver, the isr recognizes the
  line by inspecting the Control register if the associated module.
  Each Module has its own context, the different isr if there are several
  different level are working in the local stack, no jam should occurr!
*/
int icvModule_Startup(struct T_ModuleCtxt *MCtxt)
{
    unsigned char v;
    struct icv196T_s *s;
    int m, cc;

    s = MCtxt->s;
    m = MCtxt->Module;
    v = MCtxt->Vect[0];

    /* CHK (("icv:Setup: s =$%lx, Module %d ctxt add = $%lx\n", s, m, MCtxt )); */

    /* Now connect interrupt to system */
    /* DBG (("icv:Setup: module %d vector %d level %d \n",m,v, l )); */

    /* Connect interrupt from module  */
    IOINTSET(cc, v, (int (*)(void *))MCtxt->isr[0], (char *)MCtxt);
    if (cc < 0) {
	cprintf("icv196:install: iointset for  vector=%d error%d\n", v, cc);
	pseterr(EFAULT);
	return SYSERR;
    }

    DBG_INSTAL(("icv196vme_drvr:install: iointset for module %d, vector %d\n", m, v));
    return icvModule_Init_HW(MCtxt);
}

#ifndef X_NO_disable_Module
/*
  protection routine
  must be called from service level before accessing hardware, in order
  to be protected from isr processing
  the enable must be done by calling enable_Module
*/
static void disable_Module(struct T_ModuleCtxt *MCtxt)
{
	ulong ps;
	unsigned int w1;
	unsigned char msk, status, *CtrStat;

	CtrStat = MCtxt->VME_StatusCtrl;
	w1 = ~b_MIE; /* mask excludes the MIE bit (Master Interrupt Enable) */
	msk = (unsigned char) (w1 & 0xff);
	disable(ps);
	*CtrStat = MIC_reg;
	PURGE_CPUPIPELINE;
	status = *CtrStat;
	PURGE_CPUPIPELINE;
	status = status & msk;
	PURGE_CPUPIPELINE;
	*CtrStat = MIC_reg;
	PURGE_CPUPIPELINE;
	*CtrStat = status;
	PURGE_CPUPIPELINE;
	restore(ps);
}
#endif

#ifndef X_NO_enable_Module
/*
  to enable interrupt again after disable,
  called from service level
*/
static void enable_Module(struct T_ModuleCtxt *MCtxt)
{
	long ps;
	unsigned char msk, status, *CtrStat;

	CtrStat = MCtxt->VME_StatusCtrl;
	msk = b_MIE; /* mask includes the MIE bit (Master Interrupt Enable) */
	disable(ps);
	*CtrStat = MIC_reg;
	PURGE_CPUPIPELINE;
	status = *CtrStat;
	PURGE_CPUPIPELINE;
	status = status | msk;
	PURGE_CPUPIPELINE;
	*CtrStat = MIC_reg;
	PURGE_CPUPIPELINE;
	*CtrStat = status;
	PURGE_CPUPIPELINE;
	restore(ps);
}
#endif

/*
  Check if module lost its status, reinitialise it if yes
  to face a  reset without system crash
*/
int icvModule_Reinit(struct T_ModuleCtxt *MCtxt, int line)
{
	struct icv196T_s     *s;
	struct T_LineCtxt *LCtxt;
	unsigned char v, l;
	int  i, m, lx, cc;
	unsigned char bw1, *CtrStat;

	s = MCtxt->s;
	LCtxt = &MCtxt->LineCtxt[line];

	m = MCtxt->Module;
	v = MCtxt->Vect[line];

	l = MCtxt->Lvl[line];
	CtrStat = MCtxt->VME_StatusCtrl;

	*CtrStat = MIC_reg; /* master interrupt disable, permits */
	PURGE_CPUPIPELINE;
	*CtrStat = 0; /* reading interrupt vector */
	PURGE_CPUPIPELINE;

	*CtrStat = ItVct_Areg;
	PURGE_CPUPIPELINE;
	bw1 = (unsigned char)*CtrStat; /* read interrupt vector for
					  this module */
	PURGE_CPUPIPELINE;

	*CtrStat = MIC_reg;
	PURGE_CPUPIPELINE;
	*CtrStat = b_MIE; /*master interrupt enable */
	PURGE_CPUPIPELINE;

	/* Check if setting still well recorded  */
	if (bw1 != v) {
		/* The module lost its initialisation: redo it */
		/*  DBG (("icv: Reinit Module %d line %d !!! corrupted vect= %d instead of %d\n",
		    m, line, bw1, v )); */

		lx = MCtxt->LineMxNb;
		for (i = 0; i < lx; i++) {
			LCtxt = &MCtxt->LineCtxt[i];
			LCtxt->Reset = 1; /* To manage reenable of line
					     at connect */
		}
		/* reset hardware of icv196vme module */
		cc = icvModule_Init_HW(MCtxt);

		for (i = 0; i < lx; i++) {
			LCtxt = &MCtxt->LineCtxt[i];
			if (LCtxt->LHdl->SubscriberCurNb) {
				enable_Line(LCtxt);
				cprintf("icv196vmedrvr: Reinit: in module %d,"
					" reanable the active line %d\n",
					m, LCtxt->Line);
			}
			LCtxt->Reset = 1; /* To manage reenable of line
					     at connect */
		}

		if (cc == 0)
			return 0;
		else
			return -1;
	}

	return 0;
}

char *icv196install(struct icv196T_ConfigInfo *info)
{
	register struct icv196T_s  *s; /* static table pointer */
	struct icv196T_ModuleParam *MInfo;
	struct T_ModuleCtxt      *MCtxt;
	unsigned char l;
	ushort v;
	long base;
	int i, m;

	G_dbgflag = 0;
	compile_date[11] = 0;		/* overwrite LF by 0 = end of string */
	compile_time [9] = 0;
	Version[strlen(Version)-1] = 0;

	cprintf("V%s %s%s", &Version[11], compile_date, compile_time);

	/* allocate static table */
	s = (struct icv196T_s *) sysbrk(sizeof(*s));
	if (!s) {
		cprintf("\nicv196@%s() no room for static table,"
			" INSTALLATION IMPOSSIBLE\n", __FUNCTION__);
		return (char *) SYSERR;
	}

	/* Set up the static table of driver */
	s->sem_drvr  = 1; /* to protect global ressources management sequences*/
	s->UserTO    = 6000;   /* default T.O. for waiting Trigger */
	s->UserMode  = 0;    /* Flag no wait on read empty Evt Ring */
	s->UserMode |= ((short)icv_bitwait); /* set flag wait for read = wait */

	/* Initialise user'shandle */
	/* handles to get synchronized with the icv */
	for (i = ICVVME_IcvChan01; i < ICVVME_MaxChan; i++)
		Init_UserHdl(&s->ICVHdl[i], i, s);

	/* Initialize management tables  */
	Init_Dir(s); /* Initialize Directories */

	/*
	  Set up of table associated to devices
	  this depends on the info table which tell which modules are declared
	*/

	/* for each module declared in info table: Set up tables */

	for ( m=0 ; m < icv_ModuleNb; m++) {
		/* Set up pointer to current device table  */
		if (info->ModuleFlag[m] == 0) /* this module non present; */
			continue;

		MInfo = &info->ModuleInfo[m]; /* set up the current Module info */

		/* Set up of the current module tables */
		/* Check info parameters */
		base = MInfo->base;

		/* check if base address conform to hardware strapping */
		if ( ((base & 0xff000000) != 0)
		     || ((base & 0x000000ff) != 0) ) {
			cprintf("\n");
			cprintf ("icv196:install: base address out of range !.. %lx \n",base);
			cprintf("icv196:install: INSTALLATION IMPOSSIBLE\n");
			pseterr(EFAULT);
			return (char *) SYSERR;
		}

		/*  Only one vector and one level for the 8 lines of the module */
		v = MInfo->vector[0];

		if ((v < USER_VBASE) || (v > 255)) { /* USER_VBASE from interrupt.h */
			cprintf("\n");
			cprintf("icv196vmeinstall: vector value out of range !.. %d \n",v);
			cprintf("icv196:install: INSTALLATION IMPOSSIBLE\n");
			pseterr(EFAULT);
			return (char *) SYSERR;
		}
		l = MInfo->level[0];

		if ( (l > 5) || (l < 1) ) {
			cprintf("\n");
			cprintf("icv196vmeinstall: Int. level out of range must be in [2..5] !.. %d \n", l);
			cprintf("icv196:install: INSTALLATION IMPOSSIBLE\n");
			pseterr(EFAULT);
			return (char *) SYSERR;
		}

		/* Set up the tables of the  current Module */
		MCtxt = Init_ModuleCtxt(s, m, MInfo);

		/*  Update Module directory */
		s->ModuleCtxtDir[m] = MCtxt;
	}

	/*
	  Now setup devices
	  driven by the device directory  which tell which modules have been declared
	*/

	for ( m = 0; m < icv_ModuleNb; m++) {
		if ( (MCtxt = s->ModuleCtxtDir[m]) == NULL)
			continue;

		/* Startup the hardware for that module context */
		if (icvModule_Startup(MCtxt) < 0)
			MCtxt = s->ModuleCtxtDir[m] = 0;
	}

	cprintf("\rDrivOK:\n");
	return (char *) s;
}

/* Uninstalling the driver */
int icv196uninstall(struct icv196T_s *s)
{
	struct T_ModuleCtxt *MCtxt;
	unsigned char v;
	int m;

	for (m = 0 ; m < icv_ModuleNb; m++) {
		if ( (MCtxt = s->ModuleCtxtDir[m]) == NULL)
			continue;

		v = MCtxt->Vect[0];
		IOINTCLR(v);
	}

	sysfree((char *)s, sizeof(s));
	return OK;
}

/* This routine is called to open a service access point for the user */
int icv196open(struct icv196T_s *s, int dev, struct cdcm_file *f)
{
  int   chan;
  short   DevType;
  register struct T_UserHdl *UHdl;

/*  DBG (("icvvme: Open on device= %lx \n", (long) dev)); */
  chan = minor(dev);
/*  DBG (("icv196:Open: on minor device= %d \n", (int) chan)); */

  if ( (s == (struct icv196T_s *) NULL) || ( s == (struct icv196T_s *)(-1))) {
/*    DBG (("icvvme:Install wrong: static pointer is = %lx \n", (int) s));*/
	  pseterr(EACCES);
	  return SYSERR;
  }

  UHdl = NULL;
  DevType = 0;
  if (chan == ICVVME_ServiceChan) {
    /* DBG (("icvvme:Open:Handle for icv services \n"));*/
    UHdl = &(s->ServiceHdl);
    DevType = 1;
  } else {
    if ((chan >= ICVVME_IcvChan01) && (chan <= ICVVME_MaxChan)) {
      /* DBG (("icvvme:Open:Handle for synchro with ICV lines \n"));*/
      if (f->access_mode & FWRITE) {
	pseterr(EACCES);
	return SYSERR;
      }
      UHdl = &s->ICVHdl[chan];
    } else {
      pseterr(EACCES);
      return SYSERR;
    }
  }

  if ((DevType == 0) && (UHdl->usercount)) { /* synchro channel already open */
    pseterr(EACCES);
    return SYSERR;
  }

  /* Perform the open */
  swait(&s->sem_drvr, IGNORE_SIG);
  if (DevType == 0) { /* Case synchro */
    Init_UserHdl(UHdl, chan, s);
    UHdl->UserMode = s->UserMode;
    UHdl->WaitingTO = s->UserTO;
    UHdl->pid = getpid();

    /* DBG (("icvvme:open: Chanel =%d UHdl=$%lx\n", chan, UHdl ));*/
  }

  ++(s->usercounter); /* Update user counter value */
  UHdl->usercount++;
  ssignal( &s->sem_drvr);
  return OK;
}

/*
 This routine is called to close the devices access point
*/
int icv196close(struct icv196T_s *s, struct cdcm_file *f)
{
  int   chan;
  short   DevType;
  register struct T_UserHdl *UHdl;

  chan = minor(f->dev);
  UHdl = NULL;
  DevType = 0;
  if (chan == ICVVME_ServiceChan) {
/*   DBG (("icvvme:Close:Handle for icv services\n"));*/
    UHdl = &(s->ServiceHdl);
    DevType = 1;
  } else {
    if ((chan >= ICVVME_IcvChan01) && (chan <= ICVVME_MaxChan)) {
    /* DBG (("icvvme:Close:Handle for synchro with Icv \n"));*/
      UHdl = &s->ICVHdl[chan];
    } else {
      pseterr(EACCES);
      return SYSERR;
    }
  }

  /* Perform the close */
  swait(&s->sem_drvr, IGNORE_SIG);
  if (DevType == 0) { /* case of Synchro handle */
	  /*    DBG (("icv196:close: Chanel =%d UHdl=%lx \n",chan, UHdl));*/
	  ClrSynchro(UHdl); /* Clear connection established*/
  }
  --(s->usercounter); /* Update user counter value */
  --UHdl->usercount;
  ssignal(&s->sem_drvr);

  return OK;
}

int icv196read(struct icv196T_s *s, struct cdcm_file *f, char *buff, int bcount)
{
	int   count, Chan;
  struct T_UserHdl  *UHdl;
  long   *buffEvt;
  struct T_ModuleCtxt *MCtxt;
  int   i,m;
  long  Evt;
  int bn;
  ulong ps;
  int Line;

  /* Check parameters and Set up channel environnement */
  if (wbounds ((int)buff) == EFAULT) {
    /* DBG (("icv:read wbound error \n" ));*/
    pseterr(EFAULT);
    return SYSERR;
  }

  Chan = minordev(f->dev);
  if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
    UHdl = &s->ICVHdl[Chan];	/* ICV event handle */
  } else {
	  pseterr(ENODEV);
	  return SYSERR;
  }

  if (UHdl == NULL) { /* Abnormal status system corruption */
    pseterr(EWOULDBLOCK);
    return SYSERR;
  }

  /* processing of the read */
  /* Case of Reading the event buffer */
  /* Event are long and so count should be multiple of 4 bytes */
  if ((count = (bcount >> 2)) <= 0) {
      pseterr(EINVAL);
      return SYSERR;
  }

  /* BEWARE !!!!
     alignment constraint for the buffer
     should be satisfied by caller  !!!!!
  */
    buffEvt = (long *)buff;
    bn = 0;
    i = count; /* Long word count */
    /* DBG (("icv196:read:Entry Evtsem value= %d\n", scount( &UHdl -> Ring.Evtsem) )); */
    do {
      if ((Evt = PullFrom_Ring ( &UHdl->Ring)) != -1) {
	      *buffEvt = Evt;
	      buffEvt++; i--; bn += 1;
      } else { /* no event in ring, wait if requested */
	      if ((bn == 0) && ((UHdl->UserMode & icv_bitwait))) { /* Wait for Lam */
		      /* Contro waiting by t.o. */
		      UHdl->timid = 0;
		      UHdl->timid = timeout(UserWakeup, (char *)UHdl, (int)UHdl->WaitingTO);
		      swait(&UHdl->Ring.Evtsem, -1);	/* Wait Lam or Time Out */

		      disable(ps);
		      if (UHdl->timid < 0 ) {	/* time Out occured */
			      restore(ps);

			      /* check if any module setting got reset */
			      for (m = 0; m < icv_ModuleNb; m++) {
				      /*  Update Module directory */
				      MCtxt = s->ModuleCtxtDir[m];
				      if ( MCtxt != NULL) {
					      Line = 0;
					      icvModule_Reinit(MCtxt, Line);
				      }
			      }

			      pseterr(ETIMEDOUT);
			      return SYSERR;
		      }

		      cancel_timeout(UHdl->timid);
		      UHdl->Ring.Evtsem += 1;	/* to remind this event for swait by reading */
		      restore(ps);
	      } else {
		      break;
	      }
      }
    } while (i > 0); /* while  room in buffer */

    return (bn << 2);
}

/* not available */
int icv196write(struct icv196T_s *s, struct cdcm_file *f, char *buff,int bcount)
{
	/*   DBG (("icvdrvr: write: no such facility available \n"));*/
	return (bcount = s->usercounter);
}

/* This routine is called to perform ioctl function */
int icv196ioctl(struct icv196T_s *s, struct cdcm_file *f, int fct, char *arg)
{
  int   err;
  struct icv196T_ModuleInfo *Infop;
  register int  Timeout;
  register int  i,j,l;
  int   Chan;
  short group;
  short index;
  int   mode;
  int   SubsMode;
  struct T_UserHdl  *UHdl;
  struct T_ModuleCtxt *MCtxt;
  struct T_LineCtxt   *LCtxt;
  struct T_LogLineHdl *LHdl;
  struct T_Subscriber *Subs;
  struct icv196T_HandleInfo *HanInfo;
  struct icv196T_HandleLines *HanLin;
  struct icv196T_UserLine ULine;
  long   Module;
  short  Line;
  int    Type;
  int    LogIx;
  int    grp;
  int    dir;
  int    group_mask;
  unsigned long *Data;
  int Iw1;
  int Flag;

  err = 0;
  UHdl = NULL;
  DBG_IOCTL(("icv196:ioctl: function code = %x \n", fct));


  if ( (rbounds((int)arg) == EFAULT) || (wbounds((int)arg) == EFAULT)) {
    pseterr (EFAULT);
    return (SYSERR);
  }
  Chan = minordev (f -> dev);

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
	    MCtxt = &(s -> ModuleCtxt[i]);
	    Infop -> ModuleFlag = 1;
	    Infop -> ModuleInfo.base = (unsigned long)MCtxt -> SYSVME_Add;
	    Infop -> ModuleInfo.size = MCtxt -> VME_size;
	    for ( j = 0; j < icv_LineNb; j++) {
		Infop -> ModuleInfo.vector[j] = MCtxt -> Vect[j];
		Infop -> ModuleInfo.level[j] = MCtxt -> Lvl[j];
	    }
	}
    };
    break;



  case ICVVME_gethandleinfo:

    HanInfo = (struct icv196T_HandleInfo *)arg;
    UHdl = s -> ICVHdl;           /* point to first user handle */
    for ( i = 0; i < ICVVME_MaxChan; i++, UHdl++) {
	HanLin = &(HanInfo -> handle[i]);
	l = 0;
	if (UHdl -> usercount == 0)
	    continue;
	else {
	    HanLin -> pid = UHdl -> pid;
	    for ( j = 0; j < ICV_LogLineNb; j++) {
		if (TESTBIT(UHdl -> Cmap, j)) {
		    GetUserLine(j, &ULine);
		    HanLin -> lines[l].group = ULine.group;
		    HanLin -> lines[l].index = ULine.index;
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
    Flag = G_dbgflag;
    if ( *( (int *) arg) != (-1)){ /* update the flag if value  not = (-1) */
      G_dbgflag = *( (int *) arg);
    }
    *( (int *) arg) = Flag;	/* give back old value */
    break;


/*                              ioctl icvvme set reenable flag
				==============================
    Set reenable flag to allow continuous interrupts
*/
  case ICVVME_setreenable:

      /* Check channel number and Set up Handle pointer */
    if (Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
      UHdl = &(s -> ICVHdl[Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv:clearreenable: Entry:UHdl= %lx \n", (long) UHdl));*/

    if (UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    group = ((struct icv196T_UserLine *) arg) -> group;
    index = ((struct icv196T_UserLine *) arg) -> index;

    if (s -> ModuleCtxtDir[group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((group < 0) || (group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",group, Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((index < 0) || (index > (icv_LineNb -1) )) {
/*      DBG (("icv:setreenable: line index= %d out of range on channel %d\n",index,  Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    LogIx = CnvrtUserLine( (char) group,(char) index );

    /* DBG (("icv:setreenable: request on group %d index %d, associated  log line %d\n",group, index, LogIx )); */

    if (LogIx < 0 || LogIx > ICV_LogLineNb){
/*      DBG (("icv:setreenable: LogIndex outside [0..%d] for channel %d\n", ICV_LogLineNb, Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    if ((LHdl = s -> LineHdlDir[LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv:setreenable: %d no such log line in config, for channel %d \n", LogIx, Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    LCtxt = LHdl -> LineCtxt;
    if ( (LCtxt -> LHdl) != LHdl){
/*      DBG (("icv:setreenable: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", LHdl, (LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    LCtxt -> intmod = icv_ReenableOn;
    break;



/*                              ioctl icvvme clear reenable flag
				==================================
    Clear reenable: after an interrupt, further interrupts are inhibited
		    until the line is enabled again
*/
  case ICVVME_clearreenable:

      /* Check channel number and Set up Handle pointer */
    if (Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
      UHdl = &(s -> ICVHdl[Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv:clearreenable: Entry:UHdl= %lx \n", (long) UHdl));*/

    if (UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    group = ((struct icv196T_UserLine *) arg) -> group;
    index = ((struct icv196T_UserLine *) arg) -> index;

    if (s -> ModuleCtxtDir[group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((group < 0) || (group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",group, Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((index < 0) || (index > (icv_LineNb -1) )) {
/*      DBG (("icv:clearreenable: line index= %d out of range on channel %d\n",index,  Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    LogIx = CnvrtUserLine( (char) group,(char) index );

    /* DBG (("icv:clearreenable: request on group %d index %d, associated  log line %d\n",group, index, LogIx )); */

    if (LogIx < 0 || LogIx > ICV_LogLineNb){
/*      DBG (("icv:clearreenable: LogIndex outside [0..%d] for chanel %d\n", ICV_LogLineNb, Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    if ((LHdl = s -> LineHdlDir[LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv:clearreenable: %d no such log line in config, for chanel %d \n", LogIx, Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    LCtxt = LHdl -> LineCtxt;
    if ( (LCtxt -> LHdl) != LHdl){
/*      DBG (("icv:clearreenable: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", LHdl, (LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    LCtxt -> intmod = icv_ReenableOff;
    break;


/*                              ioctl icvvme disable interrupt
				==============================
    interrupts on the given logical line are disabled
*/
  case ICVVME_disable:

      /* Check channel number and Set up Handle pointer */
    if (Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
      UHdl = &(s -> ICVHdl[Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv:disableint: Entry:UHdl= %lx \n", (long) UHdl));*/

    if (UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    group = ((struct icv196T_UserLine *) arg) -> group;
    index = ((struct icv196T_UserLine *) arg) -> index;

    if (s -> ModuleCtxtDir[group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((group < 0) || (group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",group, Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((index < 0) || (index > (icv_LineNb -1) )) {
/*      DBG (("icv:disableint: line index= %d out of range on channel %d\n",index,  Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    LogIx = CnvrtUserLine( (char) group,(char) index );

    /* DBG (("icv:disableint: request on group %d index %d, associated  log line %d\n",group, index, LogIx )); */

    if (LogIx < 0 || LogIx > ICV_LogLineNb){
/*      DBG (("icv:disableint: LogIndex outside [0..%d] for chanel %d\n", ICV_LogLineNb, Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    if ((LHdl = s -> LineHdlDir[LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv:disableint: %d no such log line in config, for chanel %d \n", LogIx, Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    LCtxt = LHdl -> LineCtxt;
    if ( (LCtxt -> LHdl) != LHdl){
/*      DBG (("icv:disableint: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", LHdl, (LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    disable_Line(LCtxt);
    break;


/*                              ioctl icvvme enable interrupt
				=============================
    interrupts on the given logical line are enabled
*/
  case ICVVME_enable:

      /* Check channel number and Set up Handle pointer */
    if (Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
      UHdl = &(s -> ICVHdl[Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv:disableint: Entry:UHdl= %lx \n", (long) UHdl));*/

    if (UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    group = ((struct icv196T_UserLine *) arg) -> group;
    index = ((struct icv196T_UserLine *) arg) -> index;

    if (s -> ModuleCtxtDir[group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((group < 0) || (group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",group, Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((index < 0) || (index > (icv_LineNb -1) )) {
/*      DBG (("icv:disableint: line index= %d out of range on channel %d\n",index,  Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    LogIx = CnvrtUserLine( (char) group,(char) index );

    /* DBG (("icv:disableint: request on group %d index %d, associated  log line %d\n",group, index, LogIx )); */

    if (LogIx < 0 || LogIx > ICV_LogLineNb){
/*      DBG (("icv:disableint: LogIndex outside [0..%d] for chanel %d\n", ICV_LogLineNb, Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    if ((LHdl = s -> LineHdlDir[LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv:disableint: %d no such log line in config, for chanel %d \n", LogIx, Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    LCtxt = LHdl -> LineCtxt;
    if ( (LCtxt -> LHdl) != LHdl){
/*      DBG (("icv:disableint: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", LHdl, (LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }

    enable_Line(LCtxt);
    break;



/*                              ioctl icvvme read interrupt counters
				====================================
    Read interrupt counters for all lines in the given module
*/
  case ICVVME_intcount:

    Module = (long)((struct icv196T_Service *) arg) -> module;
    Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    MCtxt =  &(s -> ModuleCtxt[Module]);
    LCtxt =  MCtxt -> LineCtxt;
    for (i = 0; i < icv_LineNb; i++, LCtxt++)
      *Data++ = LCtxt -> loc_count;
    break;



/*                              ioctl icvvme read reenable flags
				================================
    Read reenable flags for all lines in the given module
*/
  case ICVVME_reenflags:

    Module = (long)((struct icv196T_Service *) arg) -> module;
    Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    MCtxt =  &(s -> ModuleCtxt[Module]);
    LCtxt =  MCtxt -> LineCtxt;
    for (i = 0; i < icv_LineNb; i++, LCtxt++)
      *Data++ = LCtxt -> intmod;
    break;




/*                              ioctl icvvme read interrupt enable mask
				=======================================
    Read interrupt enable mask for the given module
*/
  case ICVVME_intenmask:

    Module = (long)((struct icv196T_Service *) arg) -> module;
    Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    MCtxt =  &(s -> ModuleCtxt[Module]);
    *Data = MCtxt -> int_en_mask;
    break;




/*                              ioctl icvvme read io semaphores
				===============================
    Read io semaphores for all lines in the given module
*/
  case ICVVME_iosem:

    Module = (long)((struct icv196T_Service *) arg) -> module;
    Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    MCtxt =  &(s -> ModuleCtxt[Module]);
    LCtxt =  &(MCtxt -> LineCtxt[0]);
    for (i = 0; i < icv_LineNb; i++, LCtxt++) {
	LHdl = LCtxt -> LHdl;
	Subs = &(LHdl -> Subscriber[0]);
	for (j = 0; j < LHdl -> SubscriberCurNb; j++, Subs++) {
	    if (Subs -> Ring != NULL) {
		if ((j = 0))
		    Data++;
		continue;
	    }
	    *Data++ = Subs -> Ring -> Evtsem;
	}
    }
    break;

/*                              ioctl icvvme set io direction
				===============================
	     ICVVME_setio: set direction of input/output ports.
	     (when using the I/O part of the ICV196 board)
*/
  case ICVVME_setio:

    Module = (long)((struct icv196T_Service *) arg) -> module;
    Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    MCtxt =  &(s -> ModuleCtxt[Module]);
    grp = *Data++;
    dir = *Data;

		if ( grp < 0 || grp > 11)
		    return(SYSERR);

		group_mask = 1 << grp;
		if (dir) {            /* dir >< 0 : output */
		    *(MCtxt -> VME_CsDir) =
		    MCtxt -> old_CsDir | group_mask;
		    MCtxt -> old_CsDir =
		    MCtxt -> old_CsDir | group_mask;
		}
		else {                     /* dir = 0: input  */
		    *(MCtxt -> VME_CsDir) =
		    MCtxt -> old_CsDir & ~group_mask;
		    MCtxt -> old_CsDir =
		    MCtxt -> old_CsDir & ~group_mask;
		}
  break;


/*                              ioctl icvvme read io direction
				===============================
	     ICVVME_setio: read direction of input/output ports.
	     (when using the I/O part of the ICV196 board)
*/
  case ICVVME_readio:

    Module = (long)((struct icv196T_Service *) arg) -> module;
    Data =   ((struct icv196T_Service *)arg) -> data;
    if (s -> ModuleCtxtDir[Module] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }
    MCtxt =  &(s -> ModuleCtxt[Module]);
    *Data = MCtxt -> old_CsDir;
  break;

/* ioctl wait/nowait flag control:  no wait
   =========================================
   to set nowait mode on the read function
*/
  case ICVVME_nowait:

      /* Check channel number and Set up Handle pointer */
    if (Chan == 0) {		/* no such function on channel 0 */
      pseterr (EACCES);
      return (SYSERR);
    };
    if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
      UHdl = &(s -> ICVHdl[Chan]);
    }
    else {
      pseterr (ENODEV);
      return (SYSERR);
    }
    if (UHdl == NULL) {		/* system corruption */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }
    UHdl->UserMode &= (~((short) icv_bitwait)); /* reset flag wait */

/*     DBG (("icvdrvr:ioctl:reset wait flag UserMode now= %lx \n", UHdl -> UserMode ));*/

    break;
/*
                                ioctl wai/nowait flag control:     wait
                                ===========================================
   to set nowait mode on the read function
*/
  case ICVVME_wait:

      /* Check channel number and Set up Handle pointer */
    if (Chan == 0) {		/* no such function on channel 0 */
      pseterr (EACCES);
      return (SYSERR);
    };
    if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
      UHdl = &(s -> ICVHdl[Chan]);/* LAM handle */
    }
    else {
      pseterr (ENODEV);
      return (SYSERR);
    }
    if (UHdl == NULL) {		/* system corruption !!! ...*/
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }
    UHdl->UserMode |= ((short)icv_bitwait); /* set flag wait */

/*    DBG (("icv196:ioctl:set wait flag UserMode now= %lx \n", UHdl -> UserMode ));*/

    break;


/*
                        ioctl function: to set Time out for waiting Event
                        ================================================
   to monitor duration of wait on interrupt event before time out  declared
*/
  case ICVVME_setupTO:


      /* Check channel number and Set up Handle pointer */
      if (Chan == 0) {		/* no such function on channel 0 */
	pseterr (EACCES);
	return (SYSERR);
      };
    if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
      UHdl = &(s -> ICVHdl[Chan]);/* LAM handle */
    }
    else {
      pseterr (ENODEV);
      return (SYSERR);
    }
    if (UHdl == NULL) {		/* system corruption */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }
    if ( ( *( (int *) arg)) < 0 ){
      pseterr (EINVAL); return (SYSERR);
    }
    Timeout = UHdl -> WaitingTO; /* current value     */
    Iw1 = *( (int *) arg);	    /* new value         */
    UHdl -> WaitingTO =  Iw1;    /* set T.O.          */
    *( (int *) arg) = Timeout;    /* return old value  */

/*      DBG (("icv196:setTO: for chanel= %d oldTO= %d newTO= %d in 1/100 s\n",Chan, Timeout, Iw1 ));*/

    break;
/* ICVVME_setTO */

/*				connect function
				================
*/

  case ICVVME_connect:

    err = 0;
      /* Check channel number and Set up Handle pointer */
    if (Chan == 0) {		/* no connect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
      UHdl = &(s -> ICVHdl[Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    /* DBG (("icv196:connect: Entry:UHdl= %lx \n", (long) UHdl));*/

    if (UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    }

		/* Check parameters and Set up environnement */

    group = ((struct icv196T_connect *) arg) -> source.field.group;
    index = ((struct icv196T_connect *) arg) -> source.field.index;
    mode = ((struct icv196T_connect *) arg) -> mode;
   /* DBG (("icv196:connect: param = group = %d Line %d Mode %d \n", group, index, mode));   */
    if (mode & (~(icv_cumul | icv_disable))){
/*      DBG (("icv:connect: mode out of range on channel %d\n", Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }

    if (s -> ModuleCtxtDir[group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }

   if ((group < 0) || (group > (icv_ModuleNb - 1))) {
/*      DBG (("icv:connect: module group= %d out of range on channel %d\n",group, Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ((index < 0) || (index > (icv_LineNb -1) )) {
/*      DBG (("icv:connect: line index= %d out of range on channel %d\n",index,  Chan));*/
      pseterr (EINVAL); return (SYSERR);
    }
           /* set up Logical line handle pointer */
    LogIx = CnvrtUserLine( (char) group,(char) index );

    /* DBG (("icv196:connect: request on group %d index %d, associated  log line %d\n",group, index, LogIx )); */

    if (LogIx < 0 || LogIx > ICV_LogLineNb){
/*      DBG (("icv196:connect: LogIndex outside [0..%d] for chanel %d\n", ICV_LogLineNb, Chan ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }
    if ((LHdl = s -> LineHdlDir[LogIx]) == NULL){ /* Line not in config*/
/*      DBG (("icv196:connect: %d no such log line in config, for chanel %d \n", LogIx, Chan ));*/
      pseterr (EINVAL); return (SYSERR);
    }
    LCtxt = LHdl -> LineCtxt;
    if ( (LCtxt -> LHdl) != LHdl){
/*      DBG (("icv196:connect: computed LHdl $%lx and LLCtxt -> LHdl $%lx mismatch \n", LHdl, (LCtxt -> LHdl) ));*/
      pseterr (EWOULDBLOCK); return(SYSERR);
    }
/*  DBG (("icv196:connect: checking if connection already exists\n")); */
    if  ( TESTBIT(UHdl -> Cmap, LogIx) ) {
      if (LCtxt -> Reset) {
	enable_Line(LCtxt);
/*      DBG (("icv196:connect: line %d enabled\n", Chan));            */
      }
      err = 0;
/*      DBG (( "icv196:connect: already connected group = %d index = %d Logline =%d\n", group, index, LogIx));*/
      break;			/* already connected ok --->  exit */
    }
    SubsMode = 0;		/* default mode requested */
    if (mode & icv_cumul)
      SubsMode = icv_cumulative;

    if (mode & icv_disable)
      LCtxt -> intmod = icv_ReenableOff;

    MCtxt = LCtxt -> MCtxt;

    Type = LCtxt -> Type;
    Line = LCtxt -> Line;

       /* Reinit module if setting lost !!!!
	  beware !,  all lines loose their current state */

    icvModule_Reinit(MCtxt, Line);

    if ((Subs = LineBooking(UHdl, LHdl, SubsMode)) == NULL){
/*      DBG (("icv196:connect: booking unsuccessfull UHdl $%lx LHdl $%lx\n", UHdl, LHdl ));*/
      pseterr (EUSERS); return (SYSERR);
    }

/*    DBG(("icv196 connect chanel= %d on group = %d, index = %d, mode %d \n", Chan, group, index, mode));*/


      /*  enable line interrupt */
    if (LCtxt -> status == icv_Disabled)
	enable_Line(LCtxt);
    LCtxt -> loc_count = 0;

    break;

/*  ICVVME_connect */

/*
				disconnect function
				===================
*/

  case ICVVME_disconnect:

    err = 0;
      /* Check channel number and Set up Handle pointer */
    if (Chan == 0) {		/* no disconnect on channel 0 */
      pseterr (EACCES); return (SYSERR);
    };
    if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
      UHdl = &(s -> ICVHdl[Chan]);
    }
    else {
      pseterr (ENODEV);  return (SYSERR);
    };
    if (UHdl == NULL) {  /* Abnormal status system corruption   !!!! */
      pseterr (EWOULDBLOCK);
      return (SYSERR);
    };
                 /* Check parameters and Set up environnement */

    group = ((struct icv196T_connect *) arg) -> source.field.group;
    index = ((struct icv196T_connect *) arg) -> source.field.index;

    if (s -> ModuleCtxtDir[group] == NULL) {
      pseterr (EACCES);
      return (SYSERR);
    }


/*      DBG (("icv196:disconnect:chanel= %d from group = %lx, index = %lx \n", Chan,(long) group, (long) index));*/

    if ((group < 0) || (group >= icv_ModuleNb)) {
      pseterr (EINVAL); return (SYSERR);
    }
    if ((index < 0) || (index >= icv_LineNb)) {
      pseterr (EINVAL); return (SYSERR);
    }
    LogIx = CnvrtUserLine( (char) group, (char) index);
           /* set up Logical line handle pointer */
    if ((LHdl = s -> LineHdlDir[LogIx]) == NULL){ /*log line not affected*/
      pseterr (EINVAL); return (SYSERR);
    }
    if ( !(TESTBIT(UHdl -> Cmap, LogIx)) ){ /* non connected */
      pseterr (ENOTCONN); return (SYSERR);
    }
    if ((Subs = LineUnBooking(UHdl, LHdl)) == NULL){
      pseterr (ENOTCONN); return (SYSERR);
    }
			/* Set up line context */
    LCtxt = LHdl -> LineCtxt;

    /* disable the corresponding line */
    if (LHdl -> SubscriberCurNb == 0){
      disable_Line(LCtxt);
      LCtxt -> loc_count = -1;
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
  return (err);
}


int icv196select(struct icv196T_s *s, struct cdcm_file *f, int which, struct sel *se)
{
  int   Chan;
  register struct T_UserHdl *UHdl = NULL;

  Chan = minordev (f -> dev);
  if (Chan == 0) {			/* no select on channel 0 */
    pseterr (EACCES);
    return (SYSERR);
  };
  if (Chan >= ICVVME_IcvChan01 && Chan <= ICVVME_MaxChan) {
    UHdl = &s -> ICVHdl[Chan];	/* general handle */
  }
/*  DBG (("icv196:select:Entry: on channel %lx which = %lx \n", Chan, which ));*/
  switch (which) {
    case SREAD:
      se -> iosem = &(UHdl -> Ring.Evtsem);
      se -> sel_sem = &(UHdl -> sel_sem);
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
    struct T_UserHdl    *UHdl;
    struct T_LineCtxt   *LCtxt;
    struct T_LogLineHdl *LHdl;
    struct T_Subscriber *Subs;
    struct icvT_RingAtom Atom;
    struct icvT_RingAtom *RingAtom;
    int i,j, ns, cs;
    short count;
    unsigned short Sw1 = 0;
    unsigned char     *CtrStat;
    unsigned char   mdev, mask, status;
    static unsigned short input[16];
    struct T_ModuleCtxt *MCtxt = (struct T_ModuleCtxt *)arg;

    s = MCtxt -> s;                       /* common static area */
    m = MCtxt -> Module;
    CtrStat = MCtxt -> VME_StatusCtrl;
    /*DBX (("icv196:isr: s=$%lx MCtxt= $%lx MCtxt.Module= =$%d \n", s, (long)MCtxt,(long)m ));*/

    if ((m <0) || ( m > icv_ModuleNb)){
/*	DBG(("icv:isr: Module context pointer mismatch  \n"));*/
	return SYSERR;
    }

    if ( !(s -> ModuleCtxtDir[m]) ){
/*	DBG (( "icv196:isr: Interrupt from a non declared module \n"));*/
	return SYSERR;
    }

    if (MCtxt -> startflag == 0) {
	for (i = 0; i <= 15; i++) input[i] = 0;
	MCtxt -> startflag = 1;/*reset the input array the first */
    }                              /*time the routine is entered     */

    status = *CtrStat;        /*force board to defined state    */
      PURGE_CPUPIPELINE;
    *CtrStat = CSt_Areg;
      PURGE_CPUPIPELINE;
    status = *CtrStat;        /*read portA's status register    */
      PURGE_CPUPIPELINE;
    /*DBX(("CSt_Areg = 0x%x\n", status));*/

    if (status & CoSt_Ius) {     /*did portA cause the interrupt ? */

	mask = 1;
	*CtrStat = Data_Areg;
          PURGE_CPUPIPELINE;
	mdev = *CtrStat;      /*read portA's data register      */
          PURGE_CPUPIPELINE;
	/*DBG(("Icv196isr:Data_Areg = 0x%x\n", mdev));*/

	for (i = 0; i <= 7; i++) {

	    if (mdev & mask & MCtxt -> int_en_mask)
		input[i] = 1;   /* mark port A's active interrupt  */
	    mask <<= 1;         /* lines                           */
	}
    }

    *CtrStat = CSt_Breg;
      PURGE_CPUPIPELINE;
    status = *CtrStat;        /*read portB's status register    */
      PURGE_CPUPIPELINE;
    /*DBX(("CSt_Breg = 0x%x\n", status));*/

    if (status & CoSt_Ius) {     /*did portB cause the interrupt ? */

	mask = 1;
	*CtrStat = Data_Breg;
          PURGE_CPUPIPELINE;
	mdev = *CtrStat;      /*read portB's data register      */
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
	    LCtxt = &(MCtxt -> LineCtxt[i]);
	    LCtxt -> loc_count++;

	    LHdl = LCtxt -> LHdl;
		      /* Build the Event */
	    Atom.Evt.All = LHdl -> Event_Pattern.All;
			     /*  scan the subscriber table to send them the event */
	    ns = LHdl -> SubscriberMxNb;
	    cs = LHdl -> SubscriberCurNb;
	    Subs= &(LHdl -> Subscriber[0]);
	    for (j= 0 ; ((j < ns) && (cs >0) ) ; j++, Subs++){

	      if (Subs -> Ring == NULL) /* subscriber passive */
		continue;
	      cs--;   /* nb of subbscribers to find out in Subs table*/
	      UHdl = (Subs -> Ring) -> UHdl;
	      /*DBX(("icv:isr:UHdl=$%x j= %d  Subs=$%x\n", UHdl, j, Subs));*/
	      /*DBX(("*Ring$%x Subs->Counter$%x\n",Subs->Ring,Subs->EvtCounter));*/

	      if (( count = Subs -> EvtCounter) != (-1)){
		Sw1 = (unsigned short) count;
		Sw1++;
		if ( ( (short)Sw1 ) <0 )
		  Sw1=1;

		Subs -> EvtCounter = Sw1; /* keep counter positive */
	      }
		      /* give the event according to subscriber status  */
	      if (Subs -> mode  == icv_queuleuleu){ /* mode a la queueleuleu*/

		Atom.Subscriber = NULL;
		Atom.Evt.Word.w1  = Sw1;     /* set up event counter */
		RingAtom = PushTo_Ring(Subs -> Ring, &Atom);
	      }
	      else{                                     /* cumulative mode */
		if (count <0){      /* no event in Ring */

		  Subs -> EvtCounter = 1; /* init counter of Subscriber */
		  Atom.Subscriber = Subs; /* Link event to subscriber */
		  Atom.Evt.Word.w1  = 1;     /* set up event counter */
		  RingAtom = PushTo_Ring(Subs -> Ring, &Atom);
		  Subs -> CumulEvt = RingAtom; /* link to cumulative event, used to disconnect*/
		}
	      }
	      /* process case user stuck on select semaphore  */
	      if (UHdl -> sel_sem)
		ssignal(UHdl -> sel_sem);

	    } /* for/subscriber */
	    /* Enable the line before leaving */

	    if (LCtxt -> intmod == icv_ReenableOff )
	      disable_Line(LCtxt);
	    /*break;  */
	} /* if line active  */

    } /*  for / line */
    *CtrStat = CSt_Breg;       /*clear IP bit on portB  */
      PURGE_CPUPIPELINE;
    *CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;
    *CtrStat = CSt_Areg;       /*clear IP bit on portA  */
      PURGE_CPUPIPELINE;
    *CtrStat = CoSt_ClIpIus;
      PURGE_CPUPIPELINE;

    /*DBX (("icv:isr: end\n"));*/
      return OK;
}


#if 0
struct dldd entry_points = {
	icv196open, icv196close,
	icv196read, icv196write,
	icv196select, icv196ioctl,
	icv196install, icv196uninstall
};
#endif
