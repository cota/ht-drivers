/* ========================================================== */
/* Test program for SampLib library                           */
/* ========================================================== */

 #include <unistd.h>
#include <sys/mman.h>

char *defaultconfigpath = "testprog.config";

char *configpath = NULL;
char localconfigpath[128];  /* After a CD */

int module = 1;
int channel = 1;
int PostSamps = 512;
SLbBuffer Buffer;   /* Working buffer */
int bufreset = 0;   /* Buffer hasn't been initialized */

/* ========================================================== */

static char path[128];

char *GetFile(char *name) {
FILE *gpath = NULL;
char txt[128];
int i, j;

   if (configpath) {
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = NULL;
      }
   }

   if (configpath == NULL) {
      configpath = "./ctrtest.config";
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = "/dsc/local/data/ctrtest.config";
	 gpath = fopen(configpath,"r");
	 if (gpath == NULL) {
	    configpath = defaultconfigpath;
	    gpath = fopen(configpath,"r");
	    if (gpath == NULL) {
	       configpath = NULL;
	       sprintf(path,"./%s",name);
	       return path;
	    }
	 }
      }
   }

   bzero((void *) path,128);

   while (1) {
      if (fgets(txt,128,gpath) == NULL) break;
      if (strncmp(name,txt,strlen(name)) == 0) {
	 for (i=strlen(name); i<strlen(txt); i++) {
	    if (txt[i] != ' ') break;
	 }
	 j= 0;
	 while ((txt[i] != ' ') && (txt[i] != 0)) {
	    path[j] = txt[i];
	    j++; i++;
	 }
	 strcat(path,name);
	 fclose(gpath);
	 return path;
      }
   }
   fclose(gpath);
   return NULL;
}

/* ========================================================== */
/* Convert time to string.                                    */
/* Result is pointer to static string representing the time.  */
/*    the format is: Thu-18/Jan/2001 08:25:14.967             */
/*                   day-dd/mon/yyyy hh:mm:ss.ddd             */

#define TBLEN 128

char *TimeToStr(struct timespec *t) {

static char tbuf[TBLEN];

char tmp[TBLEN];
char *yr, *ti, *md, *mn, *dy;
double ms;
double fms;

    bzero((void *) tbuf, TBLEN);
    bzero((void *) tmp, TBLEN);

    if (t->tv_sec) {

	ctime_r (&t->tv_sec, tmp);
        tmp[3] = 0;
        dy = &(tmp[0]);
        tmp[7] = 0;
        mn = &(tmp[4]);
        tmp[10] = 0;
        md = &(tmp[8]);
        if (md[0] == ' ')
            md[0] = '0';
        tmp[19] = 0;
        ti = &(tmp[11]);
        tmp[24] = 0;
        yr = &(tmp[20]);

	if (t->tv_nsec) {
	   fms  = (double) t->tv_nsec;
	   ms = fms;
	   sprintf (tbuf, "%s-%s/%s/%s %s.%010.0f", dy, md, mn, yr, ti, ms);
	} else
	   sprintf (tbuf, "%s-%s/%s/%s %s"  , dy, md, mn, yr, ti);

    }
    else {
	sprintf (tbuf, "--- Zero ---");
    }
    return (tbuf);
}

/* ========================================================== */

#define MAX_HANDLES 8

int       cntoph = 0;           /* Count of open Handles */
int       curoph = 0;           /* Current handle */
SLbHandle handles[MAX_HANDLES]; /* Array of handles */

SLbHandle FindOpenHandle(char *dname, char *dllver) {

SLbErr err;
SLbHandle hnd;
int i;
char *cd, *cv;

   for (i=0; i<cntoph; i++) {
      hnd = handles[i];
      if (hnd) {
	 err = SLbGetDeviceName(hnd, &cd);
	 if (err != SLbErrSUCCESS) continue;

	 err = SLbGetDllVersion(hnd, &cv);
	 if (err != SLbErrSUCCESS) continue;

	 if ((strcmp(dname,cd) == 0)
	 &&  (strcmp(dllver,cv) == 0)) {
	    curoph = i;
	    return hnd;
	 }
      }
   }
   return NULL;
}

/* ========================================================== */

void ListHandles() {

SLbErr err;
SLbHandle hnd;
int i;
char *cd, *cv;

   if (cntoph == 0) printf("Open:No open handles\n");

   for (i=0; i<cntoph; i++) {
      hnd = handles[i];
      if (hnd) {
	 err = SLbGetDeviceName(hnd, &cd);
	 if (err != SLbErrSUCCESS) {
	    fprintf(stderr,"ListHandles:%s\n",SLbErrToStr(hnd,err));
	    return;
	 }
	 err = SLbGetDllVersion(hnd, &cv);
	 if (err != SLbErrSUCCESS) {
	    fprintf(stderr,"ListHandles:%s\n",SLbErrToStr(hnd,err));
	    return;
	 }

	 printf("Open:[%d]%s:%s ",i+1,cd,cv);
	 if (i==curoph) printf("<==");
	 printf("\n");
      }
   }
}

/* ========================================================== */

int InsertHandle(SLbHandle hnd) {

int i;

   for (i=0; i<cntoph; i++) {
      if (handles[i] == NULL) {
	 handles[i] = hnd;
	 curoph = i;
	 return 1;
      }
   }
   if (cntoph < MAX_HANDLES) {
      i = cntoph;
      handles[i] = hnd;
      cntoph++;
      return 1;
   }
   fprintf(stderr,"InsertHandle:Can't open more than:%d handles\n",MAX_HANDLES);
   return 0;
}

/* ========================================================== */
/* Implement test programm Commands                           */
/* ========================================================== */

/* ========================================================== */

