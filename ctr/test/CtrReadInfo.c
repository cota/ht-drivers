/* ************************************************************************** */
/*                                                                            */
/* Parse the Ctr.info ASCII text file to build the Ctr driver action table    */
/* and initialize the Ctr driver via IOCTL calls. This program may well be    */
/* used in the CTR driver installation procedure. It relies on the standard   */
/* telegram access libraries tgmlib.a and tgvlib.a which must be installed.   */
/*                                                                            */
/* Julian Lewis 1st/March/2004                                                */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>        /* Error numbers */
#include <sys/file.h>
#include <a.out.h>
#include <ctype.h>

#ifdef CTR_PCI
#include <drm.h>
#endif

#include <ctrdrvr.h>
#include <ctrdrvrP.h>

#ifdef PS_VER
#include <tgv/tgv.h>
#include <tgm/tgm.h>
#define INFO "/dsc/local/data/Ctr.info"
#else
#define INFO "/usr/local/ctr/Ctr.info"
#endif

static int ctr;

#include "CtrOpen.c"

static CtrDrvrCtimObjects             ctimo;
static CtrDrvrCtimBinding             ctim[CtrDrvrCtimOBJECTS][CtrDrvrMODULE_CONTEXTS];

static CtrDrvrPtimBinding             ptim;
static CtrDrvrTrigger                 trig[CtrDrvrPtimOBJECTS][CtrDrvrMODULE_CONTEXTS];
static CtrDrvrCounterConfiguration    conf[CtrDrvrPtimOBJECTS][CtrDrvrMODULE_CONTEXTS];

static int                            rowm[CtrDrvrMODULE_CONTEXTS];

static int                            modn = 1;
static CtrDrvrCounterConfigurationBuf cbuf;
static CtrdrvrRemoteCommandBuf        crmb;
static CtrDrvrCounterMaskBuf          cmsb;

static char atomhash[256] = {
  10,9,9,9,9,9,9,9,9,0,0,9,9,0,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  0 ,1,9,1,9,4,1,9,2,3,1,1,0,1,11,1,5,5,5,5,5,5,5,5,5,5,1,1,1,1,1,9,
  10,6,6,6,6,6,6,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6,6,6,6,6,7,9,8,9,6,
  9 ,6,6,6,6,6,6,6,6,6,6,6,6,6,6 ,6,6,6,6,6,6,6,6,6,6,6,6,2,9,3,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9 ,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 };

typedef enum {
   Seperator=0,Operator=1,Open=2,Close=3,Comment=4,Numeric=5,Alpha=6,
   Open_index=7,Close_index=8,Illegal_char=9,Terminator=10,Bit=11
 } AtomType;

char *atom_names[12] = {"Seperator","Operator","Open","Close","Comment",
			"Numeric","Alpha","OpenIdx","CloseIdx","Illegal",
			"Terminator","Bit" };
typedef struct {
   AtomType          Type;
   char              *Text;
   int               Value;
   int               Position;
   int               LineNumber;
   void              *Next;
 } Atom;

static char *pname = "CtrReadInfo";

/**************************************************************************/
/* Handle errors                                                          */
/**************************************************************************/

typedef enum {
   FATAL_RUN_TIME,
   RUN_TIME,
   FATAL_SYNTAX,
   SYNTAX } ErrorType;

#define MAX_ERROR_COUNT 10

static int error_count = 0;

/* ============================ */

void Error(ErrorType et, Atom *atom, char *text) {

int fatal = 0;

   if (error_count++ >= MAX_ERROR_COUNT) {
      fprintf(stderr,"Error reporting suppressed after: %u errors",MAX_ERROR_COUNT);
      exit(1);
   }

   switch (et) {
      case RUN_TIME:
	 perror(pname);
	 fprintf(stderr,"Run time error: ");
      break;
      case SYNTAX:   fprintf(stderr,"Syntax error: ");   break;
      case FATAL_RUN_TIME:
	 perror(pname);
	 fatal = 1;
	 fprintf(stderr,"Fatal: Run time error: ");
      break;
      case FATAL_SYNTAX:
	 fatal = 1;
	 fprintf(stderr,"Fatal: Syntax Error: ");
      break;
      default:
	 fprintf(stderr,"Fatal: BUG: Internal program error\n");
	 exit(1);
   }

   fprintf(stderr,"%s",text);

   if (atom) {
      fprintf(stderr,": at Line %u.%u",
		     (int) atom->LineNumber,
		     (int) atom->Position);
      if (atom->Type == Alpha)
	 fprintf(stderr," :%s",atom->Text);
      if (atom->Type == Numeric)
	 fprintf(stderr," :%u",(int) atom->Value);
   }
   fprintf(stderr,"\n");

   if (fatal) exit(1);

   return;
}

