/**
 * @file skeldrvr.c
 *
 * @brief Skel driver framework
 *
 * This framework implements a standard SKEL compliant driver PCI support.
 *
 * Copyright (c) 2008-09 CERN
 * @author Julian Lewis <julian.lewis@cern.ch>
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifdef CONFIG_BUS_PCI

/**
 * @brief PCI interrupt wrapper -- just calls skel_isr
 *
 * @param cookie - cookie passed to the ISR
 *
 * VME/CES Library expects the handler to return int, while Lynx' DRM expects
 * a void return. This wrapper just gets around it.
 */
void __pci_isr(void *cookie)
{
	skel_isr(cookie);
}

/*
 * clear mappings of a PCI module
 */
static void unmap_pcias(SkelDrvrModuleContext *mcon)
{
	InsLibPciModuleAddress	*pma;
	InsLibPciAddressSpace	*pas;

	pma = mcon->Modld->ModuleAddress;
	if (!pma)
		return;

	pas = pma->PciAddressSpace;
	while (pas) {
		if (pas->Mapped) {
			int resid = pas->BaseAddressRegister;

			drm_unmap_resource(mcon->PciHandle, resid +
					PCI_RESID_BAR0);
			pas->Mapped = NULL;
		}
		pas = pas->Next;
	}
}

static void RemovePciModule(SkelDrvrModuleContext *mcon)
{
	/* check if an ISR was registered */
	if (!(mcon->StandardStatus & SkelDrvrStandardStatusNO_ISR)) {
		drm_unregister_isr(mcon->PciHandle);
		mcon->StandardStatus |= SkelDrvrStandardStatusNO_ISR;
	}
	/* unmap PCI mappings (if any) */
	unmap_pcias(mcon);
}

/**
 * @brief map a PCI address space
 *
 * @param mcon - module context
 * @param pas - PCI address space
 *
 * @return 0 - on failure
 * @return 1 - on success
 */
static int map_pcias(SkelDrvrModuleContext *mcon, InsLibPciAddressSpace *pas)
{
	void *map;
	int resid;
	int cc;

	/* emulation mode */
	if (mcon->StandardStatus & SkelDrvrStandardStatusEMULATION) {
		pas->Mapped = NULL;
		mcon->StandardStatus |= SkelDrvrStandardStatusNO_HARDWARE;

		SK_INFO("module #%d -- emulation ON", mcon->ModuleNumber);

		goto out;
	}

	resid = pas->BaseAddressRegister;
	map = &pas->Mapped;
	cc = drm_map_resource(mcon->PciHandle, resid + PCI_RESID_BAR0, map);

	if (cc) { /* mapping error */
		pas->Mapped = NULL;

		SK_ERROR("PCI mapping failed while mapping BAR%d for module#%d",
			 resid, mcon->ModuleNumber);

		if (!mcon->Modld->IgnoreErrors)
			goto out_err;
		else
			goto out;
	}

	/* real hardware, mapping OK */
	SK_INFO("PCI mapping OK of BAR%d (-->virt %p) for module#%d",
		resid, map, mcon->ModuleNumber);
out:
	return 1;
out_err:
	return 0;
}

/**
 * @brief search for a given module
 *
 * @param mcon - module context
 *
 * Traverse the PCI tree looking for a particular bus/slot combination.
 */
static void *pci_module_hunt(SkelDrvrModuleContext *mcon)
{
	InsLibPciModuleAddress *pma;
	unsigned int bus, slot, cc, j;
	int i; /* handles' counter */
	struct drm_node_s *handles[SkelDrvrMODULE_CONTEXTS];

	pma = mcon->Modld->ModuleAddress;
	if (!pma)
		return NULL;
	i = 0;
	while (1) {
		cc = drm_get_handle(PCI_BUSLAYER, pma->VendorId, pma->DeviceId,
				    &handles[i]);
		if (cc) {
			handles[i] = NULL;
			goto out;
		}
		drm_device_read(handles[i], PCI_RESID_BUSNO, 0, 0, &bus);
		drm_device_read(handles[i], PCI_RESID_DEVNO, 0, 0, &slot);

		if (bus == pma->BusNumber && slot == pma->SlotNumber)
			goto out;
		/*
		 * this device is not the one we're looking for; increment the
		 * handles' counter so that it will be cleaned-up when exiting
		 */
		i++;
	}
out:
	for (j = 0; j < i; j++)  /* free all the non-matching handles */
		drm_free_handle(handles[j]);

	return handles[i];
}

/**
 * @brief set-up the mappings and ISR for a given module
 *
 * @param mcon - module context
 *
 * @return 0 - on failure
 * @return 1 - on success
 */
static int AddPciModule(SkelDrvrModuleContext *mcon)
{
	InsLibPciModuleAddress *pcima;
	InsLibPciAddressSpace  *pcias;
	InsLibModlDesc         *modld;
	InsLibIntrDesc         *intrd;
	int cc;

	modld = mcon->Modld;

	mcon->PciHandle = pci_module_hunt(mcon);
	if (mcon->PciHandle == NULL) /* no such device */
		return 0;

	/* set-up the mappings */
	pcima = modld->ModuleAddress;
	if (pcima) {
		pcias = pcima->PciAddressSpace;
		while (pcias) {
			if (!map_pcias(mcon, pcias) && !mcon->Modld->IgnoreErrors)
				goto out_err;
			pcias = pcias->Next;
		}
	}

	/* register ISR */
	intrd = modld->Isr;

	if (intrd && intrd->IsrFlag) {

		cc = drm_register_isr(mcon->PciHandle, __pci_isr, (void *)mcon);

		if (cc) { /* ISR not registered */
			if (!mcon->Modld->IgnoreErrors)
				goto out_err;
			else
				return 1;
		}
		/* ISR registered correctly */
		mcon->StandardStatus &= ~SkelDrvrStandardStatusNO_ISR;

		SK_INFO("PCI interrupt for module#%d registered OK",
			mcon->ModuleNumber);
	}

	return 1;

out_err: /* abort installation */
	RemovePciModule(mcon);
	return 0;
}

#else

static int AddPciModule(SkelDrvrModuleContext *mcon) {
   return 0;
}

static void RemovePciModule(SkelDrvrModuleContext *mcon)
{
}

#endif
