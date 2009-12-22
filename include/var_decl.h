/**
 * @file var_decl.h
 *
 * @brief An easy way to declare variables in a header file.
 *
 * @author Georgievskiy Yury. CERN AB/CO.
 *
 * @date Dec, 2007
 *
 * Inspired by http://www.keil.com/support/docs/1868.htm. Thxs!
 *
 * @version 1.0  ygeorgie  18/12/2007  Initial version.
 */
#ifndef _VAR_DECL_H_INCLUDE_
#define _VAR_DECL_H_INCLUDE_

/*----------------------------------------------
Setup variable declaration macros.
----------------------------------------------*/
#ifndef VAR_DECLS
# define _DECL extern
# define _INIT(x)
#else
# define _DECL
# define _INIT(x) = x
#endif

/*----------------------------------------------
Declare variables as follows:

_DECL [standard variable declaration] _INIT(x);

where x is the value with which to initialize
the variable.  If there is no initial value for
the varialbe, it may be declared as follows:

_DECL [standard variable declaration];

HOWTO use:
#define VAR_DECLS causes the var_decl.h include file to
actually declare and initialize the global variables.

In all other source files (which use these variables) simply
include the var_decl.h header file.

Be sure that VAR_DECLS is not defined in these other files
or else your variables will be declared twice.
----------------------------------------------*/

#endif /* _VAR_DECL_H_INCLUDE_ */
