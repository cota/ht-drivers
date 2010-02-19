/* ==================================================================== */
/* Implement general stub library for generic devices                   */
/* Julian Lewis Wed 15th April 2009                                     */
/* ==================================================================== */

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sched.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>

#include <StubLib.h>
#include <StubApi.h>

void swab(const void *from, void *to, ssize_t n);

/* ==================================================================== */
/* Private definition for handle.                                       */

#define SLbMAGIC 0xCE8A4954
#define SLbPATH_SIZE 64
#define SLbDEVICE_NAME_SIZE 16
#define SLbDLL_VER_NAME_SIZE 16

typedef struct {
   int          Magic;                            /* Magic number for a real handle */
   char         Device[SLbDEVICE_NAME_SIZE];      /* Stubler device name */
   char         SopPath[SLbPATH_SIZE];            /* Path to shared object */
   SLbDatumSize DSize;                            /* Datum size */
   void        *DllSo;                            /* Handle for shared object */
   char         DllVer[SLbDLL_VER_NAME_SIZE];     /* DLL version */
   int          FD;                               /* Users file descriptor from driver */
   char         Err[SLbErrSTRING_SIZE];           /* Users last error text */
   SLbAPI       API;                              /* API pointers */
   int          BSze;                             /* Buffer Size */
   int          Post;                             /* Post trigger samples */
 } Handle;

/* ==================================================================== */
/* Device and Error string static texts                                 */

static char *ErrTexts[SLbERRORS] = {
   "All went OK, No error                      ",  // SLbErrSUCCESS,
   "Function is not implemented on this device ",  // SLbErrNOT_IMP,
   "Invalid Start value for this device        ",  // SLbErrSTART,
   "Invalid Mode  value for this device        ",  // SLbErrMODE,
   "Invalid Clock value for this device        ",  // SLbErrCLOCK,
   "Can't open a driver file handle, fatal     ",  // SLbErrOPEN,
   "Can't connect to that interrupt            ",  // SLbErrCONNECT,
   "No connections to wait for                 ",  // SLbErrWAIT,
   "Timeout in wait                            ",  // SLbErrTIMEOUT,
   "Queue flag must be set 0 queueing is needed",  // SLbErrQFLAG,
   "IO or BUS error                            ",  // SLbErrIO,
   "Module is not enabled                      ",  // SLbErrNOT_ENAB
   "Not available for INTERNAL sources         ",  // SLbErrSOURCE,
   "Value out of range                         ",  // SLbErrVOR,
   "Bad device type                            ",  // SLbErrDEVICE,
   "Bad address                                ",  // SLbErrADDRESS,
   "Can not allocate memory                    ",  // SLbErrMEMORY,
   "Shared library error, can't open object    ",  // SLbErrDLOPEN,
   "Shared library error, can't find symbol    ",  // SLbErrDLSYM,
   "Can't open sampler device driver           ",  // SLbErrDROPEN,
   "Invalid handle                             ",  // SLbErrHANDLE,
   "Invalid module number, not installed       ",  // SLbErrMODULE,
   "Invalid channel number for this module     "   // SLbErrCHANNEL,
 };

/* ==================================================================== */
/* If library dosn't implement a function use this.                     */
/* Make sure there are enough argument places to avoid stack problems.  */

int NotImplemented(int a, int b, int c, int d, int e, int f, int g) {
   return SLbErrNOT_IMP;
}

/* ==================================================================== */
/* Bind a symbol to a function                                          */

#ifdef __linux__
#define SYMLEN 128 /* from Lynx include/conf.h */
#endif

int BindSymbol(Handle *hand, void **func, char *symb) {

char psym[SYMLEN];
int len;
void *Ni = NotImplemented;

   bzero((void *) psym, SYMLEN);

   len = SYMLEN;
   strncpy(psym, hand->Device, len);

   len -= strlen(psym);
   if (len > strlen(symb)) strncat(psym,symb,len);

   *func = dlsym(hand->DllSo,psym);
   if (*func == NULL) {
      *func = Ni;
      return 0;
   }
   return 1;
}

/* ==================================================================== */
/* Initialize the callers API                                           */
/* These routines MUST be instantiated, they are allowed to return the  */
/* error NOT-IMPLEMENTED.                                               */

