/**
 * @file libinstkernel.c
 *
 * @brief Clone the driver tree for drivers running in kernel space
 *
 * @author Copyright (C) 2009 CERN CO/HT Julian Lewis <julian.lewis@cern.ch>
 *
 * @date Created on 02/02/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#include "libinstkernel.h"

#ifdef __linux__
extern char* sysbrk(unsigned long);
extern void  sysfree(char *, unsigned long);
#endif

/* =============================================== */
/* Set the routine to the copy you want            */
/* Example: InsLibSetCopyRoutine(__copy_to_user)   */
/* Example: InsLibSetCopyRoutine(__copy_from_user) */
/* Example: InsLibSetCopyRoutine(memcpy)           */

static void default_copy(void *dest, const void *src, int n)
{
	int i;
	char *dp, *sp;

	dp = (char *)dest;
	sp = (char *)src;

	for (i = 0; i < n; i++)
		dp[i] = sp[i];
}

/* ================== */

static void (*copy_mem)(void *dest, const void *src, int n) = default_copy;

/* ================== */

/**
 * @brief Set default copy routine for the InstLib.
 *
 * @param routine - new copy routine.
 *
 * @return previous copy routine.
 */
void (*InsLibSetCopyRoutine(void (*routine)(void*, const void *, int)))
(void*, const void*, int)
{
	void (*prev)(void*, const void*, int) = copy_mem; /* save old one */
	if (routine) copy_mem = routine;
	else         copy_mem = default_copy;
	return prev;
}

/* ============================================== */

InsLibCarAddressSpace *InsLibCloneCarSpace(InsLibCarAddressSpace *caras)
{
	InsLibCarAddressSpace *caras_clone = NULL;

	if (caras) {
		caras_clone = (InsLibCarAddressSpace *)
			sysbrk(sizeof(InsLibCarAddressSpace));
		if (!caras_clone)
			return NULL;
		copy_mem((void *) caras_clone, (void *) caras,
			 sizeof(InsLibCarAddressSpace));

		caras_clone->Next = InsLibCloneCarSpace(caras->Next);
		return caras_clone;
	}
	return NULL;
}

/* ============================================== */

InsLibVmeAddressSpace *InsLibCloneVmeSpace(InsLibVmeAddressSpace *vmeas)
{
	InsLibVmeAddressSpace *vmeas_clone = NULL;

	if (vmeas) {
		vmeas_clone = (InsLibVmeAddressSpace *)
			sysbrk(sizeof(InsLibVmeAddressSpace));
		if (!vmeas_clone)
			return NULL;
		copy_mem((void *) vmeas_clone, (void *) vmeas,
			 sizeof(InsLibVmeAddressSpace));

		vmeas_clone->Next = InsLibCloneVmeSpace(vmeas->Next);
		return vmeas_clone;
	}
	return NULL;
}

/* ============================================== */

InsLibPciAddressSpace *InsLibClonePciSpace(InsLibPciAddressSpace *pcias)
{
	InsLibPciAddressSpace *pcias_clone = NULL;

	if (pcias) {
		pcias_clone = (InsLibPciAddressSpace *)
			sysbrk(sizeof(InsLibPciAddressSpace));
		if (!pcias_clone)
			return NULL;
		copy_mem((void *) pcias_clone, (void *) pcias,
			 sizeof(InsLibPciAddressSpace));

		pcias_clone->Next = InsLibClonePciSpace(pcias->Next);
		return pcias_clone;
	}
	return NULL;
}

/* ============================================== */

InsLibCarModuleAddress *InsLibCloneCar(InsLibCarModuleAddress *carma)
{
	InsLibCarModuleAddress *carma_clone = NULL;

	if (carma) {
		carma_clone = (InsLibCarModuleAddress *)
			sysbrk(sizeof(InsLibCarModuleAddress));
		if (!carma_clone)
			return NULL;
		copy_mem((void *) carma_clone, (void *) carma,
			 sizeof(InsLibCarModuleAddress));

		carma_clone->CarAddressSpace =
			InsLibCloneCarSpace(carma->CarAddressSpace);
		return carma_clone;
	}
	return NULL;
}


/**
 * @brief
 *
 * @param vmema -
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
InsLibVmeModuleAddress *InsLibCloneVme(InsLibVmeModuleAddress *vmema)
{
	InsLibVmeModuleAddress *vmema_clone = NULL;

	if (vmema) {
		vmema_clone = (InsLibVmeModuleAddress *)
			sysbrk(sizeof(InsLibVmeModuleAddress));
		if (!vmema_clone) return NULL;
		copy_mem((void *) vmema_clone, (void *) vmema,
			 sizeof(InsLibVmeModuleAddress));

		vmema_clone->VmeAddressSpace =
			InsLibCloneVmeSpace(vmema->VmeAddressSpace);
		return vmema_clone;
	}
	return NULL;
}


/* ============================================== */

