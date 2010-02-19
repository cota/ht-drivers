#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>

#include <skeluser_ioctl.h>
#include <extest.h>
#include <vd80Drvr.h>
#include <vd80hard.h>


void swab(const void *from, void *to, ssize_t n);

/* ========================================================== */

#define BUF_SHORTS 0x1000

Vd80SampleBuf sbuf;
short *buf = NULL;
unsigned int chan = 0;

/* ========================================================== */

char *defaultconfigpath = "testprog.config";

char *configpath = NULL;
char localconfigpath[128];  /* After a CD */

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

static void IErr(char *name, unsigned int *value) {

   if (value != NULL)
      printf("Vd80Drvr: [%02d] ioctl=%s arg=%d :Error\n",
	     (int) _DNFD, name, (int) *value);
   else
      printf("Fd80Drvr: [%02d] ioctl=%s :Error\n",
	     (int) _DNFD, name);
   perror("Vd80Drvr");
}

/* ========================================================== */

int GetSetClk(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, clk;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("1: External clock\n");
      printf("0: Internal clock\n");
      return(arg);
   }

   if ((++atoms)->type == Numeric) {
      clk = atoms->val;
      if (ioctl(_DNFD,Vd80IoctlSET_CLOCK,&clk) < 0) {
	 IErr("GET_CLOCK",&clk);
	 return arg;
      }
   }

   if (ioctl(_DNFD,Vd80IoctlGET_CLOCK,&clk) < 0) {
      IErr("GET_CLOCK",&clk);
      return arg;
   }

   printf("Clock:0x%X:",clk);
   if      (clk == 1) printf("EXTERNAL\n");
   else if (clk == 0) printf("INTERNAL\n");
   else               printf("Illegal clock value\n");

   return arg;
}

/* ========================================================== */

int GetSetClkDivMod(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, mde;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("1: SubSample\n");
      printf("0: Divide\n");
      return(arg);
   }

   if ((++atoms)->type == Numeric) {
      mde = atoms->val;
      if (ioctl(_DNFD,Vd80IoctlSET_CLOCK_DIVIDE_MODE,&mde) < 0) {
	 IErr("SET_CLOCK_DIVIDE_MODE",&mde);
	 return arg;
      }
   }

   if (ioctl(_DNFD,Vd80IoctlGET_CLOCK_DIVIDE_MODE,&mde) < 0) {
      IErr("GET_CLOCK_DIVIDE_MODE",&mde);
      return arg;
   }

   printf("DivideMode:0x%X:",mde);
   if      (mde == 1) printf("SUBSAMPLE\n");
   else if (mde == 0) printf("DIVIDE\n");
   else               printf("Illegal divide mode\n");

   return arg;
}

/* ========================================================== */

int GetSetClkDiv(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, div;
float f;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("16-Bit: Unsigned divisor for clock\n");
      printf("Divide by (value + 1)\n");
      return(arg);
   }

   if ((++atoms)->type == Numeric) {
      div = (atoms->val & 0x7FFF);
      if (ioctl(_DNFD,Vd80IoctlSET_CLOCK_DIVISOR,&div) < 0) {
	 IErr("SET_CLOCK_DIVISOR",&div);
	 return arg;
      }
   }

   div = 0;
   if (ioctl(_DNFD,Vd80IoctlGET_CLOCK_DIVISOR,&div) < 0) {
      IErr("GET_CLOCK_DIVISOR",&div);
      return arg;
   }

   f = 200000.00 / ((float) div + 1.0);
   printf("Divisor: 200KHz/(%d+1) = %fHz\n",div,f);

   return arg;
}

/* ========================================================== */

int GetSetClkEge(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, edg;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("1: Falling edge\n");
      printf("0: Rising edge\n");
      return(arg);
   }

   if ((++atoms)->type == Numeric) {
      edg = atoms->val;
      if (ioctl(_DNFD,Vd80IoctlSET_CLOCK_EDGE,&edg) < 0) {
	 IErr("SET_CLOCK_EDGE",&edg);
	 return arg;
      }
   }

   if (ioctl(_DNFD,Vd80IoctlGET_CLOCK_EDGE,&edg) < 0) {
      IErr("GET_CLOCK_EDGE",&edg);
      return arg;
   }

   printf("ClockEdge:0x%X:",edg);
   if      (edg == 1) printf("FALLING\n");
   else if (edg == 0) printf("RISING\n");
   else               printf("Illegal edge specification\n");

   return arg;
}

