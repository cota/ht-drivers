/* ========================================================= */
/* Xmem daemon                                               */
/* Julian Lewis 11th March 2005                              */
/* ========================================================= */

#include <ipc.h>
#include <shm.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>        /* Error numbers */
#include <sys/file.h>
#include <a.out.h>
#include <ctype.h>
#include <time.h>

#include <libxmem.h>
#include <XmemDaemon.h>

extern int symp;

int  VmicDaemonWait(int timeout);
void VmicDaemonCall();

/* ========================================================= */
/* Statics                                                   */

/* Event logging */

static XmemEventMask log_mask = XmemEventMaskMASK; /* Events to be logged */
static XdEventLogEntries *event_log = NULL;

/* ========================================================= */
/* Convert time to a standard string routine.                */
/* Result is a pointer to a static string.                   */
/*    the format is: Thu-18/Jan/2001 08:25:14                */
/*                   day-dd/mon/yyyy hh:mm:ss                */

char *TimeToStr(time_t tod) {

static char tbuf[128];

char tmp[128];
char *yr, *ti, *md, *mn, *dy;

   bzero((void *) tbuf, 128);
   bzero((void *) tmp, 128);

   if (tod) {
      ctime_r(&tod, tmp);
      tmp[ 3] = 0; dy = &(tmp[0]);
      tmp[ 7] = 0; mn = &(tmp[4]);
      tmp[10] = 0; md = &(tmp[8]); if (md[0] == ' ') md[0] = '0';
      tmp[19] = 0; ti = &(tmp[11]);
      tmp[24] = 0; yr = &(tmp[20]);
      sprintf (tbuf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);

   } else sprintf(tbuf, "--- Zero ---");
   return tbuf;
}

/* ========================================================= */
/* Get the event log table in shared memory                  */

static XdEventLogEntries *result = NULL;

XdEventLogEntries *GetEventLogTable() {

key_t smemky;
unsigned smemid;

   if (result) return result;

   smemky = XmemGetKey(EVENT_LOG_NAME);
   smemid = shmget(smemky,sizeof(XdEventLogEntries),0666);
   if (smemid == -1) {
      if (errno == ENOENT) {
	 smemid = shmget(smemky,sizeof(XdEventLogEntries),IPC_CREAT | 0666);
	 if (smemid != -1) {
	    result = (XdEventLogEntries *) shmat(smemid,(char *) 0,0);
	    if ((int) result != -1) {
	       bzero((void *) result, sizeof(XdEventLogEntries));
	       return result;
	    }
	 }
      }
   } else {
      result = (XdEventLogEntries *) shmat(smemid,(char *) 0,0);
      if ((int) result != -1) return result;
   }
   XmemErrorCallback(XmemErrorSYSTEM,errno);
   return NULL;
}

/* ========================================================= */
/* Print out a callback structure                            */

char *CallbackToStr(XmemCallbackStruct *cbs) {

static char str[128];

   if (cbs) {
      bzero((void *) str, 128);
      switch (cbs->Mask) {
	 case XmemEventMaskTIMEOUT:
	    sprintf(str,"Timeout");
	 break;

	 case XmemEventMaskUSER:
	    sprintf(str,"User event:%d From node:%s",(int) cbs->Data,XmemGetNodeName(cbs->Node));
	 break;

	 case XmemEventMaskSEND_TABLE:
	    sprintf(str,"Send table:%s From node:%s",XmemGetTableName(cbs->Table),XmemGetNodeName(cbs->Node));
	 break;

	 case XmemEventMaskTABLE_UPDATE:
	    sprintf(str,"Update table:%s From node:%s",XmemGetTableName(cbs->Table),XmemGetNodeName(cbs->Node));
	 break;

	 case XmemEventMaskINITIALIZED:
	    sprintf(str,"Initialized:%d From node:%s",(int) cbs->Data,XmemGetNodeName(cbs->Node));
	 break;

	 case XmemEventMaskKILL:
	    sprintf(str,"Kill daemon received");
	 break;

	 case XmemEventMaskIO:
	    sprintf(str,"IO error:%s",XmemErrorToString(cbs->Data));
	 break;

	 case XmemEventMaskSYSTEM:
	    sprintf(str,"System error:%d",(int) cbs->Data);
	 break;

	 case XmemEventMaskSOFTWARE:
	    if (cbs->Data == XmemErrorSUCCESS) {
	       sprintf(str,
		       "Information:");
	    } else {
	       sprintf(str,
		       "Software error:%s",
		       XmemErrorToString(cbs->Data));
	    }
	 break;

	 default: break;
      }
      return str;
   }
   return NULL;
}

/* ========================================================= */
/* Log an event in the log file                              */
/* This routine writes EVENT_LOG_ENTRIES events into a log   */
/* file each time it has collected enough events.            */

