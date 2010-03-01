/* ==================================================================== */
/* Implement general stub library for generic devices                   */
/* Julian Lewis Wed 15th April 2009                                     */
/* ==================================================================== */

#ifndef STUB_API
#define STUB_API

/* The common API that each device library should try to implement */

typedef struct {
   int (*OpenHandle)();
   void (*CloseHandle)(int fd);
   int (*ResetMod)(int fd, int mod);
   int (*GetAdcValue)(int fd, int mod, int chn, int *adc);
   int (*CallibrateChn)(int fd, int mod, int chn, int low, int hih);
   int (*GetState)(int fd, int mod, int chn, SLbState *ste);
   int (*GetStatus)(int fd, int mod, SLbStatus *sts);
   int (*SetDebug)(int fd, SLbDebug *deb);
   int (*GetDebug)(int fd, SLbDebug *deb);
   int (*GetDatumSize)(int fd, SLbDatumSize *dsz);
   int (*GetModuleCount)(int fd, int *cnt);
   int (*GetChannelCount)(int fd, int *cnt);
   int (*GetVersion)(int fd, int mod, SLbVersion *ver);
   int (*SetTrigger)(int fd, unsigned int mods, int unsigned chns, SLbInput *inp);
   int (*GetTrigger)(int fd, int mod, int chn, SLbInput *inp);
   int (*SetClock)(int fd, unsigned int mods, unsigned int chns, SLbInput *inp);
   int (*GetClock)(int fd, int mod, int chn, SLbInput *inp);
   int (*SetStop)(int fd, unsigned int mods, unsigned int chns, SLbInput *inp);
   int (*GetStop)(int fd, int mod, int chn, SLbInput *inp);
   int (*SetClockDivide)(int fd, unsigned int mods, unsigned int chns, unsigned int dvd);
   int (*GetClockDivide)(int fd, int mod, int chn, unsigned int *dvd);
   int (*StrtAcquisition)(int fd, unsigned int mods, unsigned int chns);
   int (*TrigAcquisition)(int fd, unsigned int mods, unsigned int chns);
   int (*ReadAcquisition)(int fd, unsigned int mods, unsigned int chns);
   int (*StopAcquisition)(int fd, unsigned int mods, unsigned int chns);
   int (*Connect)(int fd, unsigned int mods, unsigned int chns, unsigned int imsk);
   int (*SetTimeout)(int fd, int tmo);
   int (*GetTimeout)(int fd, int *tmo);
   int (*SetQueueFlag)(int fd, SLbQueueFlag qfl);
   int (*GetQueueFlag)(int fd, SLbQueueFlag *qfl);
   int (*Wait)(int fd, SLbIntr *intr);
   int (*SimulateInterrupt)(int fd, SLbIntr *intr);
   int (*SetBufferSize)(int fd, unsigned int mods, unsigned int chns, int bsze, int post);
   int (*GetBuffer)(int fd, int mod, int chn, SLbBuffer *buf);
   int (*SetTriggerLevels)(int fd, unsigned int mods, unsigned int chns, SLbAnalogTrig *atrg);
   int (*GetTriggerLevels)(int fd, int mod, int chn, SLbAnalogTrig *atrg);
   int (*SetTriggerConfig)(int fd, unsigned int mods, unsigned int chns, SLbTrigConfig *ctrg);
   int (*GetTriggerConfig)(int fd, int mod, int chn, SLbTrigConfig *ctrg);
 } SLbAPI;

#endif
