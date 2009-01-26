/**
 * @file XmemDaemon.c
 *
 * @brief Daemon for Xmem. Built on top of libxmem.
 *
 * @author Julian Lewis
 *
 * @date Created on 11/03/2005
 *
 * @version 1.1 Emilio G. Cota 23/01/2009
 *
 * @version 1.0 Julian Lewis
 */
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
#include <errno.h>
#include <sys/file.h>
#include <a.out.h>
#include <ctype.h>
#include <time.h>

#include <libxmem.h>
#include <XmemDaemon.h>


extern int symp;

/* Event logging */
static XmemEventMask log_mask = XmemEventMaskMASK; /* Events to be logged */
static XdEventLogEntries *event_log = NULL;
static XdEventLogEntries *result = NULL;

int  VmicDaemonWait(int timeout);
void VmicDaemonCall();


/**
 * TimeToStr - Convert time to a standard string
 *
 * @param tod: time to convert
 *
 * The format is:
 *
 * Thu-18/Jan/2001 08:25:14
 *
 * day-dd/mon/yyyy hh:mm:ss
 *
 * @return pointer to a static string
 */
char *TimeToStr(time_t tod)
{
	static char tbuf[128];

	char tmp[128];
	char *yr, *ti, *md, *mn, *dy;

	bzero((void *)tbuf, 128);
	bzero((void *)tmp, 128);

	if (tod) {
		ctime_r(&tod, tmp);

		tmp[ 3] = 0;
		dy = &(tmp[0]);

		tmp[ 7] = 0;
		mn = &(tmp[4]);

		tmp[10] = 0;
		md = &(tmp[8]);
		if (md[0] == ' ')
			md[0] = '0';

		tmp[19] = 0;
		ti = &(tmp[11]);

		tmp[24] = 0;
		yr = &(tmp[20]);

		sprintf (tbuf, "%s-%s/%s/%s %s", dy, md, mn, yr, ti);
	} else
		sprintf(tbuf, "--- Zero ---");

	return tbuf;
}


/**
 * GetEventLogTable - Get the event log table in shared memory
 *
 * @param : none
 *
 * @return a pointer to the acquired shared memory segment
 */
XdEventLogEntries *GetEventLogTable()
{
	key_t smemky;
	unsigned smemid;

	if (result)
		return result;

	smemky = XmemGetKey(EVENT_LOG_NAME);
	smemid = shmget(smemky, sizeof(XdEventLogEntries), 0666);

	if (smemid != -1) {
		result = (XdEventLogEntries *)shmat(smemid, (char *)0, 0);

		if ((int)result != -1)
			return result;
		else
			goto errsys;
	}

	if (errno == ENOENT) {
		/* a shared memory identifier does not exist; create it */
		smemid = shmget(smemky, sizeof(XdEventLogEntries),
				IPC_CREAT | 0666);

		if (smemid == -1)
			goto errsys;

		result = (XdEventLogEntries *)shmat(smemid, (char *)0, 0);

		if ((int)result == -1)
			goto errsys;

		/* clear the acquired segment and return */
		bzero((void *)result, sizeof(XdEventLogEntries));
		return result;
	}

errsys:
	XmemErrorCallback(XmemErrorSYSTEM, errno);
	return NULL;
}


/**
 * CallbackToStr - Print out a callback structure
 *
 * @param cbs: Callback structure to be printed out
 *
 * @return pointer to the static string @str
 */