int LogEvent(XmemCallbackStruct *cbs, char *text) {

int idx, cnt, logit;
FILE *fp;
time_t tod;
XdEventLogEntry *evp;
char *elog;

   tod       = time(NULL);
   idx       = event_log->NextEntry;
   evp       = &(event_log->Entries[idx]);
   evp->Time = tod;
   logit     = 0;

   if (text) {
      strncpy((char *) (evp->Text), text, EVENT_MESSAGE_SIZE -1);
      fprintf(stderr,"%s XmemDaemon: %s\n",TimeToStr(tod),text);
      logit++;
   }

   if (cbs) {
      if (log_mask & cbs->Mask) {
	 evp->CbEvent = *cbs;
	 // fprintf(stderr,"XmemDaemon: %s %s\n",TimeToStr(tod),CallbackToStr(cbs));
	 logit++;
      }
   }

   if (logit) {
      if (++idx >= EVENT_LOG_ENTRIES) {
	 umask(0);
	 elog = XmemGetFile(EVENT_LOG);
	 fp = fopen(elog,"w");
	 if (fp) {
	    evp = &(event_log->Entries[idx]);
	    cnt = fwrite(event_log, sizeof(XdEventLogEntries), 1, fp);
	    if (cnt <= 0) {
	       perror("XmemDaemon");
	       fprintf(stderr,"XmemDaemon: Error: Can't WRITE:%s\n",elog);
	       fclose(fp);
	       bzero((void *) event_log, sizeof(XdEventLogEntries));
	       return 0;
	    }
	 } else {
	    perror("XmemDaemon");
	    fprintf(stderr,"XmemDaemon: Error: Can't OPEN:%s for WRITE\n",elog);
	    bzero((void *) event_log, sizeof(XdEventLogEntries));
	    return 0;
	 }
	 fclose(fp);
	 bzero((void *) event_log, sizeof(XdEventLogEntries));
	 return 1;
      } else event_log->NextEntry = idx;
   }
   return 1;
}

/* ========================================================= */
/* Callback routins to handle events                         */

void callback(XmemCallbackStruct *cbs) {

#ifdef FLUSH_ON_STARTUP
   XmemTableId atids;
#endif

unsigned long msk;
int i;
XmemNodeId me;
XmemNodeId nodes, acnds;
XmemTableId tid;
XmemMessage mess;
XmemError err;
unsigned long user, longs, *smemad;
char txt[80];

   if (cbs == NULL) return;

   me = XmemWhoAmI();

   tid = cbs->Table;
   if (tid) {
      err = XmemGetTableDesc(tid,&longs,&nodes,&user);
      if (err != XmemErrorSUCCESS) return;
   } else { tid = 0; longs = 0; nodes = 0; user = 0; }

   for (i=0; i<XmemEventMASKS; i++) {
      msk = 1 << i;
      if (cbs->Mask & msk) {
	 switch ((XmemEventMask) msk) {


	    case XmemEventMaskSEND_TABLE:

	       if (((tid == 0) || (cbs->Node == me)) && (symp == 0)) return;

	       if (me & nodes) {
		  if (user & IN_SHMEM) {
		     smemad = XmemGetSharedMemory(tid);
		     if (smemad) {
			XmemSendTable(tid,smemad,longs,0,1);
			sprintf(txt,"Written table: %s For requesting node: %s",
				     XmemGetTableName(tid),
				     XmemGetNodeName(cbs->Node));
			LogEvent(NULL,txt);
		     }
		  } else {
		     XmemSendTable(tid,NULL,0,0,1);
		     sprintf(txt,"Flushed table: %s For requesting node: %s",
				  XmemGetTableName(tid),
				  XmemGetNodeName(cbs->Node));
		     LogEvent(NULL,txt);
		  }
	       }
	    break;

	    case XmemEventMaskTABLE_UPDATE:

	       if (((tid == 0) || (cbs->Node == me)) && (symp == 0)) return;

	       if (user & IN_SHMEM) {
		  smemad = XmemGetSharedMemory(tid);
		  if (smemad) {
		     XmemRecvTable(tid,smemad,longs,0);
		     sprintf(txt,"Received table: %s From updating node: %s",
				  XmemGetTableName(tid),
				  XmemGetNodeName(cbs->Node));
		     LogEvent(NULL,txt);
		  }
		  if (user & ON_DISC) {
		     if (nodes & me) {
			acnds = XmemGetAllNodeIds() & nodes;
			if ((unsigned long) (~me & acnds) < (unsigned long) me) {
			   XmemWriteTableFile(tid);
			   sprintf(txt,"Disc file updated for table: %s For requesting node: %s",
					XmemGetTableName(tid),
					XmemGetNodeName(cbs->Node));
			   LogEvent(NULL,txt);
			}
		     }
		  }
	       }
	    break;

	    case XmemEventMaskINITIALIZED:

	       if (cbs->Data == XmemInitMessageRESET) {

		  /* Acknowledge Init and send tables */

		  mess.MessageType = XmemMessageTypeINITIALIZE_ME;
		  mess.Data        = XmemInitMessageSEEN_BY;
		  XmemSendMessage(XmemALL_NODES,&mess);

		  /* Send tables I own if I am the highest active ID */

#ifdef FLUSH_ON_STARTUP
		  atids = XmemGetAllTableIds();
		  for (i=0; i<XmemMAX_TABLES; i++) {
		     msk = 1 << i;
		     if (msk & atids) {
			XmemGetTableDesc(msk,&longs,&nodes,&user);
			if (nodes & me) {
			   acnds = XmemGetAllNodeIds() & nodes;
			   if ((unsigned long) (~me & acnds) < (unsigned long) me) {
			      XmemSendTable(msk,NULL,0,0,1);
			      sprintf(txt,"Flushed table: %s For initialized node: %s",
					   XmemGetTableName(msk),
					   XmemGetNodeName(cbs->Node));
			      LogEvent(NULL,txt);
			   }
			}
		     }
		  }
#endif
	       }
	       if (cbs->Data != XmemInitMessageSEEN_BY) LogEvent(cbs,NULL);
	    break;

	    case XmemEventMaskKILL:
	       sprintf(txt,"Received a Kill, EXIT: Deamon is DEAD");
	       LogEvent(cbs,txt);
	       exit(1);
	    break;

	    case XmemEventMaskUSER:
	       sprintf(txt,"User message received");
	       LogEvent(cbs,txt);
	    break;

	    case XmemEventMaskTIMEOUT:
	    case XmemEventMaskIO:
	    case XmemEventMaskSYSTEM:
	    case XmemEventMaskSOFTWARE:
	    break;

	    default: break;
	 }
      }
   }
}

