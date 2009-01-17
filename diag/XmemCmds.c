#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <errno.h>
extern int errno;

#include "../daemon/XmemDaemon.h"

static XmemNodeId  all_nodes  = 0;
static XmemTableId all_tables = 0;

static XmemTableId ctid = XmemTableId_01;

static int timeout = 1000;  /* 10 Seconds */

static char *ENames[XmemEventMASKS] = {
   "TimeOut",
   "SendTable",
   "Message",
   "TableUpdate",
   "Initialized",
   "IoError",
   "Kill",
   "SoftwareError",
   "SystemError" };

static char *IOENames[XmemIoERRORS] = {
   "Parity",
   "Rogue",
   "LostContact",
   "Hardware",
   "Software" };

static char *InitMess[XmemInitMessageMESSAGES] = {
   "Acknowledged",
   "Reset",
   "UserDefined" };

static char *MSNames[XmemMessageTypeCOUNT] = {
   "SendTable",
   "Message",
   "TableUpdated",
   "Initialize",
   "Kill" };

static XdEventLogEntries *event_log = NULL;

/* ========================================================= */
/* Callback routins to handle events                         */

void callback(XmemCallbackStruct *cbs) {

unsigned long msk, emsk, data;
int i, j;

   printf("Callback:");
   for (i=0; i<XmemEventMASKS; i++) {
      msk = 1 << i;
      if (cbs->Mask & msk) {
	 data = cbs->Data;
	 if ((msk & XmemEventMaskSOFTWARE) && (cbs->Data == XmemErrorSUCCESS)) {
	    printf("Information: ");
	 } else {
	    printf("%s: ",ENames[i]);
	 }
	 switch ((XmemEventMask) msk) {

	    case XmemEventMaskTIMEOUT:
	       printf("Timeout\n");
	    break;

	    case XmemEventMaskSEND_TABLE:
	       printf("%s To:%s\n",
		      XmemGetTableName(cbs->Table),
		      XmemGetNodeName(cbs->Node));
	    break;

	    case XmemEventMaskUSER:
	       printf("0x%X (%d) From:%s\n",
		      (int) data,
		      (int) data,
		      XmemGetNodeName(cbs->Node));
	    break;

	    case XmemEventMaskTABLE_UPDATE:
	       printf("%s By:%s\n",
		      XmemGetTableName(cbs->Table),
		      XmemGetNodeName(cbs->Node));
	    break;

	    case XmemEventMaskINITIALIZED:
	       printf("%s Message:0x%X (%d/",
		      XmemGetNodeName(cbs->Node),
		      (int) data,
		      (int) data);
	       if (data < XmemInitMessageMESSAGES)
		  printf("%s)\n",InitMess[data]);
	       else
		  printf("%s)\n",InitMess[XmemInitMessageUSER_DEFINED]);
	    break;

	    case XmemEventMaskIO:
	    case XmemIoERRORS:
	       for (j=0; j<XmemIoERRORS; j++) {
		  emsk = 1 << j;
		  if (emsk & data) printf("%s ",IOENames[j]);
	       }
	       printf("\n");
	    break;

	    case XmemEventMaskKILL:
	       printf("\n");
	    break;

	    case XmemEventMaskSYSTEM:
	       printf("%s (%d)\n",
		      strerror((int) data),
		      (int) data);
	    break;

	    case XmemEventMaskSOFTWARE:
	       if (data != XmemErrorSUCCESS) {
		  printf("%s\n",
			 XmemErrorToString((XmemError) data));
	       }
	    default: break;
	 }
      }
   }
}

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

   } else sprintf (tbuf, "--- Zero ---");
   return (tbuf);
}

/* ========================================================= */
/* Get the event log table in shared memory                  */

XdEventLogEntries *GetEventLogTable() {

key_t smemky;
unsigned smemid;

static XdEventLogEntries *result = NULL;

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
	       sprintf(str,"Information:");
	    } else {
	       sprintf(str,"Software error:%s",XmemErrorToString(cbs->Data));
	    }
	 break;

	 default: break;
      }
      return str;
   }
   return NULL;
}

/* ========================================================= */
/* Register callback, arg is the event mask                  */

