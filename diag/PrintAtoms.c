/*******************************************************************/
/* Print out the command line atome in vals array                  */
/*******************************************************************/

void PrintAtoms() {
int i;
AtomType at;
CmdId cid;
OprId oid;

   printf("Arg list: %d atoms...\n",pcnt);
   for (i=0; i<pcnt; i++) {

      at = vals[i].Type;
      switch (at) {

      case Numeric:
	 printf("Arg: %2d Num: %d\n",i+1,vals[i].Number);
	 break;

      case Alpha:
	 cid = vals[i].CId;
	 if (cid)
	    printf("Cmd: %2d %s %s\n",cid,cmds[cid].Name,cmds[cid].Help);
	 else
	    printf("Arg: %2d Alp: %s\n",i+1,vals[i].Text);
	 break;

      case Operator:
	 oid = vals[i].OId;
	 if (oid)
	    printf("Opr: %2d %s %s\n",oid,oprs[oid].Name,oprs[oid].Help);
	 else
	    printf("Arg: %2d Opr: %s\n",i+1,vals[i].Text);
	 break;

      case Open:              /* FIDO start block atom */
	 printf("Arg: %2d Opn: (\n",i+1);
	 break;

      case Close:             /* FIDO end block atom */
	 printf("Arg: %2d Cls: )\n",i+1);
	 break;

      case Open_index:        /* Start Object name block */
	 printf("Arg: %2d Opn: [\n",i+1);
	 break;

      case Close_index:       /* End of Object name block */
	 printf("Arg: %2d Cls: ]\n",i+1);
	 break;

      case Bit:               /* Bit number specifier */
	 printf("Arg: %2d Bit: .\n",i+1);
	 break;

      case Terminator:        /* End of program */
	 printf("Arg: %2d End: @\n",i+1);
	 break;

      default:
	 printf("Arg: %2d ???\n",i+1);
      };
   };
}
