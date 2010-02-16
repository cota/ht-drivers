/*****************************************************************/
/* Command routines, one per command. They get an index into the */
/* vlaues array as argument. They process their arguments and    */
/* return the new argumnet index.                                */
/* STANDARD COMMANDS ONLY                                        */
/*****************************************************************/

/*****************************************************************/
/* Illegal command routine is called for garbage commands        */
/*****************************************************************/

int Illegal(arg) {
   printf("%s --> %s\n",vals[arg].Text,cmds[CmdNOCM].Help);
   return(pcnt-1);
}

/*****************************************************************/
/* Quit program                                                  */
/*****************************************************************/

int Quit(int arg) {
   printf("\nQuit\n");
   exit(0);
   return(arg+1);
}

/*****************************************************************/
/* Print help texts Optional opr argument                        */
/*****************************************************************/

int Help(int arg) {
int       i;
ArgVal   *v;
AtomType  at;
char     *cp;

   v = &(vals[++arg]);
   at = v->Type;
   cp = cmds[CmdHELP].Optns;

   if ((at == Alpha) && (strcmp(v->Text,cp) == 0)) {
      arg++;
      printf("\nOPERATORS:\n\n");
      for (i=1; i<OprOPRS; i++)
	 printf("Id: %2d Opr: %s \t--> %s\n",
		oprs[i].Id,oprs[i].Name,oprs[i].Help);
   } else {
      printf("\nCOMMANDS: Eg: qf 1, 10(wi c 500, (tgm 2)), rst\n\n");
      for (i=1; i<CmdCMDS; i++)
	    printf("Id: %2d Cmd:\t%s\t[%s]\t--> %s\n",
		   cmds[i].Id,cmds[i].Name,cmds[i].Optns,cmds[i].Help);
      printf(" ?:    Get more help/options etc\n");
   }
   return(arg);
}

/*****************************************************************/
/* Sleep in seconds                                              */
/*****************************************************************/

int Sleep(int arg) {
ArgVal   *v;
AtomType  at;

   v = &(vals[++arg]);
   at = v->Type;

   if (at == Numeric) {
      arg++;
      sleep(v->Number);
   }
   putchar('\7'); fflush(stdout);
   return(arg);
}

/*****************************************************************/
/* History                                                       */
/*****************************************************************/

int History(int arg) {

int i;

   i=arg++;

#ifdef HISTORIES
   for (i=0; i<HISTORIES; i++) {
      printf("Cmd[%02d] {%s} ",i+1,history[i]);
      if (i == cmdindx) printf("<--");
      printf("\n");
   }
#endif

   return(arg);
}

/*****************************************************************/
/* Puse for keyboard input                                       */
/*****************************************************************/

int Pause(int arg) {

   arg++;
   printf("Pause: <CR>:\7"); fflush(stdout);
   getchar();
   return(arg);
}

/*****************************************************************/
/* Execute a shell command                                       */
/*****************************************************************/

int Shell(int arg) {

AtomType  at;
ArgVal   *v;
char     *cp, sys[32];
int       i;

   v = &(vals[arg]);
   at = v->Type;
   cp = &(cmdbuf[v->Pos]);

   while ((at != Terminator) && (at != Close)) {
      v = &(vals[++arg]);
      at = v->Type;
   }
   bzero(sys,32);
   for (i=0; i<32; i++) {
      if ((*cp == ')')
      ||  (*cp == 0) ) break;
      sys[i] = *cp++;
   }
   printf("sh: %s\n",sys);
   system(sys);
   return(arg);
}

/*****************************************************************/
/* Print atoms in command line for debugging a command syntax.   */
/*****************************************************************/

int Atoms(int arg) {

   PrintAtoms();
   return(arg+1);
}