int OpenHandle(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
char *cd = NULL, *cv = NULL;
SLbHandle hnd = NULL;
SLbErr err;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("OpenHandle:[<device name> <revision>]\n");
      return arg;
   }

   if ((cntoph == 0) && (curoph == 0)) {
      bzero((void *) handles, (MAX_HANDLES * sizeof(SLbHandle)));
   }

   if (getenv("LD_LIBRARY_PATH") == NULL) {
      printf("OpenHandle:Warning:LD_LIBRARY_PATH not set\n");
   }

   if ((++atoms)->type == String) {
      cd = atoms->text;

      if ((++atoms)->type == String) {
	 cv = atoms->text;

	 hnd = FindOpenHandle(cd,cv);
	 if (hnd == NULL) {
	    err = SLbOpenHandle(&hnd,cd,cv);
	    if (err) {
	       fprintf(stderr,"OpenHandle:%s",SLbErrToStr(hnd,err));
	       return arg;
	    }
	    if (InsertHandle(hnd) == 0) {
	       err = SLbCloseHandle(hnd);
	       if (err == SLbErrSUCCESS) err = SLbErrMEMORY;
	       fprintf(stderr,"OpenHandle:%s",SLbErrToStr(hnd,err));
	       return arg;
	    }
	 }
      } else {
	 fprintf(stderr,"OpenHandle:Missing version:%s\n",cd);
      }
   }
   ListHandles();
   return arg;
}

/* ========================================================== */

int CloseHandle(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
int i;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("CloseHandle: //Close the current handle\n");
      return arg;
   }

   i = curoph;
   hnd = handles[i];
   if (hnd) {
      err = SLbCloseHandle(hnd);
      if (err != SLbErrSUCCESS) {
	 fprintf(stderr,"CloseHandle:%s",SLbErrToStr(hnd,err));
      } else {
	 handles[i] = NULL;
	 printf("CloseHandle:%d closed OK\n",i);
      }
   }
   return arg;
}

/* ========================================================== */

int ShowHandle(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("ShowHamdle: //details on current handle\n");
      return arg;
   }
   printf("ShowHandle:Not implemented yet\n");
   return arg;
}

/* ========================================================== */

int NextHandle(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
int i;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("NextHandle: //Switch to the next handle, with wrap around\n");
      return arg;
   }

   i = curoph+1;
   if (i >= cntoph) i = 0;

   for (; i<cntoph; i++) {
      if (handles[i]) {
	 curoph = i;
	 return arg;
      }
   }

   return arg;
}

/* ========================================================== */
/* Calls the function with one parameter                      */

int GetSymbol(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
int i, fd, p1;
char *cp;
int (*func)(int fd, int p1);
void *f;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSymbol:[<symb>][<param>] //Call function <symb> with [<param>]\n");
      return arg;
   }

   if ((++atoms)->type == String) {
      cp = atoms->text;

      i = curoph;
      hnd = handles[i];

      err = SLbGetSymbol(hnd,cp,&f);
      if (err) {
	 printf("GetSymbol:%s:%s\n",cp,SLbErrToStr(hnd,err));
	 return arg;
      }
      func = f;

      err = SLbGetFileDescriptor(hnd,&fd);
      if (err) {
	 printf("GetSymbol:%s:%s\n",cp,SLbErrToStr(hnd,err));
	 return arg;
      }

      p1 = 0;
      if ((++atoms)->type == Numeric) p1 = atoms->val;

      printf("GetSymbol:Calling err=%s(fd=%d,p1=%d)\n",cp,fd,p1);
      err = (SLbErr) func(fd,p1);
      if (err) {
	 printf("GetSymbol:%s:%s\n",cp,SLbErrToStr(hnd,err));
	 return arg;
      }
      return arg;
   }
   fprintf(stderr,"GetSymbol:No symbol supplied\n");
   return arg;
}

/* ========================================================== */

int SetDebug(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbDebug deb;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("SetDebug:[<mask>] //Mask bits are:\n");
      printf("   ASSERTION   0x01  - All assertion violations (BUGS) printed\n");
      printf("   TRACE       0x02  - Trace all driver calls\n");
      printf("   WARNING     0x04  - Warnings such as bad call parameters\n");
      printf("   MODULE      0x08  - Hardware module errors, bus error etc\n");
      printf("   INFORMATION 0x10  - Extra information\n");
      printf("   EMULATION   0x100 - Turn on driver emulation, no hardware access\n");
      return arg;
   }

   hnd = handles[curoph];

   if ((++atoms)->type == Numeric) {
      deb.ClientPid = 0;            /* 0 = me */
      deb.DebugFlag = atoms->val;
      err = SLbSetDebug(hnd,&deb);
      if (err) fprintf(stderr,"SetDebug:%s\n",SLbErrToStr(hnd,err));
   }

   err = SLbGetDebug(hnd,&deb);
   if (err) fprintf(stderr,"SetDebug:%s\n",SLbErrToStr(hnd,err));

   printf("SetDebug:Mask is now:0x%X\n",(int) deb.DebugFlag);

   return arg;
}

/* ========================================================== */

int ResetMod(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("ResetMod: //Reset the current module\n");
      return arg;
   }

   hnd = handles[curoph];
   err = SLbResetMod(hnd,module);
   if (err) fprintf(stderr,"ResetMod:%s\n",SLbErrToStr(hnd,err));

   return arg;
}

/* ========================================================== */

int GetAdcValue(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
int adc;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetAdcValue: //Get the current channel and module ADC value\n");
      return arg;
   }

   hnd = handles[curoph];
   err = SLbGetAdcValue(hnd,module,channel,&adc);
   if (err) fprintf(stderr,"GetAdcValue:%s\n",SLbErrToStr(hnd,err));

   printf("MOD:%d CHN:%d ADC:0x%X (%d)\n",module,channel,adc,adc);

   return arg;
}

