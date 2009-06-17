/* ==================================================================== */
/* Implement a library to handle reflective memory Network back end     */
/* Julian Lewis 9th/Feb/2005                                            */
/* ==================================================================== */

/* ==================================================================== */
/* Try to initialize the specified device                               */

XmemError NetworkInitialize() { /* Initialize hardware/software */

   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Get all currently up and running nodes.                              */
/* The set of up and running nodes is determined dynamically through a  */
/* handshake protocol between nodes.                                    */

XmemNodeId NetworkGetAllNodeIds() {

   return 0;
}

/* ==================================================================== */
/* Get the name of a node from its node Id, this is usually the name of */
/* the host on which the node is implemented.                           */

char * NetworkGetNodeName(XmemNodeId node) {

   return (char *) 0;
}

/* ==================================================================== */
/* Get the Id of a node from its name.                                  */

XmemNodeId NetworkGetNodeId(XmemName name) {

   return 0;
}

/* ==================================================================== */
/* Get all currently defined table Ids. There can be up to 32 tables    */
/* each ones Id corresponds to a single bit in an unsigned long.        */

XmemTableId NetworkGetAllTableIds() {

   return 0;
}

/* ==================================================================== */
/* Get the name of a table from its table Id. Tables are defined from   */
/* a configuration file.                                                */

char * NetworkGetTableName(XmemTableId table) {

   return (char *) 0;
}

/* ==================================================================== */
/* Get the Id of a table from its name.                                 */

XmemTableId NetworkGetTableId(XmemName name ) {

   return 0;
}

/* ==================================================================== */
/* Get the description of a table, namley its size, and which nodes can */
/* write to it.                                                         */

XmemError NetworkGetTableDesc(XmemTableId    table,
			      unsigned long *size,
			      XmemNodeId    *nodes) {

   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Register your call back to handle Xmem Events.                       */

XmemError NetworkRegisterCallback(void (*cb(XmemCallbackStruct *cbs)),
				  XmemEventMask mask) {

   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Wait for an Xmem Event with time out, an incomming event will call   */
/* any registered callback.                                             */

XmemEventMask NetworkWait(int timeout) {

   return 0;
}

/* ==================================================================== */
/* Poll for any Xmem Events, an incomming event will call any callback  */
/* that is registered for that event.                                   */

XmemEventMask NetworkPoll() {

   return 0;
}

/* ==================================================================== */
/* Send your buffer to a reflective memory table. A table update event  */
/* is broadcast automatically.                                          */

XmemError NetworkSendTable(XmemTableId table, void *buf) {

   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Update your buffer from a reflective memory table.                   */

XmemError NetworkRecvTable(XmemTableId table, void *buf) {

   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Send a message to other nodes. If the node parameter is zero, then   */
/* nothing happens, if its equal to 0xFFFFFFFF the message is sent via  */
/* broadcast, else multicast or unicast is used.                        */

XmemError NetworkSendMessage(XmemNodeId nodes, XmemMessage *mess) {

   return XmemErrorNOT_INITIALIZED;
}

/* ==================================================================== */
/* Check which tables were updated.                                     */

XmemTableId NetworkCheckTables() {

   return XmemErrorNOT_INITIALIZED;
}

XmemError NetworkSendSoftWakeup(uint32_t nodeid, uint32_t data)
{
	return 0;
}
