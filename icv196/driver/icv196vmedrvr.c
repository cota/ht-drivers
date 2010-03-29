/**
 * @file icv196vmedrvr.c
 *
 * @brief Driver for managing icv196vme module providing 16 interrupt lines
 *
 * Created on 21-mar-1992 Alain Gagnaire, F. Berlin
 *
 * @author Copyright (C) 2009-2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 18/11/2009
 *
 * See "Z8636 CIO Counter/Timer and Parallel I/O Unit" manual for more
 * details on Z8636 controller.
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

#define CPRNT(a)  cprintf a
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

static char *Version       = "4.0";
static char compile_date[] = __DATE__;
static char compile_time[] = __TIME__;
static int  G_dbgflag      = 0;

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
 used as trigger source
*/

/* structure to store for one module the logical lines
   corresponding to the physical lines */
struct T_ModuleLogLine {
	int   Module_Index;
	short LogLineIndex[icv_LineNb];
};

/* table to link for each possible module:
   actual phys line and logical unit index
*/
struct T_ModuleLogLine Module1 = {
	0,
	{
		ICV_L101, ICV_L102, ICV_L103, ICV_L104,
		ICV_L105, ICV_L106, ICV_L107, ICV_L108,
		ICV_L109, ICV_L110, ICV_L111, ICV_L112,
		ICV_L113, ICV_L114, ICV_L115, ICV_L116,
	}
};

struct T_ModuleLogLine Module2 = {
	1,
	{
		ICV_L201, ICV_L202, ICV_L203, ICV_L204,
		ICV_L205, ICV_L206, ICV_L207, ICV_L208,
		ICV_L209, ICV_L210, ICV_L211, ICV_L212,
		ICV_L213, ICV_L214, ICV_L215, ICV_L216,
	}
};

struct T_ModuleLogLine Module3 = {
	2,
	{
		ICV_L301, ICV_L302, ICV_L303, ICV_L304,
		ICV_L305, ICV_L306, ICV_L307, ICV_L308,
		ICV_L309, ICV_L310, ICV_L311, ICV_L312,
		ICV_L313, ICV_L314, ICV_L315, ICV_L316,
	}
};

struct T_ModuleLogLine Module4 = {
	3,
	{
		ICV_L401, ICV_L402, ICV_L403, ICV_L404,
		ICV_L405, ICV_L406, ICV_L407, ICV_L408,
		ICV_L409, ICV_L410, ICV_L411, ICV_L412,
		ICV_L413, ICV_L414, ICV_L415, ICV_L416,
	}
};

struct T_ModuleLogLine Module5 = {
	4,
	{
		ICV_L501, ICV_L502, ICV_L503, ICV_L504,
		ICV_L505, ICV_L506, ICV_L507, ICV_L508,
		ICV_L509, ICV_L510, ICV_L511, ICV_L512,
		ICV_L513, ICV_L514, ICV_L515, ICV_L516,
	}
};

struct T_ModuleLogLine Module6 = {
	5,
	{
		ICV_L601, ICV_L602, ICV_L603, ICV_L604,
		ICV_L605, ICV_L606, ICV_L607, ICV_L608,
		ICV_L609, ICV_L610, ICV_L611, ICV_L612,
		ICV_L613, ICV_L614, ICV_L615, ICV_L616,
	}
};

struct T_ModuleLogLine Module7 = {
	6,
	{
		ICV_L701, ICV_L702, ICV_L703, ICV_L704,
		ICV_L705, ICV_L706, ICV_L707, ICV_L708,
		ICV_L709, ICV_L710, ICV_L711, ICV_L712,
		ICV_L713, ICV_L714, ICV_L715, ICV_L716,
	}
};

struct T_ModuleLogLine Module8 = {
	7,
	{
		ICV_L801, ICV_L802, ICV_L803, ICV_L804,
		ICV_L805, ICV_L806, ICV_L807, ICV_L808,
		ICV_L809, ICV_L810, ICV_L811, ICV_L812,
		ICV_L813, ICV_L814, ICV_L815, ICV_L816,
	}
};

/* table of the maximum configuration: 4 modules */
struct T_ModuleLogLine *MConfig[icv_ModuleNb] = {
	&Module1, &Module2, &Module3, &Module4,
	&Module5, &Module6, &Module7, &Module8
};

/* table to link logical index and user line address */
struct icv196T_UserLine UserLineAdd[ICV_LogLineNb] = {
	/* group #0 */
       {0, ICV_Index00}, {0, ICV_Index01},
       {0, ICV_Index02}, {0, ICV_Index03},
       {0, ICV_Index04}, {0, ICV_Index05},
       {0, ICV_Index06}, {0, ICV_Index07},
       {0, ICV_Index08}, {0, ICV_Index09},
       {0, ICV_Index10}, {0, ICV_Index11},
       {0, ICV_Index13}, {0, ICV_Index14},
       {0, ICV_Index15}, {0, ICV_Index16},

	/* group #1 */
       {1, ICV_Index00}, {1, ICV_Index01},
       {1, ICV_Index02}, {1, ICV_Index03},
       {1, ICV_Index04}, {1, ICV_Index05},
       {1, ICV_Index06}, {1, ICV_Index07},
       {1, ICV_Index08}, {1, ICV_Index09},
       {1, ICV_Index10}, {1, ICV_Index11},
       {1, ICV_Index13}, {1, ICV_Index14},
       {1, ICV_Index15}, {1, ICV_Index16},

	/* group #2 */
       {2, ICV_Index00}, {2, ICV_Index01},
       {2, ICV_Index02}, {2, ICV_Index03},
       {2, ICV_Index04}, {2, ICV_Index05},
       {2, ICV_Index06}, {2, ICV_Index07},
       {2, ICV_Index08}, {2, ICV_Index09},
       {2, ICV_Index10}, {2, ICV_Index11},
       {2, ICV_Index13}, {2, ICV_Index14},
       {2, ICV_Index15}, {2, ICV_Index16},

	/* group #3 */
       {3, ICV_Index00}, {3, ICV_Index01},
       {3, ICV_Index02}, {3, ICV_Index03},
       {3, ICV_Index04}, {3, ICV_Index05},
       {3, ICV_Index06}, {3, ICV_Index07},
       {3, ICV_Index08}, {3, ICV_Index09},
       {3, ICV_Index10}, {3, ICV_Index11},
       {3, ICV_Index13}, {3, ICV_Index14},
       {3, ICV_Index15}, {3, ICV_Index16},

	/* group #4 */
       {4, ICV_Index00}, {4, ICV_Index01},
       {4, ICV_Index02}, {4, ICV_Index03},
       {4, ICV_Index04}, {4, ICV_Index05},
       {4, ICV_Index06}, {4, ICV_Index07},
       {4, ICV_Index08}, {4, ICV_Index09},
       {4, ICV_Index10}, {4, ICV_Index11},
       {4, ICV_Index13}, {4, ICV_Index14},
       {4, ICV_Index15}, {4, ICV_Index16},

	/* group #5 */
       {5, ICV_Index00}, {5, ICV_Index01},
       {5, ICV_Index02}, {5, ICV_Index03},
       {5, ICV_Index04}, {5, ICV_Index05},
       {5, ICV_Index06}, {5, ICV_Index07},
       {5, ICV_Index08}, {5, ICV_Index09},
       {5, ICV_Index10}, {5, ICV_Index11},
       {5, ICV_Index13}, {5, ICV_Index14},
       {5, ICV_Index15}, {5, ICV_Index16},