/* ========================================================== */

int CalibrateChn(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("CalibrateChn:[<low> <high>] //Calibrate channel ADC\n");
      return arg;
   }
   printf("CalibrateChn:Not implemented yet\n");
   return arg;
}

/* ========================================================== */

int GetState(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbState state;
char *cp;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetState: //Show current module and channels state\n");
      return arg;
   }

   hnd = handles[curoph];
   err = SLbGetState(hnd,module,channel,&state);
   if (err) fprintf(stderr,"GetState:%s\n",SLbErrToStr(hnd,err));

   if      (state == SLbStateIDLE)        cp = "IDLE";
   else if (state == SLbStatePRETRIGGER)  cp = "PRETRIGGER";
   else if (state == SLbStatePOSTTRIGGER) cp = "POSTTRIGGER";
   else                                   cp = "NOTSET";

   printf("GetState:Mod:%d Chn:%d State:%d = %s\n",module,channel,(int) state,cp);

   return arg;
}

/* ========================================================== */

int GetStatus(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbStatus status;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetStatus: //Show current module status\n");
      return arg;
   }

   hnd = handles[curoph];
   err = SLbGetStatus(hnd,module,&status);
   if (err) fprintf(stderr,"GetState:%s\n",SLbErrToStr(hnd,err));

   printf("GetStatus:Mod:%d Status:0x%X = ",module,(int) status);

   if (status & SLbStatusDISABLED)      printf("Disabled:"); else printf("Enabled:");
   if (status & SLbStatusHARDWARE_FAIL) printf("HardwareFailed:");
   if (status & SLbStatusBUS_FAULT)     printf("BusError:");
   if (status & SLbStatusEMULATION)     printf("EmulationOn:");
   if (status & SLbStatusNO_HARDWARE)   printf("NoHardwareInstalled:");
   if (status & SLbStatusHARDWARE_DBUG) printf("HardwareDebug:");
   printf("\n");

   return arg;
}

/* ========================================================== */

int GetDatumSize(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbDatumSize dsize;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetDatumSize: //Show module datum size\n");
      return arg;
   }

   hnd = handles[curoph];
   err = SLbGetDatumSize(hnd,&dsize);
   if (err) fprintf(stderr,"GetDatumSize:%s\n",SLbErrToStr(hnd,err));

   printf("GetDatumSize:Bytes:%d\n",(int) dsize);

   return arg;
}

/* ========================================================== */

int GetSetMod(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
int mod, mcn;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetMod:[<module>] //Get or set the current module\n");
      return arg;
   }

   hnd = handles[curoph];
   err = SLbGetModCount(hnd,&mcn);
   if (err) fprintf(stderr,"GetModCount:%s\n",SLbErrToStr(hnd,err));

   if ((++atoms)->type == Numeric) {
      mod = atoms->val;
      if ((mod > 0) && (mod <= mcn)) module = mod;
   }

   printf("GetSetMod:Current module:%d Installed modules:%d\n",module,mcn);

   return arg;
}

/* ========================================================== */

int GetSetChn(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
int chn, ccn;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetChn:[<channel>] //Get or set the current channel\n");
      return arg;
   }

   hnd = handles[curoph];
   err = SLbGetChnCount(hnd,&ccn);
   if (err) fprintf(stderr,"GetChnCount:%s\n",SLbErrToStr(hnd,err));

   if ((++atoms)->type == Numeric) {
      chn = atoms->val;
      if ((chn > 0) && (chn <= ccn)) channel = chn;
   }

   printf("GetSetChn:Current channel:%d Available channels:%d\n",channel,ccn);

   return arg;
}

/* ========================================================== */

int NextChn(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
int chn, ccn;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("NextChn: //Select next channel with wrap around\n");
      return arg;
   }

   hnd = handles[curoph];

   err = SLbGetChnCount(hnd,&ccn);
   if (err) fprintf(stderr,"GetChnCount:%s\n",SLbErrToStr(hnd,err));
   chn = channel +1;
   if (chn > ccn) chn = 1;
   channel = chn;

   return arg;
}

/* ========================================================== */

int NextMod(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
int mod, mcn;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("NextMod: //Select next module with wrap around\n");
      return arg;
   }

   hnd = handles[curoph];

   err = SLbGetModCount(hnd,&mcn);
   if (err) fprintf(stderr,"GetModCount:%s\n",SLbErrToStr(hnd,err));
   mod = module +1;
   if (mod > mcn) mod = 1;
   module = mod;

   return arg;
}

/* ========================================================== */

#ifndef COMPILE_TIME
#define COMPILE_TIME 0
#endif

int GetVersion(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbVersion ver;
struct timespec t;
int i;
char c, *cp;
unsigned int sv;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetVersion: //Show hardware and software versions\n");
      return arg;
   }

   err = SLbGetStubVersion(&sv);
   t.tv_sec = sv;
   t.tv_nsec = 0;
   printf("StubLibVersion:%s\n",TimeToStr(&t));

   if (COMPILE_TIME != 0) {
      t.tv_sec = COMPILE_TIME;
      t.tv_nsec = 0;
      printf("TestPrgVersion:%s\n",TimeToStr(&t));
   }

   hnd = handles[curoph];
   err = SLbGetVersion(hnd,module,&ver);
   if (err) {
      fprintf(stderr,"GetVersion:%s\n",SLbErrToStr(hnd,err));
      printf("GetVersion:Probably error due to handle not opened, or no library\n");
      return arg;
   }

   t.tv_sec = ver.LibraryVersion;
   t.tv_nsec = 0;
   printf("DllLibCompiled:%s\n",TimeToStr(&t));

   t.tv_sec = ver.DriverVersion;
   t.tv_nsec = 0;
   printf("DriverCompiled:%s\n",TimeToStr(&t));

   for (i=0; i<strlen(ver.ModVersion); i++) {
      c = ver.ModVersion[i];
      if ( ((c >= 'A') && (c <= 'Z'))
      ||   ((c >= '0') && (c <= '9')) ) continue;
      ver.ModVersion[i] = 0;
      break;
   }
   printf("ModHwrVersion :%s Module:%d\n",ver.ModVersion,module);

   err = SLbGetDllVersion(hnd,&cp);
   if (err) {
      fprintf(stderr,"GetDllVersion :%s\n",SLbErrToStr(hnd,err));
      printf("GetVersion:Can't get DLL library version\n");
      return arg;
   }
   printf("DllLibVersion :%s\n",cp);

   return arg;
}

