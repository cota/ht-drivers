/**
 * @file skelvme.c
 *
 * @brief Skel driver VME support
 *
 * This framework implements a standard SKEL compliant driver VME support.
 *
 * Copyright (c) 2008-09 CERN
 * @author Julian Lewis <julian.lewis@cern.ch>
 * @author Emilio G. Cota <emilio.garcia.cota@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifdef CONFIG_BUS_VME

/**
 * @brief set pdparam to its default values
 *
 * @param param - pdparam struct
 */
static void set_pdparam_defaults(struct pdparam_master *param)
{
	memset(param, 0, sizeof(*param));
	param->iack   = 1;		/* no iack */
	param->rdpref = 0;		/* no VME read prefetch option */
	param->wrpost = 0;		/* no VME write posting option */
	param->swap   = 1;		/* VME auto swap option */
	param->sgmin  = 0;              /* reserved, _must_ be 0  */
	param->dum[0] = VME_PG_SHARED;	/* window is shareable */
	param->dum[1] = 0;		/* XPC ADP-type */
	param->dum[2] = 0;		/* reserved, _must_ be 0 */
}

static void *do_vme_mapping(InsLibVmeAddressSpace *v, struct pdparam_master *pm)
{
	unsigned long map;
/*
 * linux' implementation of find_controller() needs a proper datawidth.
 * Note that it doesn't do a test access to the module.
 */
#ifdef __linux__
	unsigned long datawidth = v->DataWidth / 8;
#else
	unsigned long datawidth = 0;
#endif

	map = find_controller(v->BaseAddress, v->WindowSize, v->AddressModifier,
			      0, datawidth, pm);
	return (void *)map;
}

static inline int region_is_a_block(InsLibVmeAddressSpace *vas)
{
	/*
	 * TODO: define these constants for Lynx too, and use them everywhere.
	 *
	 * VME_A64_MBLT		0x00
	 * VME_A64_BLT		0x03
	 * VME_A32_USER_MBLT	0x08
	 * VME_A32_USER_BLT	0x0b
	 * VME_A32_SUP_MBLT	0x0c
	 * VME_A32_SUP_BLT	0x0f
	 * VME_A40_BLT		0x37
	 * VME_A24_USER_MBLT	0x38
	 * VME_A24_USER_BLT	0x3b
	 * VME_A24_SUP_MBLT	0x3c
	 * VME_A24_SUP_BLT	0x3f
	 */
	switch (vas->AddressModifier) {
	case 0x00:
	case 0x03:
	case 0x08:
	case 0x0b:
	case 0x0c:
	case 0x0f:
	case 0x37:
	case 0x38:
	case 0x3b:
	case 0x3c:
	case 0x3f:
		return 1;
	default:
		return 0;
	}
}

/**
 * @brief map a VME address space
 *
 * @param mcon - module context
 * @param vas - vme address space
 *
 * @return 0 - on failure
 * @return 1 - on success
 */
static int map_vmeas(SkelDrvrModuleContext *mcon, InsLibVmeAddressSpace *vas)
{
	struct pdparam_master param;

	vas->Endian = InsLibEndianBIG; /* all vme modules are BE */

	/* emulation mode */
	if (mcon->StandardStatus & SkelDrvrStandardStatusEMULATION) {
		vas->Mapped = NULL;
		mcon->StandardStatus |= SkelDrvrStandardStatusNO_HARDWARE;

		SK_INFO("module #%d -- emulation ON", mcon->ModuleNumber);

		return 1;
	}

	/*
	 * For regions described as blocks, we don't need to map anything,
	 * since we can use DMA.
	 */
	if (region_is_a_block(vas))
		return 1;

	set_pdparam_defaults(&param);
	vas->Mapped = do_vme_mapping(vas, &param);

	if (vas->Mapped == (void *)(-1)) {
		vas->Mapped = NULL;
		SK_ERROR("VME mapping failed while mapping 0x%x for module#%d",
			(unsigned int)vas->BaseAddress, mcon->ModuleNumber);
		return 0;
	}

	/* mapping successful */
	SK_INFO("VME mapping OK of 0x%x (-->virt 0x%x) for module #%d",
		vas->BaseAddress, (int)vas->Mapped, mcon->ModuleNumber);

	return 1;
}

/**
 * @brief clear mappings of a VME module
 *
 * @param mcon - module context
 * @param force - force the unmapping
 *
 * With force set to 0, only mappings with FreeModifierFlag set are cleared.
 * Otherwise, all the mappings which belong to the module are cleared.
 */
static void unmap_vmeas(SkelDrvrModuleContext *mcon, int force)
{
	InsLibVmeAddressSpace	*vas;
	InsLibVmeModuleAddress	*vma;

	vma = mcon->Modld->ModuleAddress;
	if (!vma)
		return;

	vas = vma->VmeAddressSpace;
	while (vas) {
		if (vas->Mapped && (force || vas->FreeModifierFlag)) {
			unsigned long laddr = (unsigned long)vas->Mapped;

			if (!return_controller(laddr, vas->WindowSize))
				vas->Mapped = NULL;
			else
				SK_WARN("return_controller failed.");
		}
		vas = vas->Next;
	}
}