SLbErr InitAPI(Handle *hand) {

   BindSymbol(hand,(void **) &hand->API.CloseHandle,"CloseHandle");
   BindSymbol(hand,(void **) &hand->API.ResetMod,"ResetMod");
   BindSymbol(hand,(void **) &hand->API.GetAdcValue,"GetAdcValue");
   BindSymbol(hand,(void **) &hand->API.CallibrateChn,"CallibrateChn");
   BindSymbol(hand,(void **) &hand->API.GetState,"GetState");
   BindSymbol(hand,(void **) &hand->API.GetStatus,"GetStatus");
   BindSymbol(hand,(void **) &hand->API.GetDatumSize,"GetDatumSize");
   BindSymbol(hand,(void **) &hand->API.SetDebug,"SetDebug");
   BindSymbol(hand,(void **) &hand->API.GetDebug,"GetDebug");
   BindSymbol(hand,(void **) &hand->API.GetModuleCount,"GetModuleCount");
   BindSymbol(hand,(void **) &hand->API.GetChannelCount,"GetChannelCount");
   BindSymbol(hand,(void **) &hand->API.GetVersion,"GetVersion");
   BindSymbol(hand,(void **) &hand->API.SetTrigger,"SetTrigger");
   BindSymbol(hand,(void **) &hand->API.GetTrigger,"GetTrigger");
   BindSymbol(hand,(void **) &hand->API.SetClock,"SetClock");
   BindSymbol(hand,(void **) &hand->API.GetClock,"GetClock");
   BindSymbol(hand,(void **) &hand->API.SetStop,"SetStop");
   BindSymbol(hand,(void **) &hand->API.GetStop,"GetStop");
   BindSymbol(hand,(void **) &hand->API.SetClockDivide,"SetClockDivide");
   BindSymbol(hand,(void **) &hand->API.GetClockDivide,"GetClockDivide");
   BindSymbol(hand,(void **) &hand->API.StrtAcquisition,"StrtAcquisition");
   BindSymbol(hand,(void **) &hand->API.TrigAcquisition,"TrigAcquisition");
   BindSymbol(hand,(void **) &hand->API.StopAcquisition,"StopAcquisition");
   BindSymbol(hand,(void **) &hand->API.Connect,"Connect");
   BindSymbol(hand,(void **) &hand->API.SetTimeout,"SetTimeout");
   BindSymbol(hand,(void **) &hand->API.GetTimeout,"GetTimeout");
   BindSymbol(hand,(void **) &hand->API.SetQueueFlag,"SetQueueFlag");
   BindSymbol(hand,(void **) &hand->API.GetQueueFlag,"GetQueueFlag");
   BindSymbol(hand,(void **) &hand->API.Wait,"Wait");
   BindSymbol(hand,(void **) &hand->API.SimulateInterrupt,"SimulateInterrupt");
   BindSymbol(hand,(void **) &hand->API.SetBufferSize,"SetBufferSize");
   BindSymbol(hand,(void **) &hand->API.GetBuffer,"GetBuffer");
   BindSymbol(hand,(void **) &hand->API.SetTriggerLevels,"SetTriggerLevels");
   BindSymbol(hand,(void **) &hand->API.GetTriggerLevels,"GetTriggerLevels");
   BindSymbol(hand,(void **) &hand->API.SetTriggerConfig,"SetTriggerConfig");
   BindSymbol(hand,(void **) &hand->API.GetTriggerConfig,"GetTriggerConfig");
   return SLbErrSUCCESS;
}

/* ==================================================================== */
/* Round up to the nearest chunk. The DMA mechanism of some hardware is */
/* in minimum sized chunks. This routine rounds up to chunk size.       */
/* Units are in datum size, I.E. samples.                               */

#define CHUNK 32

int GetChunkSize(int sze) {

int n, m;

   n = sze/CHUNK;
   m = n * CHUNK;
   if (m == sze) return sze;
   return (n+1) * CHUNK;
}

/* ==================================================================== */
/* Convert an SLbError to a string suitable for printing.               */
/* This routine can not return a NULL pointer. If the error is out of   */
/* range it returns a string saying just that. The pointer returned is  */
/* thread safe and there is no need to free it, the allocated memory is */
/* kept in the callers address space.                                   */
/* If the handle pointer is NULL, the error text is kept in static      */
/* memory, and is not thread safe. In this case copy it yourself.       */