/* ========================================================== */

int GetSetClk(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbInput ins;
unsigned int mmsk, cmsk;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetClk:[<source> <edge> <termination>]:\n");
      printf("   Source     :0=Internal 1=External\n");
      printf("   Edge       :0=Rising   1=Falling\n");
      printf("   Termination:0=None     1=50-Ohms\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   if ((++atoms)->type == Numeric) {
      ins.Source = (SLbSource) atoms->val;

      if ((++atoms)->type == Numeric) {
	 ins.Edge = (SLbEdge) atoms->val;

	 if ((++atoms)->type == Numeric) {
	    ins.Termination = (SLbTermination) atoms->val;

	    err = SLbSetClock(hnd,mmsk,cmsk,&ins);
	    if (err) fprintf(stderr,"SetClk:%s\n",SLbErrToStr(hnd,err));
	 }
      }
   }

   err =SLbGetClock(hnd,module,channel,&ins);
   if (err) fprintf(stderr,"GetClk:%s\n",SLbErrToStr(hnd,err));

   printf("Clock:\n");
   printf("   Source     :"); if (ins.Source      == SLbSourceINTERNAL)  printf("Internal\n"); else printf("External\n");
   printf("   Edge       :"); if (ins.Edge        == SLbEdgeRISING)      printf("Rising\n");   else printf("Falling\n");
   printf("   Termination:"); if (ins.Termination == SLbTerminationNONE) printf("None\n");     else printf("50-Ohms\n");
   printf("\n");

   return arg;
}

/* ========================================================== */

#define INT_CLKFRQ 200000

int GetSetClkDiv(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
unsigned int mmsk, cmsk, dvsr;
unsigned int fq;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetClkDiv:[<divisor>] //Get or set clock divisor\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   if ((++atoms)->type == Numeric) {
      dvsr = (SLbSource) atoms->val;
      err = SLbSetClockDivide(hnd,mmsk,cmsk,dvsr);
      if (err) fprintf(stderr,"SetClockDivide:%s\n",SLbErrToStr(hnd,err));
   }

   err = SLbGetClockDivide(hnd,module,channel,&dvsr);
   if (err) fprintf(stderr,"GetClockDivide:%s\n",SLbErrToStr(hnd,err));

   fq = INT_CLKFRQ/(dvsr+1);

   printf("ClockDivide:%d=[%d Hz]\n",dvsr,fq);

   return arg;
}

/* ========================================================== */

int GetSetTrg(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbInput ins;
unsigned int mmsk, cmsk;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetTrg:[<source> <edge> <termination>]:\n");
      printf("   Source     :0=Internal 1=External\n");
      printf("   Edge       :0=Rising   1=Falling\n");
      printf("   Termination:0=None     1=50-Ohms\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   if ((++atoms)->type == Numeric) {
      ins.Source = (SLbSource) atoms->val;

      if ((++atoms)->type == Numeric) {
	 ins.Edge = (SLbEdge) atoms->val;

	 if ((++atoms)->type == Numeric) {
	    ins.Termination = (SLbTermination) atoms->val;

	    err = SLbSetTrigger(hnd,mmsk,cmsk,&ins);
	    if (err) fprintf(stderr,"SetTrigger:%s\n",SLbErrToStr(hnd,err));
	 }
      }
   }

   err =SLbGetTrigger(hnd,module,channel,&ins);
   if (err) fprintf(stderr,"GetTrigger:%s\n",SLbErrToStr(hnd,err));

   printf("Trigger:\n");
   printf("   Source     :"); if (ins.Source      == SLbSourceINTERNAL)  printf("Internal\n"); else printf("External\n");
   printf("   Edge       :"); if (ins.Edge        == SLbEdgeRISING)      printf("Rising\n");   else printf("Falling\n");
   printf("   Termination:"); if (ins.Termination == SLbTerminationNONE) printf("None\n");     else printf("50-Ohms\n");
   printf("\n");

   return arg;
}

/* ========================================================== */

