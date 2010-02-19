/* =========================================================== */
/* Define here the resources you want to reserve for driver    */
/* working area. Max clients, modules etc.                     */
/* Julian Lewis Tue 16th Dec 2008 AB/CO/HT                     */
/* =========================================================== */

#ifndef SKELUSER
#define SKELUSER


/* =========================================================== */
/* Modify these constants to control the number of client and  */
/* module contexts.                                            */

#define SkelDrvrCLIENT_CONTEXTS 16
#define SkelDrvrMODULE_CONTEXTS 2

/* =========================================================== */
/* How much space do you want to reserve in the module context */
/* to keep copies of registers. These are useful in emulation  */
/* and to restore settings after a hardware reset.             */

#define SkelDrvrREGISTERS 256

/* =========================================================== */
/* Define how many interrupt sources you want and the size of  */
/* the queue you need to store them for each client.           */

#define SkelDrvrINTERRUPTS 32
#define SkelDrvrQUEUE_SIZE 32
#define SkelDrvrCONNECTIONS 16

/* =========================================================== */
/* Define default timeout values, in 10ms ticks                */

#define SkelDrvrDEFAULT_CLIENT_TIMEOUT 2000
#define SkelDrvrDEFAULT_MODULE_TIMEOUT 10

#endif