	/* group #6 */
       {6, ICV_Index00}, {6, ICV_Index01},
       {6, ICV_Index02}, {6, ICV_Index03},
       {6, ICV_Index04}, {6, ICV_Index05},
       {6, ICV_Index06}, {6, ICV_Index07},
       {6, ICV_Index08}, {6, ICV_Index09},
       {6, ICV_Index10}, {6, ICV_Index11},
       {6, ICV_Index13}, {6, ICV_Index14},
       {6, ICV_Index15}, {6, ICV_Index16},

	/* group #7 */
       {7, ICV_Index00}, {7, ICV_Index01},
       {7, ICV_Index02}, {7, ICV_Index03},
       {7, ICV_Index04}, {7, ICV_Index05},
       {7, ICV_Index06}, {7, ICV_Index07},
       {7, ICV_Index08}, {7, ICV_Index09},
       {7, ICV_Index10}, {7, ICV_Index11},
       {7, ICV_Index13}, {7, ICV_Index14},
       {7, ICV_Index15}, {7, ICV_Index16},
};

/* driver static table */
struct icv196T_s {
	int usercounter; /* total user amount conntected to the driver */
	int sem_drvr;    /* semaphore for exclusive access to static table */
	int timid;       /* Interval timer Identification */
	int interval;    /* Timerinterval */

	/* extention to stand interrupt processing */
	int   UserTO;   /* User time out, by waiting for ICV */
	short UserMode; /* user mode flags */

	/* Sizing: depend on needs of user interface */
	short SubscriberMxNb;

	/* User handle: as many as device created at install */
	struct T_UserHdl ICVHdl[SkelDrvrCLIENT_CONTEXTS]; /* user handle */

	/* ring buffer to serialise event coming from the modules */
	struct T_RingBuffer RingGlobal;
	long Buffer_Evt[GlobEvt_nb]; /* To put event in a sequence */

	/* Hardware configuration table */
	int mcntr; /**< counter of an active modules */
	struct T_ModuleCtxt *ModuleCtxtDir[icv_ModuleNb]; /* module directory */
	struct T_ModuleCtxt ModuleCtxt[icv_ModuleNb]; /* Modules contexts */

	/* Logical line tables */
	struct T_LogLineHdl *LineHdlDir[ICV_LogLineNb]; /* Logical line directory */
	struct T_LogLineHdl LineHdl[ICV_LogLineNb]; /* Logical line handles */
} icv196_statics = {
	.sem_drvr = 1, /* to protect global ressources management sequences */
	.UserTO   = 6000, /* default T.O. for waiting Trigger */
	.UserMode = icv_bitwait, /* set flag wait for read = wait */
	.ModuleCtxtDir = { [0 ... icv_ModuleNb-1] = 0 },
	.ModuleCtxt = { [0 ... icv_ModuleNb-1] = { 0 } }
};

/* to come out of waiting event */
static int UserWakeup(void *data)
{
	ulong ps;
	struct T_UserHdl *UHdl = (struct T_UserHdl*)data;

	ssignal(&UHdl->Ring.Evtsem);
	disable(ps);
	if (UHdl->timid >= 0)
		cancel_timeout(UHdl->timid);

	UHdl->timid = -1;
	restore(ps);
	return 0;
}

/* Management of logical/physical Line mapping */
/**
 * @brief Convert a user line address in a logical line index
 *
 * @param module -- module index [0 -- icv_ModuleNb - 1]
 * @param index  -- line index [0 -- 15]
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
static int CnvrtUserLine(short module, short index)
{
	return (int) ((module * icv_LineNb) + index);
}

/**
 * @brief Convert a module line address in a logical index
 *
 * @param midx -- module index [0 - 7]
 * @param lidx -- line index [0 - 15]
 *
 * @return logical line index (one of ICV_Lxxx)
 */
static int CnvrtModuleLine(int midx, int lidx)
{
	return ((int)(( *(MConfig[midx])).LogLineIndex[lidx]));
}

/* Get a user line address from a logical line index */
static void GetUserLine(int logindex, struct icv196T_UserLine *add)
{
	add->group = UserLineAdd[logindex].group;
	add->index = UserLineAdd[logindex].index;
}

/* Initialise Ring buffer descriptor */
static void Init_Ring(struct T_UserHdl *UHdl)
{
	UHdl->Ring.UHdl   = UHdl;
	UHdl->Ring.Buffer = UHdl->Atom;
	UHdl->Ring.Evtsem = 0;
	UHdl->Ring.mask   = Evt_msk;
	UHdl->Ring.reader = UHdl->Ring.writer = 0;

	bzero((char*)UHdl->Atom, ARRAY_SIZE(UHdl->Atom));
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
		Atom = &Ring->Buffer[0];
		Atom->Subscriber = NULL;
		Atom->Evt.All= 0;
		Atom->Evt.Word.w1 = -1;
		Ring->writer++;
		ssignal(&Ring->Evtsem);
		UHdl = Ring->UHdl;
	}

	/* now push the event in the ring */
	I = Ring->writer;
	Atom = &Ring->Buffer[I]; /* Atom to set up */
	Atom->Evt.All    = atom->Evt.All;
	Atom->Subscriber = atom->Subscriber;
	Ring->writer     = (++I & Ring->mask);
	ssignal(&Ring->Evtsem); /* manage ring */

	return Atom;
}

/* Subroutine to pull out an event from the Ring */
static unsigned long PullFrom_Ring(struct T_RingBuffer *Ring)
{
	ulong ps;
	struct icvT_RingAtom *Atom;
	struct T_Subscriber *Subs;
	short  I, ix;
	union icvU_Evt Evt;

	disable(ps); /* Protect seq. from isr access */
	ix = I = Ring->reader;
	if (Ring->Evtsem) { /* Ring non empty */
		swait(&Ring->Evtsem, SEM_SIGIGNORE); /* to manage sem counter */
		Atom = &Ring->Buffer[I]; /* point current atom
					    to pull from ring */
		/* extract the event to be read */
		Subs = Atom->Subscriber;
		Evt.All = Atom->Evt.All;
		Ring->reader = (++I & Ring->mask); /* manage ring buffer */

		/* build the value to return according to mode */
		if (Subs) { /* Case cumulative mode */
			Evt.Word.w1 = Subs->EvtCounter; /* counter from
							   subscriber */

			/* Clear event flag in subscriber context */
			Subs->CumulEvt = NULL; /* no more event in ring */
			Subs->EvtCounter = -1; /* clear event present
						  in the ring */
		}
	} else { /* ring empty */
		Evt.All = -1;
	}

	restore(ps);
	return Evt.All;
}

/* init user Handle */
static void Init_UserHdl(struct T_UserHdl *UHdl, int chanel)
{
	UHdl->chanel  = chanel;
	UHdl->pid     = 0;
	UHdl->inuse   = 0; /* not in use */
	UHdl->sel_sem = NULL; /* clear the semaphore for the select */

	/* clear up bit map of established connection */
	bzero(UHdl->Cmap, ARRAY_SIZE(UHdl->Cmap));

	Init_Ring(UHdl);
}

/* initialize driver statics table */
static void init_statics_once(void)
{
	int i;

	if (!icv196_statics.mcntr) {
		compile_date[11] = 0;
		compile_time[9]  = 0;
		SK_INFO("Vers#%s Compiled@ %s %s",
			 Version, compile_date, compile_time);

		/* Initialise user'shandle */
		for (i = 0; i < SkelDrvrCLIENT_CONTEXTS; i++)
			Init_UserHdl(&icv196_statics.ICVHdl[i], i);
	}
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
}