int RegisterCallback(int arg) {
ArgVal   *v;
AtomType  at;
XmemEventMask emsk;
XmemError err;
int i;
unsigned long msk;
   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 arg++;
	 v = &(vals[arg]);
	 at = v->Type;

	 printf("Possible events are:\n");

	 for (i=0; i<XmemEventMASKS; i++) {
	    msk = 1 << i;
	    printf("Event:0x%03X: %s\n",(int) msk, ENames[i]);
	 }

	 return arg;
      }
   }

   if (at == Numeric) {
      arg++;
      emsk = (XmemEventMask) v->Number;
   } else {
      emsk = XmemGetRegisteredEvents();
      if (emsk) {
	 for (i=0; i<XmemEventMASKS; i++) {
	    msk = 1 << i;
	    if (emsk & msk)
	       printf("Registered:0x%03X: %s\n",(int) msk, ENames[i]);
	 }
      } else printf("Registered: No events\n");
      return arg;
   }

   err = XmemRegisterCallback(callback,emsk);
   if (err != XmemErrorSUCCESS)
      printf("XmemRegisterCallback: Error:%s\n",XmemErrorToString(err));
   else
      if (emsk) printf("XmemRegisterCallback: Registered:0x%X OK\n",(int) emsk);
      else      printf("XmemRegisterCallback: Unregistered all OK\n");
   return arg;
}

/* ========================================================= */
/* Call XmemGetAllNodeIds and print list                     */

int GetAllNodeIds(int arg) {

int i;
unsigned long msk, nid;

   all_nodes = XmemGetAllNodeIds();
   printf("NodeIdMask:0x%02X\n",(int) all_nodes);
   for (i=0; i<XmemMAX_NODES; i++) {
      msk = 1 << i;
      if (all_nodes & msk) printf("NodeId:0x%02X: NodeName:%s\n",
				  (int) msk,
				  XmemGetNodeName((XmemNodeId) msk));
   }
   printf("\n");

   nid = XmemWhoAmI();
   if (nid) {
      printf("This node has Id: 0x%02X NodeName:%s\n",
	     (int) nid,
	     XmemGetNodeName((XmemNodeId) nid));
   }
   i = XmemCheckForWarmStart();
   if (i) printf("This is a WARM start\n");
   else   printf("This is a COLD start\n");
   return ++arg;
}

/* ========================================================= */
/* Get the name od a node from its Id                        */

int GetNodeName(int arg) {
ArgVal   *v;
AtomType  at;
unsigned long nid, msk;
int i;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 nid = v->Number;
      }
   } else nid = XmemGetAllNodeIds();

   for (i=0; i<XmemMAX_NODES; i++) {
      msk = 1 << i;
      if (msk & nid)
	 printf("NodeId:0x%02X NodeName:%s\n",(int) msk, XmemGetNodeName((XmemNodeId) msk));
   }
   return ++arg;
}

/* ========================================================= */
/* Get the ID of a node from its name                        */

int GetNodeId(int arg) {
ArgVal   *v;
AtomType  at;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Alpha) {
      arg++;
      printf("%s has ID:0x%X\n",
	     v->Text,
	     XmemGetNodeId(v->Text));
   } else printf("No node name supplied\n");
   return arg;
}

/* ========================================================= */
/* Get the complete list of all defined tables               */

int GetAllTableIds(int arg) {
unsigned long msk;
int i;

   all_tables = XmemGetAllTableIds();
   printf("TableIds:0x%08X\n",(int) all_tables);
   for (i=0; i<XmemMAX_TABLES; i++) {
      msk = 1 << i;
      if (msk & all_tables)
	 printf("TableId:0x%08X: TableName:%s\n",
		(int) msk,
		XmemGetTableName((XmemTableId) msk));
   }
   return ++arg;
}

/* ========================================================= */
/* Get the name of a table from its ID                       */

int GetTableName(int arg) {
ArgVal   *v;
AtomType  at;
unsigned long tid, msk;
int i;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 tid = v->Number;
      }
   } else tid = XmemGetAllTableIds();

   for (i=0; i<XmemMAX_TABLES; i++) {
      msk = 1 << i;
      if (msk & tid)
	 printf("TableId:0x%08X TableName:%s\n",(int) msk, XmemGetTableName((XmemTableId) msk));
   }
   return ++arg;
}

