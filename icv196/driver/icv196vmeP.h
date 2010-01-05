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

/*
  Ring buffer management
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
  It is pointed at from the corresponding physical line context in the device
  table. This link is set up at initialisation of the device and will allow to
  translate the line trigger in the logical chanel event.
  It manages the device-independent part of the line.
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
	short mode; /* 2 -- normal, 1 -- cumulative,
		       0 -- default (that is cumulative(1) for PLS line,
		                     and normal(2) for ??? ) */
	short EvtCounter; /* line event counter for this user */
};

/* Ring buffer manager */
struct T_RingBuffer {
	struct T_UserHdl *UHdl; /* owner of the ring buffer */
	struct icvT_RingAtom *Buffer; /* Buffer of atom */
	int   Evtsem; /* semaphore to synchronise and count evt */
	short mask;   /* mask to manage buffer wrapping */
	short reader; /* current index for reading */
	short writer; /* current index for writing */
};

/* Logical line handle */
struct T_LogLineHdl {
	int LogIndex; /* logical line index (one of ICV_Lxxx) */
	struct T_LineCtxt *LineCtxt;
	union icvU_Evt Event_Pattern; /* Pattern of event build at
					 set up of link from physical line
					 this pattern is dependent of the
					 design of logical line address */
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
	short usercount;
	int   chanel; /* channel index */
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

/*
  Structure of a physical line context:
  this depends on the type of module
*/
struct T_LineCtxt {
	struct T_ModuleCtxt *MCtxt;
	struct T_LogLineHdl *LHdl; /* Line handle linked to */
	int   Type; /* to stand specificity of lines: pls or icv */
	short Line; /* Line index [0 - 15] */
	int   status;	 /**< icv_Disabled /icv_Enabled */
	int   intmod;	 /**< icv_ReenableOn / icv_ReenableOff */
	int   loc_count; /**< interrupt counter (-1 -- line disabled) */
	short Reset;
};

struct T_ModuleCtxt {
	int            sem_module; /* mutex semaphore */
	int            dflag;      /* debug flag */
	short          Module;     /* Module index [0 - 7] */
	int            VME_size;   /* original info table values */
	short         *SYSVME_Add; /* module virtual base address */
	unsigned char *VME_StatusCtrl; /* Z8536 Control reg */
	unsigned char *VME_IntLvl;     /* ICV196 Int Level reg */
	short         *VME_CsDir; /* ICV196 I/O Direction reg */
	short          old_CsDir; /* Previous state of I/O Direction reg */
	unsigned short startflag; /*  */
	unsigned short int_en_mask; /* 16-lines interrupt bit mask */
	unsigned char  Vect; /* Int. vector */
	unsigned char  Lvl;  /* Int. level */
	struct T_LineCtxt LineCtxt[icv_LineNb]; /* 16-lines context */
};

/* write z8536 Status register */
#define z8536_wr(v)					\
({							\
	cdcm_iowrite8(v, MCtxt->VME_StatusCtrl);	\
 })

/* read z8536 Status register */
#define z8536_rd()				\
({						\
	cdcm_ioread8(MCtxt->VME_StatusCtrl);	\
 })

/* write value in z8536 internal register */
#define z8536_wr_val(r, v)				\
({							\
	cdcm_iowrite8(r, MCtxt->VME_StatusCtrl);	\
	cdcm_iowrite8(v, MCtxt->VME_StatusCtrl);	\
 })

/* read value from z8536 internal register */
#define z8536_rd_val(r)					\
({							\
	cdcm_iowrite8(r, MCtxt->VME_StatusCtrl);	\
	cdcm_ioread8(MCtxt->VME_StatusCtrl);		\
 })


/* write icv196 16bit register */
#define icv196_wr(v, reg)			\
({						\
        cdcm_iowrite16be(v, MCtxt->reg);	\
 })

/* read icv196 16bit register */
#define icv196_rd(reg)				\
({						\
        cdcm_ioread16be(MCtxt->reg);		\
 })

#endif	/* _ICV_196_VME_P_H_INCLUDE_ */
