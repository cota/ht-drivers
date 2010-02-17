/**************************************************************************/
/* Do the command at arg, count times                                     */
/* Returns end arg index                                                  */
/**************************************************************************/

int DoCmd (int arg) {

ArgVal* v;
int count, endarg, i;
AtomType at;

   count = 1;
   while (True) {
      v = &(vals[arg]);
      at = v->Type;

      switch (at) {
	 case Numeric:
	    count = v->Number;
	    arg++;
	    break;

	 case Open:
	    arg++;
	    for (i=0; i<count; i++)
	       endarg = DoCmd(arg);
	    arg = endarg;
	    break;

	 case Alpha:
	    arg = cmds[v->CId].Proc(arg);
	    break;

	 case Close:
	    arg++;
	    return(arg);

	 case Terminator:
	    return(arg);

	 default:
	    arg++;
	    return(arg);
      }
   }
}