/* ========================================================= */
/* Xmem daemon, register callback, and wait.                 */

int main(int argc,char *argv[]) {

XmemEventMask emsk;
XmemError err;
XmemMessage mes;
XmemTableId tid;
XmemNodeId me, nodes;
int i, warmst;
unsigned long longs, user, msk;
char txt[80];

   event_log = GetEventLogTable();

   err = XmemInitialize(XmemDeviceANY);
   if (err == XmemErrorSUCCESS) {

      XmemUnBlockCallbacks();

      if (event_log) {

	 warmst = XmemCheckForWarmStart();
	 if (warmst) LogEvent(NULL,"Warm Start: Begin ...");
	 else        LogEvent(NULL,"Cold Start: Initialize Tables: Begin ...");

	 emsk = XmemEventMaskMASK;
	 err = XmemRegisterCallback(callback,emsk);
	 if (err == XmemErrorSUCCESS) {

	    /* For cold starts read tables from disc */

	    if (!warmst) {
	       tid = XmemGetAllTableIds();
	       me = XmemWhoAmI();
	       for (i=0; i<XmemMAX_TABLES; i++) {
		  msk = 1 << i;
		  if (tid & msk) {
		     XmemGetTableDesc(msk,&longs,&nodes,&user);
		     if (nodes & me) {
			if ((user & IN_SHMEM) && (user & ON_DISC)) {
			   sprintf(txt,
				   "Reading table:%s id:%d from disc",
				   XmemGetTableName((XmemTableId) msk),
				   (int) msk);
			   LogEvent(NULL,txt);
			   err = XmemReadTableFile(msk);
			   if (err != XmemErrorSUCCESS) {
			      LogEvent(NULL,"WARNING: Failed to read table from disc");
			   }
			}
		     }
		  }
	       }
	       mes.MessageType = XmemMessageTypeINITIALIZE_ME;
	       mes.Data        = XmemInitMessageRESET;          /* Tell the world to send me their tables */
	       XmemSendMessage(XmemALL_NODES,&mes);             /* and that I exist */
	    } else {
	       mes.MessageType = XmemMessageTypeINITIALIZE_ME;
	       mes.Data        = XmemInitMessageSEEN_BY;        /* Tell the world I exist */
	       XmemSendMessage(XmemALL_NODES,&mes);             /* it might be interesting to do this in a loop */
	    }
	    LogEvent(NULL,"XmemDaemon: Running OK");
	    while(1) {
	      if (VmicDaemonWait(1000)) {
		XmemBlockCallbacks();
		VmicDaemonCall();
	      }
	      XmemUnBlockCallbacks();
	    }
	 } else LogEvent(NULL,"Can't register callback");
      } else LogEvent(NULL,"Can't get: EVENT_LOG shared memory segment");
   } else LogEvent(NULL,"Can't initialize library");

   LogEvent(NULL,"Fatal Error: Daemon Teminated");
   exit((int) err);
}
