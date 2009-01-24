/**
 * @file XmemDaemon.h
 *
 * @brief Header file for Xmem Daemon
 *
 * @author Julian Lewis
 *
 * @date Created on 11/03/2005
 *
 * @version 1.0 Julian Lewis
 */
/*! @name Event Logging
 */
//@{
#define EVENT_LOG "Xmem.daemonlog"
#define EVENT_LOG_NAME "EVENTLOG"
#define EVENT_LOG_ENTRIES 32
#define EVENT_MESSAGE_SIZE 128

typedef struct {
	time_t		Time;
	XmemCallbackStruct CbEvent;
	char		Text[EVENT_MESSAGE_SIZE];
} XdEventLogEntry;

typedef struct {
	int 		NextEntry;
	XdEventLogEntry Entries[EVENT_LOG_ENTRIES];
} XdEventLogEntries;
//@}

/*! @name User bit definitions for the daemon
 */
//@{
#define ON_DISC  0x01
#define IN_SHMEM 0x02
//@}
