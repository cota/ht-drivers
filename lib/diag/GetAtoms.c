/**************************************************************************/
/* String to upper case                                                   */
/**************************************************************************/

void StrToUpper(inp,out)
char *inp;
char *out; {

int        i;
AtomType   at;
char      *cp;

   for (i=0; i<strlen(inp); i++) {
      cp = &(inp[i]);
      at = atomhash[(int) (*cp)];
      if ((at == Alpha) && (*cp >= 'a')) *cp -= ' ';
      out[i] = *cp;
   }
   out[i] = 0;
}

/**************************************************************************/
/* Chop up a command line into atom values.                               */
/**************************************************************************/

void GetAtoms(char *buf) {

int        i,   nb;
char      *cp, *ep;
AtomType   at;
CmdId      cid;
OprId      oid;


   /* Repeat the last command on blank command buffer */

   if ((       buf  == NULL)
   ||  (      *buf  == '.')) return;

   /* Start a new command */

   pcnt = 0;
   bzero((void *) &(vals[pcnt]),sizeof(ArgVal));

   /* Break input string up into atoms */

   cp = &buf[0];

   while(True) {
      at = atomhash[(int) (*cp)];
      vals[pcnt].Pos = (unsigned int) (cp - &buf[0]);

      switch (at) {
    
      /* Numeric atom types have their value set to the value of   */
      /* the text string. The number of bytes field corresponds to */
      /* the number of characters in the number. */
    
      case Numeric:
	 vals[pcnt].Number = (int) strtoul(cp,&ep,0);
	 vals[pcnt].Type = Numeric;
	 cp = ep;
	 pcnt++;
	 break;
    
      /* Alpha atom types are strings of letters and numbers starting with */
      /* a letter. The text of the atom is placed in the text field of the */
      /* atom. The number of bytes field is the number of characters in    */
      /* the name. Two special alpha strings "if" and "do", are converted  */
      /* into special atom types. */
    
      case Alpha:
	 nb = 0;
	 while (at == Alpha) {
	    vals[pcnt].Text[nb] = *cp;
	    nb++; cp++;
	    if (nb >= MAX_ARG_LENGTH) break;
	    at = atomhash[(int) (*cp)];
	 }
	 vals[pcnt].Text[nb] = 0;       /* Zero terminate string */
	 vals[pcnt].Type = Alpha;
	 for (i=0; i<CmdCMDS; i++) {
	    cid = cmds[i].Id;
	    if (strcmp(cmds[i].Name,vals[pcnt].Text) == 0)
	       vals[pcnt].CId = cid;
	 }
	 pcnt++;
	 break;
    
      /* Seperators can be as long as they like, the only interesting */
      /* data about them is their byte counts. */
    
      case Seperator:
	 while (at == Seperator ) at = atomhash[(int) (*(++cp))];
	 break;
    
      /* Comments are enclosed between a pair of percent marks, the only */
      /* character not allowed in a comment is a terminator, just incase */
      /* someone has forgotten to end one. */
    
      case Comment:
	 at = atomhash[(int) (*(++cp))];
	 while (at != Comment ) {
	    at = atomhash[(int) (*(cp++))];
	    if (at == Terminator) return;
	 }
	 break;
    
      /* Operator atoms have the atom value set to indicate which operator */
      /* the atom is. The text and number of byte fields are also set. */
    
      case Operator:
	 nb = 0;
	 while (at == Operator) {
	    vals[pcnt].Text[nb] = *cp;
	    nb++; cp++;
	    if (nb >= MAX_ARG_LENGTH) break;
	    at = atomhash[(int) *cp];
	 }
	 vals[pcnt].Text[nb] = 0;       /* Zero terminate string */
	 vals[pcnt].Type = Operator;
	 for (i=0; i<OprOPRS; i++) {
	    oid = oprs[i].Id;
	    if (strcmp(oprs[oid].Name,vals[pcnt].Text) == 0)
	       vals[pcnt].OId = oid;
	 }
	 pcnt++;
	 break;
    
      /* These are simple single byte operators */
    
      case Open:              /* FIDO start block atom */
      case Close:             /* FIDO end block atom */
      case Open_index:        /* Start Object name block */
      case Close_index:       /* End of Object name block */
      case Bit:               /* Bit number specifier */
	 vals[pcnt].Type = at;
	 cp++; pcnt++;
	 break;

      default:
	 vals[pcnt].Type = at;
	 cp++; pcnt++;
	 return;
      }
   }
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