/* ========================================================== */

int GetSetClkTerm(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, trm;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("0: No termination\n");
      printf("1: 50-Ohms\n");
      return(arg);
   }

   if ((++atoms)->type == Numeric) {
      trm = atoms->val;
      if (ioctl(_DNFD,Vd80IoctlSET_CLOCK_TERMINATION,&trm) < 0) {
	 IErr("SET_CLOCK_TERMINATION",&trm);
	 return arg;
      }
   }

   if (ioctl(_DNFD,Vd80IoctlGET_CLOCK_TERMINATION,&trm) < 0) {
      IErr("GET_CLOCK_TERMINATION",&trm);
      return arg;
   }

   printf("ClockTermination:0x%X:",trm);
   if      (trm == 1) printf("50-OHMS\n");
   else if (trm == 0) printf("NONE\n");
   else               printf("Illegal clock termination\n");

   return arg;
}

/* ========================================================== */

int GetSetTrg(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, trg;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("1: External clock\n");
      printf("0: Internal clock\n");
      return(arg);
   }

   if ((++atoms)->type == Numeric) {
      trg = atoms->val;
      if (ioctl(_DNFD,Vd80IoctlSET_TRIGGER,&trg) < 0) {
	 IErr("GET_TRIGGER",&trg);
	 return arg;
      }
   }

   if (ioctl(_DNFD,Vd80IoctlGET_TRIGGER,&trg) < 0) {
      IErr("GET_TRIGGER",&trg);
      return arg;
   }

   printf("Trigger:0x%X:",trg);
   if      (trg == 1) printf("EXTERNAL\n");
   else if (trg == 0) printf("INTERNAL\n");
   else               printf("Illegal trigger value\n");

   return arg;
}

/* ========================================================== */

int GetSetTrgEge(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, edg;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("1: Falling edge\n");
      printf("0: Rising edge\n");
      return(arg);
   }

   if ((++atoms)->type == Numeric) {
      edg = atoms->val;
      if (ioctl(_DNFD,Vd80IoctlSET_TRIGGER_EDGE,&edg) < 0) {
	 IErr("SET_TRIGGER_EDGE",&edg);
	 return arg;
      }
   }

   if (ioctl(_DNFD,Vd80IoctlGET_TRIGGER_EDGE,&edg) < 0) {
      IErr("GET_TRIGGER_EDGE",&edg);
      return arg;
   }

   printf("TriggerEdge:0x%X:",edg);
   if      (edg == 1) printf("FALLING\n");
   else if (edg == 0) printf("RISING\n");
   else               printf("Illegal edge specification\n");

   return arg;
}

/* ========================================================== */

int GetSetTrgTerm(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, trm;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("0: No termination\n");
      printf("1: 50-Ohms\n");
      return(arg);
   }

   if ((++atoms)->type == Numeric) {
      trm = atoms->val;
      if (ioctl(_DNFD,Vd80IoctlSET_TRIGGER_TERMINATION,&trm) < 0) {
	 IErr("SET_TRIGGER_TERMINATION",&trm);
	 return arg;
      }
   }

   if (ioctl(_DNFD,Vd80IoctlGET_TRIGGER_TERMINATION,&trm) < 0) {
      IErr("GET_TRIGGER_TERMINATION",&trm);
      return arg;
   }

   printf("TriggerTermination:0x%X:",trm);
   if      (trm == 1) printf("50-OHMS\n");
   else if (trm == 0) printf("NONE\n");
   else               printf("Illegal trigger termination\n");

   return arg;
}

/* ========================================================== */

int GetSetPostSamples(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, psmp;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("Set/Get the number of post trigger samples\n");
      return(arg);
   }

   if ((++atoms)->type == Numeric) {
      psmp = atoms->val;
      if (ioctl(_DNFD,Vd80IoctlSET_POSTSAMPLES,&psmp) < 0) {
	 IErr("SET_POSTSAMPLES",&psmp);
	 return arg;
      }
   }

   if (ioctl(_DNFD,Vd80IoctlGET_POSTSAMPLES,&psmp) < 0) {
      IErr("GET_POSTSAMPLES",&psmp);
      return arg;
   }

   printf("PostSamples:%d\n",psmp);

   return arg;
}