static void RemoveVmeModule(SkelDrvrModuleContext *mcon)
{
	InsLibIntrDesc *intrd = mcon->Modld->Isr;

	/* check for registered ISR */
	if (!(mcon->StandardStatus & SkelDrvrStandardStatusNO_ISR)) {
		int interr;

		interr = vme_intclr(intrd->Vector, NULL);
		if (interr)
			SK_WARN("VME intclr: returned %d", interr);
		else
			mcon->StandardStatus |= SkelDrvrStandardStatusNO_ISR;
	}
	/* unmap VME mappings (if any) */
	unmap_vmeas(mcon, 1);
}

/**
 * @brief set-up the mappings and ISR for a given module
 *
 * @param mcon - module context
 *
 * @return 0 - on failure
 * @return 1 - on success
 */
static int AddVmeModule(SkelDrvrModuleContext *mcon)
{
	InsLibModlDesc		*modld = mcon->Modld;
	InsLibVmeModuleAddress	*vmema;
	InsLibVmeAddressSpace	*vmeas;
	InsLibIntrDesc		*intrd;
	int cc;

	/* set-up the mappings */
	vmema = modld->ModuleAddress;
	if (vmema) {
		vmeas = vmema->VmeAddressSpace;
		while (vmeas) {
			cc = map_vmeas(mcon, vmeas);
			if (!cc && !mcon->Modld->IgnoreErrors)
				goto out_err;
			vmeas = vmeas->Next;
		}
	}
	/* ISR, if any */
	intrd = modld->Isr;
	if (intrd && intrd->IsrFlag) {
		cc = vme_intset(intrd->Vector, skel_isr, (char *)mcon, NULL);
		if (cc < 0) {

			SK_ERROR("module#%d: ISR not installed. retval = %d",
				 mcon->ModuleNumber, cc);

			if (!mcon->Modld->IgnoreErrors)
				goto out_err;
			else
				goto out;
		}
		/* ISR installed successfully */
		mcon->StandardStatus &= ~SkelDrvrStandardStatusNO_ISR;

		SK_INFO("VME interrupt 0x%x for module#%d registered OK",
			intrd->Vector, mcon->ModuleNumber);
	}
out:
	return 1;

out_err: /* abort the installation of the module */
	RemoveVmeModule(mcon);
	return 0;
}

/**
* @brief get the virtual address of a VME address space
*
* @param mcon - module context
* @param am - address modifier
* @param dw - data width
*
* Avoid open coding the retrieval of a particular VME address space,
* since this has to be done on every user's driver
*
* @return virtual address of the VME mapping - on success
* @return NULL - on failure
*/
void *get_vmemap_addr(SkelDrvrModuleContext *mcon, int am, int dw)
{
	InsLibVmeModuleAddress *vma;
	InsLibVmeAddressSpace  *vas;

	if (!mcon->Modld) {
		report_module(mcon, SkelDrvrDebugFlagWARNING,
			      "NULL module descriptor");
		return NULL;
	}

	vma = mcon->Modld->ModuleAddress;
	if (!vma) {
		report_module(mcon, SkelDrvrDebugFlagWARNING,
			      "NULL module address");
		return NULL;
	}

	vas = vma->VmeAddressSpace;
	while (vas) {
		if (vas->AddressModifier == am && vas->DataWidth == dw)
			return vas->Mapped;
		vas = vas->Next;
	}

	report_module(mcon, SkelDrvrDebugFlagWARNING,
		"Address modifier 0x%X not found", am);
	return NULL;

}

/*
 * Get the VME address of a given VME region.
 * return 0 if the VME address is found, -1 otherwise.
 */
int get_vme_addr(SkelDrvrModuleContext *mcon, int am, int dw, uint32_t *addr)
{
	InsLibVmeModuleAddress *vma;
	InsLibVmeAddressSpace  *vas;

	if (!mcon->Modld) {
		report_module(mcon, SkelDrvrDebugFlagWARNING,
			      "NULL module descriptor");
		return -1;
	}

	vma = mcon->Modld->ModuleAddress;
	if (!vma) {
		report_module(mcon, SkelDrvrDebugFlagWARNING,
			      "NULL module address");
		return -1;
	}

	vas = vma->VmeAddressSpace;
	while (vas) {
		if (vas->AddressModifier == am && vas->DataWidth == dw) {
			*addr = vas->BaseAddress;
			return 0;
		}
		vas = vas->Next;
	}
	report_module(mcon, SkelDrvrDebugFlagWARNING,
		"Region with am 0x%x and dw %d not found", am, dw);
	return -1;

}

#else

static void unmap_vmeas(SkelDrvrModuleContext *mcon, int force) {
   return;
}

static void RemoveVmeModule(SkelDrvrModuleContext *mcon) {
   return;
}

static int AddVmeModule(SkelDrvrModuleContext *mcon) {
   return 0;
}

#endif