char *SLbErrToStr(SLbHandle handle, SLbErr error) {

char         *ep;    /* Error text string pointer */
char         *cp;    /* Char pointer */
unsigned int i;
Handle       *hand;
static char  err[SLbErrSTRING_SIZE]; /* Static text area when null handle */

   i = (unsigned int) error;
   if (i >= SLbERRORS) return "Invalid error code";

   hand = (Handle *) handle;
   if (hand) {
      if (hand->Magic != SLbMAGIC) return "Invalid non void handle";
      ep = hand->Err;
   } else ep = err;

   bzero((void *) ep, SLbErrSTRING_SIZE);

   strncpy(ep,ErrTexts[i],(size_t) SLbErrSTRING_SIZE);

   /* Extra info available from loader */

   if ((error == SLbErrDLOPEN)
   ||  (error == SLbErrDLSYM)) {
     cp = dlerror();
     if (cp) {
	strcat(ep,": ");
	i = SLbErrSTRING_SIZE - strlen(ep) -1;
	strncat(ep,cp,i);
     }
   }

   /* Extra info available from errno */

   if ((error == SLbErrIO)
   ||  (error == SLbErrDROPEN)
   ||  (error == SLbErrMEMORY)) {
     cp = strerror(errno);
     if (cp) {
	strcat(ep,": ");
	i = SLbErrSTRING_SIZE - strlen(ep) -1;  /* -1 is 0 terminator byte */
	strncat(ep,cp,i);
     }
   }

   return ep;
}

/* ==================================================================== */
/* Internal routine: Get the DLL path name for a device                 */

SLbErr SLbSetSopPath(Handle *handle, char *dev, char *ver) {

char   *cd, *cv;

   if (dev) cd = dev; else cd = "samp";
   if (ver) cv = ver; else cv = "1.0";

   strncpy((void *) handle->Device, cd, SLbDEVICE_NAME_SIZE);
   strncpy((void *) handle->DllVer, cv, SLbDLL_VER_NAME_SIZE);

   sprintf(handle->SopPath,"lib%s.so.%s",dev,ver);
   return SLbErrSUCCESS;
}

/* ==================================================================== */
/* Each thread in an application should get a handle and use it in      */
/* all subsequent calls. A limited number of concurrently open handles  */
/* is available for a given driver. The client thread should close its  */
/* handle when it no longer needs it. The library will then free memory */
/* and close the file asociated file descriptor. The handle contains a  */
/* file descriptor that can be used from within a Linux select call.    */
/*                                                                      */
/* Example:                                                             */
/*                                                                      */
/*    SlbHandle my_handle;                                              */
/*    SlbErr    my_error;                                               */
/*                                                                      */
/*    my_error = SLbOpenHandle(&my_handle,"VD80","1.0");                */
/*    if ( != SLbErrSUCCESS) {                                          */
/*       stprintf(stderr,"Fatal: %s\n",SLbErrTopStr(NULL, my_error));   */
/*       exit((int) my_error);                                          */
/*    }                                                                 */
/*                                                                      */

SLbErr SLbOpenHandle(SLbHandle *handle, char *device, char *version) {

Handle *hand;

   if (handle == NULL) return SLbErrADDRESS;

   hand = (Handle *) malloc(sizeof(Handle));
   if (hand == NULL) return SLbErrMEMORY;
   *handle = (SLbHandle) hand;              /* Store the address of the handle */

   bzero((void *) hand, sizeof(Handle));
   hand->Magic = SLbMAGIC;

   SLbSetSopPath(hand,device,version);

#ifdef __linux__
   hand->DllSo = dlopen(hand->SopPath, RTLD_LOCAL | RTLD_LAZY);
#else
   hand->DllSo = dlopen(hand->SopPath, RTLD_NOW);
#endif

   if (hand->DllSo == NULL) return SLbErrDLOPEN;

   if (BindSymbol(hand,(void **) &hand->API.OpenHandle,"OpenHandle") == 0) return SLbErrDLSYM;

   hand->FD = hand->API.OpenHandle();
   if (hand->FD <= 0) return SLbErrDROPEN;

   return InitAPI(hand);
}

/* ==================================================================== */
/* Close handle                                                         */

SLbErr SLbCloseHandle(SLbHandle handle) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   hand->API.CloseHandle(hand->FD);
   dlclose(hand->DllSo);

   bzero((void *) hand, sizeof(Handle));
   free((void *) hand);

   return SLbErrSUCCESS;
}