/* ========================================================== */

int GetState(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, val, ste;

   arg = cmddint->pa + 1;

   if (ioctl(_DNFD,Vd80IoctlGET_STATE,&val) < 0) {
      IErr("GET_TRIGGER_TERMINATION",&val);
      return arg;
   }

   printf("State:0x%X:",val);
   ste = VD80_STATE_MASK & val;
   if      (ste == VD80_STATE_IDLE)     printf("IDLE");
   else if (ste == VD80_STATE_PRETRIG)  printf("PRE-TRIGGER");
   else if (ste == VD80_STATE_POSTTRIG) printf("POST-TRIGGER");
   else                                 printf("Bad state value");
   printf("\n");

   if (val & VD80_MEMOUTRDY)     printf("Data available for reading\n");
   if (val & VD80_TRIGINTEVENT)  printf("External trigger interrupt\n");
   if (val & VD80_SHOTINTEVENT)  printf("Shot ready interrupt\n");
   if (val & VD80_SMPLINTEVENT)  printf("SubSample interrupt\n");
   if (val & VD80_IOERRINTEVENT) printf("IOError interrupt\n");
   if (val & VD80_CALINTEVENT)   printf("Calibration interrupt\n");
   if (val & VD80_IOERRCLK)      printf("Clock short circuit interrupt\n");
   if (val & VD80_IOERRTRIG)     printf("Trigger short circuit interrupt\n");

   return arg;
}

/* ========================================================== */

int SetCommand(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, cmd;
char *cp;

   arg = cmddint->pa + 1;

   if ((++atoms)->type == Numeric) {
      cmd = atoms->val;

      if ((cmd & VD80_COMMAND_MASK) != cmd) {
	 printf ("Illegal command:0x%X (%d)\n",cmd,cmd);
	 return arg;
      }

      printf("Command:0x%X:",cmd);
      if      (cmd == VD80_COMMAND_SINGLE  ) cp = "Store-Single-Sample";
      else if (cmd == VD80_COMMAND_TRIG    ) cp = "Generate-Internal-Trigger";

      else if (cmd == VD80_COMMAND_READ    ) {
	 printf("Channel:%d:",chan +1);
	 cp = "Read-Data";
	 cmd |= (VD80_OPERANT_MASK & (chan << VD80_OPERANT_SHIFT));
      }

      else if (cmd == VD80_COMMAND_SUBSTOP ) cp = "Stop-Sub-Sampling";
      else if (cmd == VD80_COMMAND_SUBSTART) cp = "Start-Sub-Sampling";
      else if (cmd == VD80_COMMAND_START   ) cp = "Start-Sampling";
      else if (cmd == VD80_COMMAND_CLEAR   ) cp = "Clear-Pre-Trigger";
      else if (cmd == VD80_COMMAND_STOP    ) cp = "Stop-Sampling";
      else                                   cp = "No-Operation";
      printf("%s\n",cp);

      if (ioctl(_DNFD,Vd80IoctlSET_COMMAND,&cmd) < 0) {
	 IErr("SET_COMMAND",&cmd);
	 return arg;
      }
   } else printf("No command\n");

   return arg;
}

/* ========================================================== */

int GetAdc(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, adc;

   arg = cmddint->pa + 1;
   adc = chan +1;
   if (ioctl(_DNFD,Vd80IoctlREAD_ADC,&adc) < 0) {
      IErr("READ_ADC",&adc);
      return arg;
   }

   printf("ADC Value:0x%X (%d)\n",adc, adc);

   return arg;
}

/* ========================================================== */

int GetSetChan(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg, ch;

   arg = cmddint->pa + 1;

   if ((++atoms)->type == Numeric) {
      ch = atoms->val;
      if ((ch < 1) || (ch > VD80_CHANNELS)) {
	 printf("Illegal channel number:%d\n",ch);
      } else {
	 chan = ch -1;
      }
   }
   printf("Channel:%d\n",chan +1);
   return arg;
}

/* ========================================================== */

int NextChan(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;

   arg = cmddint->pa + 1;

   chan++;
   if (chan >= VD80_CHANNELS) chan = 0;
   printf("Next Channel:%d\n",chan +1);

   return arg;
}

/* ========================================================== */

int ReadFile(struct cmd_desc *cmddint, struct atom *atoms) { return cmddint->pa + 1; }