char *CallbackToStr(XmemCallbackStruct *cbs)
{
	static char str[128];

	if (! cbs)
		return NULL;

	bzero((void *)str, 128);

	switch (cbs->Mask) {

	case XmemEventMaskTIMEOUT:
		sprintf(str, "Timeout");
		break;

	case XmemEventMaskUSER:
		sprintf(str, "User event:%d From node:%s",
			(int)cbs->Data,	XmemGetNodeName(cbs->Node));
		break;

	case XmemEventMaskSEND_TABLE:
		sprintf(str, "Send table:%s From node:%s",
			XmemGetTableName(cbs->Table),
			XmemGetNodeName(cbs->Node));
		break;

	case XmemEventMaskTABLE_UPDATE:
		sprintf(str, "Update table:%s From node:%s",
			XmemGetTableName(cbs->Table),
			XmemGetNodeName(cbs->Node));
		break;

	case XmemEventMaskINITIALIZED:
		sprintf(str, "Initialized:%d From node:%s",
			(int)cbs->Data, XmemGetNodeName(cbs->Node));
		break;

	case XmemEventMaskKILL:
		sprintf(str, "Kill daemon received");
		break;

	case XmemEventMaskIO:
		sprintf(str, "IO error:%s", XmemErrorToString(cbs->Data));
		break;

	case XmemEventMaskSYSTEM:
		sprintf(str, "System error:%d", (int)cbs->Data);
		break;

	case XmemEventMaskSOFTWARE:
		if (cbs->Data == XmemErrorSUCCESS)
			sprintf(str, "Information:");
		else
			sprintf(str, "Software error:%s",
				XmemErrorToString(cbs->Data));
		break;

	default:
		break;
	}

	return str;
}


/**
 * LogEvent - Log an event in the log file
 *
 * @param cbs: callback struct with the information to log
 * @param text: text to log
 *
 * This routine writes EVENT_LOG_ENTRIES events into a log file each time it
 * has collected enough events.
 *
 * @return 1 on success; 0 otherwise
 */
int LogEvent(XmemCallbackStruct *cbs, char *text)
{
	int 	idx, cnt, logit;
	time_t 	tod;
	FILE 	*fp;
	char 	*elog;
	XdEventLogEntry *evp;

	if (event_log == NULL)
		return 0;

	idx       = event_log->NextEntry;
	evp       = &event_log->Entries[idx];

	tod       = time(NULL);
	evp->Time = tod;

	logit     = 0;

	if (text) {
		strncpy(evp->Text, text, EVENT_MESSAGE_SIZE - 1);
		fprintf(stderr, "%s XmemDaemon: %s\n", TimeToStr(tod), text);
		logit++;
	}

	if (cbs && log_mask & cbs->Mask) {
		evp->CbEvent = *cbs;
		// fprintf(stderr, "XmemDaemon: %s %s\n",
		// TimeToStr(tod),CallbackToStr(cbs));
		logit++;
	}

	if (!logit)
		return 1;

	if (++idx >= EVENT_LOG_ENTRIES) {
		/* event_log filled up; write to the file */
		umask(0);
		elog = XmemGetFile(EVENT_LOG);
		fp = fopen(elog, "w");

		if (!fp) {
			perror("XmemDaemon");
			fprintf(stderr,
				"XmemDaemon: Error: Can't OPEN:%s for WRITE\n",
				elog);
			bzero((void *)event_log, sizeof(XdEventLogEntries));
			return 0;
		}

		evp = &event_log->Entries[idx];
		cnt = fwrite(event_log, sizeof(XdEventLogEntries), 1, fp);

		if (cnt <= 0) {
			perror("XmemDaemon");
			fprintf(stderr, "XmemDaemon: Error: Can't WRITE:%s\n",
				elog);
			fclose(fp);

			bzero((void *) event_log, sizeof(XdEventLogEntries));
			return 0;
		}

		fclose(fp);
		bzero((void *) event_log, sizeof(XdEventLogEntries));

		return 1;
	}
	else
		event_log->NextEntry = idx;

	return 1;
}

/*
 * __flush_startup is only called by the callback handler when handling a
 * pending initialisation; it alleviates an already bloated callback handler.
 */
