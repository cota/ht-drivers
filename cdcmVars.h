/**
 * @file cdcmVars.h
 *
 * @brief An easy way to declare my variables in a header file.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 18/12/2007
 *
 * Inspired by http://www.keil.com/support/docs/1868.htm. Thxs!
 *
 * @version $Id: cdcmVars.h,v 1.2 2009/01/09 10:26:03 ygeorgie Exp $
 */
#ifndef _CDCM_VARS_H_INCLUDE_
#define _CDCM_VARS_H_INCLUDE_

/*----------------------------------------------
Setup variable declaration macros.
----------------------------------------------*/
#ifndef CDCM_VAR_DECLS
# define _DECL extern
# define _INIT(x)
#else
# define _DECL
# define _INIT(x)  = x
#endif

/*----------------------------------------------
Declare variables as follows:

_DECL [standard variable declaration] _INIT(x);

where x is the value with which to initialize
the variable.  If there is no initial value for
the varialbe, it may be declared as follows:

_DECL [standard variable declaration];

HOWTO use:
#define CDCM_VAR_DECLS causes the cdcmVars.h include file to 
actually declare and initialize the global variables.

In all other source files (which use these variables) simply
include the cdcmVars.h header file.

Be sure that CDCM_VAR_DECLS is not defined in these other files
or else your variables will be declared twice.
----------------------------------------------*/

#endif /* _CDCM_VARS_H_INCLUDE_ */