/* ========================================================== */

int WriteFile(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int i, arg;
char sbfname[128];
FILE *sbffile = NULL;

   arg = cmddint->pa + 1;

   GetFile("VD80SAMP");
   sprintf(sbfname,"%s.%02d",path,chan+1);
   sbffile = fopen(sbfname,"w");
   if (sbffile) {
      for (i=0; i<sbuf.Samples; i++) {
	 fprintf(sbffile,"%4d %d",i,sbuf.SampleBuf[i]);
	 if (i==sbuf.TrigPosition) fprintf(sbffile,"   # Trigger");
	 fprintf(sbffile,"\n");
      }
      fclose(sbffile);
      printf("Written:%d samples to:%s\n",sbuf.Samples,sbfname);
   } else {
      perror("fopen");
      printf("Error: Could not open the file: %s for write\n",sbfname);
   }
   return arg;
}

/* ========================================================== */

int PlotSamp(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
FILE *pltfile = NULL;
char pltname[128];
char sbfname[128];
char pltcmd[128];

   arg = cmddint->pa + 1;

   GetFile("VD80.plot");
   strcpy(pltname,path);
   pltfile = fopen(path,"w");
   if (pltfile) {

      GetFile("VD80SAMP");
      sprintf(sbfname,"%s.%02d",path,chan+1);
      fprintf(pltfile,"plot \"%s\" with line\npause 20\n",sbfname);
      fclose(pltfile);
      sprintf(pltcmd,"set term = xterm; gnuplot %s",pltname);
      system(pltcmd);
   }
   return arg;
}

/* ========================================================== */

int ReadSamp(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int arg;
int bsz, tps;

   arg = cmddint->pa + 1;

   if (atoms == (struct atom *) VERBOSE_HELP) {
      printf("ReadCount: 0..%d, TriggerPosition: 0..%d\n",BUF_SHORTS,BUF_SHORTS);
      return(arg);
   }

   if (buf == NULL) {

      /* Aligne buffer on 4096 page boundary */

#ifdef __linux__
      if (posix_memalign((void **) &buf,4096,BUF_SHORTS*sizeof(short))) {
	 perror("Cant allocate buffer");
	 return arg;
      }
#else
      buf = (short *) malloc(BUF_SHORTS*sizeof(short));
      if (buf == NULL) {
	 perror("Cant allocate buffer");
	 return arg;
      }
#endif
   }
   bzero((void *) buf,BUF_SHORTS*sizeof(short));

   bsz = BUF_SHORTS;
   if ((++atoms)->type == Numeric) {
      bsz = atoms->val;
      if ((bsz < 0) || (bsz > BUF_SHORTS)) {
	 printf("Illegal ReadCount:%d#[0..%d]\n",bsz,BUF_SHORTS);
      }
   }

   tps = 0;
   if ((++atoms)->type == Numeric) {
      tps = atoms->val;
      if ((tps < 0) || (tps > bsz)) {
	 printf("TriggerPosition not in ReadCount:%d#[0..%d]\n",tps,bsz);
      }
   }

   sbuf.SampleBuf      = buf;
   sbuf.BufSizeSamples = bsz;
   sbuf.Channel        = chan + 1;
   sbuf.TrigPosition   = tps;
   sbuf.Samples        = 0;

   if (ioctl(_DNFD,Vd80IoctlREAD_SAMPLE,&sbuf) < 0) {
      IErr("READ_SAMPLE",(unsigned int *)&sbuf.Samples);
      return arg;
   }

//   swab(buf,buf,sbuf.Samples*2);

   printf("Read back:%d Samples\n",sbuf.Samples);
   printf("Read back:%d Trigger\n",sbuf.TrigPosition);
   return arg;
}

/* ========================================================== */

int PrintSamp(struct cmd_desc *cmddint, struct atom *atoms) {

unsigned int i, val, arg;

   arg = cmddint->pa + 1;

   printf("Sample buffer for channel:%02d\n",chan +1);

   for (i=0; i<sbuf.Samples; i++) {
      if (i%8 == 0) printf("\n%04d: ",i);
      val = buf[i];
      if (i==sbuf.TrigPosition) printf("Trig =>>");
      printf("%5d ",val);
   }
   printf("\n%4d: EOF\n",i);
   return arg;
}