/* ========================================================= */
/* Get the ID of a table from its name                       */

int GetTableId(int arg) {
ArgVal   *v;
AtomType  at;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Alpha) {
      arg++;
      printf("%s has ID:0x%X\n",
	     v->Text,
	     XmemGetTableId(v->Text));
   } else printf("No table name supplied\n");
   return arg;
}

/* ========================================================= */
/* Get the description of a table from its ID                */

int GetTableDesc(int arg) {
ArgVal   *v;
AtomType  at;
unsigned long tid, nid, msk, user, longs;
int i;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 tid = v->Number;
      }
   } else tid = XmemGetAllTableIds();

   for (i=0; i<XmemMAX_TABLES; i++) {
      msk = 1 << i;
      if (msk & tid) {
	 XmemGetTableDesc(msk,(unsigned long *) &longs,(XmemNodeId *) &nid, &user);
	 printf("0x%08X:%s L:0x%04X[%03d] B:0x%06X[%03d] N:0x%02X U:0x%X\n",
		(int) msk,
		XmemGetTableName((XmemTableId) msk),
		(int) longs,
		(int) longs,
		(int) longs*sizeof(long),
		(int) longs*sizeof(long),
		(int) nid,
		(int) user);
      }
   }
   return arg;
}

/* ========================================================= */
/* Wait for an event. Arg contains optional timeout          */

int Wait(int arg) {
ArgVal   *v;
AtomType  at;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 timeout = v->Number;
      }
   }
   printf("Wait:EventMask:0x%X\n",XmemWait(timeout));
   return arg;
}

/* ========================================================= */
/* Non blocking event poll                                   */

int Poll(int arg) {

   arg++;
   if (XmemPoll() == 0) printf("Poll:No events on queue\n");
   return arg;
}

/* ========================================================= */
/* Send table from shared memory                             */

int SendTable(int arg) {
ArgVal   *v;
AtomType  at;
unsigned long tid, nid, user, longs;

long *table;
XmemError err;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   tid = ctid;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 tid = v->Number;
      }
   }
   table = XmemGetSharedMemory(tid);
   if (table) {
      ctid = tid;
      err = XmemGetTableDesc(tid,&longs,(XmemNodeId *) &nid, &user);
      if (err != XmemErrorSUCCESS) return arg;
      err = XmemSendTable(tid,table,longs,0,1);
      if (err == XmemErrorSUCCESS) printf("Sent:%s from shmem OK\n",
					  XmemGetTableName(tid));
      else                         printf("Error:Table NOT sent:%s\n",
					  XmemErrorToString(err));
      return arg;
   }
   printf("Can't get shared memory segment\n");
   return arg;
}

/* ========================================================= */
/* Recieve table and update shared memory                    */

int RecvTable(int arg) {
ArgVal   *v;
AtomType  at;
unsigned long tid, nid, user, longs;

long *table;
XmemError err;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   tid = ctid;
   if (at == Numeric) {
      arg++;
      if (v->Number) {
	 tid = v->Number;
      }
   }
   table = XmemGetSharedMemory(tid);
   if (table) {
      ctid = tid;
      err = XmemGetTableDesc(tid,&longs,(XmemNodeId *) &nid, &user);
      if (err != XmemErrorSUCCESS) return arg;
      err = XmemRecvTable(tid,table,longs,0);
      if (err == XmemErrorSUCCESS) printf("Received:%s in shmem OK\n",
					  XmemGetTableName(tid));
      else                         printf("Error:Table NOT received:%s\n",
					  XmemErrorToString(err));
      return arg;
   }
   printf("Error:Can't get shared memory segment\n");
   return arg;
}

/* ========================================================= */
/* Send a message                                            */