InsLibPciModuleAddress *InsLibClonePci(InsLibPciModuleAddress *pcima)
{
	InsLibPciModuleAddress *pcima_clone = NULL;

	if (pcima) {
		pcima_clone = (InsLibPciModuleAddress *)
			sysbrk(sizeof(InsLibPciModuleAddress));
		if (!pcima_clone)
			return NULL;
		copy_mem((void *) pcima_clone, (void *) pcima,
			 sizeof(InsLibPciModuleAddress));

		pcima_clone->PciAddressSpace =
			InsLibClonePciSpace(pcima->PciAddressSpace);
		return pcima_clone;
	}
	return NULL;
}


/**
 * @brief Clone all memory for a module
 *
 * @param modld -
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
InsLibModlDesc *InsLibCloneModule(InsLibModlDesc *modld)
{
	InsLibModlDesc *modld_clone = NULL;
	int sz;

	if (modld) {
		modld_clone = (InsLibModlDesc *) sysbrk(sizeof(InsLibModlDesc));
		if (!modld_clone) {
			//pseterr(ENOMEM);
			printk("InsLib: not enough memory for modld_clone\n");
			return NULL;
		}
		copy_mem((void *) modld_clone, (void *) modld,
			 sizeof(InsLibModlDesc));

		if (modld->BusType == InsLibBusTypeCARRIER) {
			modld_clone->ModuleAddress =
				InsLibCloneCar(modld->ModuleAddress);
		}
		else if (modld->BusType == InsLibBusTypeVME) {
			modld_clone->ModuleAddress =
				InsLibCloneVme(modld->ModuleAddress);
		}
		else if ((modld->BusType == InsLibBusTypePMC)
			 ||  (modld->BusType == InsLibBusTypePCI)) {
			modld_clone->ModuleAddress =
				InsLibClonePci(modld->ModuleAddress);
		}

		if (modld->Extra) {
			sz = strlen(modld_clone->Extra) +1;
			modld_clone->Extra = (char *) sysbrk(sz);
			if (modld_clone->Extra)
				copy_mem((void *) modld_clone->Extra,
					 (void *) modld->Extra, sz);
		}

		if (modld->Isr) {
			modld_clone->Isr = (InsLibIntrDesc *)
				sysbrk(sizeof(InsLibIntrDesc));
			if (modld_clone->Isr)
				copy_mem((void *) modld_clone->Isr,
					 (void *)modld->Isr,
					 sizeof(InsLibIntrDesc));
		}
		modld_clone->Next = InsLibCloneModule(modld->Next);
		return modld_clone;
	}
	return NULL;
}

/* ============================================== */
/* Clone all memory for a driver                  */

/**
 * @brief Clone all memory for all the host drivers
 *
 * @param drvrd - driver description to start cloning from.
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
InsLibDrvrDesc *InsLibCloneDriver(InsLibDrvrDesc *drvrd)
{
	InsLibDrvrDesc *drvrd_clone = NULL;

	if (drvrd) {
		drvrd_clone = (InsLibDrvrDesc *) sysbrk(sizeof(InsLibDrvrDesc));
		if (!drvrd_clone)
			return NULL;
		copy_mem((void *) drvrd_clone, (void *) drvrd,
			 sizeof(InsLibDrvrDesc));
		drvrd_clone->Modules = InsLibCloneModule(drvrd->Modules);
		drvrd_clone->Next = InsLibCloneDriver(drvrd->Next);
		return drvrd_clone;
	}
	return NULL;
}


/**
 * @brief Clone all memory for a single host driver
 *
 * @param drvrd -
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
InsLibDrvrDesc *InsLibCloneOneDriver(InsLibDrvrDesc *drvrd)
{
	InsLibDrvrDesc *drvrd_clone = NULL;

	if (drvrd) {
		drvrd_clone = (InsLibDrvrDesc *) sysbrk(sizeof(InsLibDrvrDesc));
		if (!drvrd_clone) {
			printk("InsLib: Not enough memory for drvrd_clone\n");
			return NULL;
		}
		copy_mem((void *) drvrd_clone, (void *) drvrd,
			 sizeof(InsLibDrvrDesc));

		drvrd_clone->Modules = InsLibCloneModule(drvrd->Modules);
		drvrd_clone->Next = NULL;
		return drvrd_clone;
	}
	return NULL;
}

/* ============================================== */
/* Clone all memory from a host driver description */

InsLibHostDesc *InsLibCloneHost(InsLibHostDesc *hostd) {

InsLibHostDesc *hostd_clone = NULL;

   if (hostd) {
      hostd_clone = (InsLibHostDesc *) sysbrk(sizeof(InsLibHostDesc));
      if (!hostd_clone) return NULL;
      copy_mem((void *) hostd_clone, (void *) hostd, sizeof(InsLibHostDesc));
      hostd_clone->Drivers = InsLibCloneDriver(hostd->Drivers);
      return hostd_clone;
   }
   return NULL;
}