void __flush_startup(unsigned long *longs, XmemNodeId *nodes,
		     unsigned long *user, XmemNodeId me, XmemNodeId init_node)
{
#ifdef FLUSH_ON_STARTUP
	int 		i;
	unsigned long 	msk;
	XmemTableId 	atids;
	XmemNodeId 	acnds;
	char 		txt[80];

	atids = XmemGetAllTableIds();

	for (i = 0; i < XmemMAX_TABLES; i++) {
		msk = 1 << i;
		if (! (msk & atids))
			continue;

		XmemGetTableDesc(msk, longs, nodes, user);
		if (! (*nodes & me))
			continue;

		acnds = XmemGetAllNodeIds() & *nodes;
		/* only the highest node ID issues the write */
		if ((~me & acnds) > me)
			continue;

		XmemSendTable(msk, NULL, 0, 0, 1);

		sprintf(txt, "Flushed table: %s For initialized node: %s",
			XmemGetTableName(msk),
			XmemGetNodeName(init_node));
		LogEvent(NULL, txt);
	}
#endif
	return ;
}

/**
 * callback - callback handler
 *
 * @param cbs: callback struct delivered to the callback
 *
 */
void callback(XmemCallbackStruct *cbs)
{
	int 		i;
	unsigned long 	msk, user, longs;

	XmemError 	err;
	XmemNodeId 	me;
	XmemNodeId 	nodes, acnds;
	XmemTableId 	tid;
	XmemMessage 	mess;

	char 		txt[80];
	unsigned long 	*smemad;

	if (cbs == NULL)
		return;

	me = XmemWhoAmI();

	tid = cbs->Table;

	if (tid) {
		err = XmemGetTableDesc(tid, &longs, &nodes, &user);
		if (err != XmemErrorSUCCESS)
			return;
	}
	else {
		tid = 0;
		longs = 0;
		nodes = 0;
		user = 0;
	}

	for (i = 0; i < XmemEventMASKS; i++) {
		msk = 1 << i;
		if (! (cbs->Mask & msk))
			continue;

		switch ((XmemEventMask) msk) {

		case XmemEventMaskSEND_TABLE:

			if ((!tid || cbs->Node == me) && !symp)
				return;
			if (! (me & nodes))
				break;

			if (user & IN_SHMEM) {
				smemad = XmemGetSharedMemory(tid);
				if (!smemad)
					break;

				XmemSendTable(tid, smemad, longs, 0, 1);
				sprintf(txt,
					"Written table: %s For requesting node: %s",
					XmemGetTableName(tid),
					XmemGetNodeName(cbs->Node));
				LogEvent(NULL, txt);
			}
			else {
				XmemSendTable(tid, NULL, 0, 0, 1);
				sprintf(txt,
					"Flushed table: %s For requesting node: %s",
					XmemGetTableName(tid),
					XmemGetNodeName(cbs->Node));
				LogEvent(NULL, txt);
			}
			break;

		case XmemEventMaskTABLE_UPDATE:
			if ((!tid || cbs->Node == me) && !symp)
				return;
			if (! (user & IN_SHMEM))
				break;

			smemad = XmemGetSharedMemory(tid);
			if (smemad) {
				/* read from reflective memory */
				XmemRecvTable(tid, smemad, longs, 0);

				sprintf(txt,
					"Received table: %s From updating node: %s",
					XmemGetTableName(tid),
					XmemGetNodeName(cbs->Node));
				LogEvent(NULL, txt);
			}
			if (user & ON_DISC && nodes & me) {
				acnds = XmemGetAllNodeIds() & nodes;
				/* only the highest node ID issues the write */
				if ((~me & acnds) > me)
					break;

				XmemWriteTableFile(tid);
				sprintf(txt,
					"Disc file updated for table: %s For requesting node: %s",
					XmemGetTableName(tid),
					XmemGetNodeName(cbs->Node));
				LogEvent(NULL, txt);
			}
			break;

		case XmemEventMaskINITIALIZED:
			if (cbs->Data == XmemInitMessageRESET) {
				/* Acknowledge Init and send tables */
				mess.MessageType = XmemMessageTypeINITIALIZE_ME;
				mess.Data        = XmemInitMessageSEEN_BY;
				XmemSendMessage(XmemALL_NODES, &mess);

				__flush_startup(&longs, &nodes, &user,
					      me, cbs->Node);
			}
			if (cbs->Data != XmemInitMessageSEEN_BY)
				LogEvent(cbs, NULL);
			break;

		case XmemEventMaskKILL:
			sprintf(txt, "Received a Kill, EXIT: Deamon is DEAD");
			LogEvent(cbs, txt);
			exit(1);
			break;

		case XmemEventMaskUSER:
			sprintf(txt, "User message received");
			LogEvent(cbs, txt);
			break;

		case XmemEventMaskTIMEOUT:
		case XmemEventMaskIO:
		case XmemEventMaskSYSTEM:
		case XmemEventMaskSOFTWARE:
			break;

		default:
			break;
		}
	}
}


