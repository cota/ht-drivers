/**************************************************************************/
/* Geniric Assembler/Disassembler.                                        */
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

static int debug = 0;

struct stat assStat;
struct stat objStat;

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
/* Absorb white space                                                     */
/**************************************************************************/

char WhiteSpace(cp,ep)
char *cp;
char **ep; {

char c;

   if (cp) {
      while ((c = *cp)) {
	 if (atomhash[(int) c] != Seperator) break;
	 cp++;
      }
      *ep = cp;
      return c;
   }
   return 0;
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
/* Print error messages                                                   */
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
int i;

   if (error_count++ >= MAX_ERROR_COUNT) {
      fprintf(stderr,"Error reporting suppressed after: %d errors",MAX_ERROR_COUNT);
      exit(1);
   }

   switch (et) {
      case RUN_TIME: fprintf(stderr,"Run time error:"); break;
      case SYNTAX:   fprintf(stderr,"Syntax error:");   break;
      case FATAL_RUN_TIME:
	 fatal = 1;
	 fprintf(stderr,"Fatal: Run time error:");
      break;
      case FATAL_SYNTAX:
	 fatal = 1;
	 fprintf(stderr,"Fatal: Syntax Error:");
      break;
      default:
	 fprintf(stderr,"Fatal: BUG: Internal program error\n");
	 exit(1);
   }

   fprintf(stderr,"%s",text);

   if (atom) {
      fprintf(stderr,": at Line %d.%d\n",(int) atom->LineNumber,(int) atom->Position);
      fprintf(stderr,"\t\t\t\t....."); for (i=0;i<atom->Position;i++) { fprintf(stderr,"."); }
      fprintf(stderr,"v");
   }
   fprintf(stderr,"\n");

   if (fatal) exit(1);

   return;
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
/* Look up an atom to see if its special                                  */
/**************************************************************************/

long LookUpOpCode(Atom *atom) {

int i;
char upr[MAX_OP_CODE_STRING_LENGTH +1], *cp;

   cp = atom->Text;
   if (cp) {
      if (strlen(cp) <= MAX_OP_CODE_STRING_LENGTH) {
	 StrToUpper(cp,upr);
	 for (i=0; i<OPCODES; i++) {
	    if (strcmp(upr,InstructionSet[i].Name) == 0) {
	       atom->Type = Op_code;
	       atom->Value = i;
	       return 1;
	    }
	 }
      }
   }
   return 0;
}

/**************************************************************************/
/* Look up an atom to see if its special                                  */
/**************************************************************************/

long LookUpRegister(Atom *atom) {

int i;
unsigned long rn, en, st, of;
char upr[MAX_REGISTER_STRING_LENGTH +1],
     *cp,
     *ep;

   cp = atom->Text;
   if (cp) {
      if (strlen(cp) <= MAX_REGISTER_STRING_LENGTH) {
	 StrToUpper(cp,upr);
	 for (i=0; i<REGNAMES; i++) {
	    if (strncmp(upr,Regs[i].Name,strlen(Regs[i].Name)) == 0) {

	       st = Regs[i].Start;
	       en = Regs[i].End;
	       of = Regs[i].Offset;

	       if (st < en) {
		  if (strlen(Regs[i].Name) < strlen(atom->Text)) {
		     cp = atom->Text + strlen(Regs[i].Name);
		     rn = strtoul(cp,&ep,0);
		     if (cp != ep) {
			if ((rn >= of)
			&&  (rn <= en - st + of)) {
			   atom->Type = Register;
			   atom->Value = st + rn - of;
			   return 1;
			}
		     }
		  }
	       } else {
		  atom->Type = Register;
		  atom->Value = st;
		  return 1;
	       }
	    }
	 }
      }
   }
   return 0;
}

/**************************************************************************/
/* Get Register Name                                                      */
/**************************************************************************/

char *GetRegisterName(int regnum) {
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
/* Look up an atom to see if its special                                  */
/**************************************************************************/

long LookUpOperator(Atom *atom) {
int i;
   for (i=0; i<OprOPRS; i++) {
      if (strcmp(atom->Text,OprNames[i].Name) == 0) {
	 atom->Type = Operator;
	 atom->Value = i;
	 return 1;
      }
   }
   return 0;
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

   /* Repeat the last command on blank command buffer */

   if ((       buf  == NULL)
   ||  (      *buf  == '.')) return NULL;

   /* Start an atom list */

   head = atom = NewAtom(NULL);
   pos = 1; line = 1;

   /* Break input string up into atoms */

   cp = &buf[0];

   while(pos <= assStat.st_size) {
      at = atomhash[(int) (*cp)];

      switch (at) {
    
      /* Numeric atom types have their value set to the value of   */
      /* the text string. The number of bytes field corresponds to */
      /* the number of characters in the number. */
    
      case Numeric:
	 atom->Value = (int) strtoul(cp,&ep,0);
	 nb = ((unsigned long) ep - (unsigned long) cp);
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
	 if (!LookUpOpCode(atom)) LookUpRegister(atom);
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
	    if ((*cp=='\n') || (*cp==0)) {
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
	 LookUpOperator(atom);
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
   return head;
}

/**************************************************************************/
/* Debug routine to list all atoms                                        */
/**************************************************************************/

void ListAtoms(Atom *atom) {

   while (atom) {
      if (atom->Text) {
	 printf("Typ:%d Txt:%s Val:%d Pos:%d Lin:%d\n",
		(int) atom->Type,
		      atom->Text,
		(int) atom->Value,
		(int) atom->Position,
		(int) atom->LineNumber);
      } else {
	 printf("Typ:%d Txt:NULL Val:%d Pos:%d Lin:%d\n",
		(int) atom->Type,
		(int) atom->Value,
		(int) atom->Position,
		(int) atom->LineNumber);

      }
      atom = atom->Next;
   }
}

/* *************************************************** */
/* Handel the symbol table                             */

typedef struct {
   char *Name;
   unsigned short Index;
   long Value;
   long Defined;
 } Symbol;

#define SYMBOLS 1024

static Symbol symbolTable[SYMBOLS];
static long symbolTableSize = 0;

/* ============================ */

Symbol *GetSymbol(char *name) {

int i;

   /* Dumb straight linear search */

   for (i=0; i<symbolTableSize; i++) {
      if (strcmp(name,symbolTable[i].Name) == 0) {
	 return &(symbolTable[i]);
      }
   }
   i = symbolTableSize++;
   if (i >= SYMBOLS) {
      Error(FATAL_RUN_TIME,NULL,"Symbol table overflow");
      return NULL;
   }
   symbolTable[i].Name = name;
   symbolTable[i].Value = 0;
   symbolTable[i].Defined = 0;
   symbolTable[i].Index = i;
   return &(symbolTable[i]);
}

/* *************************************************** */
/* Expression stacks, reverse polish                   */

typedef struct {
   Symbol *SymbolRef;
   long   LiteralValue;
 } Operand;

#define OPERAND_STACK  64
#define OPERATOR_STACK 32

static long operandStackSize  = 0;
static long operatorStackSize = 0;

static Operand operandStack[OPERAND_STACK];
static OprId operatorStack[OPERATOR_STACK];

/* ============================ */

long PushOperand(Operand *operand) {

   if (operandStackSize < OPERAND_STACK) {
      operandStack[operandStackSize++] = *operand;
      return 1;
   }
   Error(FATAL_RUN_TIME,NULL,"Operand stack overflow");
   return 0; /* Stack overflow */
}

/* ============================ */

long PushOperator(OprId *oprId) {

   if (operatorStackSize < OPERATOR_STACK) {
      operatorStack[operatorStackSize++] = *oprId;
      return 1;
   }
   Error(FATAL_RUN_TIME,NULL,"Operator stack overflow");
   return 0; /* Stack overflow */
}

/* ============================ */

long PopOperand(Operand *operand) {

   if (operandStackSize > 0) {
      *operand = operandStack[--operandStackSize];
      return 1;
   }
   Error(RUN_TIME,NULL,"Operand stack underflow");
   return 0; /* Stack underflow */
}

/* ============================ */

long PopOperator(OprId *oprId) {

   if (operatorStackSize > 0) {
      *oprId = operatorStack[--operatorStackSize];
      return 1;
   }
   Error(RUN_TIME,NULL,"Operator stack underflow");
   return 0; /* Operator stack under flow */
}

/* ============================ */

void ResetStacks() {

   operandStackSize = 0;
   operatorStackSize = 0;
}

/* ============================ */

long GetValue(Operand operand) {

   if (operand.SymbolRef) return operand.SymbolRef->Value;
   return operand.LiteralValue;
}

/* *************************************************** */
/* Evaluate expression                                 */

long DoExpression() {

OprId  operator;
Operand left,       right,      temp;
long    left_value, right_value;

static Operand  true_operand = {NULL, 0xFFFFFFFF};
static Operand false_operand = {NULL, 0x00000000};
static Operand  zero_operand = {NULL, 0x00000000};

   while (1) {

      if (!PopOperator(&operator)) return 0;

      switch (operator) {

      case OprNE:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 if (left_value != right_value)
	    PushOperand(&true_operand);
	 else
	    PushOperand(&false_operand);
      break;

      case OprEQ:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 if (left_value == right_value)
	    PushOperand(&true_operand);
	 else
	    PushOperand(&false_operand);
      break;

      case OprGT:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 if (left_value > right_value)
	    PushOperand(&true_operand);
	 else
	    PushOperand(&false_operand);
      break;

      case OprGE:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 if (left_value >= right_value)
	    PushOperand(&true_operand);
	 else
	    PushOperand(&false_operand);
      break;

      case OprLT:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 if (left_value < right_value)
	    PushOperand(&true_operand);
	 else
	    PushOperand(&false_operand);
      break;

      case OprLE:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 if (left_value <= right_value)
	    PushOperand(&true_operand);
	 else
	    PushOperand(&false_operand);
      break;

      case OprPL:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 temp = zero_operand;
	 temp.LiteralValue = left_value + right_value;
	 PushOperand(&temp);
      break;

      case OprMI:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 temp = zero_operand;
	 temp.LiteralValue = left_value - right_value;
	 PushOperand(&temp);
      break;

      case OprTI:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 temp = zero_operand;
	 temp.LiteralValue = left_value * right_value;
	 PushOperand(&temp);
      break;

      case OprDI:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 temp = zero_operand;
	 temp.LiteralValue = left_value / right_value;
	 PushOperand(&temp);
      break;

      case OprAND:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 temp = zero_operand;
	 temp.LiteralValue = left_value & right_value;
	 PushOperand(&temp);
      break;

      case OprOR:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 temp = zero_operand;
	 temp.LiteralValue = left_value | right_value;
	 PushOperand(&temp);
      break;

      case OprXOR:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 temp = zero_operand;
	 temp.LiteralValue = left_value ^ right_value;
	 PushOperand(&temp);
      break;

      case OprNOT:
	 if (!PopOperand(&left)) return 0;
	 left_value = GetValue(left);
	 temp = zero_operand;
	 temp.LiteralValue = ~left_value;
	 PushOperand(&temp);
      break;

      case OprNEG:
	 if (!PopOperand(&left)) return 0;
	 left_value = GetValue(left);
	 temp = zero_operand;
	 temp.LiteralValue = -left_value;
	 PushOperand(&temp);
      break;

      case OprLSH:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 temp = zero_operand;
	 temp.LiteralValue = left_value << right_value;
	 PushOperand(&temp);
      break;

      case OprRSH:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 temp = zero_operand;
	 temp.LiteralValue = left_value >> right_value;
	 PushOperand(&temp);
      break;

      case OprINC:
	 if (!PopOperand(&left)) return 0;
	 left_value = GetValue(left);
	 temp = zero_operand;
	 temp.LiteralValue = left_value +1;
	 if (left.SymbolRef) {
	    left.SymbolRef->Value = temp.LiteralValue;
	    PushOperand(&temp);
	 } else
	    Error(RUN_TIME,NULL,"Increment: Attempt to write to a literal");
      break;

      case OprDECR:
	 if (!PopOperand(&left)) return 0;
	 left_value = GetValue(left);
	 temp = zero_operand;
	 temp.LiteralValue = left_value -1;
	 if (left.SymbolRef) {
	    left.SymbolRef->Value = temp.LiteralValue;
	    PushOperand(&temp);
	 } else
	    Error(RUN_TIME,NULL,"Decrement: Attempt to write to a literal");
      break;

      case OprPOP:
	 if (!PopOperand(&right)) return 0;
	 return (right.LiteralValue);
      break;

      case OprAS:
	 if (!PopOperand(&right)) return 0;
	 if (!PopOperand(&left )) return 0;
	 left_value = GetValue(left);
	 right_value = GetValue(right);
	 left.LiteralValue = right_value;
	 if (left.SymbolRef) {
	    left.SymbolRef->Value = right_value;
	    left.SymbolRef->Defined = 1;
	    PushOperand(&right);
	 } else
	    Error(RUN_TIME,NULL,"Becomes Equal: Attempt to write to a literal");
      break;

      case OprSTM:
	 return operandStack[operandStackSize].LiteralValue;

      default:
	 return 0;
      }
   }
}

/* *************************************************** */
/* Compare atom types                                  */

int CompareAtomTypes(AtomType x, AtomType y) {

   if (x == y) return 1;

   if ((x == Not_set) || (y == Not_set)) return 1;

   if (x == Expression)
      if ((y == Numeric) || (y == Alpha)) return 1;

   if (y == Expression)
      if ((x == Numeric) || (x == Alpha)) return 1;

   if ((x == Seperator) && (y == Comment)) return 1;

   if ((y == Seperator) && (x == Comment)) return 1;

   return 0;
}

/* *************************************************** */
/* Check syntax is what we expect                      */

long GetConfirm(Atom   *atom,
		OpCode *opcd,
		int    *expect_sr1,
		int    *expect_sr2,
		int    *expect_lit,
		int    *expect_dst) {


AtomType  a;
char     *expct;

   *expect_sr1 = 0;
   *expect_sr2 = 0;
   *expect_lit = 0;
   *expect_dst = 0;

   if (opcd->Src1 == AtLiteral ) *expect_lit = 1;
   if (opcd->Src1 == AtAddress ) *expect_lit = 1;
   if (opcd->Src1 == AtRegister) *expect_sr1 = 1;
   if (opcd->Src2 == AtRegister) *expect_sr2 = 1;
   if (opcd->Dest == AtRegister) *expect_dst = 1;

   expct = NULL;

   if (atom->Next) atom = (Atom *) atom->Next;
   while (CompareAtomTypes(atom->Type,Seperator)) {
      if (atom->Next) atom = (Atom *) atom->Next;
      else break;
   }

   if (*expect_lit) {
      a = Expression;
      if (CompareAtomTypes(atom->Type,a) == 0) expct = "Expected: Symbol or literal value";
      else do {
	 if (atom->Next) atom = (Atom *) atom->Next;
	 else break;
      } while (CompareAtomTypes(atom->Type,Seperator));
   }

   if (*expect_sr1) {
      a = Register;
      if (CompareAtomTypes(atom->Type,a) == 0) expct = "Expected: Register";
      else do {
	 if (atom->Next) atom = (Atom *) atom->Next;
	 else break;
      } while (CompareAtomTypes(atom->Type,Seperator));
   }

   if (*expect_sr2) {
      a = Register;
      if (CompareAtomTypes(atom->Type,a) == 0) expct = "Expected: Register";
      else do {
	 if (atom->Next) atom = (Atom *) atom->Next;
	 else break;
      } while (CompareAtomTypes(atom->Type,Seperator));
   }

   if (*expect_dst) {
      a = Register;
      if (CompareAtomTypes(atom->Type,a) == 0) expct = "Expected: Register";
      else do {
	 if (atom->Next) atom = (Atom *) atom->Next;
	 else break;
      } while (CompareAtomTypes(atom->Type,Seperator));
   }

   if (expct) {
      Error(SYNTAX,atom,expct);
      return 0;
   }
   return 1;
}

/**************************************************************************/
/* Compile source file into object code                                   */
/**************************************************************************/

typedef struct {
   Symbol *Symb;
   Instruction *Inst;
 } ForwardReference;

#define FORWARD_REFS 100

static int fwix = 0;
static ForwardReference forward[FORWARD_REFS];

/* ============================ */

long Assemble(char *text, ProgramBuf *code, int listing) {

Atom *atom, *patom;
AtomType at;
Symbol *symbol;
Operand operand;
OprId oprId;
Instruction *inst;
OpCode *opcd;
int index, pc, i;

unsigned int expect_lit = 0;
unsigned int expect_sr1 = 0;
unsigned int expect_sr2 = 0;
unsigned int expect_dst = 0;

char *cp;

   atom = GetAtoms(text);
   if (atom == NULL) {
      Error(FATAL_SYNTAX,NULL,"Null buffer, or garbage source code");
      return error_count;
   }

   if (debug) ListAtoms(atom);

   pc = 0;
   index = 0;
   inst = &(code->Program[index]);
   bzero((void *) inst, sizeof(Instruction));

   printf("\nLn:%03d\t\t",(int) atom->LineNumber);

   while (atom) {
      at = atom->Type;
      switch (at) {
	 case Numeric:
	    operand.SymbolRef = NULL;
	    operand.LiteralValue = atom->Value;
	    if (expect_lit) {
	       inst->Src1 = atom->Value;
	       expect_lit = 0;
	    } else
	       if (!PushOperand(&operand)) return error_count;
	    if (listing) printf(" 0x%x",(unsigned int) atom->Value);
	 break;

	 case Alpha:
	    symbol = GetSymbol(atom->Text);
	    operand.SymbolRef = symbol;
	    operand.LiteralValue = 0;
	    if (expect_lit) {
	       inst->Src1 = symbol->Value;
	       expect_lit = 0;
	       if (inst->Src1 == 0) {
		  forward[fwix].Inst = inst;
		  forward[fwix].Symb = symbol;
		  if (++fwix >= FORWARD_REFS)
		     Error(FATAL_RUN_TIME,atom,"Too many forward references\n");
	       }
	    } else if (!PushOperand(&operand)) return error_count;

	    if (listing) printf(" %s",atom->Text);
	 break;

	 case Operator:
	    oprId = atom->Value;
	    if (oprId == OprLBL) {
	       if (patom->Type == Alpha) {
		  symbol = GetSymbol(patom->Text);
		  if (symbol->Value) {
		     atom->Text = patom->Text;
		     Error(SYNTAX,atom,"Label: double definition");
		  }
		  symbol->Value = pc;
		  symbol->Defined = 1;
	       } else if (patom->Type == Numeric) {
		  if (!pc) {
		     pc = patom->Value;
		     code->LoadAddress = pc;
		  } else {
		     Error(SYNTAX,atom,"Code block must be contiguous, one start address only");
		  }
	       }
	    } else if (!PushOperator(&oprId)) return error_count;

	    if (listing) {
	       printf(" %s",OprNames[oprId].Name);
	       if (oprId == OprLBL)
		  printf("\n         Pc:%04X\t",pc);
	   }
	 break;

	 case Op_code:
	    inst = &(code->Program[index]);
	    bzero((void *) inst, sizeof(Instruction));

	    opcd = &(InstructionSet[atom->Value]);
	    GetConfirm(atom,opcd,&expect_sr1,&expect_sr2,&expect_lit,&expect_dst);
	    inst->Number = opcd->Number;
	    if (listing) printf("\t\t%s",opcd->Name);
	    pc++; index++;
	    code->InstructionCount = index;
	 break;

	 case Register:
	     if      (expect_sr1) { inst->Src1 = atom->Value; expect_sr1 = 0; }
	     else if (expect_sr2) { inst->Src2 = atom->Value; expect_sr2 = 0; }
	     else if (expect_dst) { inst->Dest = atom->Value; expect_dst = 0; }
	     if (listing) printf(" %s",GetRegisterName(atom->Value));
	 break;

	 case Open:
	 case Open_index:
	    oprId = OprSTM;
	    if (!PushOperator(&oprId)) return error_count;
	    expect_lit = 0;
	    if (listing) printf("(");
	 break;

	 case Close:
	 case Close_index:
	    inst->Src1 = DoExpression();
	    expect_lit = 0;
	    if (listing) printf(" )");
	 break;

	 case Seperator:
	    ResetStacks();
	    if (expect_lit) {
	       Error(SYNTAX,atom,"Unexpected seperator");
	       expect_lit = 0;
	    }
	    if (listing) {
	       cp = "%";
	       if ((atom->Text) && strlen(atom->Text)) printf("\t %s %s %s",cp,atom->Text,cp);
	       if (atom->LineNumber) {
		  if (pc) printf("\nLn:%03d - Pc:%04X\t",(int) atom->LineNumber +1,pc);
		  else    printf("\nLn:%03d\t\t",(int) atom->LineNumber +1);
	       }
	    }
	 break;

	 case Comment:
	    cp = "%";
	    if (listing && atom->Text) printf("\t%s %s %s",cp,atom->Text,cp);
	 break;

	 default: break;
      }
      patom = atom;
      atom = (Atom *) atom->Next;
   }
   if (listing) {
      printf("\n\t\t---Symbol Table---\n\n");
      for (i=0; i<symbolTableSize; i++) {
	 if (symbolTable[i].Defined) {
	    printf("%s\t= %d \t[0x%x]\n",
		   symbolTable[i].Name,(int) symbolTable[i].Value,(int) symbolTable[i].Value);
	 } else {
	    printf("%s\t*** UNDEFINED SYMBOL ***\n",
		   symbolTable[i].Name);
	 }
      }
      printf("\n\t\t---Symbol Table Size: %d total---\n\n",(int) symbolTableSize);
   }
   return error_count;
}

/**************************************************************************/
/* Disassemble program back into source                                   */
/**************************************************************************/

int DisAssemble(ProgramBuf *code) {

Instruction *inst;
int i, j, pc, fnd, adr;
OpCode *opcd;
char oln[128], tmp[128];

   bzero((void *) oln, 128);
   strcat(oln,"       %");
   strcat(oln,TabTo(oln,16));
   strcat(oln,"Src-1");
   strcat(oln,TabTo(oln,28));
   strcat(oln,"Src-2");
   strcat(oln,TabTo(oln,36));
   strcat(oln,"Dest");
   printf("%s\n",oln);

   pc  = 0;
   adr = code->LoadAddress;
   for (i=0; i<code->InstructionCount; i++) {
      inst = &(code->Program[i]);

      fnd = 0;
      for (j=0; j<OPCODES; j++) {
	 opcd = &(InstructionSet[j]);
	 if (opcd->Number == inst->Number) {
	    fnd = 1;
	    break;
	 }
      }

      if (fnd) {
	 bzero((void *) oln, 128);
	 bzero((void *) tmp, 128);

	 sprintf(oln,"%04X:%03X %s",adr,pc,opcd->Name);
	 strcat(oln,TabTo(oln,16));

	 switch (opcd->Src1) {
	    case AtNothing:
	    break;

	    case AtLiteral: sprintf(tmp,"0x%X"  ,(int) inst->Src1);
	    break;

	    case AtAddress: sprintf(tmp,"0x%X" ,(int) inst->Src1);
	    break;

	    case AtRegister: sprintf(tmp,"%s",GetRegisterName(inst->Src1));
	    break;

	    default: sprintf(tmp,"???");
	 }
	 strcat(oln,tmp);
	 bzero((void *) tmp, 128);
	 strcat(oln,TabTo(oln,28));
	 switch (opcd->Src2) {
	    case AtNothing:
	    break;

	    case AtLiteral: sprintf(tmp,"0x%X"  ,(int) inst->Src2);
	    break;

	    case AtAddress: sprintf(tmp,"0x%X" ,(int) inst->Src2);
	    break;

	    case AtRegister: sprintf(tmp,"%s",GetRegisterName(inst->Src2));
	    break;

	    default: sprintf(tmp,"???");
	 }
	 strcat(oln,tmp);
	 bzero((void *) tmp, 128);
	 strcat(oln,TabTo(oln,36));
	 switch (opcd->Dest) {
	    case AtNothing:
	    break;

	    case AtLiteral: sprintf(tmp,"0x%X"  ,(int) inst->Dest);
	    break;

	    case AtAddress: sprintf(tmp,"0x%X" ,(int) inst->Dest);
	    break;

	    case AtRegister: sprintf(tmp,"%s",GetRegisterName(inst->Dest));
	    break;

	    default: sprintf(tmp,"???");
	 }
	 strcat(oln,tmp);
	 strcat(oln,TabTo(oln,44));
	 strcat(oln,"% ");
	 strcat(oln,opcd->Comment);
	 printf("%s\n",oln);
	 pc++; adr++;
      } else printf("Garbage object file\n");
   }
   return 0;
}

/**************************************************************************/
/* Get file name extension                                                */
/**************************************************************************/

typedef enum {ASM,OBJ,INVALID} FileType;

static char *extNames[] = {".ass",".obj"};

FileType GetFileType(char *path) {

int i, j;
char c, *cp, ext[5];

   for (i=strlen(path)-1; i>=0; i--) {
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
/* Print help text                                                        */
/**************************************************************************/

void Help() {

   printf("To assemble a source file: asm <source>.ass <object>.obj\n");
   printf("Dissasemble to stdout:     asm <object>.obj\n");
   exit(0);
}

/**************************************************************************/
/* Write object code binary to file                                       */
/**************************************************************************/

int WriteObject(ProgramBuf *code, FILE *objFile) {

Instruction *inst;
int i;

   fprintf(objFile,"0x%X 0x%X\n",
	   (int) code->LoadAddress,
	   (int) code->InstructionCount);

   for (i=0; i<code->InstructionCount; i++) {
      inst = &(code->Program[i]);
      fprintf(objFile,"0x%X 0x%X 0x%X 0x%X 0x%X\n",
	      (int) inst->Number,
	      (int) inst->Src1,
	      (int) inst->Src2,
	      (int) inst->Dest,
	      (int) inst->Crc);

   }
   return 0;
}

/**************************************************************************/
/* Read object code binary from file                                      */
/**************************************************************************/

int ReadObject(FILE *objFile, ProgramBuf *code) {

Instruction *inst;
int i;
char ln[128], *cp, *ep;

   if (fgets(ln,128,objFile)) {
      cp = ln;
      code->LoadAddress      = strtoul(cp,&ep,0);
      cp = ep;
      code->InstructionCount = strtoul(cp,&ep,0);
   }
   inst = (Instruction *) malloc(sizeof(Instruction) * code->InstructionCount);
   code->Program = inst;

   for (i=0; i<code->InstructionCount; i++) {
      inst = &(code->Program[i]);
      bzero((void *) ln, 128);
      if (fgets(ln,128,objFile)) {
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
/* Main program                                                           */
/**************************************************************************/

#define FN_SIZE 80

int main(int argc,char *argv[]) {

int assemble = 0;
int i, sourceSize, sourceLines;
char *cp, c, *source;
char ass[FN_SIZE], obj[FN_SIZE];
Instruction *object;
ProgramBuf prog;

FileType ft = INVALID;

FILE *objFile = NULL;
FILE *assFile = NULL;

   bzero(ass,FN_SIZE);
   bzero(obj,FN_SIZE);

   printf("\nasm: Version: %s %s\n",__DATE__,__TIME__);

   if (argc < 2) Help();

   for (i=1; i<argc; i++) {
      ft = GetFileType(argv[i]);
      switch (ft) {
	 case INVALID:
	    fprintf(stderr,"asm: Invalid file extension: %s\n",argv[i]);
	    Help();
	 break;

	 case ASM:
	    strcpy(ass,argv[i]);
	    if (i==1) {
	       assemble = 1;
	       if (stat(ass,&assStat) == -1) {
		  fprintf(stderr,"asm: Can't get file statistics: %s for read\n",ass);
		  perror("asm");
		  exit(1);
	       }
	       if (assStat.st_size < 2) {
		  fprintf(stderr,"asm: File: %s is empty\n",ass);
		  exit(1);
	       }
	       assFile = fopen(ass,"r");
	       if (assFile == NULL) {
		  fprintf(stderr,"asm: Can't open: %s for read\n",ass);
		  perror("asm");
		  exit(1);
	       }
	       break;
	    }
	    fprintf(stderr,"asm: Source file: %s must be supplied as first argument\n",argv[i]);
	    Help();
	 break;

	 case OBJ:
	    strcpy(obj,argv[i]);
	    if (i==1) {
	       assemble = 2;
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
	    } else {
	       objFile = fopen(obj,"w");
	       if (objFile == NULL) {
		  fprintf(stderr,"asm: Can't open: %s for write\n",obj);
		  perror("asm");
		  exit(1);
	       }
	    }
	 break;
      }
   }

   if (assemble == 0) {
      fprintf(stderr,"asm: No source or object file supplied\n");
      Help();
   }

   umask(0);

   /* =================================================== */
   /* Assemble source                                     */

   if (assemble == 1) {
      if (objFile == NULL) {
	 strcpy(obj,ass);
	 for (i=strlen(obj)-1; i>=0; i--) {
	    cp = &obj[i]; c = *cp;
	    if (c == '.') {
	       strcpy(cp,extNames[OBJ]);
	       objFile = fopen(obj,"w");
	       if (objFile == NULL) {
		  fprintf(stderr,"asm: Can't open: %s for write\n",obj);
		  exit(1);
	       }
	       break;
	    }
	 }
      }

      printf("\nasm: Assemble Source: %s (%d bytes) Object: %s\n",
	     ass,(int) assStat.st_size, obj);

      sourceSize = assStat.st_size + 16;
      source = (char *) malloc(sourceSize);
      if (source == NULL) {
	 fprintf(stderr,"asm: Not enough memory to run the assembler\n");
	 exit(1);
      }
      bzero((void *) source, sourceSize);
      sourceLines = 0;
      for (i=0; i<sourceSize; i++) {
	 c = fgetc(assFile);
	 if (c == '\n') sourceLines++;
	 if ((c == (char) EOF) || (c == (char) 0)) break;
	 source[i] = c;
      }
      printf("asm: Pass 1: %d characters %d lines read from: %s\n",i,sourceLines,ass);
      object = (Instruction *) malloc(sizeof(Instruction) * sourceLines);
      if (source == NULL) {
	 fprintf(stderr,"asm: Not enough memory to hold object\n");
	 exit(1);
      }
      bzero((void *) &prog,sizeof(ProgramBuf));
      prog.Program = object;

      if (!Assemble(source, &prog, 0)) {
	 if (fwix) {
	    printf("asm: Pass 2: Process: %d forward references\n",fwix);
	    for (i=0; i<fwix; i++) forward[i].Inst->Src1 = forward[i].Symb->Value;
	 }
	 printf("asm: %s Compiled OK\n\n",ass);
      } else
	 printf("asm: %s [%d] Compilation errors detected\n\n",ass,error_count);
      fclose(assFile);

      WriteObject(&prog,objFile);
      fclose(objFile);

   /* =================================================== */
   /* Disassemble object                                  */

   } else {
      printf("\nasm: Dissasemble Source Object: %s\n\n", obj);

      ReadObject(objFile,&prog);
      fclose(objFile);
      DisAssemble(&prog);
      printf("\nasm: %d Instructions dissasembled\n\n",(int) prog.InstructionCount);
   }
   exit(0);
}