/* Initialise the subscriber directory of a logical line handle */
static void Init_LineSubscribers(struct T_LogLineHdl *LHdl)
{
	struct T_Subscriber *Subs;
	int i, mode;

	if (LHdl->LineCtxt->Type == icv_FpiLine)
		mode = icv_cumulative; /* default for pls line
					  cumulative mode */
	else
		mode = icv_queuleuleu; /*default for fpi line a la
					 queue leuleu */

	Subs = LHdl->Subscriber;
	for (i = 0; i < LHdl->SubscriberMxNb; i++, Subs++) {
		Subs->LHdl = LHdl;
		Subs->CumulEvt = NULL; /* to prevent active event processing */
		Init_SubscriberHdl(Subs, mode);
	}
}

/* initialise a Line handle associated to a physical line */
static struct T_LogLineHdl *Init_LineHdl(int lli, struct T_LineCtxt *LCtxt)
{
	struct T_LogLineHdl *LHdl;
	union  icv196U_UserLine UserLine;

	LHdl = icv196_statics.LineHdlDir[lli] = &icv196_statics.LineHdl[lli];
	LHdl->LogIndex = lli;
	LHdl->LineCtxt = LCtxt;	/* Link Line handle and Line context */

	/* Get the user line address pattern */
	GetUserLine(lli, (struct icv196T_UserLine *) &UserLine);

	/* build the event pattern for that line */
	LHdl->Event_Pattern.Word.w2 = UserLine.All;
	LHdl->Event_Pattern.Word.w1 = 0;
	LHdl->SubscriberCurNb = 0; /* Current Subscriber nb set to 0 */

	if (LCtxt->Type == icv_FpiLine)
		/* Fpi line only one subscriber allowed */
		LHdl->SubscriberMxNb = 1;
	else
		/* Icv line multi user to wait */
		LHdl->SubscriberMxNb = icv_SubscriberNb;

	Init_LineSubscribers(LHdl);

	return LHdl;
}

/**
 * @brief Establish connection for a channel with a given logical line
 *
 * @param UHdl --
 * @param LHdl --
 * @param mode --
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
static struct T_Subscriber* LineBooking(struct T_UserHdl *UHdl,
					struct T_LogLineHdl *LHdl,
					int mode)
{
	ulong ps;
	int i, ns = LHdl->SubscriberMxNb;
	struct T_Subscriber *Subs = LHdl->Subscriber, *new = NULL;
	struct T_LineCtxt *LCtxt = LHdl->LineCtxt;

	disable(ps);

	for (i = 0; i < ns; i++, Subs++) {
		if (Subs->Ring) /* subscriber occupied, scan on */
			continue;

		new = Subs; /* found free one, take it */
		LHdl->SubscriberCurNb++; /* update current subscriber number */
		Subs->Ring = &UHdl->Ring; /* link to Ring buffer */
		Subs->CumulEvt = NULL;

		/* flag booking of the line in the User handle */
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
	return new;
}

/* Rub out connection of a chanel from a given logical line */
static struct T_Subscriber *LineUnBooking(struct T_UserHdl *UHdl,
					  struct T_LogLineHdl *LHdl)
{
	ulong ps;
	int i, ns = LHdl->SubscriberMxNb;
	struct T_Subscriber *Subs = LHdl->Subscriber;
	struct T_Subscriber *val = NULL;
	struct T_RingBuffer *UHdlRing = &UHdl->Ring;

	disable(ps);