/* ==================================================================== */
/* Get the Driver file descriptor to be used within a select call.      */

SLbErr SLbGetFileDescriptor(SLbHandle handle, int *fd) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   *fd = hand->FD;

   return SLbErrSUCCESS;
}

/* ==================================================================== */
/* Set debug level                                                      */

SLbErr SLbSetDebug(SLbHandle handle, SLbDebug *deb) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SetDebug(hand->FD,deb);
}

/* ==================================================================== */
/* Get debug level                                                      */

SLbErr SLbGetDebug(SLbHandle handle, SLbDebug *deb) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetDebug(hand->FD,deb);
}

/* ==================================================================== */
/* Get the sampler device name string.                                  */

SLbErr SLbGetDeviceName(SLbHandle handle, char **dname) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   *dname = hand->Device;

   return SLbErrSUCCESS;
}

/* ==================================================================== */
/* Get the Dll Version string.                                          */

SLbErr SLbGetDllVersion(SLbHandle handle, char **dver) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   *dver = hand->DllVer;

   return SLbErrSUCCESS;
}

/* ==================================================================== */
/* The address of any library function can be got by name. This can be  */
/* useful if you want to call a device library function directly, for   */
/* example a specific device function.                                  */
/*                                                                      */
/* Example:                                                             */
/*                                                                      */
/* SLbErr (*my_funct)(int p);                                           */
/* SLbErr my_error;                                                     */
/*                                                                      */
/*    my_error = SlbGetSymbol(my_handle,"Vd80LibSpecial",&my_funct);    */
/*    if (my_error != SLbErrSUCCESS) {                                  */
/*       stprintf(stderr,"Fatal: %s\n",SLbErrTopStr(NULL, my_error));   */
/*       exit((int) my_error);                                          */
/*    }                                                                 */
/*                                                                      */
/*    my_error = my_funct(1); ...                                       */

SLbErr SLbGetSymbol(SLbHandle handle, char *name, void **func) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   *func = dlsym(hand->DllSo,name);
   if (*func == NULL) return SLbErrDLSYM;

   return SLbErrSUCCESS;
}

/* ==================================================================== */
/* Reset is the equivalent to a VME bus reset. All settings and data on */
/* the module will be lost.                                             */

SLbErr SLbResetMod(SLbHandle handle, int module) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.ResetMod(hand->FD,module);
}

/* ==================================================================== */
/* Calibrate requires that zero and Max volts are applied to an input   */
/* and the Adc value then read back. Once done the values obtained can  */
/* be used to calibrate the module.                                     */

SLbErr SLbGetAdcValue(SLbHandle handle, int module, int channel, int *adc) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetAdcValue(hand->FD,module,channel,adc);
}

/* ==================================================================== */
/* Callibrate a channels ADC                                            */

SLbErr SLbCalibrateChn(SLbHandle handle, int module, int channel, int low, int high) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.CallibrateChn(hand->FD,module,channel,low,high);
}

/* ==================================================================== */
/* A channel has a state, the state must be IDLE to read back data.     */

SLbErr SLbGetState(SLbHandle handle, int module, int channel, SLbState *state) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetState(hand->FD,module,channel,state);
}

/* ==================================================================== */
/* Mod status indicates hardware failures, enable state etc.            */

SLbErr SLbGetStatus(SLbHandle handle, int module, SLbStatus *status) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetStatus(hand->FD,module,status);
}

/* ==================================================================== */
/* This is a fixed value for each device type.                          */

SLbErr SLbGetDatumSize(SLbHandle handle, SLbDatumSize *datsze) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetDatumSize(hand->FD,datsze);
}

/* ==================================================================== */
/* Discover how many modules and channels are installed.                */

SLbErr SLbGetModCount(SLbHandle handle, int *count) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetModuleCount(hand->FD,count);
}

/* ==================================================================== */
/* Discover how many modules and channels are installed.                */

SLbErr SLbGetChnCount(SLbHandle handle, int *count) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetChannelCount(hand->FD,count);
}

/* ==================================================================== */
/* This returns the hardware, driver and library versions.              */

SLbErr SLbGetVersion(SLbHandle handle, int module, SLbVersion *version) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetVersion(hand->FD,module,version);
}

/* ==================================================================== */
/* Get the version of this stub library                                 */

#ifndef COMPILE_TIME
#define COMPILE_TIME 0
#endif

