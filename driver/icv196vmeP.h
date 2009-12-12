/*
  _________________________________________________________________
 |                                                                 |
 |   file name :        icv196vmeP.h                               |
 |                                                                 |
 |   created:     21-mar-1992 Alain Gagnaire, F. Berlin            |
 |   last update: 02-dec-1992 A. gagnaire                          |
 |_________________________________________________________________|
 |                                                                 |
 |   private header file for the icv196vme                         |
 |                                                                 |
 |_________________________________________________________________|

 updated:
 =======
 21-mar-1992 A.G., F.B.: version integrated with the event handler common to
                   sdvme, and fpiplsvme driver
 02dec-1992 A.G. : add element in struct : CumulEvt in T_Subscriber
                                           SubscriberCurnb in T_LogLineHdl
 10-may-1994 A.G.: update to stand 8 modules
*/

/*
  On the design of the driver for interrupt

  In order to stand several type of module as interrupt source with the same
  driver interface, the design of drivers separates 2 level of processing:
	- The level dependent on the hardware which issued the interrupt
	  for each kind of hardware module; this part must be specific
	  as from tables structure point of view as from a processing one.
	- The level of event processing which is the same whatever is the
	  device source.

  Consequently the same event processing provides users with the
  synchronization service (Access method level) whatever device is used
  to generate the external signals: new PS pls module, icv196 module.
  This aim explains the splitting of code, data and definition into 3 sets:
	-  a set completely independent of the device: Event management
	   and user interface
	-  a set dependent on devices giving the triggers
	-  an hybrid set which make the link beetween the 2 previous sets
*/

/*
  Declaration dependent of Event management
  =========================================

  These structures an declaration don't depend on hardware module which
  provides the interrupt.
*/
#ifndef _ICV_196_VME_P_H_INCLUDE_
#define _ICV_196_VME_P_H_INCLUDE_

/* define some macroes to manage bit map of the connection */
/* to operate othe bit given by its number in a byte table map  */
#define CLRBIT(ar,n)  ar[(n) / 8] &= (~(0x1 << ( (n) & 0x7)))
#define SETBIT(ar,n)  ar[(n) / 8] |= (  0x1 << ( (n) & 0x7))
#define TESTBIT(ar,n) ar[(n) / 8] &  (  0x1 << ( (n) & 0x7))

/* Ring buffer management
   ----------------------
   The buffer size must be a power of 2 To follow ring buffer
   management principle based on a wrapping of the index in the ring
   buffer by increasing the index modulo a power of 2 (role of the mask
   which is the power of 2 minus 1)
*/
#define Evt_nb  (1 << 3)       /* size = 2**3 = (base 2#01000 */
#define Evt_msk (Evt_nb - 1)   /* mask = 2**3 - 1 = (base 2#00111) */

/* for global ring buffer to pass interrupt from isr to Thread */
#define GlobEvt_nb  (1 << 6) /* 2 time as many as lines number in config */
#define GlobEvt_msk (GlobEvt_nb - 1) /*  */

/* these structures  are used to buffer the event */
/*
  Logical logical line handle
  ---------------------------
  A physical line is accessed by a user via a logical line associated to
  a table called logical line handle.
  it is pointed at from the corresponding physical line context in the device
  table. This link is set up at initialisation of the device and will allow to
  translate the line trigger in the logical chanel event.
  it manages the device-independent part of the line
*/

/* Subtable in a logical line handle */
#define icv_SubscriberNb 8

/*
  Subscriber management:
  ----------------------

  normal mode:
  each trigger give one event in the ring buffer

  cumulative mode:
  only one event from a given trigger source is present in the ring,
  At arrival of the next trigger, if event not already read, no event is put
  in the ring, it only count the occurance in the subscriber
  table.
*/
struct T_Subscriber {
	struct T_LogLineHdl * LHdl;
	struct T_RingBuffer *Ring; /* Point at the user Handle
				      providing the Ring buffer */
	struct icvT_RingAtom *CumulEvt; /* Cumulative event pointer */
	short mode;        /* 0 means normal, 1 means cumulative */
	short EvtCounter; /* line event counter for this user */
};


/* Ring buffer manager */
struct T_RingBuffer {
	struct T_UserHdl *UHdl; /* owner of the ring buffer */
	struct icvT_RingAtom *Buffer; /* Buffer of atom */
	int Evtsem;	  /* semaphore to synchronise and count evt */
	short   mask; /* mask to manage buffer wrapping */
	short reader; /* current index for reading */
	short writer; /* current index for writing */
};

/* Logical line handle */
struct T_LogLineHdl {
	struct icv196T_s *s;
	int LogIndex;
	struct T_LineCtxt *LineCtxt;
	union icvU_Evt Event_Pattern; /* Pattern of event build at
					 set up of link from physical line
					 this pattern is dependent of the
					 design of logical line address
				      */
	/* Subscriber table */
	int SubscriberMxNb; /* size of subscriber table */
	int SubscriberCurNb; /* current number of subscriber */
	struct T_Subscriber Subscriber[icv_SubscriberNb]; /* Where to cast
							     the received
							     trigger */
};

/*
  User Handle
  structure associated to each minor device
*/
struct T_UserHdl {
	struct icv196T_s *s;
	short usercount;
	int   chanel;
	long  count;
	int   timid;
	int   pid;       /* process id  */
	int   *sel_sem;	/* semaphore for select */
	int   WaitingTO;
	short UserMode;
	char  Cmap[ICV_mapByteSz]; /* bit map of connection */
	struct T_RingBuffer Ring; /* Ring buffer to manage events */
	struct icvT_RingAtom Atom[Evt_nb]; /* Buffer of Ring buffer */
};


/* Declaration dependent of the device level */

/* structure of  the physical line address */
union U_LineAdd {
	long all;
	struct {
		short M; /* module index */
		short L; /* line index in the module */
	} field;
};

/*
  Structure of a physical line context:
  this depends on the type of module
*/
struct T_LineCtxt {
	struct icv196T_s *s;
	struct T_ModuleCtxt *MCtxt;
	struct T_LogLineHdl *LHdl; /* Line handle linked to */
	int Type;    /* to stand specificity of lines: pls or icv */
	short  Line; /* Line index */
	int status;
	int intmod;
	int loc_count;
	short  Reset;
};

struct T_ModuleCtxt {
	struct  icv196T_s *s;
	int     sem_module;    /* mutex semaphore */
	int     dflag;         /* debug flag */
	short   Module;        /* Module index */
	int     VME_size;      /* original info table values */
	unsigned long CPUVME_Add; /* Module physical 32 bits address
				     can be get by the user (via ioctl)
				     and used to open a shared segment
				     on this VME module address */

	short              *SYSVME_Add; /* module virtual base address */
	unsigned char      *VME_StatusCtrl;
	unsigned char      *VME_IntLvl;
	short              *VME_CsDir;
	short              old_CsDir;
	unsigned short     startflag;
	unsigned short     int_en_mask;

	/* context of the lines */
	unsigned char Vect; /* for each line a Int. vector */
	unsigned char Lvl;  /* for each line an Int. level */
	struct T_LineCtxt LineCtxt[icv_LineNb];
};

#endif	/* _ICV_196_VME_P_H_INCLUDE_ */