/* ============================================== */

void InsLibFreeCarSpace(InsLibCarAddressSpace *caras)
{
	if (caras) {
		InsLibFreeCarSpace(caras->Next);
		sysfree((char *) caras, sizeof(InsLibCarAddressSpace));
	}
}

void InsLibFreeVmeSpace(InsLibVmeAddressSpace *vmeas)
{
	if (vmeas) {
		InsLibFreeVmeSpace(vmeas->Next);
		sysfree((char *) vmeas, sizeof(InsLibVmeAddressSpace));
	}
}

void InsLibFreePciSpace(InsLibPciAddressSpace *pcias)
{
	if (pcias) {
		InsLibFreePciSpace(pcias->Next);
		sysfree((char *) pcias, sizeof(InsLibPciAddressSpace));
	}
}

/* ============================================== */

void InsLibFreeCar(InsLibCarModuleAddress *carma)
{
	if (carma) {
		InsLibFreeCarSpace(carma->CarAddressSpace);
		sysfree((char *) carma, sizeof(InsLibCarModuleAddress));
	}
}

void InsLibFreeVme(InsLibVmeModuleAddress *vmema)
{
	if (vmema) {
		InsLibFreeVmeSpace(vmema->VmeAddressSpace);
		sysfree((char *) vmema, sizeof(InsLibVmeModuleAddress));
	}
}

void InsLibFreePci(InsLibPciModuleAddress *pcima)
{
	if (pcima) {
		InsLibFreePciSpace(pcima->PciAddressSpace);
		sysfree((char *) pcima, sizeof(InsLibPciModuleAddress));
	}
}

/* ============================================== */
/* Free all memory for a module                   */

void InsLibFreeModule(InsLibModlDesc *modld)
{
	if (modld) {
		InsLibFreeModule(modld->Next);

		if (modld->BusType == InsLibBusTypeCARRIER)
			InsLibFreeCar(modld->ModuleAddress);
		else if (modld->BusType == InsLibBusTypeVME)
			InsLibFreeVme(modld->ModuleAddress);
		else if ( (modld->BusType == InsLibBusTypePMC) ||
			  (modld->BusType == InsLibBusTypePCI) )
			InsLibFreePci(modld->ModuleAddress);

		if (modld->Extra)
			sysfree((char *) modld->Extra, strlen(modld->Extra));
		if (modld->Isr)
			sysfree((char *) modld->Isr, sizeof(InsLibIntrDesc));
	}
}

/* ============================================== */
/* Free all memory for a driver                   */

void InsLibFreeDriver(InsLibDrvrDesc *drvrd)
{
	if (drvrd) {
		InsLibFreeDriver(drvrd->Next);
		InsLibFreeModule(drvrd->Modules);
		sysfree((char *) drvrd, sizeof(InsLibDrvrDesc));
	}
}

/* ============================================== */
/* Free all memory from a host driver description */

void InsLibFreeHost(InsLibHostDesc *hostd)
{
	if (hostd) {
		InsLibFreeDriver(hostd->Drivers);
		sysfree((char *) hostd, sizeof(InsLibHostDesc));
	}
}


/**
 * @brief Get a driver by name from host installation list
 *
 * @param hostd - host description
 * @param dname - driver name to search for. If NULL - then get all declared
 *            drivers in subsequent calls.
 *
 * If @ref dname is NULL - will return first found driver in the list and will
 * move driver description pointer to the next driver description for
 * incremental search. So that next time function will be called with NULL
 * @ref dname parameter - next driver description (or the first one, if end
 * of the list reached) will be returned to the user.
 *
 * @return NULL               - driver not found in host description
 * @return drvr descr pointer - if OK
 */
InsLibDrvrDesc *InsLibGetDriver(InsLibHostDesc *hostd, char *dname)
{
	InsLibDrvrDesc *drvrd = NULL;
	static InsLibDrvrDesc *last = NULL; /* drvr description pointer
					       for incremental search */

	if (!hostd)
		return NULL;

	drvrd = hostd->Drivers;

	if (!dname) {
		if (last)
			last = last->Next;
		else
			last = drvrd;

		return last;
	}

	while (drvrd) {
		if (strcmp(drvrd->DrvrName, dname) == 0)
			return drvrd;
		drvrd = drvrd->Next;
	}

	return NULL;
}


/* ================================================ */
/* Get a module from a driver                       */

/**
 * @brief Get a module from a driver, based on module LUN
 *
 * @param drvrd         - driver description
 * @param module_number - LUN in question
 *
 * Search for the module with provided LUN (@ref module_number parameter)
 * in the list of all the modules driver controls.
 *
 * @return NULL                 - module with given LUN not found
 * @return module descr pointer - module found
 */