/**************************************************************************/
/* String to upper case                                                   */
/**************************************************************************/

void StrToUpper(inp,out)
char *inp;
char *out; {

int         i;
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
/* Get a new string                                                       */
/**************************************************************************/

char *NewString(int size) {
char *cp;

   if (size > 0) {
      cp = (char *) malloc(size);
      if (cp) return cp;
      Error(FATAL_RUN_TIME,NULL,"No more dynamic memory available");
      exit(1);
   }
   Error(FATAL_RUN_TIME,NULL,"Called NewString with zero size");
   return NULL;
}

/**************************************************************************/
/* Dispose atom list                                                      */
/**************************************************************************/

void DisposeAtoms(Atom *list) {

   if (list == NULL) return;
   DisposeAtoms(list->Next);
   if (list->Text) free(list->Text);
   free(list);
}

/**************************************************************************/
/* Get a new atom                                                         */
/**************************************************************************/

Atom *NewAtom(Atom *head) {
Atom *atom;

   atom = (Atom *) malloc(sizeof(Atom));
   if (atom == NULL) {
      Error(FATAL_RUN_TIME,NULL,"No more dynamic memory available");
      exit(1);
   }
   bzero((void *) atom, sizeof(Atom));
   if (head) {
      while (head->Next != NULL) head = head->Next;
      head->Next = atom;
   }
   return atom;
}

/**************************************************************************/
/* Chop up a source text into atoms                                       */
/**************************************************************************/

#define TEXT 128

Atom *GetAtoms(char *buf) {

char      *cp, *ep, text[TEXT];
AtomType   at;
Atom      *head, *atom;
int        nb, pos, line, newline;

   if (buf == NULL) return NULL;

   /* Start an atom list */

   head = atom = NewAtom(NULL);
   pos = 1; line = 1;

   /* Break input string up into atoms */

   cp = &buf[0];

   while(1) {
      at = atomhash[(int) (*cp)];

      switch (at) {
    
      /* Numeric atom types have their value set to the value of   */
      /* the text string. The number of bytes field corresponds to */
      /* the number of characters in the number. */
    
      case Numeric:
	 atom->Value = (int) strtoul(cp,&ep,0);
	 nb = ((unsigned int) ep - (unsigned int) cp);
	 cp = ep;
	 atom->Type = Numeric;
	 atom->Position = pos;
	 atom->LineNumber = line;
	 pos += nb;
	 atom = NewAtom(atom);
      break;
    
      /* Alpha atom types are strings of letters and numbers starting with */
      /* a letter. The text of the atom is placed in the text field of the */
      /* atom. The number of bytes field is the number of characters in    */
      /* the name.                                                         */
    
      case Alpha:
	 nb = 0;
	 bzero((void *) text,TEXT);
	 while (((at == Alpha) || (at == Numeric)) && (nb < TEXT)) {
	    text[nb] = *cp;
	    nb++; cp++;
	    at = atomhash[(int) (*cp)];
	 }
	 atom->Text = NewString(strlen(text)+1);
	 strcpy(atom->Text,text);
	 atom->Type = Alpha;
	 atom->Position = pos;
	 atom->LineNumber = line;
	 pos += nb;
	 atom = NewAtom(atom);
      break;
    
      /* Seperators can be as long as they like, the only interesting */
      /* data about them is their byte counts. */
    
      case Seperator:
	 newline = 0;
	 bzero((void *) text, TEXT);
	 while (at == Seperator ) {
	    if (*cp=='\n') {
	       atom->Type = Seperator;
	       atom->Position = pos; pos = 0;
	       newline = 1;
	       atom->LineNumber = line;
	       atom->Text = NULL;
	       atom = NewAtom(atom);
	       cp++;
	       break;
	    }
	    at = atomhash[(int) (*(++cp))]; pos++;
	 }
	 line += newline;
      break;
    
      /* Comments are enclosed between a pair of percent marks, the only */
      /* characters not allowed in a comment is a terminator or a new line */
      /* just incase someone has forgotten to end one. */
    
      case Comment:
	 at = atomhash[(int) (*(++cp))];
	 atom->Type = Comment;
	 bzero((void *) text, TEXT);
	 nb = 0;
	 while (at != Comment) {
	    if (*cp=='\n') {
	       atom->Type = Seperator;
	       cp++;
	       newline = 1;
	       break;
	    }
	    text[nb] = *cp;
	    nb++; cp++;
	    at = atomhash[(int) (*cp)];
	 }
	 atom->Text = NewString(strlen(text)+1);
	 strcpy(atom->Text,text);
	 atom->Position = pos;
	 atom->LineNumber = line;
	 pos += nb;
	 atom = NewAtom(atom);
	 line += newline;
      break;
    
      /* Operator atoms have the atom value set to indicate which operator */
      /* the atom is. The text and number of byte fields are also set. */
    
      case Operator:
	 nb = 0;
	 bzero((void *) text,TEXT);
	 while ((at == Operator) && (nb < TEXT)) {
	    text[nb] = *cp;
	    nb++; cp++;
	    at = atomhash[(int) (*cp)];
	 }
	 atom->Text = NewString(strlen(text)+1);
	 strcpy(atom->Text,text);
	 atom->Type = Operator;
	 atom->Position = pos;
	 atom->LineNumber = line;
	 pos += nb;
	 atom = NewAtom(atom);
      break;
    
      /* These are simple single byte operators */
    
      case Open:
      case Close:
      case Open_index:
      case Close_index:
      case Bit:
	 atom->Type = at;
	 atom->Position = pos;
	 atom->LineNumber = line;
	 cp++; pos++;;
	 atom = NewAtom(atom);
      break;

      case Illegal_char:
	 Error(SYNTAX,atom,"Illegal character");

      default:
	 return head;
      }
   }
}

/**************************************************************************/
/* Debug routine to list all atoms                                        */
/**************************************************************************/

void ListAtoms(Atom *atom) {

   while (atom) {
      if (atom->Text) {
	 printf("Typ:%s Txt:%s Val:%u Pos:%u Lin:%u\n",
		      atom_names[atom->Type],
		      atom->Text,
		(int) atom->Value,
		(int) atom->Position,
		(int) atom->LineNumber);
      } else {
	 printf("Typ:%s Txt:NULL Val:%u Pos:%u Lin:%u\n",
		      atom_names[atom->Type],
		(int) atom->Value,
		(int) atom->Position,
		(int) atom->LineNumber);

      }
      atom = atom->Next;
   }
}

/**************************************************************************/
/* Lookup a CTIM equipment                                                */
/**************************************************************************/

int LookUpCtimBinding(unsigned int eqp,int row) {
int i;

   for (i=0; i<ctimo.Size; i++) {
      if (eqp == ctimo.Objects[i].EqpNum) {
	 ctim[row][modn-1].EqpNum = eqp;
	 ctim[row][modn-1].Frame = ctimo.Objects[i].Frame;
	 return 1;
      }
   }
   return 0;
}

/**************************************************************************/
/* { CTIM: <equip> [<wildcards>] }                                        */
/**************************************************************************/

Atom *GetCtimBinding(Atom *atom, int row) {

   if (atom->Type == Numeric) {
      ctim[row][modn-1].EqpNum = atom->Value;
      atom = atom->Next;
      if (atom->Type == Numeric) {
	 if (LookUpCtimBinding(ctim[row][modn-1].EqpNum,row)) {
	    ctim[row][modn-1].Frame.Struct.Value = (unsigned short) (atom->Value & 0xFFFF);
	 } else {
	    Error(FATAL_RUN_TIME,atom,"No such Equipment in CTIM spec");
	    ctim[row][modn-1].Frame.Long = (unsigned int) atom->Value;
	 }
	 atom = atom->Next;
      }
   } else Error(SYNTAX,atom,"Missing Equipment in CTIM spec");
   return atom;
}

/**************************************************************************/
/* { PTIM: <equip> <module> <counter> }                                   */
/**************************************************************************/

Atom *GetPtimBinding(Atom *atom) {

   if (atom->Type == Numeric) {
      ptim.EqpNum = atom->Value;
      atom = atom->Next;
      if (atom->Type == Numeric) {
	 ptim.ModuleIndex = atom->Value -1;
	 modn = ptim.ModuleIndex+1;
	 atom = atom->Next;
	 if (atom->Type == Numeric) {
	    ptim.Counter = atom->Value;
	    atom = atom->Next;
	 } else Error(SYNTAX,atom,"Missing Counter in PTIM spec");
      } else Error(SYNTAX,atom,"Missing Module in PTIM spec");
   } else Error(SYNTAX,atom,"Missing Equipment in PTIM spec");
   return atom;
}

/**************************************************************************/
/* { TRIG: <machine> <condition> <group> <value> }                        */
/**************************************************************************/

#ifdef PS_VER
static char machine_names[TgmMACHINES][TgmNAME_SIZE];
#endif
char *condition_names[CtrDrvrTriggerCONDITIONS] = {"NO_CHECK","EQUALS","AND"};

Atom *GetTrigger(Atom *atom, int row) {
int m, i, ok, tm;

#ifdef PS_VER
TgmLineNameTable ltab;
TgmGroupDescriptor desc;
#endif

   ok = 0; m = 0;

#ifdef PS_VER
   if (atom->Type == Alpha) {
      for (i=1; i<CtrDrvrMachineMACHINES; i++) {
	 if (strcmp(atom->Text,machine_names[i]) == 0) {
	    ok = 1;
	    trig[row][modn-1].Machine = m = i;
	    atom = atom->Next;
	    break;
	  }
       }
   } else Error(SYNTAX,atom,"Missing Machine in TRIG spec");
   if (!ok) Error(SYNTAX,atom,"Illegal Machine name in TRIG spec");

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<CtrDrvrTriggerCONDITIONS; i++) {
	 if (strcmp(atom->Text,condition_names[i]) == 0) {
	    ok = 1;
	    trig[row][modn-1].TriggerCondition = i;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing Condition in TRIG spec");
   if (!ok) Error(SYNTAX,atom,"Illegal Condition name in TRIG spec");

   if (atom->Type == Alpha) {
      tm = TgvTgvToTgmMachine(m);
      trig[row][modn-1].Group.GroupNumber = TgmGetGroupNumber(tm,atom->Text);
      atom = atom->Next;
      if (trig[row][modn-1].Group.GroupNumber == 0)
	 Error(FATAL_RUN_TIME,atom,"No such Group name in TRIG spec");
   } else if (atom->Type == Numeric) {
      trig[row][modn-1].Group.GroupNumber = atom->Value;
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing Group name/number in TRIG spec");

   if (atom->Type == Alpha) {
      ok = 0;
      tm = TgvTgvToTgmMachine(m);
      TgmGetGroupDescriptor(tm,trig[row][modn-1].Group.GroupNumber,&desc);
      TgmGetLineNameTable(tm,desc.Name,&ltab);
      for (i=0; i<ltab.Size; i++) {
	 if (strcmp(ltab.Table[i].Name,atom->Text) == 0) {
	    ok = 1;
	    trig[row][modn-1].Group.GroupValue = (unsigned short) i+1;
	    break;
	 }
      }
      if (!ok) Error(FATAL_RUN_TIME,atom,"No such Group Line name in TRIG spec");
      atom = atom->Next;
   } else if (atom->Type == Numeric) {
      trig[row][modn-1].Group.GroupValue = (unsigned short) atom->Value;
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing Group value in TRIG spec");
#endif

   return atom;
}

/**************************************************************************/
/* { CONF: <enable> <start> <mode> <clock> <delay> <pulsewidth> }         */
/**************************************************************************/

char *enable_names[2] = {"DISABLE","ENABLE"};
char *start_names[CtrDrvrCounterSTARTS] = {"NORMAL","EXT1","EXT2","CHAINED","SELF","REMOTE","PPS","CHSTOP"};
char *mode_names[CtrDrvrCounterMODES] = {"NORMAL","MULTIPLE","BURST"};
char *clock_names[CtrDrvrCounterCLOCKS] = {"KHZ1","MHZ10","MHZ40","EXT1","EXT2","CHAINED"};

Atom *GetConfig(Atom *atom, int row) {
int i, ok;

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<2; i++) {
	 if (strcmp(atom->Text,enable_names[i]) == 0) {
	    ok = 1;
	    if (i) conf[row][modn-1].OnZero = CtrDrvrCounterOnZeroOUT;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing Enable/Disable in CONF spec");
   if (!ok) Error(SYNTAX,atom,"Illegal Enable/Disable name in CONF spec");

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<CtrDrvrCounterSTARTS; i++) {
	 if (strcmp(atom->Text,start_names[i]) == 0) {
	    ok = 1;
	    conf[row][modn-1].Start = i;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing Start in CONF spec");
   if (!ok) Error(SYNTAX,atom,"Illegal Start name in CONF spec");

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<CtrDrvrCounterMODES; i++) {
	 if (strcmp(atom->Text,mode_names[i]) == 0) {
	    ok = 1;
	    conf[row][modn-1].Mode = i;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing Mode in CONF spec");
   if (!ok) Error(SYNTAX,atom,"Illegal Mode name in CONF spec");

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<CtrDrvrCounterCLOCKS; i++) {
	 if (strcmp(atom->Text,clock_names[i]) == 0) {
	    ok = 1;
	    conf[row][modn-1].Clock = i;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing Clock in CONF spec");
   if (!ok) Error(SYNTAX,atom,"Illegal Clock name in CONF spec");

   if (atom->Type == Numeric) {
      conf[row][modn-1].Delay = atom->Value;
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing Delay value in CONF spec");

   if (atom->Type == Numeric) {
      conf[row][modn-1].PulsWidth = atom->Value;
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing PulseWidth value in CONF spec");

   return atom;
}

/*****************************************************************************/
/* {REMOTE <module> <channel> <start> <clock> <delay> <pulsewidth> <onzero>} */
/*****************************************************************************/

static char *onzero_names[4] = {"NO_OUT", "BUS", "OUT", "BUS_OUT" };

Atom *GetRemote(Atom *atom) {

unsigned int ch;
int i, ok;

   bzero((void *) &cbuf, sizeof(CtrDrvrCounterConfigurationBuf));
   bzero((void *) &crmb, sizeof(CtrdrvrRemoteCommandBuf));

   if (atom->Type == Numeric) {
      modn = atom->Value;
      if ((modn < 1) || (modn > CtrDrvrMODULE_CONTEXTS)) {
	 Error(SYNTAX,atom,"Module number out of range in REMOTE spec");
      }
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing module number in REMOTE spec");

   if (atom->Type == Numeric) {
      ch = atom->Value;
      if ((ch < 1) || (ch > CtrDrvrCOUNTERS)) {
	 Error(SYNTAX,atom,"Counter number out of range in REMOTE spec");
      }
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing channel number in REMOTE spec");

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<CtrDrvrCounterSTARTS; i++) {
	 if (strcmp(atom->Text,start_names[i]) == 0) {
	    ok = 1;
	    cbuf.Config.Start = i;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing Start in REMOTE spec");
   if (!ok) Error(SYNTAX,atom,"Illegal Start name in REMOTE spec");

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<CtrDrvrCounterMODES; i++) {
	 if (strcmp(atom->Text,mode_names[i]) == 0) {
	    ok = 1;
	    cbuf.Config.Mode = i;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing Mode in REMOTE spec");
   if (!ok) Error(SYNTAX,atom,"Illegal Mode name in REMOTE spec");

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<CtrDrvrCounterCLOCKS; i++) {
	 if (strcmp(atom->Text,clock_names[i]) == 0) {
	    ok = 1;
	    cbuf.Config.Clock = i;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing Clock in REMOTE spec");
   if (!ok) Error(SYNTAX,atom,"Illegal Clock name in REMOTE spec");

   if (atom->Type == Numeric) {
      cbuf.Config.Delay = atom->Value;
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing Delay value in REMOTE spec");

   if (atom->Type == Numeric) {
      cbuf.Config.PulsWidth = atom->Value;
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing PulseWidth value in REMOTE spec");

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<4; i++) {
	 if (strcmp(atom->Text,onzero_names[i]) == 0) {
	    ok = 1;
	    cbuf.Config.OnZero = i;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing OnZero in REMOTE spec");
   if (!ok) Error(SYNTAX,atom,"Illegal OnZero name in REMOTE spec");

   if (ioctl(ctr,CtrDrvrSET_MODULE,&modn) < 0) {
      Error(RUN_TIME,atom,"Can't select module in REMOTE spec");
   }

   crmb.Counter = ch;
   crmb.Remote = 1;

   if (ioctl(ctr,CtrDrvrSET_MODULE,&modn) < 0) {
      Error(FATAL_RUN_TIME,NULL,"Can't select that module");
   }

   if (ioctl(ctr,CtrDrvrSET_REMOTE,&crmb) < 0) {
      Error(RUN_TIME,atom,"Can't set channel in remote in REMOTE spec");
   }

   cbuf.Counter = ch;

   if (ioctl(ctr,CtrDrvrSET_CONFIG,&cbuf) < 0) {
      Error(RUN_TIME,atom,"Can't set remote configuration in REMOTE spec");
   }

   crmb.Counter = ch;
   crmb.Remote = CtrDrvrRemoteLOAD;

   if (ioctl(ctr,CtrDrvrREMOTE,&crmb) < 0) {
      Error(RUN_TIME,atom,"Can't issue remote command LOAD in REMOTE spec");
   }

   return atom;
}

/**************************************************************************/
/* {OUTMSK <module> <ioport> <mask> <polarity>}                           */
/**************************************************************************/

static char *polarity_names[2] = {"TTL_POS", "TTL_BAR" };

Atom *GetMask(Atom *atom) {

unsigned int ch, msk;
int i, ok;

   bzero((void *) &cmsb, sizeof(CtrDrvrCounterMaskBuf));

   if (atom->Type == Numeric) {
      modn = atom->Value;
      if ((modn < 1) || (modn > CtrDrvrMODULE_CONTEXTS)) {
	 Error(SYNTAX,atom,"Module number out of range in OUTMSK spec");
      }
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing module number in OUTMSK spec");

   if (atom->Type == Numeric) {
      ch = atom->Value;
      if ((ch < 1) || (ch > CtrDrvrCOUNTERS)) {
	 Error(SYNTAX,atom,"Counter number out of range in OUTMSK spec");
      }
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing I/O port number in OUTMSK spec");

   if (atom->Type == Numeric) {
      msk = atom->Value;
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing output mask in OUTMSK spec");

   ok = 0;
   if (atom->Type == Alpha) {
      for (i=0; i<2; i++) {
	 if (strcmp(atom->Text,polarity_names[i]) == 0) {
	    ok = 1;
	    cmsb.Polarity = i;
	    atom = atom->Next;
	    break;
	 }
      }
   } else Error(SYNTAX,atom,"Missing Polarity in OUTMSK spec");
   if (!ok) Error(SYNTAX,atom,"Illegal polarity name in OUTMSK spec");

   if (ioctl(ctr,CtrDrvrSET_MODULE,&modn) < 0) {
      Error(RUN_TIME,atom,"Can't select module in OUTMSK spec");
   }

   cmsb.Counter = ch;
   cmsb.Mask = msk;

   if (ioctl(ctr,CtrDrvrSET_OUT_MASK,&cmsb) < 0) {
      Error(RUN_TIME,atom,"Can't set output mask in OUTMSK spec");
   }

   return atom;
}

/**************************************************************************/
/* {OUTBYTE <module> <byte>}                                              */
/**************************************************************************/

Atom *GetOByte(Atom *atom) {

unsigned int oby;

   if (atom->Type == Numeric) {
      modn = atom->Value;
      if ((modn < 1) || (modn > CtrDrvrMODULE_CONTEXTS)) {
	 Error(SYNTAX,atom,"Module number out of range in OUTBYTE spec");
      }
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing module number in OUTBYTE spec");

   if (atom->Type == Numeric) {
      oby = atom->Value;
      if ((oby < 1) || (oby > 8)) {
	 Error(SYNTAX,atom,"Output byte number out of range in OUTBYTE spec");
      }
      atom = atom->Next;
   } else Error(SYNTAX,atom,"Missing output byte number in OUTMSK spec");

   if (ioctl(ctr,CtrDrvrSET_MODULE,&modn) < 0) {
      Error(RUN_TIME,atom,"Can't select module in OUTBYTE spec");
   }

#ifdef CTR_VME
   if (ioctl(ctr,CtrDrvrSET_OUTPUT_BYTE,&oby) < 0) {
      Error(RUN_TIME,atom,"Can't set output byte in OUTBYTE spec");
   }
#endif

   return atom;
}

/**************************************************************************/
/* Create a Ptim object                                                   */
/**************************************************************************/

void CreatePtim(int rows) {
int i;
char ermes[128];
CtrDrvrAction actn;

   bzero((void *) &actn, sizeof(CtrDrvrAction));

   ptim.Size = rows;
   if (rowm[modn-1]) ptim.StartIndex = rowm[modn-1] -1;

   if (ioctl(ctr,CtrDrvrCREATE_PTIM_OBJECT,&(ptim)) < 0) {
      sprintf(ermes,"Cant create PTIM object: %u\n",(int) ptim.EqpNum);
      Error(RUN_TIME,NULL,ermes);
      if (ioctl(ctr,CtrDrvrGET_PTIM_BINDING,&(ptim)) < 0) {
	 sprintf(ermes,"Cant read PTIM object: %u\n",(int) ptim.EqpNum);
	 Error(FATAL_RUN_TIME,NULL,ermes);
      }
      return;
   }

   modn = ptim.ModuleIndex+1;
   if (ioctl(ctr,CtrDrvrSET_MODULE,&modn) < 0) {
      sprintf(ermes,"No such module: %d installed\n",modn);
      Error(FATAL_RUN_TIME,NULL,ermes);
   }

   for (i=ptim.StartIndex; i<ptim.StartIndex + ptim.Size; i++) {
      actn.TriggerNumber   = i+1;
      actn.EqpNum          = ptim.EqpNum;
      actn.EqpClass        = CtrDrvrConnectionClassPTIM;
      actn.Trigger         = trig[i][modn-1];
      actn.Trigger.Ctim    = ctim[i][modn-1].EqpNum;
      actn.Trigger.Frame   = ctim[i][modn-1].Frame;
      actn.Config          = conf[i][modn-1];
      actn.Trigger.Counter = ptim.Counter;

      if (ioctl(ctr,CtrDrvrSET_ACTION,&actn) < 0) {
	 sprintf(ermes,"Cant set Action: %u for PTIM: %u\n",i+1,(int) ptim.EqpNum);
	 Error(FATAL_RUN_TIME,NULL,ermes);
      }

   }
   return;
}

/**************************************************************************/
/* Open input file, read configuration, parse it, and set up driver.      */
/**************************************************************************/

#define FN_SIZE 80
#define KEYWORDS 7
static char *keywords[KEYWORDS] = {"PTIM","CTIM","TRIG","CONF","REMOTE","OUTMSK","OUTBYTE"};
typedef enum {PTIM,CTIM,TRIG,CONF,REMOTE,OUTMSK,OUTBYTE} keynums;

int main(int argc,char *argv[]) {

char ass[FN_SIZE];
FILE *assFile = NULL;
struct stat assStat;

int i, j, found, sourceSize, sourceLines, equips, row, rows, level, remflg;
char c, *source;
char text[FN_SIZE];

Atom *head=NULL, *atom=NULL;

#ifdef PS_VER
TgmMachine *mch;
TgmNetworkId nid;
#endif

CtrDrvrAction tact;
unsigned int modn, modcnt;

   printf("%s: Compiled %s %s\n",pname,__DATE__,__TIME__);

   remflg = 0;

#ifdef PS_VER
   nid = TgmGetDefaultNetworkId();
   mch = TgmGetAllMachinesInNetwork(nid);
   if (mch != NULL) {
      for (i=0; i<TgmMACHINES; i++) {
	 if (mch[i] == TgmMACHINE_NONE) break;
	 strcpy(machine_names[TgvTgmToTgvMachine(mch[i])],TgmGetMachineName(mch[i]));
      }
   }
#endif

   for (j=1; j<argc; j++) {
      for (i=0; i<CtrDrvrDEVICES; i++) {
	 if (strcmp(argv[j],devnames[i]) == 0) {
	    ctr_device = (CtrDrvrDevice) i;
	    printf("Forcing CTR device type:%s\n",devnames[i]);
	    break;
	 }
      }
   }

   ctr = CtrOpen();
   if (ctr == 0) {
      fprintf(stderr,"\nFATAL: Could not open CTR driver");
      exit(1);
   }
   ioctl(ctr,CtrDrvrLIST_CTIM_OBJECTS,&ctimo);

   if (argc < 2) strcpy(ass,INFO);
   else          strcpy(ass,argv[1]);

   if (stat(ass,&assStat) == -1) {
      fprintf(stderr,"%s: Can't get file statistics: %s for read\n",pname,ass);
      perror("CtrInfo.c");
      exit(1);
   }
   if (assStat.st_size < 2) {
      fprintf(stderr,"%s: File: %s is empty\n",pname,ass);
      exit(1);
   }
   assFile = fopen(ass,"r");
   if (assFile == NULL) {
      fprintf(stderr,"%s: Can't open: %s for read\n",pname,ass);
      perror(pname);
      exit(1);
   }
   printf("%s: Parsing file: %s (%u bytes) for CTR timing declarations\n",
	   pname,
	   ass,
	   (int) assStat.st_size);

   sourceSize = assStat.st_size + 16;
   source = (char *) malloc(sourceSize);
   bzero((void *) source, sourceSize);
   sourceLines = 0;
   for (i=0; i<sourceSize; i++) {
      c = fgetc(assFile);
      if (c == '\n') sourceLines++;
      if ((c == (char) EOF) || (c == (char) 0)) break;
      source[i] = c;
   }
   printf("%s: Parser: %u characters %u lines read from: %s\n",pname,i,sourceLines,ass);
   printf("%s: Begin...\n\n",pname);

   head = GetAtoms(source);
   if (argc > 2) {
      StrToUpper(argv[2],text);
      if (strcmp(text,"DEBUG") == 0) {
	 printf("Atom List: DEBUG option\n");
	 ListAtoms(head);
	 printf("\n---End DEBUG---\n");
      }
   }

   ioctl(ctr,CtrDrvrGET_MODULE_COUNT,&modcnt);
   for (i=0; i<modcnt; i++) {
      modn = i+1;
      ioctl(ctr,CtrDrvrSET_MODULE,&modn);
      for (j=0; j<CtrDrvrRamTableSIZE; j++) {
	 bzero((void *) &tact, sizeof(CtrDrvrAction));
	 tact.TriggerNumber = j+1;
	 ioctl(ctr,CtrDrvrGET_ACTION,&tact);
	 if (tact.EqpNum == 0) {
	    rowm[i] = j;
	    break;
	 }
      }
   }
   row = rowm[0]; modn = 1;

   equips = 0; rows = 0; level = 0; atom = head;
   while (atom) {
      switch (atom->Type) {
	 case Seperator:
	 case Comment:
	    atom = atom->Next;
	 break;

	 case Open:
	 case Open_index:
	    level++;
	    atom = atom->Next;
	 break;

	 case Alpha:
	    if (level == 2) {
	       found = 0;
	       for (i=0; i<KEYWORDS; i++) {
		  if (strcmp(keywords[i],atom->Text) == 0) {
		     found = 1;
		     row = rowm[modn-1];
		     switch ((keynums) i) {

			case PTIM:
			   remflg = 0;
			   atom = atom->Next;
			   atom = GetPtimBinding(atom);
			   equips++; rows = 0;
			break;

			case CTIM:
			   atom = atom->Next;
			   atom = GetCtimBinding(atom,row);
			break;

			case TRIG:
			   atom = atom->Next;
			   atom = GetTrigger(atom,row);
			break;

			case CONF:
			   atom = atom->Next;
			   atom = GetConfig(atom,row);
			   rowm[modn-1]++; rows++;
			break;

			case REMOTE:
			   remflg++;
			   atom = atom->Next;
			   atom = GetRemote(atom);
			break;

			case OUTMSK:
			   atom = atom->Next;
			   atom = GetMask(atom);
			break;

			case OUTBYTE:
			   atom = atom->Next;
			   atom = GetOByte(atom);
			break;
		     }
		     break;
		  }
	       }
	       if (!found) {
		  Error(SYNTAX,atom,"Invalid keyword");
		  atom = atom->Next;
		  break;
	       }
	    }
	 break;

	 case Close:
	 case Close_index:
	    if (level) level--;
	    else Error(SYNTAX,atom,"Too many close brackets");
	    if ((level == 0) && (remflg == 0)) {
	       if (rows) {
		  CreatePtim(rows);
		  rowm[modn-1] = ptim.StartIndex + ptim.Size;
		  rows = 0;
	       }
	    }
	    atom = atom->Next;
	 break;

	 case Numeric:
	    Error(SYNTAX,atom,"Unexpected numeric value");
	    atom = atom->Next;
	 break;

	 case Terminator:
	    atom = NULL;
	 break;

	 default:
	    Error(SYNTAX,atom,"Illegal character");
	    atom = atom->Next;
	 break;
      }
   }
   printf("%s: Found: %u PTIM equipment declarations\n",pname,equips);
   if (remflg) printf("%s: Configured: %d Counters in remote\n",pname,remflg);
   exit(0);
}