int GetSetAnalogTrg(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbAnalogTrig atrg;
unsigned int mmsk, cmsk;
float volts;
short shvlt;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetAnalogTrg:[<Control> <Level>]:\n");
      printf("   Control     :0=Disable,1=Above,2=Below\n");
      printf("   Level       :[16Bit] Scales to +/- 10 Volts\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   if ((++atoms)->type == Numeric) {
      if      (atoms->val == 0) { atrg.AboveBelow = SLbAtrgOFF;   printf("SetTrig:OFF\n");  }
      else if (atoms->val == 1) { atrg.AboveBelow = SLbAtrgABOVE; printf("SetTrig:ABOVE\n");}
      else if (atoms->val == 2) { atrg.AboveBelow = SLbAtrgBELOW; printf("SetTrig:BELOW\n");}
      else {
	 fprintf(stderr,"GetSetAnalogTrg:Bad Control value:%d Not in:[0..2]\n",atoms->val);
	 return arg;
      }

      if ((++atoms)->type == Numeric) {
	 atrg.TrigLevel = (atoms->val & 0x0FFF);
	 if (atoms->val & 0xFFFFF000) {
	    printf("Level is 12-Bit:0x%X => 0x%X\n",(int) atoms->val,(int) atrg.TrigLevel);
	 }
	 shvlt = (short) atrg.TrigLevel << 4;
	 volts = ( ((float) shvlt) * 10.0)/32767.0;
	 printf("SetLevl:%d[0x%X]=%f Volts\n",atrg.TrigLevel,atrg.TrigLevel,volts);

	 err = SLbSetTriggerLevels(hnd,mmsk,cmsk,&atrg);
	 if (err) fprintf(stderr,"SetTriggerLevels:%s\n",SLbErrToStr(hnd,err));
      } else {
	 fprintf(stderr,"GetSetAnalogTrg:No trigger level, abort\n");
	 return arg;
      }
   }

   err = SLbGetTriggerLevels(hnd,module,channel,&atrg);
   if (err) fprintf(stderr,"GetTriggerLevels:%s\n",SLbErrToStr(hnd,err));

   shvlt = (short) atrg.TrigLevel << 4;
   volts = ( ((float) shvlt) * 10.0)/32767.0;

   printf("Trigger:\n");
   if      (atrg.AboveBelow == SLbAtrgOFF)   printf("   Trig:OFF\n");
   else if (atrg.AboveBelow == SLbAtrgABOVE) printf("   Trig:ABOVE\n");
   else if (atrg.AboveBelow == SLbAtrgBELOW) printf("   Trig:BELOW\n");
   else                                      printf("   Trig:Illegal value:%d\n",(int) atrg.AboveBelow);

   printf("   Levl:"); printf("%d[0x%X]=%f Volts\n",atrg.TrigLevel,atrg.TrigLevel,volts);
   printf("\n");

   return arg;
}

/* ========================================================== */

int GetSetStop(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbInput ins;
unsigned int mmsk, cmsk;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetStop:[<source> <edge> <termination>]:\n");
      printf("   Source     :0=Internal 1=External\n");
      printf("   Edge       :0=Rising   1=Falling\n");
      printf("   Termination:0=None     1=50-Ohms\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   if ((++atoms)->type == Numeric) {
      ins.Source = (SLbSource) atoms->val;

      if ((++atoms)->type == Numeric) {
	 ins.Edge = (SLbEdge) atoms->val;

	 if ((++atoms)->type == Numeric) {
	    ins.Termination = (SLbTermination) atoms->val;

	    err = SLbSetStop(hnd,mmsk,cmsk,&ins);
	    if (err) fprintf(stderr,"SetStop:%s\n",SLbErrToStr(hnd,err));
	 }
      }
   }

   err =SLbGetStop(hnd,module,channel,&ins);
   if (err) fprintf(stderr,"GetStop:%s\n",SLbErrToStr(hnd,err));

   printf("Stop:\n");
   printf("   Source     :"); if (ins.Source      == SLbSourceINTERNAL)  printf("Internal\n"); else printf("External\n");
   printf("   Edge       :"); if (ins.Edge        == SLbEdgeRISING)      printf("Rising\n");   else printf("Falling\n");
   printf("   Termination:"); if (ins.Termination == SLbTerminationNONE) printf("None\n");     else printf("50-Ohms\n");
   printf("\n");

   return arg;
}

/* ========================================================== */

#define ALIGN_MEMORY_BUFFER 0
#define MIN_BUF 1024