	for (i = 0; i < ns; i++, Subs++) {
		if (!Subs->Ring || Subs->Ring != UHdlRing)
			continue;

		/* subscriber found -- get rid of connection */
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
	struct T_Subscriber *val = NULL;
	struct T_RingBuffer *UHdlRing = &UHdl->Ring;

	disable(ps);
	ns = LHdl->SubscriberMxNb;
	Subs = LHdl->Subscriber;

	for (i = 0; i < ns; i++, Subs++) {
		if (!Subs->Ring || Subs->Ring != UHdlRing)
			continue;

		val = Subs;
		break;
	}

	restore(ps);
	return val;
}
#endif

/* to enable interrupt from a line */
static void enable_Line(struct T_LineCtxt *LCtxt)
{
	ulong ps;
	int locdev;
	unsigned char status, dummy;
	unsigned short mask;
	struct T_ModuleCtxt *MCtxt = LCtxt->MCtxt;

	locdev = LCtxt->Line;

	disable(ps);

	mask = 1 << locdev;
	MCtxt->int_en_mask |= mask; /* set interrupt enable mask */

	if (locdev >= PortA_nln) { /* if locdev is on portB */

		/* enable interrupt on line nr. locdev on portB */
		mask = 1 << (locdev - PortA_nln);

		/* Port B Command & Status reg */
		z8536_wr_val(CSt_Breg, CoSt_ClIpIus); /* Clear IP & IUS bits */

		/* Port B Pattern Polarity reg */
		status = z8536_rd_val(PtrPo_Breg);
		status = status | mask;
		z8536_wr_val(PtrPo_Breg, status);

		/* Port B Pattern Transition reg */
		status = z8536_rd_val(PtrTr_Breg);
		status = status | mask;
		z8536_wr_val(PtrTr_Breg, status);

		/* Port B Pattern Mask reg */
		status = z8536_rd_val(PtrMsk_Breg);
		status = status | mask;
		z8536_wr_val(PtrMsk_Breg, status);

		/* TEST */
		dummy = z8536_rd_val(PtrMsk_Breg);
		dummy = z8536_rd_val(PtrPo_Breg);
		dummy = z8536_rd_val(PtrTr_Breg);

		/* Port B Command & Status reg */
		z8536_wr_val(CSt_Breg, CoSt_ClIpIus); /* Clear IP & IUS bits */
	} else { /* Case port A */

		/* Port A Command & Status reg */
		z8536_wr_val(CSt_Areg, CoSt_ClIpIus); /* Clear IP & IUS bits */

		/* Port A Pattern Polarity reg */
		status = z8536_rd_val(PtrPo_Areg);
		status = status | mask;
		z8536_wr_val(PtrPo_Areg, status);

		/* Port A Pattern Transition reg */
		status = z8536_rd_val(PtrTr_Areg);
		status = status | mask;
		z8536_wr_val(PtrTr_Areg, status);

		/* Port A Pattern Mask reg */
		status = z8536_rd_val(PtrMsk_Areg);
		status = status | mask;
		z8536_wr_val(PtrMsk_Areg, status);

		/* TEST */
		dummy = z8536_rd_val(PtrMsk_Areg);
		dummy = z8536_rd_val(PtrPo_Areg);
		dummy = z8536_rd_val(PtrTr_Areg);

		/* Port A Command & Status reg */
		z8536_wr_val(CSt_Areg, CoSt_ClIpIus); /* Clear IP & IUS bits */
	}

	LCtxt->status = icv_Enabled;
	restore(ps);
}

/* to stop interrupt from this line this is done when user request it */
static void disable_Line(struct T_LineCtxt *LCtxt)
{
	ulong ps;
	int locdev;
	unsigned char status, dummy;
	unsigned short mask;
	struct T_ModuleCtxt *MCtxt = LCtxt->MCtxt;

	locdev = LCtxt->Line;

	disable(ps);

	mask = ~(1 << locdev);
	MCtxt->int_en_mask &= mask; /* clear interrupt enable */

	if (locdev >= PortA_nln) { /* if locdev is on portB */
		mask = ~(1 << (locdev - PortA_nln));

		/*  */
		z8536_wr_val(CSt_Breg, CoSt_ClIpIus);

		/*  */
		status = z8536_rd_val(PtrPo_Breg);
		status = status & mask;
		z8536_wr_val(PtrPo_Breg, status);

		/*  */
		status = z8536_rd_val(PtrTr_Breg);
		status = status & mask;
		z8536_wr_val(PtrTr_Breg, status);

		/*  */
		status = z8536_rd_val(PtrMsk_Breg);
		status = status & mask;
		z8536_wr_val(PtrMsk_Breg, status);

		/* TEST */
		dummy = z8536_rd_val(PtrMsk_Breg);
		dummy = z8536_rd_val(PtrPo_Breg);
		dummy = z8536_rd_val(PtrTr_Breg);

		/*  */
		z8536_wr_val(CSt_Breg, CoSt_ClIpIus);
	} else { /* port#A */

		/*  */
		z8536_wr_val(CSt_Areg, CoSt_ClIpIus);

		/*  */
		status = z8536_rd_val(PtrPo_Areg);
		status = status & mask;
		z8536_wr_val(PtrPo_Areg, status);

		/*  */
		status = z8536_rd_val(PtrTr_Areg);
		status = status & mask;
		z8536_wr_val(PtrTr_Areg, status);

		/*  */
		status = z8536_rd_val(PtrMsk_Areg);
		status = status & mask;
		z8536_wr_val(PtrMsk_Areg, status);

		/* TEST */
		dummy = z8536_rd_val(PtrMsk_Areg);
		dummy = z8536_rd_val(PtrPo_Areg);
		dummy = z8536_rd_val(PtrTr_Areg);

		/*  */
		z8536_wr_val(CSt_Areg, CoSt_ClIpIus);
	}

	LCtxt->status = icv_Disabled;
	restore(ps);
}

/* to get rid of all connections established on a channel
   called at close of channel */
static void ClrSynchro(struct T_UserHdl *UHdl)
{
	ulong ps;
	int i, j;
	struct T_LineCtxt   *LCtxt;
	struct T_LogLineHdl *LHdl;
	struct T_Subscriber *Subs;
	unsigned char w;
	char *cptr;

	if (UHdl->timid >= 0)
		cancel_timeout(UHdl->timid);

	/* scan map byte array */
	for (i = 0, cptr = UHdl->Cmap; i < ICV_mapByteSz; i++, cptr++) {
		if ((w = *cptr & 255) != 0) {
			*cptr = 0; /* Clear bits */
			for (j = 7; j >= 0; j--, w <<= 1 ) {
				/* scan each bit of the current byte */
				if (!w)
					break;
				if ( ((char) w) < 0 ) {	/* line connected */
					if ( (LHdl = icv196_statics.LineHdlDir[(i*8 + j)])
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
					if (!LHdl->SubscriberCurNb)
						disable_Line(LCtxt);

					restore(ps);
				} /* if line connected */
			} /* end for/ byte bit scanning */
		}
	} /* end for/ Cmap byte scanning  */

	sreset(&UHdl->Ring.Evtsem); /* clear semaphore */

	bzero(UHdl->Cmap, ARRAY_SIZE(UHdl->Cmap));
}

/*
  Initialise a line context table of a icv196 module
  table to manage the hardware of a physical line of a module
*/
static void Init_LineCtxt(int line, int type, struct T_ModuleCtxt *MCtxt)
{
	struct T_LineCtxt *LCtxt = &MCtxt->LineCtxt[line];
	int LogIndex;

	LCtxt->MCtxt = MCtxt;
	LCtxt->Line = line;
	LCtxt->Type = type;
	LCtxt->intmod = icv_ReenableOn;
	LCtxt->status = icv_Disabled;
	LCtxt->loc_count = -1;

	/* Link physical line to logical line and vise versa */
	LogIndex = CnvrtModuleLine(MCtxt->Module, line);
	LCtxt->LHdl = Init_LineHdl(LogIndex, LCtxt);
}

/*
  Initialise a module context for a icv196 module
  table to manage the hardware of a physical module
*/
static struct T_ModuleCtxt* Init_ModuleCtxt(InsLibModlDesc *md)
{
	InsLibVmeAddressSpace *vmeinfo = ((InsLibVmeModuleAddress*)md->ModuleAddress)->VmeAddressSpace;

	struct T_ModuleCtxt *MCtxt = &icv196_statics.ModuleCtxt[icv196_statics.mcntr];
	int type, i;

	/* init context table */
	MCtxt->sem_module = 1;	/* semaphore for exclusive access */
	MCtxt->Module = icv196_statics.mcntr;
	MCtxt->dflag = 0;

	/* save VME window size */
	MCtxt->VME_size = vmeinfo->WindowSize;

	/* Set HW mapping pointers to access the module */
	MCtxt->SYSVME_Add     = (short *) vmeinfo->Mapped;
	MCtxt->VME_StatusCtrl = (unsigned char *)(vmeinfo->Mapped + CoReg_Z8536);
	MCtxt->VME_IntLvl     = (unsigned char *)(vmeinfo->Mapped + CSNIT_ICV);
	MCtxt->VME_CsDir      = (short *)(vmeinfo->Mapped + CSDIR_ICV);

	/* Initialise the associated line contexts */
	MCtxt->Vect = md->Isr->Vector;
	MCtxt->Lvl  = md->Isr->Level;
	type = 0; /* to select default line type */
	for (i = 0; i < icv_LineNb; i++)
		Init_LineCtxt(i, type, MCtxt);

	/* Update Module directory */
	icv196_statics.ModuleCtxtDir[icv196_statics.mcntr] = MCtxt;

	/* increase declared module counter */
	icv196_statics.mcntr++;
	return MCtxt;
}

/*
  Initializes the hardware of a icv196 module.  Upon the initialization,
  all interrupt lines are in the disabled state.
*/
int icvModule_Init_HW(struct T_ModuleCtxt *MCtxt)
{
	unsigned char v = MCtxt->Vect;
	unsigned char l = MCtxt->Lvl;
	unsigned char dummy;
	ulong ps;

	disable(ps);

	/* The following squence will reset Z8536 anf put it in <state 0>
	   (See Chapter 6. CIO Initialization) */
	dummy = z8536_rd(); /* force the board to <state 0> */
	z8536_wr(MIC_reg); /* Master Interrupt Control reg */
	dummy = z8536_rd();
	z8536_wr(MIC_reg);
	z8536_wr(b_RESET); /* reset the board */
	z8536_wr(0); /* go from <reset state> to <state 0> (normal operation) */

	icv196_wr_8(~l, VME_IntLvl); /* set interrupt level for this module */
	icv196_wr_16(0, VME_CsDir); /* set all I/O ports to input */

	MCtxt->old_CsDir = 0;
	MCtxt->startflag = 0;
	MCtxt->int_en_mask = 0; /* clear interrupt enable mask */

	/* set up hardware for portA and portB (group#0 && #1) on the icv module */

	/* Master Config Control reg */
	z8536_wr_val(MCC_reg, PortA_Enable | PortB_Enable); /* enable ports A && B */

	/* portA and portB operate independently */

	/* Port A Mode Specification reg */
	z8536_wr_val(MSpec_Areg, PtrM_Or | Latch_On_Pat_Match); /* set to operate as bitport */

	/* Port B Mode Specification reg */
	z8536_wr_val(MSpec_Breg, PtrM_Or | Latch_On_Pat_Match);  /* set to operate as bitport */

	/* Port A Command & Status reg */
	z8536_wr_val(CSt_Areg, CoSt_SeIe);  /* interrupts enabled, no interrupt on error */

	/* Port B Command & Status reg */
	z8536_wr_val(CSt_Breg, CoSt_SeIe); /* interrupts enabled, no interrupt on error */

	/* Port A Data Path polarity reg */
	z8536_wr_val(DPPol_Areg, Non_Invert); /* non inverting */

	/* Port B Data Path polarity reg */
	z8536_wr_val(DPPol_Breg, Non_Invert); /* non inverting */

	/* Port A Data direction reg */
	z8536_wr_val(DDir_Areg, All_Input); /* input port */

	/* Port B Data direction reg */
	z8536_wr_val(DDir_Breg, All_Input);  /* input port */

	/* Port A Special I/O control reg */
	z8536_wr_val(SIO_Areg, Norm_Inp); /* normal input */

	/* Port B Special I/O control reg */
	z8536_wr_val(SIO_Breg, Norm_Inp); /* normal input */


	/*
	  set Pattern Specificatioin for Port#A && Port#B:
	  PM PT PP
	  ---------
	  0  0  0   -- Bit Masked OFF
	*/
	/* port#A -- masked off */
	/* Port A Pattern Mask reg */
	z8536_wr_val(PtrMsk_Areg, All_Masked);

	/* Port A Pattern Transition reg */
	z8536_wr_val(PtrTr_Areg, 0);

	/* Port A Pattern Polarity reg */
	z8536_wr_val(PtrPo_Areg, 0);

	/* port#B -- masked off */
	/* Port B Pattern Mask reg */
	z8536_wr_val(PtrMsk_Breg, All_Masked);

	/* Port B Pattern Transition reg */
	z8536_wr_val(PtrTr_Breg, 0);

	/* Port B Pattern Polarity reg */
	z8536_wr_val(PtrPo_Breg, 0);


	/* write interrupt vectors for portA and portB */
	/* Port A Interrupt Vector */
	z8536_wr_val(ItVct_Areg, v);

	/* Port B Interrupt Vector */
	z8536_wr_val(ItVct_Breg, v);

	/* Master Interrupt Control reg */
	z8536_wr_val(MIC_reg, b_MIE); /* master interrupt enable, no status in interrupt vector */

	restore(ps);
	return 0;
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
	unsigned char msk, status;

	w1 = ~b_MIE; /* mask excludes the MIE bit (Master Interrupt Enable) */
	msk = (unsigned char) (w1 & 0xff);
	disable(ps);
	status = z8536_rd_val(MIC_reg);
	status = status & msk;
	z8536_wr_val(MIC_reg, status);
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
	unsigned char msk, status;

	msk = b_MIE; /* mask includes the MIE bit (Master Interrupt Enable) */
	disable(ps);

	status = z8536_rd_val(MIC_reg);
	status = status | msk;
	z8536_wr_val(MIC_reg, status);
	restore(ps);
}
#endif

/*
  Check if module lost its status, reinitialise it if yes.
  To face a reset without system crash.
*/
int icvModule_Reinit(struct T_ModuleCtxt *MCtxt, int line)
{
	struct T_LineCtxt *LCtxt = &MCtxt->LineCtxt[line];
	int  i, cc;
	unsigned char bw1;

	/* master interrupt disable, permits reading interrupt vector */
	z8536_wr_val(MIC_reg, 0);

	/* read interrupt vector for this module */
	bw1 = z8536_rd_val(ItVct_Areg);

	/*master interrupt enable */
	z8536_wr_val(MIC_reg, b_MIE);

	/* Check if setting still well recorded */
	if (bw1 != MCtxt->Vect) {
		/* The module lost its initialisation: redo it */
		for (i = 0; i < icv_LineNb; i++) {
			LCtxt = &MCtxt->LineCtxt[i];
			LCtxt->Reset = 1; /* To manage reenable of line
					     at connect */
		}
		/* reset hardware of icv196vme module */
		cc = icvModule_Init_HW(MCtxt);

		for (i = 0; i < icv_LineNb; i++) {
			LCtxt = &MCtxt->LineCtxt[i];
			if (LCtxt->LHdl->SubscriberCurNb) {
				enable_Line(LCtxt);
				cprintf("icv196vmedrvr: Reinit: in module %d,"
					" reanable the active line %d\n",
					MCtxt->Module, LCtxt->Line);
			}
			LCtxt->Reset = 1; /* To manage reenable of line
					     at connect */
		}

		if (!cc)
			return 0;
		else
			return -1;
	}

	return 0;
}

/* Install Entry Point */
char *icv196_install(InsLibModlDesc *md)
{
	struct T_ModuleCtxt *MCtxt;

	if (icv196_statics.mcntr == icv_ModuleNb)
		return NULL; /* can control only 8 modules */

	init_statics_once();

	/* Set up the tables for the current Module */
	MCtxt = Init_ModuleCtxt(md);

	/* Startup the hardware for that module */
	icvModule_Init_HW(MCtxt);

	return (char *) MCtxt;
}

/* Uninstall Entry Point */
int icv196_uninstall(struct icv196T_s *s)
{
	/* nothing to do */
	return OK;
}

/* Open Entry Point */
int icv196_open(SkelDrvrClientContext *ccon)
{
	struct T_UserHdl *UHdl;
	int   chan = minor(ccon->cdcmf->dev); /* [0 - 15] */

	UHdl = &icv196_statics.ICVHdl[chan];
	if (UHdl->inuse) { /* channel already open */
		pseterr(EACCES);
		return SYSERR;
	}

	/* Perform the open */
	swait(&icv196_statics.sem_drvr, SEM_SIGRETRY);

	Init_UserHdl(UHdl, chan);
	UHdl->UserMode  = icv196_statics.UserMode;
	UHdl->WaitingTO = icv196_statics.UserTO;
	UHdl->pid       = ccon->Pid;

	icv196_statics.usercounter++; /* Update user counter value */
	UHdl->inuse++;

	ssignal(&icv196_statics.sem_drvr);

	return OK;
}

/* Close Entry Point */
int icv196_close(SkelDrvrClientContext *ccon)
{
	struct T_UserHdl *UHdl;
	int chan = minor(ccon->cdcmf->dev); /* [0 - 15] */

	UHdl = &icv196_statics.ICVHdl[chan];

	/* Perform close */
	swait(&icv196_statics.sem_drvr, SEM_SIGRETRY);

	ClrSynchro(UHdl); /* Clear connection established */

	icv196_statics.usercounter--; /* Update user counter value */
	UHdl->inuse--;

	ssignal(&icv196_statics.sem_drvr);

	return OK;
}

/**
 * @brief Read Entry Point
 *
 * @param wa     -- working area (SkelDrvrWorkingArea)
 * @param f      --
 * @param buff   --
 * @param bcount --
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
int icv196_read(void *wa, struct cdcm_file *f,
		char *buff, int bcount)
{
	struct T_UserHdl  *UHdl;
	long   *buffEvt;
	struct T_ModuleCtxt *MCtxt;
	int i, m;
	long Evt;
	int bn, count;
	ulong ps;
	int Line;
	int Chan = minordev(f->dev);

	if (WITHIN_RANGE(ICVVME_IcvChan01, Chan, SkelDrvrCLIENT_CONTEXTS)) {
		UHdl = &icv196_statics.ICVHdl[Chan-1]; /* ICV event handle */
	} else {
		pseterr(ENODEV);
		return SYSERR;
	}

