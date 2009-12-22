/**
 * @file skeldrvr.c
 *
 * @brief Skel driver framework
 *
 * This framework implements a standard SKEL compliant driver CAR support.
 *
 * Copyright (c) 2008-09 CERN
 * @author Julian Lewis <julian.lewis@cern.ch>
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifdef CONFIG_BUS_CAR

static void RemoveCarModule(SkelDrvrModuleContext *mcon)
{
	InsLibCarModuleAddress *cma;
	InsLibCarAddressSpace  *cas;

	if (!(mcon->StandardStatus & SkelDrvrStandardStatusNO_ISR)) {
		; /* @todo Free carrier ISR */
		mcon->StandardStatus |= SkelDrvrStandardStatusNO_ISR;
	}

	cma = mcon->Modld->ModuleAddress;
	if (!cma)
		return;

	cas = cma->CarAddressSpace;
	while (cas) {
		if (cas->Mapped) {
			; /* @todo unmap carrier board address space */
			cas->Mapped = NULL;
		}
		cas = cas->Next;
	}
}

static int AddCarModule(SkelDrvrModuleContext *mcon)
{
	return 0;
}

#else

static void RemoveCarModule(SkelDrvrModuleContext *mcon) {
   return;
}

static int AddCarModule(SkelDrvrModuleContext *mcon) {
	return 0;
}

#endif