int GetSetBufferSize(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
unsigned int mmsk, cmsk;
int bsze, post, cc;
SLbDatumSize dsze;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetBufferSize:[<TotalSize><PostTrigSize>] //Allocate memory\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   err = SLbGetDatumSize(hnd,&dsze);
   if (err) fprintf(stderr,"GetDatumSize:%s\n",SLbErrToStr(hnd,err));

   if (bufreset == 0) {
      bzero((void *) &Buffer, sizeof(SLbBuffer));
      bufreset = 1;
   }

   if ((++atoms)->type == Numeric) {
      bsze = atoms->val;
      if (bsze < MIN_BUF) {
	 printf("GetSetBufferSize:Buffer Size:%d too small, setting:%d\n",bsze,MIN_BUF);
	 bsze = MIN_BUF;
      }

      if ((++atoms)->type == Numeric) {
	 post = atoms->val;
	 if (post >= bsze) {
	    post = bsze>>1;
	    printf("GetSetBufferSize:Post Size too big, setting:%d\n",post);
	 }

	 if (post <= 0) {
	    post = bsze>>1;
	    printf("GetSetBufferSize:Post Size too small, setting:%d\n",post);
	 }

	 if (Buffer.BSze != bsze) {
	    if (Buffer.Addr) {
//             munlock(Buffer.Addr,Buffer.BSze);
	       free(Buffer.Addr);
	    }
#if ALIGN_MEMORY_BUFFER
	    printf("Allocating aligned buffer via posix_memalign\n");
	    cc = posix_memalign((void **) &Buffer.Addr, getpagesize(), (size_t) (bsze*dsze));
	    if (cc) {
#else
	    printf("Allocating NON aligned buffer via malloc\n");
	    Buffer.Addr = malloc(bsze*dsze);
	    cc = errno;
	    if (Buffer.Addr == NULL) {
#endif
	       printf("GetSetBufferSize:Cant allocate memory:%d\n",cc);
	       perror("GetSetBufferSize");
	       return arg;
	    }
//          mlock(Buffer.Addr,Buffer.BSze);
	    bzero((void *) Buffer.Addr,(size_t) (bsze*dsze));
	 }
	 Buffer.BSze = bsze;
	 Buffer.Post = post;
	 PostSamps   = post; /* Need to keep a copy */
      }
   }

   bsze = Buffer.BSze;
   post = Buffer.Post;

   if ((bsze == 0) || (post == 0)) {
      printf("GetSetBufferSize:Size:%d Post:%d Error illegal values, no buffer defined\n",bsze,post);
      return arg;
   }

   err = SLbSetBufferSize(hnd,mmsk,cmsk,&bsze,&post);
   if (err) fprintf(stderr,"SetBufferSize:%s\n",SLbErrToStr(hnd,err));

   printf("GetSetBufferSize:AllocatedBuffer:%d samples, PostTrgRead:%d PostTrgSet:%d\n",
	  bsze,post,PostSamps);

   return arg;
}

/* ========================================================== */

int StrtAcquisition(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
unsigned int mmsk, cmsk;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("StrtAcquisition:Start acquisition\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   err = SLbStrtAcquisition(hnd,mmsk,cmsk);
   if (err) fprintf(stderr,"StrtAcquisition:%s\n",SLbErrToStr(hnd,err));

   return arg;
}

/* ========================================================== */

int TrigAcquisition(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
unsigned int mmsk, cmsk;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("TrigAcquisition:Trigger acquisition\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   err = SLbTrigAcquisition(hnd,mmsk,cmsk);
   if (err) fprintf(stderr,"TrigAcquisition:%s\n",SLbErrToStr(hnd,err));

   return arg;
}

/* ========================================================== */

int StopAcquisition(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
unsigned int mmsk, cmsk;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("StopAcquisition:Stop acquisition\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   err = SLbStopAcquisition(hnd,mmsk,cmsk);
   if (err) fprintf(stderr,"StopAcquisition:%s\n",SLbErrToStr(hnd,err));

   return arg;
}

/* ========================================================== */

static int IMask = 0;

int Connect(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
unsigned int mmsk, cmsk, imsk;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("Connect:[<mask>]:\n");
      printf("   TRIGGER        = 0x1 //Trigger interrupt\n");
      printf("   ACQUISITION    = 0x2 //Data ready interrupt\n");
      printf("   HARDWARE ERROR = 0x4 // Hardware error interrupt\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   if ((++atoms)->type == Numeric) {
      imsk = atoms->val;
      err = SLbConnect(hnd,mmsk,cmsk,imsk);
      if (err) fprintf(stderr,"Connect:%s\n",SLbErrToStr(hnd,err));
      if (imsk) IMask |= imsk;
      else      IMask = 0;
   }
   if (IMask) printf("Connect:0x%X Done\n",IMask);
   else       printf("Connect:No connections\n");
   return arg;
}

/* ========================================================== */

int GetSetTimeout(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
int tmo;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetTimeout:[<milliseconds>] //0=No timeout\n");
      return arg;
   }

   hnd  = handles[curoph];

   if ((++atoms)->type == Numeric) {
      tmo = atoms->val;
      err = SLbSetTimeout(hnd,tmo);
      if (err) fprintf(stderr,"SetTimeout:%s\n",SLbErrToStr(hnd,err));
   }

   err = SLbGetTimeout(hnd,&tmo);
   if (err) fprintf(stderr,"GetTimeout:%s\n",SLbErrToStr(hnd,err));

   printf("GetSetTimeout:%d X 10 ms ",tmo);
   if (tmo == 0) printf("Wait forever");
   printf("\n");

   return arg;
}

/* ========================================================== */

int GetSetQueueFlag(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbQueueFlag qfl;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetQueueFlag:[<flag>] // 1-NoQueue 0-Queue\n");
      return arg;
   }

   hnd  = handles[curoph];

   if ((++atoms)->type == Numeric) {
      qfl = atoms->val;
      err = SLbSetQueueFlag(hnd,qfl);
      if (err) fprintf(stderr,"SetQueueFlag:%s\n",SLbErrToStr(hnd,err));
   }

   err = SLbGetQueueFlag(hnd,&qfl);
   if (err) fprintf(stderr,"GetQueueFlag:%s\n",SLbErrToStr(hnd,err));

   printf("GetSetQueueFlag:%d Queue:",(int) qfl);
   if (qfl == SLbQueueFlagON) printf("ON");
   else                       printf("OFF");
   printf("\n");

   return arg;
}

/* ========================================================== */

#define LOOPS 100

int WaitInterrupt(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbIntr intr;
int wi, loops;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("WaitInterrupt:[<mask>] //Wait for connected interrupt or timeout\n");
      return arg;
   }

   hnd  = handles[curoph];

   if ((++atoms)->type == Numeric) wi = atoms->val;
   else                            wi = -1;


   if ((IMask == 0) || ((IMask & wi) == 0)) {
      printf("WaitInterrupt:Not connected to that interrupt\n");
      return arg;
   }

   if (wi == -1) {
      err = SLbWait(hnd,&intr);
      if (err) fprintf(stderr,"Wait:%s\n",SLbErrToStr(hnd,err));
   } else {
      loops = 0; intr.Mask = 0;
      while ((wi & intr.Mask) == 0) {
	 err = SLbWait(hnd,&intr);
	 if (err) {
	    fprintf(stderr,"Wait:%s\n",SLbErrToStr(hnd,err));
	    break;
	 }
	 if (++loops > LOOPS) {
	    printf("WaitInterrupt:Mask not found after:%d loops\n",loops);
	    break;
	 }
      }
   }
   printf("WaitInterrupt:Msk:0x%X Mod:%d Chn:%d QSz:%d Msd:%d %s\n",
	   (int) intr.Mask,
	   (int) intr.Module,
	   (int) intr.Channel,
	   (int) intr.QueueSize,
	   (int) intr.Missed,
		 TimeToStr(&intr.Time));

   return arg;
}

/* ========================================================== */

int SimulateInterrupt(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbIntr intr;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("SimulateInterrupt:[<mask>]:\n");
      printf("   TRIGGER        = 0x1 //Trigger interrupt\n");
      printf("   ACQUISITION    = 0x2 //Data ready interrupt\n");
      printf("   HARDWARE ERROR = 0x4 // Hardware error interrupt\n");
      return arg;
   }

   hnd  = handles[curoph];

   if ((++atoms)->type == Numeric) {
      bzero((void *) &intr, sizeof(SLbIntr));

      intr.Mask = atoms->val;
      intr.Module = module;
      intr.Channel = channel;

      err = SLbSimulateInterrupt(hnd,&intr);
      if (err) fprintf(stderr,"SetQueueFlag:%s\n",SLbErrToStr(hnd,err));
   }
   printf("SimulateInterrupt:0x%X\n",intr.Mask);
   return arg;
}

