/* ==================================================================== */
/* Implement Vd80 sampler library                                       */
/* Julian Lewis Wed 15th April 2009                                     */
/* ==================================================================== */

/* ==================================================================== */
/* ==================================================================== */
/* I very strongly recomend that this library is used in its ".so" form */
/* where it is loaded from the STUB library.  The stub library has many */
/* extra routines and generic features, and calls any version of this   */
/* library. If you insist on a static link with this library, at least  */
/* take a look at the stub library for examples on how to use this one. */
/* The test program calls this library via a dynamic link and can call  */
/* any of these functions.                                              */
/* ==================================================================== */
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
#include <dlfcn.h>
#include <unistd.h>
#include <errno.h>

#include <skeluser.h>
#include <skeluser_ioctl.h>
#include <skel.h>
#include <vd80Drvr.h>
#include <vd80hard.h>
#include <Vd80Lib.h>

/* ==================================================================== */
/* ==================================================================== */
/* Static private non exported routines for local use only.             */
/* ==================================================================== */
/* ==================================================================== */

/* ==================================================================== */
/* Internal routine to select module                                    */

static int SetModule(int fd, int mod) {

int cnt;

   if (ioctl(fd, SkelDrvrIoctlGET_MODULE_COUNT,&cnt) < 0) return Vd80ErrIO;
   if ((mod < 1) || (mod > cnt)) return Vd80ErrMODULE;
   if (ioctl(fd,SkelDrvrIoctlSET_MODULE,&mod) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */

static int DoCommand(int fd, int mod, int chn, int cmd) {

Vd80Err err;
int val;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   val = cmd;
   if (cmd == VD80_COMMAND_READ) {
      val |= (VD80_OPERANT_MASK & ((chn -1) << VD80_OPERANT_SHIFT));
   }

   if (ioctl(fd,Vd80IoctlSET_COMMAND,&val) < 0) return Vd80ErrIO;

   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* ==================================================================== */
/* Exported routines                                                    */
/* ==================================================================== */
/* ==================================================================== */

/* ==================================================================== */
/* Open driver handle                                                   */

int vd80OpenHandle() {

char fnm[32];
int  i, fd;

   for (i = 1; i <=SkelDrvrCLIENT_CONTEXTS; i++) {
      sprintf(fnm,"/dev/vd80.%1d",i);
      if ((fd = open(fnm,O_RDWR,0)) > 0) return fd;
   }
   return 0;
}

/* ==================================================================== */
/* Close driver handle                                                  */

void vd80CloseHandle(int fd) {

   close(fd);
}

/* ==================================================================== */
/* Reset module                                                         */

Vd80Err vd80ResetMod(int fd, int mod) {

Vd80Err err;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;
   if (ioctl(fd,SkelDrvrIoctlRESET,NULL) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Set debug mask options                                               */

Vd80Err vd80SetDebug(int fd, Vd80Debug *deb) {

   if (ioctl(fd,SkelDrvrIoctlSET_DEBUG,deb) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get debug mask options                                               */

Vd80Err vd80GetDebug(int fd, Vd80Debug *deb) {

   if (ioctl(fd,SkelDrvrIoctlGET_DEBUG,deb) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get instantaneous ADC value for a given channel and module           */

Vd80Err vd80GetAdcValue(int fd, int mod, int chn, int *adc) {

Vd80Err err;
int val;
short sval;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   val = chn;
   if (ioctl(fd,Vd80IoctlREAD_ADC,&val) < 0) return Vd80ErrIO;
   sval = val & 0xFFFF;
   *adc = sval;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get State, the chn parameter is ignored, the state is for the module */

Vd80Err vd80GetState(int fd, int mod, int chn, Vd80State *ste) {

Vd80Err err;
int val;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   if (ioctl(fd,Vd80IoctlGET_STATE,&val) < 0) return Vd80ErrIO;
   *ste = (Vd80State) val;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get Status                                                           */

Vd80Err vd80GetStatus(int fd, int mod, Vd80Status *sts) {

Vd80Err err;
int val;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   if (ioctl(fd, SkelDrvrIoctlGET_STATUS, &val) < 0) return Vd80ErrIO;
   *sts = (Vd80Status) val;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get datum size                                                       */

Vd80Err vd80GetDatumSize(int fd, Vd80DatumSize *dsz) {

   *dsz = Vd80DatumSize16;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get installed module count                                           */

Vd80Err vd80GetModuleCount(int fd, int *cnt) {

   if (ioctl(fd, SkelDrvrIoctlGET_MODULE_COUNT,cnt) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get number of channels                                               */

Vd80Err vd80GetChannelCount(int fd, int *cnt) {

   *cnt = VD80_CHANNELS;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get driver and hardware version                                      */

#ifndef COMPILE_TIME
#define COMPILE_TIME 0
#endif

Vd80Err vd80GetVersion(int fd, int mod, Vd80Version *ver) {

Vd80Err err;
SkelDrvrVersion skver;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   if (ioctl(fd,SkelDrvrIoctlGET_VERSION,&skver) < 0) return Vd80ErrIO;

   ver->LibraryVersion = COMPILE_TIME;
   ver->DriverVersion  = skver.DriverVersion;
   strncpy(ver->ModVersion,skver.ModuleVersion,Vd80VERSION_SIZE);
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Actually there is one trigger setting per module only, not for each  */
/* channel !!! chns is ignored.                                         */

Vd80Err vd80SetTrigger(int fd, unsigned int mods, unsigned int chns, Vd80Input *inp) {

Vd80Err err;
int i, mod;
unsigned int mski;
int val;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = SetModule(fd,mod);
	 if (err != Vd80ErrSUCCESS) return err;

	 val = inp->Edge;
	 if (ioctl(fd,Vd80IoctlSET_TRIGGER_EDGE       ,&val) < 0) return Vd80ErrIO;
	 val = inp->Termination;
	 if (ioctl(fd,Vd80IoctlSET_TRIGGER_TERMINATION,&val) < 0) return Vd80ErrIO;
	 val = inp->Source;
	 if (ioctl(fd,Vd80IoctlSET_TRIGGER            ,&val) < 0) return Vd80ErrIO;
      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get trigger input options. chn is ignored                            */

Vd80Err vd80GetTrigger(int fd, int mod, int chn, Vd80Input *inp) {

Vd80Err err;
int val;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   if (ioctl(fd,Vd80IoctlGET_TRIGGER_EDGE       ,&val) < 0) return Vd80ErrIO;
   inp->Edge = val;
   if (ioctl(fd,Vd80IoctlGET_TRIGGER_TERMINATION,&val) < 0) return Vd80ErrIO;
   inp->Termination = val;
   if (ioctl(fd,Vd80IoctlGET_TRIGGER            ,&val) < 0) return Vd80ErrIO;
   inp->Source = val;

   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Set clock input settings. chns are ignored                           */

Vd80Err vd80SetClock(int fd, unsigned int mods, unsigned int chns, Vd80Input *inp) {

Vd80Err err;
int i, mod;
unsigned int mski;
int val;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = SetModule(fd,mod);
	 if (err != Vd80ErrSUCCESS) return err;

	 val = inp->Edge;
	 if (ioctl(fd,Vd80IoctlSET_CLOCK_EDGE       ,&val) < 0) return Vd80ErrIO;
	 val = inp->Termination;
	 if (ioctl(fd,Vd80IoctlSET_CLOCK_TERMINATION,&val) < 0) return Vd80ErrIO;
	 val = inp->Source;
	 if (ioctl(fd,Vd80IoctlSET_CLOCK            ,&val) < 0) return Vd80ErrIO;
      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get clock input settings                                             */

Vd80Err vd80GetClock(int fd, int mod, int chn, Vd80Input *inp) {

Vd80Err err;
int val;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   if (ioctl(fd,Vd80IoctlGET_CLOCK_EDGE       ,&val) < 0) return Vd80ErrIO;
   inp->Edge = val;
   if (ioctl(fd,Vd80IoctlGET_CLOCK_TERMINATION,&val) < 0) return Vd80ErrIO;
   inp->Termination = val;
   if (ioctl(fd,Vd80IoctlGET_CLOCK            ,&val) < 0) return Vd80ErrIO;
   inp->Source = val;

   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Set clock sample rate                                                */

Vd80Err vd80SetClockDivide(int fd, unsigned int mods, unsigned int chns, unsigned int dvd) {

int i, mod;
unsigned int mski;
int val;
Vd80Err err;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = SetModule(fd,mod);
	 if (err != Vd80ErrSUCCESS) return err;

	 val = VD80_CLKDIVMODE_DIVIDE;
	 if (ioctl(fd,Vd80IoctlSET_CLOCK_DIVIDE_MODE,&val) < 0) return Vd80ErrIO;

	 val = dvd;
	 if (ioctl(fd,Vd80IoctlSET_CLOCK_DIVISOR,&val) < 0) return Vd80ErrIO;
      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get clock sample rate                                                */

Vd80Err vd80GetClockDivide(int fd, int mod, int chn, unsigned int *dvd) {

Vd80Err err;
int val;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   if (ioctl(fd,Vd80IoctlGET_CLOCK_DIVISOR,&val) < 0) return Vd80ErrIO;
   *dvd = val;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Start acquisition, state will be pretrigger                          */

Vd80Err vd80StrtAcquisition(int fd, unsigned int mods, unsigned int chns) {

int i, mod;
unsigned int mski;
Vd80Err err;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = DoCommand(fd,mod,0,Vd80DrvrCommandSTART);
	 if (err != Vd80ErrSUCCESS) return err;
      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Trigger acquisition, state will be posttrigger                       */

Vd80Err vd80TrigAcquisition(int fd, unsigned int mods, unsigned int chns) {

int i, mod;
unsigned int mski;
Vd80Err err;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = DoCommand(fd,mod,0,Vd80DrvrCommandTRIGGER);
	 if (err != Vd80ErrSUCCESS) return err;
      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Stop acquisition, state will be idle                                 */

Vd80Err vd80StopAcquisition(int fd, unsigned int mods, unsigned int chns) {

int i, mod;
unsigned int mski;
Vd80Err err;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = DoCommand(fd,mod,0,Vd80DrvrCommandSTOP);
	 if (err != Vd80ErrSUCCESS) return err;
      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Connect to module interrupt                                          */

Vd80Err vd80Connect(int fd, unsigned int mods, unsigned int chns, unsigned int imsk) {

SkelDrvrConnection conn;

int i, mod;
unsigned int mski;
Vd80Err err;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = SetModule(fd,mod);
	 if (err != Vd80ErrSUCCESS) return err;

	 conn.Module  = mod;
	 conn.ConMask = imsk;
	 if (ioctl(fd,SkelDrvrIoctlCONNECT, &conn) < 0) return Vd80ErrIO;

      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Set wait timeout, 0 means wait for ever                              */

Vd80Err vd80SetTimeout(int fd, int tmo) {

   if (ioctl(fd,SkelDrvrIoctlSET_TIMEOUT,&tmo) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get wait timeout, 0 means wait for ever                              */

Vd80Err vd80GetTimeout(int fd, int *tmo) {

   if (ioctl(fd,SkelDrvrIoctlGET_TIMEOUT,tmo) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Set interrupt queueing on or off                                     */

Vd80Err vd80SetQueueFlag(int fd, Vd80QueueFlag qfl) {

   if (ioctl(fd,SkelDrvrIoctlSET_QUEUE_FLAG,&qfl) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get interrupt queueing on or off                                     */

Vd80Err vd80GetQueueFlag(int fd, Vd80QueueFlag *qfl) {

   if (ioctl(fd,SkelDrvrIoctlGET_QUEUE_FLAG,qfl) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Wait for an interrupt                                                */

Vd80Err vd80Wait(int fd, Vd80Intr *intr) {

SkelDrvrReadBuf rbuf;
int ret, qsz, qov;

   bzero((void *) intr, sizeof(Vd80Intr));

   ret = read(fd,&rbuf,sizeof(SkelDrvrReadBuf));
   if (ret == 0) return Vd80ErrTIMEOUT;

   if (ioctl(fd,SkelDrvrIoctlGET_QUEUE_SIZE,&qsz) < 0) return Vd80ErrIO;
   if (ioctl(fd,SkelDrvrIoctlGET_QUEUE_OVERFLOW,&qov) < 0) return Vd80ErrIO;

   intr->Mask         = rbuf.Connection.ConMask;
   intr->Time.tv_sec  = rbuf.Time.Second;
   intr->Time.tv_nsec = rbuf.Time.NanoSecond;
   intr->Module       = rbuf.Connection.Module;
   intr->Channel      = 0;
   intr->QueueSize    = qsz;
   intr->Missed       = qov;

   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Simulate an interrupt                                                */

Vd80Err vd80SimulateInterrupt(int fd, Vd80Intr *intr) {

SkelDrvrReadBuf rbuf;
int ret;

   rbuf.Connection.ConMask = intr->Mask;
   rbuf.Connection.Module  = intr->Module;

   ret = write(fd,&rbuf,sizeof(SkelDrvrReadBuf));
   if (ret == 0) return Vd80ErrTIMEOUT;

   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Set module buffer size and post sample count                         */

Vd80Err vd80SetBufferSize(int fd, unsigned int mods, unsigned int chns, int bsze, int post) {

int i, mod;
unsigned int mski;
Vd80Err err;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = SetModule(fd,mod);
	 if (err != Vd80ErrSUCCESS) return err;

	 if (ioctl(fd,Vd80IoctlSET_POSTSAMPLES,&post) < 0) return Vd80ErrIO;
      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get requested number of post samples                                 */

Vd80Err vd80GetBufferSize(int fd, unsigned int mod, int *post) {

Vd80Err err;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;
   if (ioctl(fd,Vd80IoctlGET_POSTSAMPLES,&post) < 0) return Vd80ErrIO;
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Need to know if bytes must be swapped                                */

static int little_endian() {
int   i = 1;
char *p = (char *) &i;

   return *p;
}

/* ==================================================================== */
/* Transfer module data by DMA to users buffer                          */

Vd80Err vd80GetBuffer(int fd, int mod, int chn, Vd80Buffer *buf) {

Vd80SampleBuf sbuf;
Vd80Err err;
int tps;

   err = DoCommand(fd,mod,chn,Vd80DrvrCommandREAD);
   if (err != Vd80ErrSUCCESS) return err;

   tps = buf->BSze - buf->Post -1;
   if (tps < 0) tps = 0;

   sbuf.SampleBuf      = buf->Addr;
   sbuf.BufSizeSamples = buf->BSze;
   sbuf.Channel        = chn;
   sbuf.TrigPosition   = tps;
   sbuf.Samples        = 0;

   if (ioctl(fd,Vd80IoctlREAD_SAMPLE,&sbuf) < 0) return Vd80ErrIO;

   buf->Tpos = sbuf.TrigPosition;
   buf->ASze = sbuf.Samples;
   buf->Ptsr = sbuf.PreTrigStat;

   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Set the trigger analogue levels                                      */

Vd80Err vd80SetTriggerLevels(int fd, unsigned int mods, unsigned int chns, Vd80AnalogTrig *atrg) {

int i, j, mod, chn;
unsigned int mski, mskj;
Vd80Err err;
Vd80DrvrAnalogTrig adtrg;

   if (mods == Vd80ModNONE) mods = Vd80Mod01;
   if (chns == Vd80ChnNONE) chns = Vd80Chn01;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = SetModule(fd,mod);
	 if (err != Vd80ErrSUCCESS) return err;

	 for (j=0; j<Vd80CHANNELS; j++) {
	    mskj = 1 << j;

	    if (chns & mskj) {
	       chn = j+1;

	       adtrg.Channel = chn;
	       adtrg.Control = atrg->AboveBelow;
	       adtrg.Level   = atrg->TrigLevel;

	       if (ioctl(fd,Vd80IoctlSET_ANALOGUE_TRIGGER,&adtrg) < 0) return Vd80ErrIO;
	    }
	 }
      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get the trigger analogue level                                       */

Vd80Err vd80GetTriggerLevels(int fd, int mod, int chn, Vd80AnalogTrig *atrg) {

Vd80Err err;
Vd80DrvrAnalogTrig adtrg;

   if (mod == Vd80ModNONE) mod = Vd80Mod01;
   if (chn == Vd80ChnNONE) chn = Vd80Chn01;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   adtrg.Channel = chn;

   if (ioctl(fd,Vd80IoctlGET_ANALOGUE_TRIGGER,&adtrg) < 0) return Vd80ErrIO;

   atrg->AboveBelow = adtrg.Control;
   atrg->TrigLevel  = adtrg.Level;

   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Set trigger configuration params, delay and min pretrig samples      */

Vd80Err vd80SetTriggerConfig(int fd, unsigned int mods, unsigned int chns, Vd80TrigConfig *ctrg) {

int i, j, mod, chn;
unsigned int mski, mskj;
Vd80Err err;
Vd80DrvrTrigConfig cdtrg;

   if (mods == Vd80ModNONE) mods = Vd80Mod01;
   if (chns == Vd80ChnNONE) chns = Vd80Chn01;

   for (i=0; i<Vd80MODULES; i++) {
      mski = 1 << i;

      if (mods & mski) {
	 mod = i+1;
	 err = SetModule(fd,mod);
	 if (err != Vd80ErrSUCCESS) return err;

	 for (j=0; j<Vd80CHANNELS; j++) {
	    mskj = 1 << j;

	    if (chns & mskj) {
	       chn = j+1;

	       cdtrg.TrigDelay  = ctrg->TrigDelay;
	       cdtrg.MinPreTrig = ctrg->MinPreTrig;

	       if (ioctl(fd,Vd80IoctlSET_TRIGGER_CONFIG,&cdtrg) < 0)
		  return Vd80ErrIO;
	       break;
	    }
	 }
      }
   }
   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Get trigger configuration params, delay and min pretrig samples      */

Vd80Err vd80GetTriggerConfig(int fd, int mod, int chn, Vd80TrigConfig *ctrg) {

Vd80Err err;
Vd80DrvrTrigConfig cdtrg;

   if (mod == Vd80ModNONE) mod = Vd80Mod01;
   if (chn == Vd80ChnNONE) chn = Vd80Chn01;

   err = SetModule(fd,mod);
   if (err != Vd80ErrSUCCESS) return err;

   if (ioctl(fd,Vd80IoctlGET_TRIGGER_CONFIG,&cdtrg) < 0) return Vd80ErrIO;

   ctrg->TrigDelay  = cdtrg.TrigDelay;
   ctrg->MinPreTrig = cdtrg.MinPreTrig;

   return Vd80ErrSUCCESS;
}

/* ==================================================================== */
/* Convert an error to a string                                         */

static char *ErrTexts[Vd80ERRORS] = {
   "All went OK, No error                      ",  // Vd80ErrSUCCESS,
   "Function is not implemented on this device ",  // Vd80ErrNOT_IMP,
   "Invalid Start value for this device        ",  // Vd80ErrSTART,
   "Invalid Mode  value for this device        ",  // Vd80ErrMODE,
   "Invalid Clock value for this device        ",  // Vd80ErrCLOCK,
   "Can't open a driver file handle, fatal     ",  // Vd80ErrOPEN,
   "Can't connect to that interrupt            ",  // Vd80ErrCONNECT,
   "No connections to wait for                 ",  // Vd80ErrWAIT,
   "Timeout in wait                            ",  // Vd80ErrTIMEOUT,
   "Queue flag must be set 0 queueing is needed",  // Vd80ErrQFLAG,
   "IO or BUS error                            ",  // Vd80ErrIO,
   "Module is not enabled                      ",  // Vd80ErrNOT_ENAB
   "Not available for INTERNAL sources         ",  // Vd80ErrSOURCE,
   "Value out of range                         ",  // Vd80ErrVOR,
   "Bad device type                            ",  // Vd80ErrDEVICE,
   "Bad address                                ",  // Vd80ErrADDRESS,
   "Can not allocate memory                    ",  // Vd80ErrMEMORY,
   "Shared library error, can't open object    ",  // Vd80ErrDLOPEN,
   "Shared library error, can't find symbol    ",  // Vd80ErrDLSYM,
   "Can't open sampler device driver           ",  // Vd80ErrDROPEN,
   "Invalid handle                             ",  // Vd80ErrHANDLE,
   "Invalid module number, not installed       ",  // Vd80ErrMODULE,
   "Invalid channel number for this module     "   // Vd80ErrCHANNEL,
 };

char *vd80ErrToStr(int fd, Vd80Err error) {

char         *cp;    /* Char pointer */
unsigned int i;
static char  err[Vd80ErrSTRING_SIZE]; /* Static text area when null handle */

   bzero((void *) err, Vd80ErrSTRING_SIZE);

   i = (unsigned int) error;
   if (i >= Vd80ERRORS) {
      sprintf(err,"Vd80Lib:Invalid error code:%d Not in[0..%d]\n",i,Vd80ERRORS);
      return err;
   }

   sprintf(err,"Vd80Lib:%s",ErrTexts[i]);

   /* Extra info available from errno */

   if ((error == Vd80ErrIO)
   ||  (error == Vd80ErrDROPEN)
   ||  (error == Vd80ErrMEMORY)) {
     cp = strerror(errno);
     if (cp) {
	strcat(err,":");
	i = Vd80ErrSTRING_SIZE - strlen(err) -1;  /* -1 is 0 terminator byte */
	strncat(err,cp,i);
     }
   }

   return err;
}