InsLibModlDesc *InsLibGetModule(InsLibDrvrDesc *drvrd, int module_number)
{
	InsLibModlDesc *modld;

	if (!drvrd)
		return NULL;
	modld = drvrd->Modules;
	while (modld) {
		if (module_number == modld->ModuleNumber)
			return modld;
		modld = modld->Next;
	}
	return NULL;
}

/* ================================================ */
/* Get an address space from a module               */
InsLibAnyAddressSpace *InsLibGetAddressSpace(InsLibModlDesc *modld,
					     int space_number)
{
	InsLibAnyModuleAddress *anyma = NULL;
	InsLibAnyAddressSpace  *anyas = NULL;

	if (modld) {
		anyma = modld->ModuleAddress;
		if (anyma) {
			anyas = anyma->AnyAddressSpace;
			while (anyas) {
				if (anyas->SpaceNumber == space_number)
					return anyas;
				anyas = anyas->Next;
			}
		}
	}

	return NULL;
}

/* for debugging only */
/* ============================================== */
void InsLibPrintCarSpace(InsLibCarAddressSpace *caras)
{
	if (caras) {
		printk("[CAR:Dw:0x%X:Ws:%d]",
		       caras->DataWidth,
		       caras->WindowSize);
		InsLibPrintCarSpace(caras->Next);
	}
}

void InsLibPrintVmeSpace(InsLibVmeAddressSpace *vmeas)
{
	if (vmeas) {
		printk("[VME:Am:0x%X:Dw:%d:Ws:0x%X:Ba:0x%X:Fm:%d]",
		       vmeas->AddressModifier,
		       vmeas->DataWidth,
		       vmeas->WindowSize,
		       vmeas->BaseAddress,
		       vmeas->FreeModifierFlag);
		InsLibPrintVmeSpace(vmeas->Next);
	}
}

void InsLibPrintPciSpace(InsLibPciAddressSpace *pcias)
{
	if (pcias) {
		printk("[PCI:Bar:%d:En:%d]",
		       pcias->BaseAddressRegister,
		       pcias->Endian);
		InsLibPrintPciSpace(pcias->Next);
	}
}

/* ============================================== */

void InsLibPrintCar(InsLibCarModuleAddress *carma)
{
	if (carma) {
		printk("[CAR:Bn:%d:Bp:%d]",
		       carma->BoardNumber,
		       carma->BoardPosition);
		InsLibPrintCarSpace(carma->CarAddressSpace);
   }
}

void InsLibPrintVme(InsLibVmeModuleAddress *vmema)
{
	if (vmema) {
		InsLibPrintVmeSpace(vmema->VmeAddressSpace);
	}
}

void InsLibPrintPci(InsLibPciModuleAddress *pcima)
{
	if (pcima) {
		printk("[PCI:Bn:%d:Sn:%d:VId:0x%X:DId:0x%X:"
		       "SVId:0x%X:SDId:0x%X]",
		       pcima->BusNumber,
		       pcima->SlotNumber,
		       pcima->VendorId,
		       pcima->DeviceId,
		       pcima->SubVendorId,
		       pcima->SubDeviceId);
		InsLibPrintPciSpace(pcima->PciAddressSpace);
	}
}

/* ============================================== */
/* Print all memory for a module                   */

void InsLibPrintModule(InsLibModlDesc *modld)
{
	if (modld) {
		printk("[MOD:Bt:%d:Mn:%d",
		       modld->BusType,
		       modld->ModuleNumber);
		if (modld->Isr) {
			printk(":Fl:%d:Vec:0x%X:Lvl:%d",
			       modld->Isr->IsrFlag,
			       modld->Isr->Vector,
			       modld->Isr->Level);
		}
		printk("]");
		if (modld->BusType == InsLibBusTypeVME)
			InsLibPrintVme(modld->ModuleAddress);
		else if (modld->BusType == InsLibBusTypeCARRIER)
			InsLibPrintCar(modld->ModuleAddress);
		else
			InsLibPrintPci(modld->ModuleAddress);
		printk("\n");
		InsLibPrintModule(modld->Next);
	}
}


/* ============================================== */
/* Print all memory for a driver                   */

void InsLibPrintDriver(InsLibDrvrDesc *drvrd)
{
	if (drvrd) {
		printk("[DRV:%s:Df:0x%X:Ef:0x%X:Mc:%d]\n",
		       drvrd->DrvrName,
		       drvrd->DebugFlags,
		       drvrd->EmulationFlags,
		       drvrd->ModuleCount);

		InsLibPrintModule(drvrd->Modules);
		InsLibPrintDriver(drvrd->Next);
	}
}