/* ========================================================== */

#define TXTLEN 64

int ReadFile(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
char bname[TBLEN];
FILE *bfile = NULL;
int i, j, datum;
SLbDatumSize dsze;
SLbHandle hnd = NULL;
SLbErr err;

char  *cp, *ep, txt[TXTLEN];
short *sp;
int   *ip;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("ReadFile:[<filename>] // Default VD80SAMP\n");
      printf("   //Filename is appended with .<module>.<channel>\n");
      return arg;
   }

   if (Buffer.BSze == 0) {
      printf("ReadFile:No buffer has been configured, set the buffer up first\n");
      return arg;
   }

   hnd  = handles[curoph];

   if ((++atoms)->type == String)
      strncpy(bname,atoms->text,TBLEN);
   else
      strncpy(bname,"VD80SAMP",TBLEN);

   GetFile(bname);
   sprintf(bname,"%s.%02d.%02d",path,module,channel);
   bfile = fopen(bname,"r");
   if (bfile) {

      err = SLbGetDatumSize(hnd,&dsze);
      if (err) {
	 fprintf(stderr,"GetDatumSize:%s\n",SLbErrToStr(hnd,err));
	 printf("ReadFile:Assuming 16-bit datum size\n");
	 dsze = SLbDatumSize16;
      }

      Buffer.ASze = 0;
      Buffer.Tpos = 0;
      Buffer.Post = 0;

      while (fgets(txt,TXTLEN,bfile) != NULL) {

	 cp = txt;
	 i  = strtol(cp,&ep,0);
	 if (cp == ep) break;

	 cp = ep;
	 datum = strtol(cp,&ep,0);
	 if (cp == ep) break;

	 Buffer.ASze = i+1;
	 if (Buffer.ASze >= Buffer.BSze) break;
	 if (Buffer.Tpos) Buffer.Post++;
	 j = 0; while ((ep[j] != '\n') && (*ep != 0)) {
	    if (ep[j++] == '#')  Buffer.Tpos = Buffer.ASze;
	 }

	 if (dsze == SLbDatumSize08) {
	    cp = (char *) Buffer.Addr;
	    cp[i] = (char) datum;
	 } else if (dsze == SLbDatumSize16) {
	    sp = (short *) Buffer.Addr;
	    sp[i] = (short) datum;
	 } else {
	    ip = (int *) Buffer.Addr;
	    ip[i] = (int) datum;
	 }
      }
      fclose(bfile);

      printf("ReadFile:Read:%d samples to:%s\n",Buffer.ASze,bname);
      printf("ReadFile:Trig:%d\n",Buffer.Tpos);
      printf("ReadFile:Post:%d\n",Buffer.Post);

   } else {
      perror("fopen");
      printf("Error: Could not open the file: %s for read\n",bname);
   }
   return arg;
}

/* ========================================================== */

int WriteFile(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
char bname[TBLEN];
FILE *bfile = NULL;
int i, datum;
SLbDatumSize dsze;
SLbHandle hnd = NULL;
SLbErr err;

char  *cp;
short *sp;
int   *ip;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("WriteFile:[<filename>] // Default VD80SAMP\n");
      printf("   //Filename is appended with .<module>.<channel>\n");
      return arg;
   }

   hnd  = handles[curoph];

   if ((++atoms)->type == String)
      strncpy(bname,atoms->text,TBLEN);
   else
      strncpy(bname,"VD80SAMP",TBLEN);

   GetFile(bname);
   sprintf(bname,"%s.%02d.%02d",path,module,channel);
   bfile = fopen(bname,"w");
   if (bfile) {

      err = SLbGetDatumSize(hnd,&dsze);
      if (err) {
	 fprintf(stderr,"GetDatumSize:%s\n",SLbErrToStr(hnd,err));
	 printf("WriteFile:Assuming 16-bit datum size\n");
	 dsze = SLbDatumSize16;
      }

      for (i=0; i<Buffer.ASze; i++) {

	 if (dsze == SLbDatumSize08) {
	    cp = (char *) Buffer.Addr;
	    datum = (int) cp[i];
	 } else if (dsze == SLbDatumSize16) {
	    sp = (short *) Buffer.Addr;
	    datum = (int) sp[i];
	 } else {
	    ip = (int *) Buffer.Addr;
	    datum = (int) ip[i];
	 }
	 fprintf(bfile,"%3d %d",i,datum);

	 if (i==Buffer.Tpos) fprintf(bfile," %d  # Trigger",datum);
	 fprintf(bfile,"\n");
      }
      fclose(bfile);

      printf("WriteFile:Written:%d samples to:%s\n",Buffer.ASze,bname);
      printf("WriteFile:Trigger at:%d\n",Buffer.Tpos);
      printf("WriteFile:Post trigger:%d\n",Buffer.Post);

   } else {
      perror("fopen");
      printf("Error: Could not open the file: %s for write\n",bname);
   }

   return arg;
}

/* ========================================================== */