/*
 * __init_coldstart() reads the tables for the node from memory.
 */
void __init_coldstart()
{
	int 		i;
	unsigned long 	msk, longs, user;
	XmemError 	err;
	XmemNodeId 	me, nodes;
	XmemTableId 	tid;

	char 		txt[80];


	me = XmemWhoAmI();

	tid = XmemGetAllTableIds();

	for (i = 0; i < XmemMAX_TABLES; i++) {
		msk = 1 << i;
		if (! (tid & msk))
			continue;

		XmemGetTableDesc(msk, &longs, &nodes, &user);
		if (! (nodes & me))
			continue;
		if (! (user & IN_SHMEM && user & ON_DISC))
			continue;

		sprintf(txt, "Reading table:%s id:%d from disc",
			XmemGetTableName((XmemTableId)msk), (int)msk);
		LogEvent(NULL, txt);

		err = XmemReadTableFile(msk);
		if (err == XmemErrorSUCCESS)
			continue;

		LogEvent(NULL, "WARNING: Failed to read table from disc");
	}
}


/**
 * main - Xmem Daemon
 *
 * @param argc: argument counter
 * @param char *argv[]: arguments' values
 *
 * The daemon initialises the segments (it can be a warm/cold start), then
 * registers the callback handler, and waits in an infinite loop.
 *
 * @return if there's an error, main will exit with an appropriate error code.
 */
int main(int argc, char *argv[])
{
	XmemEventMask emsk;
	XmemError err;
	XmemMessage mes;
	int warmst;

	event_log = GetEventLogTable();

	err = XmemInitialize(XmemDeviceANY);
	if (err != XmemErrorSUCCESS) {
		LogEvent(NULL, "Can't initialize library");
		goto fatal;
	}

	XmemUnBlockCallbacks();

	if (!event_log) {
		LogEvent(NULL, "Can't get: EVENT_LOG shared memory segment");
		goto fatal;
	}

	emsk = XmemEventMaskMASK;
	/* subscribe the daemon to XmemEventMaskMASK */
	err = XmemRegisterCallback(callback, emsk);
	if (err != XmemErrorSUCCESS) {
		LogEvent(NULL, "Can't register callback");
		goto fatal;
	}

	warmst = XmemCheckForWarmStart();
	if (warmst) {
		LogEvent(NULL,"Warm Start: Begin ...");
		/* Tell the world I exist */
		mes.MessageType = XmemMessageTypeINITIALIZE_ME;
		mes.Data        = XmemInitMessageSEEN_BY;
		XmemSendMessage(XmemALL_NODES, &mes);
		/* ...it might be interesting to do this in a loop */
	}
	else {
		LogEvent(NULL, "Cold Start: Initialize Tables: Begin ...");
		/* For cold starts, read tables from disc */
		__init_coldstart();
		/*Tell the world to send me their tables and that I exist */
		mes.MessageType = XmemMessageTypeINITIALIZE_ME;
		mes.Data        = XmemInitMessageRESET;
		XmemSendMessage(XmemALL_NODES, &mes);
	}

	/*
	 * ...start-up finished.
	 */
	LogEvent(NULL, "XmemDaemon: Running OK");

	/*
	 * This is the infinite wait-loop of the daemon
	 */
	while(1) {
		if (VmicDaemonWait(1000)) {
			XmemBlockCallbacks();
			VmicDaemonCall();
		}
		XmemUnBlockCallbacks();
	}

fatal:
	LogEvent(NULL, "Fatal Error: Daemon Teminated");
	exit((int)err);
}