SLbErr SLbGetStubVersion(unsigned int *version) {
   *version = COMPILE_TIME;
   return SLbErrSUCCESS;
}

/* ==================================================================== */
/* Setup the trigger input                                              */

SLbErr SLbSetTrigger(SLbHandle handle, SLbMod modules, SLbChn channels, SLbInput *trigger) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SetTrigger(hand->FD,modules,channels,trigger);
}

/* ==================================================================== */
/* Get the trigger input settings                                       */

SLbErr SLbGetTrigger(SLbHandle handle, int module, int channel, SLbInput *trigger) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetTrigger(hand->FD,module,channel,trigger);
}

/* ==================================================================== */
/* Setup the clock input                                                */

SLbErr SLbSetClock(SLbHandle handle, SLbMod modules, SLbChn channels, SLbInput *clock) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SetClock(hand->FD,modules,channels,clock);
}

/* ==================================================================== */
/* Get the clock input settings                                         */

SLbErr SLbGetClock(SLbHandle handle, int module, int channel, SLbInput *clock) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetClock(hand->FD,module,channel,clock);
}

/* ==================================================================== */
/* Some hardware has an external stop input.                            */

SLbErr SLbSetStop(SLbHandle handle, SLbMod modules, SLbChn channels, SLbInput *stop) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SetStop(hand->FD,modules,channels,stop);
}

/* ==================================================================== */
/* Some hardware has an external stop input.                            */

SLbErr SLbGetStop(SLbHandle handle, int module, int channel, SLbInput *stop) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetStop(hand->FD,module,channel,stop);
}

/* ==================================================================== */
/* Divide the clock frequency by an integer.                            */
/* One is added to the divisor, so 0 means divide by 1.                 */

SLbErr SLbSetClockDivide(SLbHandle handle, SLbMod modules, SLbChn channels, unsigned int divisor) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SetClockDivide(hand->FD,modules,channels,divisor);
}

/* ==================================================================== */
/* Divide the clock frequency by an integer.                            */
/* One is added to the divisor, so 0 means divide by 1.                 */

SLbErr SLbGetClockDivide(SLbHandle handle, int module, int channel, unsigned int *divisor) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetClockDivide(hand->FD,module,channel,divisor);
}

/* ==================================================================== */
/* Start acquisition by software.                                       */

SLbErr SLbStrtAcquisition(SLbHandle handle, SLbMod modules, SLbChn channels) { /* Zero all channels */

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.StrtAcquisition(hand->FD,modules,channels);
}

/* ==================================================================== */
/* Trigger acquisition by software.                                     */

SLbErr SLbTrigAcquisition(SLbHandle handle, SLbMod modules, SLbChn channels) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.TrigAcquisition(hand->FD,modules,channels);
}

/* ==================================================================== */
/* Force reading to stop                                                */

SLbErr SLbStopAcquisition(SLbHandle handle, SLbMod modules, SLbChn channels) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.StopAcquisition(hand->FD,modules,channels);
}

/* ==================================================================== */
/* Make an acquisition buffer readable                                  */

SLbErr SLbReadAcquisition(SLbHandle handle, SLbMod modules, SLbChn channels) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.StopAcquisition(hand->FD,modules,channels);
}

/* ==================================================================== */
/* Connecting to zero disconnects from all previous connections         */

SLbErr SLbConnect(SLbHandle handle, SLbMod modules, SLbChn channels, SLbIntrMask imask) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.Connect(hand->FD,modules,channels,imask);
}

/* ==================================================================== */

SLbErr SLbSetTimeout(SLbHandle handle, int tmout) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SetTimeout(hand->FD,tmout);
}

/* ==================================================================== */

SLbErr SLbGetTimeout(SLbHandle handle, int *tmout) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetTimeout(hand->FD,tmout);
}

/* ==================================================================== */

SLbErr SLbSetQueueFlag(SLbHandle handle, SLbQueueFlag flag) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SetQueueFlag(hand->FD,flag);
}

/* ==================================================================== */

SLbErr SLbGetQueueFlag(SLbHandle handle, SLbQueueFlag *flag) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetQueueFlag(hand->FD,flag);
}

/* ==================================================================== */

SLbErr SLbWait(SLbHandle handle, SLbIntr *intr) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.Wait(hand->FD,intr);
}

/* ==================================================================== */

SLbErr SLbSimulateInterrupt(SLbHandle handle, SLbIntr *intr) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SimulateInterrupt(hand->FD,intr);
}

