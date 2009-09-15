/**
 * @file libinst.h
 *
 * @brief Installer library public definitions
 *
 * @author Copyright (C) 2009 CERN CO/HT Julian Lewis
 *
 * @date Created on 26/01/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#ifndef _LIB_INST_H_INCLUDE_
#define _LIB_INST_H_INCLUDE_

#include <string.h>
#include <config_data.h>

extern int InsLibErrorCount;	/**< Counter for errors */

/** @defgroup xmllib Library routines
 *@{
 */
InsLibHostDesc *InsLibParseInstallFile(char *, int);
InsLibDrvrDesc *InsLibGetDriver(InsLibHostDesc *, char *);
InsLibModlDesc *InsLibGetModule(InsLibDrvrDesc *, int);
void            InsLibFreeCarSpace(InsLibCarAddressSpace *);
void            InsLibFreeVmeSpace(InsLibVmeAddressSpace *);
void            InsLibFreePciSpace(InsLibPciAddressSpace *);
void            InsLibFreeCar(InsLibCarModuleAddress *);
void            InsLibFreeVme(InsLibVmeModuleAddress *);
void            InsLibFreePci(InsLibPciModuleAddress *);
void            InsLibFreeModule(InsLibModlDesc *);
void            InsLibFreeDriver(InsLibDrvrDesc *);
void            InsLibFreeHost(InsLibHostDesc *);
void            InsLibPrintCarSpace(InsLibCarAddressSpace *);
void            InsLibPrintVmeSpace(InsLibVmeAddressSpace *);
void            InsLibPrintPciSpace(InsLibPciAddressSpace *);
void            InsLibPrintCar(InsLibCarModuleAddress *);
void            InsLibPrintVme(InsLibVmeModuleAddress *);
void            InsLibPrintPci(InsLibPciModuleAddress *);
void            InsLibPrintModule(InsLibModlDesc *);
void            InsLibPrintDriver(InsLibDrvrDesc *);
void            InsLibPrintHost(InsLibHostDesc *);
/*@} end of group*/


#endif	/* _LIB_INST_H_INCLUDE_ */
