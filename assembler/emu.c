/**************************************************************************/
/* Geniric Hardware Emulator                                              */
/* Julian Lewis AB/CO/HT                                                  */
/* Version Monday 21st August 2006                                        */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <asm.h>
#include <asmP.h>
#include <opcodes.c>

#define LN 128
#define HEXF "Mtt.hex"

/**************************************************************************/
/* Get a file configuration path                                          */
/**************************************************************************/

#ifdef __linux__
static char *defaultconfigpath = "/dsrc/drivers/mtt/test/mttconfig.linux";
#else
static char *defaultconfigpath = "/dsc/data/mtt/Mtt.conf";
#endif

static char *configpath = NULL;

char *GetFile(char *name) {
FILE *gpath = NULL;
char txt[LN];
int i, j;

static char path[LN];

   if (configpath) {
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = NULL;
      }
   }

   if (configpath == NULL) {
      configpath = defaultconfigpath;
      gpath = fopen(configpath,"r");
      if (gpath == NULL) {
	 configpath = NULL;
	 sprintf(path,"./%s",name);
	 return path;
      }
   }

   bzero((void *) path,LN);

   while (1) {
      if (fgets(txt,LN,gpath) == NULL) break;
      if (strncmp(name,txt,strlen(name)) == 0) {
	 for (i=strlen(name); i<strlen(txt); i++) {
	    if (txt[i] != ' ') break;
	 }
	 j= 0;
	 while ((txt[i] != ' ') && (txt[i] != 0) && (txt[i] != '\n')) {
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

/**************************************************************************/
/* String to upper case                                                   */
/**************************************************************************/

void StrToUpper(inp,out)
char *inp;
char *out; {

long        i;
AtomType   at;
char        c;

   for (i=0; i<strlen(inp); i++) {
      c = inp[i];
      at = atomhash[(int) (c)];
      if ((at == Alpha) && (c >= 'a')) c -= ' ';
      out[i] = c;
   }
   out[i] = 0;
}

/**************************************************************************/
/* Tab                                                                    */
/**************************************************************************/

char *TabTo(char *cp, int pos) {
unsigned int i, j;
size_t l;
static char tab[128];

   if (cp) {
      l = strlen(cp);
      for (i=l,j=0; i<pos; i++,j++) tab[j]=' ';
      tab[j]=0;
   }
   return tab;
}

/**************************************************************************/
/* Get file name extension                                                */
/**************************************************************************/

typedef enum {ASM,OBJ,INVALID} FileType;

static char *extNames[] = {".ass",".obj"};

FileType GetFileType(char *path) {

int i, j;
char c, *cp, ext[5];

   for (i=strlen(path); i>=0; i--) {
      cp = &(path[i]); c = *cp;
      if (c == '.') {
	 strncpy(ext,cp,4); ext[4] = 0;
	 for (j=0; j<INVALID; j++) {
	    if (strcmp(ext,extNames[j]) == 0) break;
	 }
	 return (FileType) j;
      }
   }
   return INVALID;
}

/**************************************************************************/
/* Read object code binary from file                                      */
/**************************************************************************/

int ReadObject(FILE *objFile, ProgramBuf *code) {

Instruction *inst;
int i;
char ln[LN], *cp, *ep;

   if (fgets(ln,LN,objFile)) {
      cp = ln;
      code->LoadAddress      = strtoul(cp,&ep,0);
      cp = ep;
      code->InstructionCount = strtoul(cp,&ep,0);
   }
   inst = (Instruction *) malloc(sizeof(Instruction) * code->InstructionCount);
   code->Program = inst;

   for (i=0; i<code->InstructionCount; i++) {
      inst = &(code->Program[i]);
      bzero((void *) ln, LN);
      if (fgets(ln,LN,objFile)) {
	 cp = ln;
	 inst->Number = strtoul(cp,&ep,0);
	 cp = ep;
	 inst->Src1   = strtoul(cp,&ep,0);
	 cp = ep;
	 inst->Src2   = strtoul(cp,&ep,0);
	 cp = ep;
	 inst->Dest   = strtoul(cp,&ep,0);
	 cp = ep;
	 inst->Crc    = strtoul(cp,&ep,0);
      }
   }
   return 0;
}

/**************************************************************************/
/* Dump object in hex for VHDL simulation                                 */
/**************************************************************************/

static char hexf[LN];

void HexDump(ProgramBuf *code) {

int i;
FILE *fp;
Instruction *inst;
unsigned long hx;

   strcpy(hexf,GetFile(HEXF));
   fp = fopen(hexf,"w");
   if (fp) {
      for (i=0; i<code->InstructionCount; i++) {
	 inst = &(code->Program[i]);
	 hx = (inst->Dest   << 24)
	    | (inst->Src2   << 16)
	    | (inst->Number << 8 )
	    | (inst->Crc         );
	 fprintf(fp,"X\"%08X\"\n",(int) hx);
	 fprintf(fp,"X\"%08X\"\n",(int) inst->Src1);
      }
      fclose(fp);
      printf("Written hex dump: %s OK\n",hexf);
   } else {
      fprintf(stderr,"Can't open: %s for write\n",hexf);
      perror(hexf);
   }
}

/**************************************************************************/
/* String to Register                                                     */
/**************************************************************************/

unsigned long StringToReg(char *name) {

int i;
unsigned long rn, en, st, of;
char upr[MAX_REGISTER_STRING_LENGTH +1],
     *cp,
     *ep;

   cp = name;
   if (cp) {
      if (strlen(cp) <= MAX_REGISTER_STRING_LENGTH) {
	 StrToUpper(cp,upr);
	 for (i=0; i<REGNAMES; i++) {
	    if (strncmp(upr,Regs[i].Name,strlen(Regs[i].Name)) == 0) {

	       st = Regs[i].Start;
	       en = Regs[i].End;
	       of = Regs[i].Offset;

	       if (st < en) {
		  if (strlen(Regs[i].Name) < strlen(name)) {
		     cp = name + strlen(Regs[i].Name);
		     rn = strtoul(cp,&ep,0);
		     if (cp != ep) {
			if ((rn >= of)
			&&  (rn <= en - st + of)) {
			   return st + rn - of;
			}
		     }
		  }
	       } else return st;
	    }
	 }
      }
   }
   return 0;
}

/**************************************************************************/
/* Register To String                                                     */
/**************************************************************************/

char *RegToString(int regnum) {
static char name[MAX_REGISTER_STRING_LENGTH];
int rn, i;
RegisterDsc *reg;

   for (i=0; i<REGNAMES; i++) {
      reg = &(Regs[i]);
      if ((regnum >= reg->Start) && (regnum <= reg->End)) {
	 rn = (regnum - reg->Start) + reg->Offset;
	 if (reg->Start < reg->End) sprintf(name,"%s%1d",reg->Name,rn);
	 else                       sprintf(name,"%s"   ,reg->Name);
	 return name;
      }
   }
   return "???";
}

/**************************************************************************/
/* Convert an instruction to a string                                     */
/**************************************************************************/

char *InstToStr(Instruction *inst) {

int j, fnd;
char tmp[LN];

OpCode *opcd;
static char ln[LN];

   bzero((void *) ln, LN);

   fnd = 0;
   for (j=0; j<OPCODES; j++) {
      opcd = &(InstructionSet[j]);
      if (opcd->Number == inst->Number) {
	 fnd = 1;
	 break;
      }
   }

   if (fnd) {
      sprintf(ln,"%s",opcd->Name);
      strcat(ln,TabTo(ln,8));

      bzero((void *) tmp, LN);
      switch (opcd->Src1) {
	 case AtNothing:
	 break;

	 case AtLiteral: sprintf(tmp,"0x%X"  ,(int) inst->Src1);
	 break;

	 case AtAddress: sprintf(tmp,"0x%X" ,(int) inst->Src1);
	 break;

	 case AtRegister: sprintf(tmp,"%s",RegToString(inst->Src1));
	 break;

	 default: sprintf(tmp,"??? ");
      }
      strcat(ln,tmp);
      strcat(ln,TabTo(ln,16));

      bzero((void *) tmp, LN);
      switch (opcd->Src2) {
	 case AtNothing:
	 break;

	 case AtLiteral: sprintf(tmp,"0x%X"  ,(int) inst->Src2);
	 break;

	 case AtAddress: sprintf(tmp,"0x%X" ,(int) inst->Src2);
	 break;

	 case AtRegister: sprintf(tmp,"%s",RegToString(inst->Src2));
	 break;

	 default: sprintf(tmp,"??? ");
      }
      strcat(ln,tmp);
      strcat(ln,TabTo(ln,24));

      bzero((void *) tmp, LN);
      switch (opcd->Dest) {
	 case AtNothing:
	 break;

	 case AtLiteral: sprintf(tmp,"0x%X"  ,(int) inst->Dest);
	 break;

	 case AtAddress: sprintf(tmp,"0x%X" ,(int) inst->Dest);
	 break;

	 case AtRegister: sprintf(tmp,"%s",RegToString(inst->Dest));
	 break;

	 default: sprintf(tmp,"???");
      }
      strcat(ln,tmp);
      strcat(ln,TabTo(ln,32));
   }
   return ln;
}

/**************************************************************************/
/* Registers                                                              */
/**************************************************************************/

#define GREGS 224
#define LREGS 32
#define REGS (GREGS + LREGS)

static int Regm[REGS];
static int PSwd;
static int Pc;

#define PsEQ 0x1 /* Last instruction result was Zero */
#define PsLT 0x2 /* Last instruction result was Less Than Zero */
#define PsGT 0x4 /* Last instruction result was Greater Than Zero */
#define PsCR 0x8 /* Last instruction result Set the Carry */

static int waiting = 0;
static int waitr = 0;
static int regwa = 0;

/**************************************************************************/
/* Show differences                                                       */
/**************************************************************************/

static char Dfs[LN];
static int regm[REGS];

char *ShowDiffs() {
int i;
char txt[LN];

   bzero(Dfs,LN);
   bzero(txt,LN);

   for (i=0; i<REGS; i++) {
      if (regm[i] != Regm[i]) {
	 sprintf(Dfs,"%s:0x%X=>0x%X",
		 RegToString(i),
		 (int) regm[i],
		 (int) Regm[i]);
	 break;
      }
   }
   for (i=0; i<REGS; i++) {
      regm[i] = Regm[i];
   }

   if (waiting) {
      sprintf(txt,"Wait[%s/0x%X]",
	      RegToString(regwa),
	      waitr);
      strcat(Dfs,txt);
   }

   return Dfs;
}

/**************************************************************************/
/* Set processor status word, ignore carry                                */
/**************************************************************************/

void SetPSwd(int val) {

   PSwd = 0;
   if (val == 0) PSwd |= PsEQ;
   if (val >  0) PSwd |= PsGT;
   if (val <  0) PSwd |= PsLT;
   return;
}

/**************************************************************************/
/* Execute an instruction                                                 */
/**************************************************************************/

void Execute(Instruction *inst) {

int irg;

   switch (inst->Number) {

      case HALT:
      break;

      case NOOP:
	 Pc++;
      break;

      case INT:
	 Pc++;
      break;

      case WEQV:
	 if (waiting == 0) {
	    waiting = 1;
	    waitr = inst->Src1;
	    regwa = inst->Src2;
	 }
	 if (Regm[inst->Src2] == inst->Src1) {
	    waiting = 0;
	    Pc++;
	 }
	 ShowDiffs();
      break;

      case WEQR:
	 if (waiting == 0) {
	    waiting = 1;
	    waitr = Regm[inst->Src1];
	    regwa =      inst->Src2;
	 }
	 if (Regm[inst->Src2] == Regm[inst->Src1]) {
	    waiting = 0;
	    Pc++;
	 }
	 ShowDiffs();
      break;

      case WRLV:
	 if (waiting == 0) {
	    waiting = 1;
	    waitr = Regm[inst->Src2] + inst->Src1;
	    regwa =      inst->Src2;
	 }
	 if (Regm[inst->Src2] >= waitr) {
	    waiting = 0;
	    Pc++;
	 }
	 ShowDiffs();
      break;

      case WRLR:
	 if (waiting == 0) {
	    waiting = 1;
	    waitr = Regm[inst->Src2] + Regm[inst->Src1];
	    regwa =      inst->Src2;
	 }
	 if (Regm[inst->Src2] >= waitr) {
	    waiting = 0;
	    Pc++;
	 }
	 ShowDiffs();
      break;

      case WORV:
	 if (waiting == 0) {
	    waiting = 1;
	    waitr = inst->Src1;
	    regwa = inst->Src2;
	 }
	 if (Regm[inst->Src2] & inst->Src1) {
	    waiting = 0;
	    Pc++;
	 }
	 ShowDiffs();
      break;

      case WORR:
	 if (waiting == 0) {
	    waiting = 1;
	    waitr = Regm[inst->Src1];
	    regwa = inst->Src2;
	 }
	 if (Regm[inst->Src2] & Regm[inst->Src1]) {
	    waiting = 0;
	    Pc++;
	 }
	 ShowDiffs();
      break;

      case MOVV:
	 Regm[inst->Dest] = inst->Src1;
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case MOVR:
	 Regm[inst->Dest] = Regm[inst->Src1];
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case MOVIR:
	 irg = Regm[GREGS];
	 if ((irg >= 0) && (irg < REGS)) {
	    Regm[inst->Dest] = Regm[irg];
	    SetPSwd(Regm[inst->Dest]);
	    ShowDiffs();
	 }
	 Pc++;
      break;

      case MOVRI:
	 irg = Regm[GREGS];
	 if ((irg >= 0) && (irg < REGS)) {
	    Regm[irg] = Regm[inst->Dest];
	    SetPSwd(Regm[irg]);
	    ShowDiffs();
	 }
	 Pc++;
      break;

      case ADDR:
	 Regm[inst->Dest] = Regm[inst->Src1] + Regm[inst->Src2];
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case SUBR:
	 Regm[inst->Dest] = Regm[inst->Src2] - Regm[inst->Src1];
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case LORR:
	 Regm[inst->Dest] = Regm[inst->Src2] | Regm[inst->Src1];
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case ANDR:
	 Regm[inst->Dest] = Regm[inst->Src2] & Regm[inst->Src1];
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case XORR:
	 Regm[inst->Dest] = Regm[inst->Src2] ^ Regm[inst->Src1];
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case LSR:
	 Regm[inst->Dest] = Regm[inst->Src2] << Regm[inst->Src1];
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case RSR:
	 Regm[inst->Dest] = Regm[inst->Src2] >> Regm[inst->Src1];
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case ADDV:
	 Regm[inst->Dest] = Regm[inst->Src2] + inst->Src1;
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case SUBV:
	 Regm[inst->Dest] = Regm[inst->Src2] - inst->Src1;
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case LORV:
	 Regm[inst->Dest] = Regm[inst->Src2] | inst->Src1;
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case ANDV:
	 Regm[inst->Dest] = Regm[inst->Src2] & inst->Src1;
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case XORV:
	 Regm[inst->Dest] = Regm[inst->Src2] ^ inst->Src1;
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case LSV:
	 Regm[inst->Dest] = Regm[inst->Src2] << inst->Src1;
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case RSV:
	 Regm[inst->Dest] = Regm[inst->Src2] >> inst->Src1;
	 SetPSwd(Regm[inst->Dest]);
	 ShowDiffs();
	 Pc++;
      break;

      case WDOG:
	 inst->Number++;
	 Pc++;
      break;

      case JMP:
	 Pc = inst->Src1;
      break;

      case BEQ:
	 if (PSwd & PsEQ) Pc = inst->Src1;
	 else Pc++;
      break;

      case BNE:
	 if ((PSwd & PsEQ) == 0) Pc = inst->Src1;
	 else Pc++;
      break;

      case BLT:
	 if (PSwd & PsLT) Pc = inst->Src1;
	 else Pc++;
      break;

      case BGT:
	 if (PSwd & PsGT) Pc = inst->Src1;
	 else Pc++;
      break;

      case BLE:
	 if ((PSwd & PsEQ) || (PSwd & PsLT)) Pc = inst->Src1;
	 else Pc++;
      break;

      case BGE:
	 if ((PSwd & PsEQ) || (PSwd & PsGT)) Pc = inst->Src1;
	 else Pc++;
      break;

      case BCR:
	 if (PSwd & PsCR) Pc = inst->Src1;
	 else Pc++;
      break;
   }
}

/**************************************************************************/
/* Processor Status to String                                             */
/**************************************************************************/

char *PsToString(int ps) {
static char res[16];

   bzero(res,16);

   if (PsEQ & ps) strcat(res,"EQ.");
   else           strcat(res,"eq.");

   if (PsLT & ps) strcat(res,"LT.");
   else           strcat(res,"lt.");

   if (PsGT & ps) strcat(res,"GT.");
   else           strcat(res,"gt.");

   if (PsCR & ps) strcat(res,"CR");
   else           strcat(res,"cr");

   return res;
}

/**************************************************************************/
/* Edit registers                                                         */
/**************************************************************************/

void EditRegs(int reg, int min, int max) {
char ln[LN];
char *cp, *ep, c;
unsigned long trv, val;

   while (1) {
      fprintf(stdout,"%03d:%s 0x%x: ",reg,RegToString(reg),(int) Regm[reg]);
      fflush(stdout);

      fgets(ln,LN,stdin); cp = ln; c = *cp;

      if (c == '.') return;

      if (c == '/') {
	 ep = ++cp;
	 trv = strtoul(cp,&ep,0) + min;
	 if (cp != ep) {
	    if ((trv < max) && (trv >= min)) reg = trv;
	    continue;
	 }
	 if (strlen(ln) > 1) {
	    trv = StringToReg(cp);
	    if ((trv < max) && (trv >= min)) reg = trv;
	    continue;
	 }
      }

      if (c == '\n') {
	 reg++;
	 if (reg >= max) {
	    fprintf(stdout,"\n");
	    reg = min;
	 }
	 continue;
      }

      ep = cp;
      val = strtoul(cp,&ep,0);
      if (cp != ep) {
	 regm[reg] = val;
	 Regm[reg] = val;
	 continue;
      }
   }
}

/**************************************************************************/
/* Edit Global reg                                                        */
/**************************************************************************/

void EditGlobalReg(int reg) {

   if (reg >= GREGS) return;
   EditRegs(reg,0,GREGS);
}

/**************************************************************************/
/* Edit Local reg                                                         */
/**************************************************************************/

void EditLocalReg(int reg) {

   if ((reg >= REGS) || (reg < GREGS)) return;
   EditRegs(reg,GREGS,REGS);
}

/**************************************************************************/
/* Emulate program                                                        */
/*                                                                        */
/* <crtn>           Next                                                  */
/* x[<count>]<crtn> Execute count instructions (Default = 1)              */
/* /<address><crtn> Goto program instruction at address                   */
/* g[<reg>]<crtn>   Open global reg (Default = Last)                      */
/* l[<reg>]<crtn>   Open local reg  (Default = Last)                      */
/* p[<PC>]<crtn>    Change PC                                             */
/* <value><crtn>    Change register contents to value                     */
/* .<crtn>          Exit command                                          */
/* ?                Help                                                  */
/*                                                                        */
/**************************************************************************/

void Emulate(ProgramBuf *code) {

Instruction *inst;
int i, j, pc, fnd, cnt, reg, exc;
OpCode *opcd;
char ln[LN];

char *cp, *ep, c;
AtomType at;

   Pc = code->LoadAddress;
   i = 0;
   exc = 0;

   while (1) {
      inst = &(code->Program[i]);

      fnd = 0;
      for (j=0; j<OPCODES; j++) {
	 opcd = &(InstructionSet[j]);
	 if (opcd->Number == inst->Number) {
	    fnd = 1;
	    break;
	 }
      }

      if (exc == 0) fprintf(stdout,"LsAd:%04X: %s ",
			    (int) i + (int) code->LoadAddress,
			    InstToStr(inst));

      fflush(stdout);
      fgets(ln,LN,stdin); cp = ln; c = *cp;

      at = atomhash[(int) c];
      switch (at) {

	 case Bit:
	    if (exc == 0) {
	       fprintf(stdout,"Exit emulator\n");
	       return;
	    } else {
	       exc = 0;
	       fprintf(stdout,"MODE:LsAd (List Address mode)\n");
	    }
	 break;

	 case Seperator:
	    if (c == '\n') {
	       if (exc == 0) {
		  i++;
		  if (i >= code->InstructionCount) {
		     i = 0;
		     printf("\n");
		  }
		  inst = &(code->Program[i]);
	       } else {
		  pc = Pc - code->LoadAddress;
		  inst = &(code->Program[pc]);
		  fprintf(stdout,"ExPc:%04X:",(int) Pc);
		  Execute(inst);
		  fprintf(stdout,"%s ",InstToStr(inst));
		  fprintf(stdout,"%s ",PsToString(PSwd));
		  fprintf(stdout,"%s ",Dfs);
		  *Dfs = '\0';
	       }
	    }
	 break;

	 case Alpha:
	    if (c == 'x') {
	       if (exc == 0) {
		  exc = 1;
		  printf("MODE:ExPc (Execute PC mode)");
	       }
	       cp++; ep = cp;
	       cnt = strtoul(cp,&ep,0);
	       if (cnt <= 0) cnt = 1;
	       fprintf(stdout,"\n");
	       for (j=0; j<cnt; j++) {
		  pc = Pc - code->LoadAddress;
		  inst = &(code->Program[pc]);
		  if (j) fprintf(stdout,"\n");
		  fprintf(stdout,"ExPc:%04X:",(int) Pc);
		  Execute(inst);
		  fprintf(stdout,"%s ",InstToStr(inst));
		  fprintf(stdout,"%s ",PsToString(PSwd));
		  fprintf(stdout,"%s ",Dfs);
		  *Dfs = '\0';
	       }

	    } else if (c == 'p') {
	       cp++; ep = cp;
	       pc = strtoul(cp,&ep,0);
	       if ((pc >= code->LoadAddress)
	       &&  (pc <  code->LoadAddress + code->InstructionCount)) {
		  waitr = 0;
		  Pc = pc;
		  pc = Pc - code->LoadAddress;
		  inst = &(code->Program[pc]);
		  fprintf(stdout,"\nPC:%06X: %s",(int) Pc,InstToStr(inst));
	       }
	       fprintf(stdout,"\n\n");

	    } else if (c == 'g') {
	       cp++; ep = cp;
	       reg = strtoul(cp,&ep,0);
	       EditGlobalReg(reg);

	    } else if (c == 'l') {
	       cp++; ep = cp;
	       reg = strtoul(cp,&ep,0) + GREGS;
	       EditLocalReg(reg);
	    }
	 break;


	 case Operator:
	    if (c == '/') {
	       cp++; ep = cp;
	       pc = strtoul(cp,&ep,0);
	       if (cp != ep) {
		  if ((pc >= code->LoadAddress)
		  &&  (pc <  code->LoadAddress + code->InstructionCount)) {
		     i = pc - code->LoadAddress;
		  }
	       }

	    } else if (c == '?') {
	       fprintf(stdout,
		       "<crtn>           Next Address/Instruction/Register\n"
		       "x[<count>]<crtn> Goto Execute PC Mode; Run count instructions (Default = 1)\n"
		       "/<address><crtn> Goto program instruction at address\n"
		       "p[<PC>]<crtn>    Change PC\n"
		       "g[<reg>]<crtn>   Open global reg (Default = Last)\n"
		       "l[<reg>]<crtn>   Open local reg  (Default = Last)\n"
		       "<value><crtn>    Change register contents to value\n"
		       ".<crtn>          Back to List Address Mode / Exit command\n"
		       "?                Help\n");
	    }
	 break;

	 default: break;
      }
   }
}

/**************************************************************************/

#define FN_SIZE 120
struct stat objStat;

int main(int argc,char *argv[]) {

int i;
char obj[FN_SIZE];
ProgramBuf prog;

FileType ft = INVALID;
FILE *objFile = NULL;

   umask(0);

   bzero(obj,FN_SIZE);

   for (i=1; i<argc; i++) {
      ft = GetFileType(argv[i]);
      switch (ft) {
	 case INVALID:
	    fprintf(stderr,"asm: Invalid file extension: %s\n",argv[i]);
	 break;

	 case ASM:
	    fprintf(stderr,"asm: Source file: %s must be compile first\n",argv[i]);
	 break;

	 case OBJ:
	    strcpy(obj,argv[i]);
	    if (stat(obj,&objStat) == -1) {
	       fprintf(stderr,"asm: Can't get file statistics: %s for read\n",obj);
	       perror("asm");
	       exit(1);
	    }
	    if (objStat.st_size < 2) {
	       fprintf(stderr,"asm: File: %s is empty\n",obj);
	       exit(1);
	    }
	    objFile = fopen(obj,"r");
	    if (objFile == NULL) {
	       fprintf(stderr,"asm: Can't open: %s for read\n",obj);
	       perror("asm");
	       exit(1);
	    }
	 break;
      }
   }
   if (objFile) {
      ReadObject(objFile,&prog);
      fclose(objFile);
      fprintf(stdout,"Code emulator MTT Object:%s (? for Help)\n",obj);
      fprintf(stdout,"Making VHDL simulator HexDump: %s\7\n",obj);
      HexDump(&prog);
      Emulate(&prog);
      exit(0);
   }
   fprintf(stderr,"MTT Code emulator: Error: No object file\n");
   exit(1);
}