/* ==================================================================== */

SLbErr SLbSetTriggerLevels(SLbHandle handle, unsigned int modules, unsigned int channels, SLbAnalogTrig *atrg) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SetTriggerLevels(hand->FD,modules,channels,atrg);
}

/* ==================================================================== */

SLbErr SLbGetTriggerLevels(SLbHandle handle, int module, int channel, SLbAnalogTrig *atrg) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetTriggerLevels(hand->FD,module,channel,atrg);
}

/* ==================================================================== */
/* Buffers contain samples that occur before and after the trigger.     */
/* The buffer is allocated by the library in an optimum way for DMA.    */
/* Any byte ordering operations are performed by the library.           */
/* The user specifies the number of post trigger samples and the size   */
/* of the buffer. In some hardware such as the VD80 the DMA transfers   */
/* are carried out in chunks. Thus the exact triigger position within   */
/* the buffer is adjusted to the nearset size/post boundary. Hence the  */
/* actual number of pre/post trigger samples may vary.                  */
/* One buffer is allocated for each module/channel specified in the     */
/* module/channel masks. Calling SetBuffers will deallocate previous    */
/* memory allocations if the new size is larger and reallocate.         */

SLbErr SLbSetBufferSize(SLbHandle handle, SLbMod modules, SLbChn channels, int *size, int  *post) {

int i, j;
unsigned int mski, mskj;
SLbDatumSize dsze;
SLbErr err;
Handle *hand;
SLbBuffer buf;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   err = hand->API.GetDatumSize(hand->FD,&dsze);
   if (err != SLbErrSUCCESS) return err; /* Bad handle */

   *size = GetChunkSize(*size);
   *post = GetChunkSize(*post);

   hand = (Handle *) handle;
   for (i=0; i<SLbMODULES; i++) {
      mski = 1 << i;
      if (mski & modules) {
	 for (j=0; j<SLbCHANNELS; j++) {
	    mskj = 1 << j;
	    if (mskj & channels) {
	       bzero((void *) &buf, sizeof(SLbBuffer));
	       buf.Post = *post; hand->Post = *post;
	       buf.BSze = *size; hand->BSze = *size;
	       err = hand->API.SetBufferSize(hand->FD, i+1, j+1, *size, *post);
	       if (err != SLbErrSUCCESS) return err;
	    }
	 }
      }
   }
   return SLbErrSUCCESS;
}

/* ==================================================================== */

SLbErr SLbGetBufferSize(SLbHandle handle, int module, int channel, int *size, int *post) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   if ((module  < 1) || (module  > SLbMODULES )) return SLbErrMODULE;
   if ((channel < 1) || (channel > SLbCHANNELS)) return SLbErrCHANNEL;

   *size = hand->BSze;
   *post = hand->Post;

   return SLbErrSUCCESS;
}

/* ==================================================================== */
/* Get information about a buffer so that the data, if any, can be used */
/* This function transfers data to the user buffer.                     */

SLbErr SLbGetBuffer(SLbHandle handle, int module, int channel, SLbBuffer *buffer) {

Handle *hand;
SLbErr err;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   if ((module  < 1) || (module  > SLbMODULES )) return SLbErrMODULE;
   if ((channel < 1) || (channel > SLbCHANNELS)) return SLbErrCHANNEL;

   err = hand->API.GetBuffer(hand->FD,module,channel,buffer); /* Read buffer */

   if ((buffer->Tpos + buffer->Post) > buffer->ASze) { /* Chopped ? */
       buffer->Post = buffer->ASze - buffer->Tpos;
   }

   return err;
}

/* ==================================================================== */

SLbErr SLbSetTriggerConfig(SLbHandle handle, unsigned int modules, unsigned int channels, SLbTrigConfig *ctrg) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.SetTriggerConfig(hand->FD,modules,channels,ctrg); /* Set trigger config parameters */
}


/* ==================================================================== */

SLbErr SLbGetTriggerConfig(SLbHandle handle, int module, int channel, SLbTrigConfig *ctrg) {

Handle *hand;

   hand = (Handle *) handle;
   if (hand == NULL) return SLbErrADDRESS;
   if (hand->Magic != SLbMAGIC) return SLbErrHANDLE;

   return hand->API.GetTriggerConfig(hand->FD,module,channel,ctrg); /* Get trigger config parameters */
}