int SendMessage(int arg) {
ArgVal   *v;
AtomType  at;
int i, j;
unsigned long dta, nid;
XmemMessageType mtyp;
XmemMessage mess;
XmemError err;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   if (at == Operator) {
      if (v->OId == OprNOOP) {
	 arg++;

	 for (i=0; i<XmemMessageTypeCOUNT; i++) {
	    printf("%d:%s\t",i,MSNames[i]);
	    switch ((XmemMessageType) i) {
	       case XmemMessageTypeTABLE_UPDATE:
	       case XmemMessageTypeSEND_TABLE:
		  printf("<NdIdMask><TbIdMask>");
	       break;

	       case XmemMessageTypeUSER:
		  printf("<NdIdMask><Data>");
	       break;

	       case XmemMessageTypeINITIALIZE_ME:
		  printf("<NdIdMask><Sub:");
		  for (j=0; j<XmemInitMessageMESSAGES; j++) {
		     printf("%d:%s ",j,InitMess[j]);
		  }
		  printf("[Or greater]>");
	       break;

	       case XmemMessageTypeKILL:
		  printf("<NdIdMask>");
	       break;

	       default: break;
	    }
	    printf("\n");
	 }
      }
      return arg;
   }

   if (at == Numeric) {
      arg++;
      mtyp = v->Number;
      v = &(vals[arg]);
      at = v->Type;
      if (mtyp >= XmemMessageTypeCOUNT) {
	 printf("Illegal message type: %d Range is [0..%d]\n",
		(int) mtyp,
		(int) XmemMessageTypeCOUNT -1);
	 return arg;
      }
   } else {
      printf("Error:No message specified\n");
      return arg;
   }

   nid = XmemWhoAmI();
   if (at == Numeric) {
      arg++;
      nid = v->Number;
      if (nid == 0) nid = XmemALL_NODES;
      v = &(vals[arg]);
      at = v->Type;
   }

   dta = 0;
   if (at == Numeric) {
      arg++;
      dta = v->Number;
      v = &(vals[arg]);
      at = v->Type;
   }

   mess.MessageType = mtyp;
   mess.Data        = dta;
   err = XmemSendMessage(nid,&mess);
   if (err == XmemErrorSUCCESS) printf("Message:%s %d %d sent OK\n",
				       MSNames[mtyp],
				       (int) nid,
				       (int) dta);
   else                         printf("Error:Message NOT sent:%s\n",
				       XmemErrorToString(err));
   return arg;
}

/* ========================================================= */
/* Check for table updates                                   */

int CheckTables(int arg) {
unsigned long tid;
unsigned long msk;
int i;

   arg++;
   tid = XmemCheckTables();
   if (tid) {
      printf("Updated: TableIdMask:0x%08X\n",(int) tid);
      for (i=0; i<XmemMAX_TABLES; i++) {
	 msk = 1 << i;
	 if (msk & tid)
	    printf("Updated: TableId:0x%08X TableName:%s\n",
		   (int) msk,
		   XmemGetTableName((XmemTableId) msk));
      }
      return arg;
   }
   printf("Updated: 0 => No table updates since last call\n");
   return arg;
}

/* ========================================================= */
/* Save a table to disc                                      */

int SaveTable(int arg) {
ArgVal   *v;
AtomType  at;

XmemNodeId nodes, nid;
XmemTableId tid;
unsigned long user, longs;
XmemError err;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   tid = ctid;
   if (at == Numeric) {
      arg++;
      tid = v->Number;
   }
   err = XmemGetTableDesc(tid,&longs,&nodes,&user);
   if (err != XmemErrorSUCCESS) {
      printf("Error:Can't get Id:0x%08x table description:%s\n",
	     (int) tid,
	     XmemErrorToString(err));
      return arg;
   }
   ctid = tid;
   nid = XmemWhoAmI();
   if (nid & nodes) {
      err = XmemWriteTableFile(tid);
      if (err != XmemErrorSUCCESS) printf("Error:Can't write table to disc:%s\n",
					  XmemErrorToString(err));
      else                         printf("Saved:%s to disc: OK\n",
					  XmemGetTableName(tid));
   } else printf("Error:No write-access: File belongs to another node\n");
   return arg;
}

/* ========================================================= */
/* Load a table from disc                                    */

