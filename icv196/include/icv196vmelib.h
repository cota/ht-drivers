/**
 * @file icv196vmelib.h
 *
 * @brief
 *
 * Created on 26-mar-1992 Alain Gagnaire
 *
 * @author Copyright (C) 2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
 *
 * @date Created on 06/01/2010
 *
 * @section license_sec License
 *          Released under the GPL
 */
#ifndef  _icv196vmelib
#define  _icv196vmelib

#include <skeluser.h>

#define icv_LineNb    16 /* number of lines per module */
#define icv_ModuleNb  8  /* Max Number of modules */
#define ICV_LogLineNb (icv_ModuleNb * icv_LineNb + 1) /* total number of logical lines + 1 */

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
	unsigned char group;	/**<  */
	unsigned char index;	/**< [0 -- 15] */
};

struct icv196T_HandleLines {
	int pid;		/**<  */
	struct icv196T_UserLine lines[ICV_LogLineNb]; /**<  */
};

struct icv196T_HandleInfo {
	struct icv196T_HandleLines handle[SkelDrvrCLIENT_CONTEXTS]; /**<  */
};

/* structure passed at gethandleinfo ioctl call */
struct icv196T_ModuleInfo {
	int ModuleFlag;		/**<  */
	struct icv196T_ModuleParam ModuleInfo; /**<  */
};

struct icv196U_LogLine {
	unsigned char group;	/**<  */
	unsigned char index;	/**<  */
};


struct icv196T_connect {
	struct icv196U_LogLine source; /**<  */
	int                    mode;   /**<  */
};

struct icv196T_Service {
	unsigned char module;	/**<  */
	unsigned char line;	/**<  */
	unsigned long data[icv_LineNb]; /**<  */
};

/* structure of a user line Address */
struct icv196T_LineUserAdd {
	char group;		/**<  */
	char index;		/**<  */
};

struct T_icv196Arg {
	struct icv196T_LineUserAdd Add; /**<  */
	short mode;			/**<  */
};

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup icv196lib Library API functions
 *@{
 */
	int icv196_get_handle(void);
	int icv196_put_handle(int h);
	int icv196_init_channel(int h, int module, int grp, int size, int dir);
	int icv196_read_channel(int h, int module, int grp, char *data);
	int icv196_write_channel(int h, int module, int grp, char *data);
	int icv196_connect(int h, short module, short line, short mode);
	int icv196_disconnect(int h, short module, short line);
	int icv196_get_info(int h, int m, int buff_sz, struct icv196T_ModuleInfo *buff);
	int icv196_set_to(int h, int *val);
/*@} end of group*/


#ifdef __cplusplus
}
#endif

#endif