int PlotSamp(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
FILE *pltfile = NULL;
char bname[128];
char pltcmd[128];

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("PlotSamp:Plot buffer with GnuPlot\n");
      return arg;
   }

   if ((++atoms)->type == String)
      strncpy(bname,atoms->text,TBLEN);
   else
      strncpy(bname,"VD80SAMP",TBLEN);

   GetFile(bname);
   sprintf(bname,"%s.%02d.%02d",path,module,channel);

   GetFile("VD80.plot");
   pltfile = fopen(path,"w");
   if (pltfile) {
      fprintf(pltfile,"plot \"%s\" with line, \"%s\" using 1:3 with boxes\npause 10\n",bname,bname);
      fclose(pltfile);
      sprintf(pltcmd,"set term = xterm; gnuplot %s",path);
      printf("PlotSamp:%s\n",pltcmd);
      system(pltcmd);
   }

   return arg;
}

/* ========================================================== */

int ReadSamp(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbDatumSize dsze;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("ReadSamp:Read sample buffer from current module and channel\n");
      return arg;
   }

   hnd  = handles[curoph];

   err = SLbGetDatumSize(hnd,&dsze);
   if (err) fprintf(stderr,"GetDatumSize:%s\n",SLbErrToStr(hnd,err));
   else bzero(Buffer.Addr, (Buffer.BSze * dsze));

   Buffer.Post = PostSamps;
   err = SLbGetBuffer(hnd,module,channel,&Buffer);
   if (err) fprintf(stderr,"GetBuffer:%s\n",SLbErrToStr(hnd,err));

   printf("ReadSamp:Samples:%d\n",Buffer.ASze);
   printf("ReadSamp:Trigger:%d\n",Buffer.Tpos);
   printf("ReadSamp:PostSze:%d\n",Buffer.Post);
   printf("ReadSamp:PrTrgSr:%d\n",Buffer.Ptsr);

   return arg;
}

/* ========================================================== */

int PrintSamp(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
int i, datum;
SLbDatumSize dsze;
SLbHandle hnd = NULL;
SLbErr err;

char  *cp;
short *sp;
int   *ip;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("PrintSamp: //Print the current sample buffer\n");
      return arg;
   }

   err = SLbGetDatumSize(hnd,&dsze);
   if (err) {
      fprintf(stderr,"GetDatumSize:%s\n",SLbErrToStr(hnd,err));
      printf("WriteFile:Assuming 16-bit datum size\n");
      dsze = SLbDatumSize16;
   }

   printf("PrintSamp:Module :%d Channel:%d\n",module,channel);
   printf("PrintSamp:Samples:%d\n",Buffer.ASze);
   printf("PrintSamp:Trigger:%d\n",Buffer.Tpos);
   printf("PrintSamp:PostSze:%d\n",Buffer.Post);
   printf("\n");

   for (i=0; i<Buffer.ASze; i++) {

      if (dsze == SLbDatumSize08) {
	 cp = (char *) Buffer.Addr;
	 datum = (int) cp[i] & 0xFF;
      } else if (dsze == SLbDatumSize16) {
	 sp = (short *) Buffer.Addr;
	 datum = (int) sp[i] & 0xFFFF;
      } else {
	 ip = (int *) Buffer.Addr;
	 datum = (int) ip[i] & 0xFFFFFFFF;
      }
      printf("%3d:%6d ",i,datum);

      if (i==Buffer.Tpos) printf("<Trig> ");
      if (((i+1) % 6) == 0) printf("\n");
   }
   printf("\n   --- End ---\n");
   return arg;
}

/* ========================================================== */

int GetSetTrgConfig(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
SLbHandle hnd = NULL;
SLbErr err;
SLbTrigConfig ctrg;
unsigned int mmsk, cmsk;
int chng;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("GetSetTrgConfig:[<TrigDelay> <MinPreTrigSamples>]:\n");
      printf("   TrigDelay   :[Delay in sample clock ticks]\n");
      printf("   MinPreTrig  :[Min pre trigger samples]\n");
      return arg;
   }

   hnd  = handles[curoph];
   mmsk = 1 << (module -1);
   cmsk = 1 << (channel-1);

   err = SLbGetTriggerConfig(hnd,module,channel,&ctrg);
   if (err) {
      fprintf(stderr,"GetTriggerConfig:%s\n",SLbErrToStr(hnd,err));
      return arg;
   }

   chng = 0; ctrg.TrigDelay = 0; ctrg.MinPreTrig = 0;
   if ((++atoms)->type == Numeric) { ctrg.TrigDelay  = atoms->val; chng = 1; }
   if ((++atoms)->type == Numeric) { ctrg.MinPreTrig = atoms->val; chng = 1; }

   if (chng) {
      err = SLbSetTriggerConfig(hnd, mmsk, cmsk, &ctrg);
      if (err) {
	 fprintf(stderr,"SetTriggerConfig:%s\n",SLbErrToStr(hnd,err));
	 return arg;
      }
   }

   err = SLbGetTriggerConfig(hnd,module,channel,&ctrg);
   if (err) {
      fprintf(stderr,"GetTriggerConfig:%s\n",SLbErrToStr(hnd,err));
      return arg;
   }

   printf("TriggerConfig:module:%d TrigDelay :%d\n",module,ctrg.TrigDelay);
   printf("TriggerConfig:module:%d MinPreTrig:%d\n",module,ctrg.MinPreTrig);

   return arg;
}

/* ========================================================== */

int MsSleep(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
unsigned int usec;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("MsSleep:[<MilliSeconds>]\n");
      printf("   Default is 1 ms\n");
      return arg;
   }

   usec = 1000;
   if ((++atoms)->type == Numeric) usec = atoms->val*1000;

   usleep(usec);

   return arg;
}