	/* processing of the read */
	/* Case of Reading the event buffer */
	/* Event are long and so count should be multiple of 4 bytes */
	if ((count = (bcount >> 2)) <= 0) {
		pseterr(EINVAL);
		return SYSERR;
	}

	/*
	  BEWARE!
	  alignment constraint for the buffer
	  should be satisfied by caller!
	*/
	buffEvt = (long *)buff;
	bn = 0;
	i = count; /* Long word count */
	do {
		if ((Evt = PullFrom_Ring(&UHdl->Ring)) != -1) {
			*buffEvt = Evt;
			buffEvt++; i--; bn += 1;
		} else { /* no event in ring, wait if requested */
			if (!bn && (UHdl->UserMode & icv_bitwait)) { /* Wait for Lam */
				/* Control waiting by t.o. */
				UHdl->timid = 0;
				UHdl->timid = timeout(UserWakeup, (char *)UHdl,
						      (int)UHdl->WaitingTO);
				swait(&UHdl->Ring.Evtsem, SEM_SIGIGNORE); /* Wait Lam or Time Out */
				disable(ps);
				if (UHdl->timid < 0) {	/* time Out occured */
					restore(ps);
					/* check if any module setting got reset */
					for (m = 0; m < icv_ModuleNb; m++) {
						/* Update Module directory */
						MCtxt = icv196_statics.ModuleCtxtDir[m];
						if (MCtxt) {
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
	} while (i > 0); /* while room in buffer */

	return (bn << 2);
}

/* Write Entry Point. not available */
int icv196_write(void *wa, struct cdcm_file *f, char *buff, int bcount)
{
	pseterr(ENOSYS);
        return SYSERR;
}

/* IOCTL Entry Point */
int icv196_ioctl(int Chan, int fct, char *arg)
{
	int mode, SubsMode, Timeout, i, j, l, err = 0;
	short Module, index, Line;
	struct T_UserHdl    *UHdl = NULL;
	struct T_ModuleCtxt *MCtxt;
	struct T_LineCtxt   *LCtxt;
	struct T_LogLineHdl *LHdl;
	struct T_Subscriber *Subs;
	struct icv196T_HandleInfo *HanInfo;
	struct icv196T_HandleLines *HanLin;
	struct icv196T_UserLine ULine;
	struct icv196T_ModuleInfo *Infop;
	int    Type, LogIx, grp, dir, group_mask, Iw1, Flag;
	unsigned long *Data;

	//printk("%s() function code = %x \n", __FUNCTION__, fct);

	switch (fct) {
	case ICVVME_getmoduleinfo:
		/* get device information */
		Infop = (struct icv196T_ModuleInfo *)arg;
		for (i = 0; i < icv_ModuleNb; i++, Infop++) {
			if (!icv196_statics.ModuleCtxtDir[i]) {
				Infop->ModuleFlag = 0;
				continue;
			}

			MCtxt = &icv196_statics.ModuleCtxt[i];
			Infop->ModuleFlag = 1;
			Infop->ModuleInfo.base =
				(unsigned long) MCtxt->SYSVME_Add;
			Infop->ModuleInfo.size = MCtxt->VME_size;
			for (j = 0; j < icv_LineNb; j++) {
				Infop->ModuleInfo.vector =
					MCtxt->Vect;
				Infop->ModuleInfo.level =
					MCtxt->Lvl;
			}
		}
		break;
	case ICVVME_gethandleinfo:
		HanInfo = (struct icv196T_HandleInfo *)arg;
		UHdl = icv196_statics.ICVHdl; /* point to first user handle */
		for (i = 0; i < SkelDrvrCLIENT_CONTEXTS; i++, UHdl++) {
			HanLin = &HanInfo->handle[i];
			l = 0;
			if (!UHdl->inuse)
				continue;
			else {
				HanLin->pid = UHdl->pid;
				for (j = 0; j < ICV_LogLineNb; j++) {
					if (TESTBIT(UHdl->Cmap, j)) {
						GetUserLine(j, &ULine);
						HanLin->lines[l].group =
							ULine.group;
						HanLin->lines[l].index =
							ULine.index;
						l++;
					}
				}
			}
		}
		break;
	case ICVVME_reset:
		/* perform hardware reset of icv196 module */
		break;
	case ICVVME_setDbgFlag:
		/* dynamic debugging */
		Flag = G_dbgflag;
		if ( *( (int *) arg) != -1) {
			/* update the flag if value not = -1 */
			G_dbgflag = *( (int *) arg);
		}
		*( (int *) arg) = Flag;	/* give back old value */
		break;
	case ICVVME_setreenable:
		/* Set reenable flag to allow continuous interrupts */
		UHdl = &icv196_statics.ICVHdl[Chan];

		/* Check parameters and Set up environnement */
		Module = ((struct icv196T_UserLine *) arg)->group;
		index = ((struct icv196T_UserLine *) arg)->index;

		if (!WITHIN_RANGE(0, Module, icv_ModuleNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!WITHIN_RANGE(0, index, icv_LineNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		/* set up Logical line handle pointer */
		LogIx = CnvrtUserLine(Module, index);

		if (!WITHIN_RANGE(0, LogIx, ICV_LogLineNb)) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		if ((LHdl = icv196_statics.LineHdlDir[LogIx]) == NULL) {
			/* Line not in config */
			pseterr(EINVAL);
			return SYSERR;
		}

		LCtxt = LHdl->LineCtxt;
		if (LCtxt->LHdl != LHdl) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		LCtxt->intmod = icv_ReenableOn;
		break;
	case ICVVME_clearreenable:
		/* Clear reenable flag.
		   After an interrupt, further interrupts are inhibited
		   until the line is enabled again */
		UHdl = &icv196_statics.ICVHdl[Chan];

		/* Check parameters and Set up environnement */
		Module = ((struct icv196T_UserLine *) arg)->group;
		index = ((struct icv196T_UserLine *) arg)->index;

		if (!WITHIN_RANGE(0, Module, icv_ModuleNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!WITHIN_RANGE(0, index, icv_LineNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		/* set up Logical line handle pointer */
		LogIx = CnvrtUserLine(Module, index);

		if (!WITHIN_RANGE(0, LogIx, ICV_LogLineNb)) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		if ((LHdl = icv196_statics.LineHdlDir[LogIx]) == NULL) {
			/* Line not in config */
			pseterr(EINVAL);
			return SYSERR;
		}

		LCtxt = LHdl->LineCtxt;
		if (LCtxt->LHdl != LHdl) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		LCtxt->intmod = icv_ReenableOff;
		break;
	case ICVVME_disable:
		/* Disable interrupt. Interrupts on the given logical
		   line are disabled */
		UHdl = &icv196_statics.ICVHdl[Chan];

		/* Check parameters and Set up environnement */
		Module = ((struct icv196T_UserLine *) arg)->group;
		Line = ((struct icv196T_UserLine *) arg)->index;

		if (!WITHIN_RANGE(0, Module, icv_ModuleNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!WITHIN_RANGE(0, Line, icv_LineNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		/* set up Logical line handle pointer */
		LogIx = CnvrtUserLine(Module, Line);

		if (!WITHIN_RANGE(0, LogIx, ICV_LogLineNb)) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		if ((LHdl = icv196_statics.LineHdlDir[LogIx]) == NULL) {
			/* Line not in config */
			pseterr(EINVAL);
			return SYSERR;
		}

		LCtxt = LHdl->LineCtxt;
		if (LCtxt->LHdl != LHdl) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		disable_Line(LCtxt);
		break;
	case ICVVME_enable:
		/* Enable interrupt.
		   Interrupts on the given logical line are enabled */
		printk("ICVVME_enable entered\n");
		UHdl = &icv196_statics.ICVHdl[Chan];

		/* Check parameters and Set up environnement */
		Module = ((struct icv196T_UserLine *) arg)->group;
		Line = ((struct icv196T_UserLine *) arg)->index;

		if (!WITHIN_RANGE(0, Module, icv_ModuleNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!WITHIN_RANGE(0, Line, icv_LineNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		/* set up Logical line handle pointer */
		LogIx = CnvrtUserLine(Module, Line);

		if (!WITHIN_RANGE(0, LogIx, ICV_LogLineNb)) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		if ((LHdl = icv196_statics.LineHdlDir[LogIx]) == NULL) {
			/* Line not in config */
			pseterr(EINVAL);
			return SYSERR;
		}

		LCtxt = LHdl->LineCtxt;
		if (LCtxt->LHdl != LHdl) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		enable_Line(LCtxt);
		break;
	case ICVVME_intcount:
		/* Read interrupt counters for all lines in the given module */
		Module = ((struct icv196T_Service *) arg)->module;
		Data   = ((struct icv196T_Service *)arg)->data;
		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		MCtxt = &icv196_statics.ModuleCtxt[Module];
		LCtxt = MCtxt->LineCtxt;
		for (i = 0; i < icv_LineNb; i++, LCtxt++)
			*Data++ = LCtxt->loc_count;
		break;
	case ICVVME_reenflags:
		/* Read reenable flags for all lines in the given module */
		Module = ((struct icv196T_Service *) arg)->module;
		Data   = ((struct icv196T_Service *)arg)->data;
		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		MCtxt = &icv196_statics.ModuleCtxt[Module];
		LCtxt = MCtxt->LineCtxt;
		for (i = 0; i < icv_LineNb; i++, LCtxt++)
			*Data++ = LCtxt->intmod;
		break;
	case ICVVME_intenmask:
		/* Read interrupt enable mask for the given module */
		Module = ((struct icv196T_Service *) arg)->module;
		Data   = ((struct icv196T_Service *)arg)->data;
		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		MCtxt = &icv196_statics.ModuleCtxt[Module];
		*Data = MCtxt->int_en_mask;
		break;
	case ICVVME_iosem:
		/* Read i/o semaphores for all lines in the given module */
		Module = ((struct icv196T_Service *) arg)->module;
		Data   = ((struct icv196T_Service *)arg)->data;
		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		MCtxt = &icv196_statics.ModuleCtxt[Module];
		LCtxt = MCtxt->LineCtxt;

		for (i = 0; i < icv_LineNb; i++, LCtxt++) {
			LHdl = LCtxt->LHdl;
			Subs = &LHdl->Subscriber[0];
			for (j = 0; j < LHdl->SubscriberCurNb; j++, Subs++) {
				if (Subs->Ring) {
					if (!j)
						Data++;
					continue;
				}
				*Data++ = Subs->Ring->Evtsem;
			}
		}
		break;
	case ICVVME_setio:
		/* Set direction of input/output ports */
		Module = ((struct icv196T_Service *)arg)->module;
		Data   = ((struct icv196T_Service *)arg)->data;
		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		MCtxt = &icv196_statics.ModuleCtxt[Module];
		grp = *Data++;
		dir = *Data;

		if (!WITHIN_RANGE(0, grp, 11) || !WITHIN_RANGE(0, dir, 1))
			return SYSERR;

		group_mask = 1 << grp;
		if (dir) /* output */
			MCtxt->old_CsDir |= group_mask;
		else /* input */
			MCtxt->old_CsDir &= ~group_mask;

		icv196_wr_16(MCtxt->old_CsDir, VME_CsDir);

		/* in case of output channel -- reset its value to 0 */
		if (dir)
			icv196_grp_wr_8(0, grp);
		break;
	case ICVVME_readio:
		/* Read direction of i/o ports */
		Module = ((struct icv196T_Service *)arg)->module;
		Data   = ((struct icv196T_Service *)arg)->data;

		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		MCtxt = &icv196_statics.ModuleCtxt[Module];
		*Data = MCtxt->old_CsDir;
		break;
	case ICVVME_nowait:
		/* Set nowait mode on the read function */
		UHdl = &icv196_statics.ICVHdl[Chan];

		/* reset flag wait */
		UHdl->UserMode &= (~((short) icv_bitwait));
		break;
	case ICVVME_wait:
		/* Set wait mode on the read function */
		UHdl = &icv196_statics.ICVHdl[Chan];

		UHdl->UserMode |= ((short)icv_bitwait); /* set flag wait */
		break;
	case ICVVME_setupTO:
		/* Set Time out for waiting Event.
		   Monitor duration of wait on interrupt event before
		   time out  declared */
		UHdl = &icv196_statics.ICVHdl[Chan];

		if (( *( (int *) arg)) < 0) {
			pseterr(EINVAL);
			return SYSERR;
		}

		Timeout = UHdl->WaitingTO; /* current value */
		Iw1 = *( (int *) arg);	    /* new value */
		UHdl->WaitingTO =  Iw1;    /* set T.O. */
		*( (int *) arg) = Timeout;    /* return old value */
		break;
	case ICVVME_connect:
		/* connect function */
		err = 0;
		UHdl = &icv196_statics.ICVHdl[Chan];

		/* Check parameters and Set up environnement */
		Module = ((struct icv196T_connect *) arg)->source.group;
		index = ((struct icv196T_connect *) arg)->source.index;
		mode  = ((struct icv196T_connect *) arg)->mode;

		if (mode & (~(icv_cumul | icv_disable))) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!WITHIN_RANGE(0, Module, icv_ModuleNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!WITHIN_RANGE(0, index, icv_LineNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		/* set up Logical line handle pointer */
		LogIx = CnvrtUserLine(Module, index);

		if (!WITHIN_RANGE(0, LogIx, ICV_LogLineNb)) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		if ((LHdl = icv196_statics.LineHdlDir[LogIx]) == NULL) {
			/* Line not in config */
			pseterr(EINVAL);
			return SYSERR;
		}

		LCtxt = LHdl->LineCtxt;
		if (LCtxt->LHdl != LHdl) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		if (TESTBIT(UHdl->Cmap, LogIx)) {
			if (LCtxt->Reset)
				enable_Line(LCtxt);
			err = 0;
			break; /* already connected ok -->  exit */
		}

		SubsMode = 0; /* default mode requested */
		if (mode & icv_cumul)
			SubsMode = icv_cumulative;

		if (mode & icv_disable)
			LCtxt->intmod = icv_ReenableOff;

		MCtxt = LCtxt->MCtxt;
		Type  = LCtxt->Type;
		Line  = LCtxt->Line;

		/* Reinit module if setting lost!
		   beware! all lines loose their current state */
		icvModule_Reinit(MCtxt, Line);

		if ((Subs = LineBooking(UHdl, LHdl, SubsMode)) == NULL) {
			pseterr(EUSERS);
			return SYSERR;
		}

		/* enable line interrupt */
		if (LCtxt->status == icv_Disabled)
			enable_Line(LCtxt);
		LCtxt->loc_count = 0;
		break;
	case ICVVME_disconnect:
		/* disconnect function */
		UHdl = &icv196_statics.ICVHdl[Chan];
		err = 0;

		/* Check parameters and Set up environnement */
		Module = ((struct icv196T_connect *) arg)->source.group;
		index = ((struct icv196T_connect *) arg)->source.index;

		if (!WITHIN_RANGE(0, Module, icv_ModuleNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!WITHIN_RANGE(0, index, icv_LineNb - 1)) {
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!icv196_statics.ModuleCtxtDir[Module]) {
			pseterr(EACCES);
			return SYSERR;
		}

		/* set up Logical line handle pointer */
		LogIx = CnvrtUserLine(Module, index);

		if (!WITHIN_RANGE(0, LogIx, ICV_LogLineNb)) {
			pseterr(EWOULDBLOCK);
			return SYSERR;
		}

		if ((LHdl = icv196_statics.LineHdlDir[LogIx]) == NULL) {
			/* log line not affected */
			pseterr(EINVAL);
			return SYSERR;
		}

		if (!(TESTBIT(UHdl->Cmap, LogIx))) { /* non connected */
			pseterr(ENOTCONN);
			return SYSERR;
		}

		if ((Subs = LineUnBooking(UHdl, LHdl)) == NULL) {
			pseterr(ENOTCONN);
			return SYSERR;
		}

		/* Set up line context */
		LCtxt = LHdl->LineCtxt;

		/* disable the corresponding line */
		if (!LHdl->SubscriberCurNb) {
			disable_Line(LCtxt);
			LCtxt->loc_count = -1;
		}
		break;
	case ICV196_GR_READ:
		Module = ((struct icv196T_Service *)arg)->module;
		grp = (int)((struct icv196T_Service *)arg)->line;
                Data   = ((struct icv196T_Service *)arg)->data;
                if (!icv196_statics.ModuleCtxtDir[Module]) {
                        pseterr(EACCES);
                        return SYSERR;
                }
		MCtxt = &icv196_statics.ModuleCtxt[Module];

		if (*Data == 1)
			*Data = icv196_grp_rd_8(grp);
		else
			*Data = icv196_grp_rd_16(grp);
		break;
	case ICV196_GR_WRITE:
		{
			long offs;
		Module = ((struct icv196T_Service *)arg)->module;
		grp = (int)((struct icv196T_Service *)arg)->line;
                Data   = ((struct icv196T_Service *)arg)->data;

                if (!icv196_statics.ModuleCtxtDir[Module]) {
                        pseterr(EACCES);
                        return SYSERR;
                }
		MCtxt = &icv196_statics.ModuleCtxt[Module];
		if (grp & 1) /* odd */
			offs = 1;
		else /* even */
			offs = 3;
#ifdef __ICV196_DEBUG__
		printk("Module is %hd. Grp is %d. BA is %p. Data is 0x%hx. Grp Addr is 0x%lx. Offset is %ld\n",
		       Module, grp, MCtxt->SYSVME_Add, (short)*Data,
		       (long)((long)MCtxt->SYSVME_Add + (long)(grp + offs)),
		       grp + offs);
		printk("Should write %ld bytes in group %d\n", Data[1], grp);
#endif
		if (Data[1] == 1)
			icv196_grp_wr_8((char)*Data, grp);
		else
			icv196_grp_wr_16((short)*Data, grp);
		break;
		}
	default: /* Ioctl function code out of range */
#ifdef __ICV196_DEBUG__
		printk("ioctl function code is out-of-range\n");
#endif
		pseterr(EINVAL);
		return SYSERR;
	}
	return err;
}

/* Select Entry Point */
int icv196_select(struct icv196T_s *s, struct cdcm_file *f,
		 int which, struct sel *se)
{
	int Chan = minordev(f->dev);
	struct T_UserHdl *UHdl = NULL;

	if (WITHIN_RANGE(ICVVME_IcvChan01, Chan, SkelDrvrCLIENT_CONTEXTS)) {
		UHdl = &s->ICVHdl[Chan-1]; /* general handle */
	} else {
		pseterr(EACCES);
		return SYSERR;
	}

	switch (which) {
	case SREAD:
		se->iosem   = &UHdl->Ring.Evtsem;
		se->sel_sem = &UHdl->sel_sem;
		break;
	case SWRITE:
		pseterr(EACCES);
		return SYSERR;
		break;
	case SEXCEPT:
		pseterr(EACCES);
		return SYSERR;
		break;
	}
	return OK;
}

/*
  interrupt service routine
  of the icv196 module driver
  work on the module context which start the corresponding isr
*/
int icv196_isr(void *arg)
{
	SkelDrvrModuleContext *mcon  = arg;
	struct T_ModuleCtxt   *MCtxt = mcon->UserData;
	struct T_UserHdl      *UHdl;
	struct T_LineCtxt     *LCtxt;
	struct T_LogLineHdl   *LHdl;
	struct T_Subscriber   *Subs;
	struct icvT_RingAtom  *RingAtom;
	struct icvT_RingAtom   Atom;
	int m, i, j, ns, cs;
	short count;
	unsigned short Sw1 = 0;
	unsigned char mdev, mask, status;
	static unsigned short input[16] = { 0 }; /* input array */

	m = MCtxt->Module; /* module index */

	printk("--------------> %s() called <----------------\n", __FUNCTION__);

	if (!(WITHIN_RANGE(0, m, icv_ModuleNb-1)))
		return SYSERR;

	if (!icv196_statics.ModuleCtxtDir[m])
		return SYSERR;

	if (!MCtxt->startflag) {
		/* reset the input array the first
		   time the routine is entered */
		for (i = 0; i < ARRAY_SIZE(input); i++)
			input[i] = 0;

		MCtxt->startflag = 1;
	}

	status = z8536_rd(); /* force board to defined state */

	/* read portA status reg */
	status = z8536_rd_val(CSt_Areg);

	if (status & CoSt_Ius) { /* did portA cause the interrupt? */
		mask = 1;
		mdev = z8536_rd_val(Data_Areg); /* read portA's data reg */
		for (i = 0; i <= 7; i++) {
			if (mdev & mask & MCtxt->int_en_mask)
				/* mark port A's active interrupt lines */
				input[i] = 1;
			mask <<= 1;
		}
	}

	 /* read portB status reg */
	status = z8536_rd_val(CSt_Breg);

	if (status & CoSt_Ius) { /* did portB cause the interrupt? */
		mask = 1;
		mdev = z8536_rd_val(Data_Breg); /* read portB's data reg */
		for (i = 8; i <= 15; i++) {
			if (mdev & mask & (MCtxt->int_en_mask >> 8))
				/* mark port B's active interrupt lines */
				input[i] = 1;
			mask <<= 1;
		}
	}

	/* loop on active line */
	for (i = 0; i < icv_LineNb; i++) {
		if (!input[i])	/* line not active */
			continue;

		/* line is active */
		input[i] = 0;
		LCtxt = &MCtxt->LineCtxt[i];
		LCtxt->loc_count++;
		LHdl = LCtxt->LHdl;

		/* Build the Event */
		Atom.Evt.All = LHdl->Event_Pattern.All;
		/* scan the subscriber table to send them the event */
		ns   = LHdl->SubscriberMxNb;
		cs   = LHdl->SubscriberCurNb;
		Subs = LHdl->Subscriber;

		for (j = 0; j < ns && cs > 0; j++, Subs++) {
			if (!Subs->Ring) /* subscriber passive */
				continue;
			cs--; /* nb of subbscribers to find out in Subs table */
			UHdl = Subs->Ring->UHdl;
			if ((count = Subs->EvtCounter) != -1) {
				Sw1 = (unsigned short) count;
				Sw1++;
				if ((short)Sw1 < 0)
					Sw1 = 1;
				Subs->EvtCounter = Sw1; /* keep counter positive */
			}

			/* give the event according to subscriber status  */
			if (Subs->mode == icv_queuleuleu) {
				/* mode a la queueleuleu */
				Atom.Subscriber = NULL;
				Atom.Evt.Word.w1 = Sw1; /* set up event counter */
				RingAtom = PushTo_Ring(Subs->Ring, &Atom);
			} else { /* cumulative mode */
				if (count < 0) { /* no event in Ring */
					Subs->EvtCounter = 1; /* init counter of Subscriber */
					Atom.Subscriber  = Subs; /* Link event to subscriber */
					Atom.Evt.Word.w1 = 1;     /* set up event counter */
					RingAtom = PushTo_Ring(Subs->Ring, &Atom);
					Subs->CumulEvt = RingAtom; /* link to cumulative event, used to disconnect */
				}
			}
			/* process case user stuck on select semaphore */
			if (UHdl->sel_sem)
				ssignal(UHdl->sel_sem);

		} /* for/subscriber */

		/* Enable the line before leaving */
		if (LCtxt->intmod == icv_ReenableOff)
			disable_Line(LCtxt);
	} /* for/line */

	/* clear IP bit on portB */
	z8536_wr_val(CSt_Breg, CoSt_ClIpIus);

	/* clear IP bit on portA */
	z8536_wr_val(CSt_Areg, CoSt_ClIpIus);
	return OK;
}