int LoadTable(int arg) {
ArgVal   *v;
AtomType  at;

XmemNodeId nodes, nid;
XmemTableId tid;
unsigned long user, longs;
XmemError err;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   tid = ctid;
   if (at == Numeric) {
      arg++;
      tid = v->Number;
   }
   err = XmemGetTableDesc(tid,&longs,&nodes,&user);
   if (err != XmemErrorSUCCESS) {
      printf("Error:Can't get Id:0x%08x table description:%s\n",
	     (int) tid,
	     XmemErrorToString(err));
      return arg;
   }
   ctid = tid;
   nid = XmemWhoAmI();
   if (nid & nodes) {
      err = XmemReadTableFile(tid);
      if (err != XmemErrorSUCCESS) printf("Error:Can't read table from disc:%s\n",
					  XmemErrorToString(err));
      else                         printf("Loaded:%s from disc: OK\n",
					  XmemGetTableName(tid));
   } else printf("Error:This node should receive the table from memory, not disc\n");
   return arg;
}

/*****************************************************************/

void EditMemory(char *name, unsigned long *table, unsigned long longs, int flag) {

unsigned long addr, data, nadr;
char c, *cp, str[128];
int n, radix, cnt;

   printf("EditMemory: ?:Comment /:Open CR:Next .:Exit x:Hex d:Dec s:String\n\n");

   addr = 0;
   radix = 16;

   c = '\n';

   while (1) {

      printf("%s ",name);
      if (radix == 16) {
	 if (c=='\n') printf("Addr:0x%04x: 0x%04x ",(int) addr,(int) table[addr]);
      } else {
	 if (c=='\n') printf("Addr:%04d: %5d ",(int) addr,(int) table[addr]);
      }

      c = (char) getchar();

      if (c == '/') {
	 bzero((void *) str, 128); n = 0;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 nadr = strtoul(str,&cp,radix);
	 if (cp != str) addr = nadr;
      }

      else if (c == 'x')  { radix = 16; addr--; continue; }
      else if (c == 'd')  { radix = 10; addr--; continue; }
      else if (c == '.')  { c = getchar(); printf("\n"); break; }
      else if (c == 'q')  { c = getchar(); printf("\n"); break; }
      else if (c == 's')  {
	 cnt = 0; cp = (char *) &(table[addr]);
	 while ((c = (char) getchar()) != '\n') {
	    cp[0] = c;
	    cp[1] = '\0';
	    if (cnt++ > 32) break;
	    cp++;
	 }
	 cnt = 0; cp = (char *) &(table[addr]);
	 while (isascii((int) *cp)) {
	    putchar(*cp);
	    if ((*cp == '\0') || (*cp == '\n') || (cnt++ > 32)) break;
	    cp++;
	 }
	 printf("\n");
	 continue;
      } else if (c == '\n') {
	 addr++;
	 if (addr >= longs) {
	    addr = 0;
	    printf("\n===\n");
	 }
      }
      else {
	 bzero((void *) str, 128); n = 0;
	 str[n++] = c;
	 while ((c != '\n') && (n < 128)) c = str[n++] = (char) getchar();
	 data = strtoul(str,&cp,radix);
	 if (cp != str) {
	    if (flag) table[addr] = data;
	    else printf("\7");
	 }
      }
   }
}

/* ========================================================= */
/* Edit shared memory                                        */

int EditSharedMemory(int arg) {
ArgVal   *v;
AtomType  at;

XmemNodeId nodes, nid;
XmemTableId tid;
unsigned long user, longs;
XmemError err;
unsigned long *table;
char *name;

   arg++;
   v = &(vals[arg]);
   at = v->Type;

   tid = ctid;
   if (at == Numeric) {
      arg++;
      tid = v->Number;
   }
   table = XmemGetSharedMemory(tid);
   if (table) {
      ctid = tid;
      err = XmemGetTableDesc(tid,&longs,&nodes,&user);
      if (err != XmemErrorSUCCESS) {
	 printf("Error:Can't get Id:0x%08x table description:%s\n",
		(int) tid,
		XmemErrorToString(err));
	 return arg;
      }
      nid = XmemWhoAmI();
      name = XmemGetTableName(tid);
      if (nid & nodes) {
	 printf("Editing: %s in RW mode: FULL-ACCESS\n",name);
	 EditMemory(name,table,longs,1);
      } else {
	 printf("Editing: %s in RO mode: No WRITE-ACCESS\n",name);
	 EditMemory(name,table,longs,0);
      }
   }
   return arg;
}

