/*
   __________________________________________________________________
  |                                                                 |
  |   file name :       icv196vmelib.h                              |
  |                                                                 |
  |   created: 26-mar-1992 Alain Gagnaire                           |
  |   updated: 22-nov-1993 A. G.                                    |
  |                                                                 |
  |_________________________________________________________________|
  |                                                                 |
  |   .h file to compile icv196lib file                             |
  |                                                                 |
  |_________________________________________________________________|

  update :
  ========

  22-nov-1993 Alain GAGNAIRE : add icv196_DioPorts declaration
  10-may-1994 A.G.: update to stand 8 modules
*/
#ifndef  _icv196vmelib
#define  _icv196vmelib

/* definition link to configuration */
/* module specification */
#define icv_LineNb  16 /* number of line per module */
#define ICV_IndexNb 16 /* index number in a group */

/* limits of the configuration */
#define icv_ModuleNb  8 /* Max Number of icv modules */
#define ICV_LogLineNb (icv_ModuleNb * icv_LineNb + 1) /* logical lines number + 1 */

/* Channel to get synchronised with icv int */
#define ICVVME_IcvChan01 1
#define ICVVME_IcvChan02 2
#define ICVVME_IcvChan03 3
#define ICVVME_IcvChan04 4
#define ICVVME_IcvChan05 5
#define ICVVME_IcvChan06 6
#define ICVVME_IcvChan07 7
#define ICVVME_IcvChan08 8

/* for direct io from direct pointer in dioaiolib */
union icv196_DoubleGroups {
	unsigned short  grp1_2;
	struct {
		unsigned char grp1;
		unsigned char grp2;
	} grps;
};

struct icv196_DioPorts {
	unsigned short             cs_clear;
	union icv196_DoubleGroups  dble_grp[6];
	unsigned short             cs_dir;
};

/**
 * @brief structure for connect from application program
 *
 * @param module -- module number
 * @param line   -- line number
 * @param mode   -- cumulative [1] / non-cumulative [0]
 */
struct icv196T_UserConnect {
	unsigned char module;
	unsigned char line;
	short         mode;
};

/**
 * @brief structure of an event seen from an application program
 *
 * @param count  -- event counter
 *                  Depens on mode parameter
 *                  (cumulative [1] / non-cumulative [0]) in icv196T_UserConnect
 *                  structure.
 * @param module -- module, where the interrupt occured
 * @param line   -- line in the module, where the interrupt occurred
 */
struct icv196T_UserEvent {
	short         count;
	unsigned char module;
	unsigned char line;
};

/*-----------------------------------------------------------------
	       definitions for ioctl interface
-----------------------------------------------------------------*/
struct icv196T_ModuleParam {
	unsigned long base;
	unsigned long size;
	unsigned char vector;
	unsigned char level;
};

struct icv196T_ConfigInfo {
	int ModuleFlag[icv_ModuleNb]; /* 0 -- module not declared
					 1 -- module declared */
	struct icv196T_ModuleParam ModuleInfo[icv_ModuleNb];
	struct icv196T_ModuleParam trailer;
};

union icv196U_UserLine {
	unsigned short All;
	struct {
		unsigned char group;
		unsigned char index;
	} elt;
};

struct icv196T_UserLine {
	unsigned char group;
	unsigned char index;
};

struct icv196T_HandleLines {
	int pid;
	struct icv196T_UserLine lines[ICV_LogLineNb];
};

struct icv196T_HandleInfo {
	struct icv196T_HandleLines handle[SkelDrvrCLIENT_CONTEXTS];
};

/* structure passed at gethandleinfo ioctl call */
struct icv196T_ModuleInfo {
	int ModuleFlag;
	struct icv196T_ModuleParam ModuleInfo;
};

struct icv196U_LogLine {
	unsigned char group;
	unsigned char index;
};


struct icv196T_connect {
	struct icv196U_LogLine source;
	int                    mode;
};

struct icv196T_Service {
	unsigned char module;
	unsigned char line;
	unsigned long data[ICV_IndexNb];
};

/* structure of a user line Address */
struct icv196T_LineUserAdd {
	char group;
	char index;
};

struct T_icv196Arg {
	struct icv196T_LineUserAdd Add;
	short   mode;
};

#ifdef __cplusplus
extern "C" {
#endif

/* to get information about the lines used by the icv196test test program */
int icv196GetInfo(int  m, int  buff_sz, struct icv196T_ModuleInfo *buff);

/*
  set time out for waiting Event on read of synchronization channel
  the synchronization is requested by mean of the general purpose
  function gpevtconnect see in real time facilities: rtfclty/gpsynchrolib.c
*/
int icv196SetTO(int fdid,	/* file Id returned by sdevtconnect */
		int *val);	/* at call: new TO, at return the old one */

#ifdef __cplusplus
}
#endif

#endif