/* ========================================================= */
/* Show what in the shared memory segment                    */

int ShowDaemonEventLog(int arg) {

XdEventLogEntry *evp;
int i;

   arg++;
   event_log = GetEventLogTable();
   if (event_log) {
      if (event_log->NextEntry) {
	 for (i=0; i<event_log->NextEntry; i++) {
	    evp = &(event_log->Entries[i]);
			   printf("%02d: %s ",i,TimeToStr(evp->Time));
			   printf("%s: ",CallbackToStr(&(evp->CbEvent)));
	    if (evp->Text) printf("%s",(char *) &(evp->Text[0]));
			   printf("\n");
	 }
      } else printf("Empty: No logged events in memory\n");
   }
   return arg;
}

/* ========================================================= */
/* Print out the file for a node                             */

int ShowDaemonEventHistory(int arg) {

ArgVal   *v;
AtomType  at;
XdEventLogEntries elog;
XdEventLogEntry *evp;
int i, cnt;
char *elg;
FILE *fp;
XmemNodeId nid;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   nid = XmemWhoAmI();
   if (at == Numeric) {
      arg++;
      nid = v->Number;
   }
   umask(0);
   elg = XmemGetFile(EVENT_LOG);
   fp = fopen(elg,"r");
   if (fp) {
      bzero((void *) &elog, sizeof(XdEventLogEntries));
      cnt = fread(&elog, sizeof(XdEventLogEntries), 1, fp);
      if (cnt <= 0) {
	 perror("ShowDaemonEventHistory");
	 fprintf(stderr,"ShowDaemonEventHistory: Error: Can't READ:%s\n",elg);
	 fclose(fp);
	 return arg;
      }
   } else {
      perror("ShowDaemonEventHistory");
      fprintf(stderr,"ShowDaemonEventHistory: Error: Can't OPEN:%s for READ\n",elg);
      return arg;
   }
   fclose(fp);

   printf("Showing file:%s\n",elg);
   for (i=0; i<elog.NextEntry; i++) {
      evp = &(elog.Entries[i]);
		     printf("%02d: %s ",i,TimeToStr(evp->Time));
		     printf("%s: ",CallbackToStr(&(evp->CbEvent)));
      if (evp->Text) printf("%s",(char *) &(evp->Text[0]));
		     printf("\n");
   }
   return arg;
}

/* ========================================================= */
/* Wipe errors out                                           */

int ClearDaemonEventLog(int arg) {

   arg++;

   event_log = GetEventLogTable();
   if (event_log) {
      bzero((void *) event_log, sizeof(XdEventLogEntries));
      printf("ClearDaemonEventLog: Done OK\n");
      return arg;
   }
   return arg;
}

/* ========================================================= */

int KillDaemon(int arg) {

ArgVal   *v;
AtomType  at;
XmemMessage mes;
XmemNodeId nid;
XmemError err;

   arg++;
   v = &(vals[arg]);
   at = v->Type;
   nid = XmemWhoAmI();
   if (at == Numeric) {
      arg++;
      nid = v->Number;
      if (nid == 0) nid = XmemALL_NODES;
   }
   mes.MessageType = XmemMessageTypeKILL;
   mes.Data = XmemWhoAmI();
   err = XmemSendMessage(nid,&mes);
   if (err == XmemErrorSUCCESS) printf("Sent KILL to nodes: 0x%02X\n",(int) nid);
   else printf("KillDaemon: Error: %s\n",XmemErrorToString(err));
   return arg;
}

/* ========================================================= */

int Init(int arg) {

XmemMessage mes;
XmemError err;

   arg++;

   mes.MessageType = XmemMessageTypeINITIALIZE_ME;
   mes.Data = XmemInitMessageRESET;
   err = XmemSendMessage(XmemALL_NODES,&mes);
   if (err == XmemErrorSUCCESS) printf("Sent Initialize to all nodes");
   else printf("Init: Error: %s\n",XmemErrorToString(err));
   return arg;
}
