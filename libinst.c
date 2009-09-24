/**
 * @file libinst.c
 *
 * @brief Driver installer library.
 *
 * Parse XML driver installer file and build installer structures.
 *
 * @author Copyright (C) 2009 CERN CO/HT Julian Lewis <julian.lewis@cern.ch>
 *
 * @date Created on 09/04/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libinst.h>

#include <general_both.h> /* common definitions && macros */
#include <vme_am.h>

int InsLibErrorCount = 0;

/**< XML Entity Key words */
typedef enum {
	EMPTY,
	INSTALL,
	HOST,
	DRIVER,
	NAME,
	DRIVERGEN,
	INSTALL_DEBUG_LEVEL,
	EMULATION_FLAG,
	COMMENT,
	MODULE,
	BUS_TYPE,
	LOGICAL_MODULE_NUMBER,
	EXTRA,
	PCI,
	BUS_NUMBER,
	SLOT_NUMBER,
	VENDOR_ID,
	DEVICE_ID,
	SUBVENDOR_ID,
	SUBDEVICE_ID,
	CARRIER,
	BOARD_NUMBER,
	BOARD_POSITION,
	INTERRUPT,
	VECTOR,
	LEVEL,
	PCI_SPACE,
	BAR,
	ENDIAN,
	VME_SPACE,
	MODIFIER,
	DATA_WIDTH,
	WINDOW_SIZE,
	ADDRESS,
	FREE_FLAG,
	CARRIER_SPACE,
	VME,
	IGNORE_INSTALL_ERRORS,
	KEY_WORDS
} KeyWord;

static char *keywords[KEY_WORDS] = {
	"empty",
	"install",
	"host",
	"driver",
	"name",
	"drivergen",
	"install_debug_level",
	"emulation_flag",
	"comment",
	"module",
	"bus_type",
	"logical_module_number",
	"extra",
	"pci",
	"bus_number",
	"slot_number",
	"vendor_id",
	"device_id",
	"subvendor_id",
	"subdevice_id",
	"carrier",
	"board_number",
	"board_position",
	"interrupt",
	"vector",
	"level",
	"pci_space",
	"bar",
	"endian",
	"vme_space",
	"modifier",
	"data_width",
	"window_size",
	"address",
	"free_flag",
	"carrier_space",
	"vme",
	"ignore_install_errors"
 };

/* =============================================================== */
/* Private routines used inside library                            */
/* =============================================================== */

static InsLibPciModuleAddress *parse_pci_address_tag(xmlNode *, int,
						     InsLibModlDesc *);
static InsLibVmeModuleAddress *parse_vme_address_tag(xmlNode *, int,
						     InsLibModlDesc *);
static InsLibCarModuleAddress *parse_car_address_tag(xmlNode *, int,
						     InsLibModlDesc *);
static void add_vme_addr_space(InsLibVmeModuleAddress *,
			       InsLibVmeAddressSpace *);


static InsLibHostDesc* handle_install_node(xmlNode *, int);

static void add_driver_node(InsLibHostDesc *, InsLibDrvrDesc *);
static InsLibDrvrDesc* handle_driver_node(xmlNode *, int, InsLibHostDesc *);

static void add_module_node(InsLibDrvrDesc *, InsLibModlDesc *);
static InsLibModlDesc* handle_module_node(xmlNode *, int, InsLibDrvrDesc *);

static InsLibVmeAddressSpace* handle_vme_space_node(xmlNode *, int,
						    InsLibVmeModuleAddress *,
						    InsLibModlDesc *);

/* ========================================== */
/* Indenter to print spaces if printing is on */

#define FORWARD 1
#define BACKWARDS 0
#define MAX_WHITE 32

/**
 * @brief
 *
 * @param direction --
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
static char *indent(int direction)
{
	static char addendum[MAX_WHITE];
	static int i = 0;
	int k;

	if (direction == FORWARD)
		i++;
	else /* BACKWARD */
		i--;

	bzero((void *) addendum, MAX_WHITE);
	for (k = 1; k < i; k++)
		strcat(addendum, "  ");
	return addendum;
}

/**
 * @brief Look up a special XML installer keyword
 *
 * @param name --
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
static KeyWord LookUp(char *name)
{
	int i;

	if (name) {
		for (i = 0; i < KEY_WORDS; i++) {
			if (strcmp(keywords[i], name) == 0) {
				return (KeyWord) i;
			}
		}
	}
	return (KeyWord) 0;
}


/**
 * @brief Get the next property from element
 *
 * @param prop  --
 * @param pname --
 * @param pvalu --
 * @param pflag --
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
static xmlAttr *GetNextProp(xmlAttr *prop, char **pname, char **pvalu,
			    int pflag)
{
	if (prop) {
		*pname = (char *) prop->name;
		if (prop->children)
			*pvalu = (char *)prop->children->content;

		if (pflag)
			printf("\033[0m[\033[33m%s:\033[32m%s\033[0m]",
			       *pname, *pvalu);

		return prop->next;
	}

	return NULL;
}

/**
 * @brief printout errors
 *
 * @param txt  -- format string
 *                Special case (0 or 1) is possible
 *                0 -- printing is turned off
 *                1 -- do stderr printout
 * @param  ... -- arguments
 *
 * <long-description>
 */
static void prnterr(char *txt, ...)
{
	static int doprnt = 1;
	va_list ap;

	InsLibErrorCount++;

	if (!doprnt)
		/* no error printing */
		return;

	fprintf(stderr, "\n%s", ERR_MSG);
	va_start(ap, txt);
	vfprintf(stderr, txt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

/**
 * @brief Get a number from a string
 *
 * @param txt --
 *
 * <long-description>
 *
 * @return <ReturnValue>
 */
static unsigned int GetNumber(char *txt)
{
	char *cp, *ep;
	unsigned int val;

	cp = &(txt[0]);
	val = strtoul(cp, &ep, 0);
	if (ep == cp)
		prnterr("Missing value");
	return val;
}

/**
 * @brief Process elements and build descriptor
 *
 * @param a_node -- root element
 * @param pflag  -- Debug print flag to print the tree
 *
 * @return host description pointer - if OK
 * @return NULL                     - if FAILED
 */
static InsLibHostDesc *DoElement(xmlNode *a_node, int pflag)
{
	static InsLibHostDesc         *hostd = NULL;
	static InsLibDrvrDesc         *drvrd = NULL;
	static InsLibModlDesc         *modld = NULL;
	static InsLibPciModuleAddress *pcima = NULL;
	static InsLibPciAddressSpace  *pcias = NULL, *npcias = NULL;
	static InsLibVmeModuleAddress *vmema = NULL;
	static InsLibVmeAddressSpace  *vmeas = NULL;
	static InsLibCarModuleAddress *carma = NULL;
	static InsLibCarAddressSpace  *caras = NULL, *ncaras = NULL;
	static InsLibIntrDesc         *isrds = NULL;
	xmlNode *cur_node = NULL;
	xmlAttr *prop     = NULL;

	KeyWord pky;
	char *ip, *pname, *pvalu;

	if (pflag)
		ip = indent(FORWARD);

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			pky = LookUp((char *) cur_node->name);
			if (pky == EMPTY)
				prnterr("Crap in XML file");

			if (pflag)
				printf("\033[36m%s%s", ip, cur_node->name);

			switch (pky) {
			case INSTALL:
				hostd = handle_install_node(cur_node, pflag);
				if (!hostd)
					return hostd;
				break;
			case DRIVER:
				if (!hostd) {
					prnterr("Encountered DRIVER clause"
						   " with no previous INSTALL"
						   " clause");
					return NULL;
				}

				drvrd = handle_driver_node(cur_node, pflag,
							   hostd);
				if (!drvrd) {
					InsLibFreeHost(hostd);
					return NULL;
				}
				break;
			case MODULE:
				if (!drvrd) {
					prnterr("Encountered MODULE clause"
						   " with no previous DRIVER"
						   " clause");
					InsLibFreeHost(hostd);
					return NULL;
				}

				modld = handle_module_node(cur_node, pflag,
							   drvrd);

				if (!modld) {
					InsLibFreeHost(hostd);
					return NULL;
				}
				break;
			case INTERRUPT:
				if (!modld) {
					prnterr("Encountered INTERRUPT"
						   " clause with no previous"
						   " MODULE clause");
					InsLibFreeHost(hostd);
					return NULL;
				}

				isrds = (InsLibIntrDesc *)
					malloc(sizeof(InsLibIntrDesc));
				if (!isrds) {
					prnterr("Cant allocate memory");
					InsLibFreeHost(hostd);
					return NULL;
				}

				bzero((void *) isrds, sizeof(InsLibIntrDesc));

				modld->Isr = isrds;
				isrds->IsrFlag = 1;

				prop = cur_node->properties;
				while (prop) {
					prop = GetNextProp(prop, &pname,
							   &pvalu, pflag);
					pky = LookUp((char *) pname);
					if (pky == VECTOR) {
						isrds->Vector = GetNumber(pvalu);
						if ((isrds->Vector < 1) || (isrds->Vector > 255))
							prnterr("Bad VECTOR in INTERRUPT clause");

					} else if (pky == LEVEL) {
						isrds->Level = GetNumber(pvalu);
						if ((isrds->Level < 1) || (isrds->Level > 8))
							prnterr("Bad LEVEL in INTERRUPT clause");

					} else if (pky == COMMENT) {
						strncpy(isrds->Comment,pvalu,InsLibCOMMENT_SIZE);

					} else break;
				}

				if (pflag) printf("\n");
				break;
			case PCI:
				pcima = parse_pci_address_tag(cur_node,
							      pflag, modld);
				break;
			case PCI_SPACE:
				if (!pcima) {
					prnterr("Encountered PCI space"
						   " clause with no previous"
						   " PCI clause");
					InsLibFreeHost(hostd);
					return NULL;
				}

				npcias = (InsLibPciAddressSpace *) malloc(sizeof(InsLibPciAddressSpace));
				if (!npcias) {
					prnterr("Cant allocate memory");
					InsLibFreeHost(hostd);
					return NULL;
				}

				bzero((void *) npcias, sizeof(InsLibPciAddressSpace));

				if (pcima->PciAddressSpace == NULL)
					pcima->PciAddressSpace = npcias;
				else
					pcias->Next = npcias;
				pcias = npcias;

				prop = cur_node->properties;
				while (prop) {
					prop = GetNextProp(prop, &pname,
							   &pvalu,pflag);
					pky = LookUp((char *) pname);

					if (pky == BAR) {
						pcias->BaseAddressRegister = GetNumber(pvalu);
						if (pcias->BaseAddressRegister > 6) prnterr("Bad BASE ADDRESS REGISTER in PCI_SPACE clause");

					} else if (pky == ENDIAN) {
						if      (strcmp(pvalu,"BIG")    == 0) pcias->Endian = InsLibEndianBIG;
						else if (strcmp(pvalu,"LITTLE") == 0) pcias->Endian = InsLibEndianLITTLE;
						else prnterr("Bad ENDIAN specification in PCI_SPACE clause");

					} else if (pky == COMMENT) {
						strncpy(pcias->Comment,pvalu,InsLibCOMMENT_SIZE);

					} else break;
				}

				if (pflag) printf("\n");
				break;

			case VME:
				vmema = parse_vme_address_tag(cur_node,
							      pflag, modld);
				break;
			case VME_SPACE:
				if (!vmema) {
					prnterr("Encountered VME space"
						   " clause with no previous"
						   " VME clause");
					InsLibFreeHost(hostd);
					return NULL;
				}

				if (!modld) {
					prnterr("Encountered VME SPACE"
						   " clause with no previous"
						   " MODULE clause");
					InsLibFreeHost(hostd);
					return NULL;
				}

				vmeas = handle_vme_space_node(cur_node,
							      pflag,
							      vmema,
							      modld);

				if (!vmeas) {
					InsLibFreeHost(hostd);
					return NULL;
				}
				break;
			case CARRIER:
				carma = parse_car_address_tag(cur_node,
							      pflag, modld);
				break;
			case CARRIER_SPACE:
				if (!carma) {
					prnterr("Encountered CARRIER SPACE"
						   " clause with no previous"
						   " CARRIER clause");
					InsLibFreeHost(hostd);
					return NULL;
				}

				ncaras = (InsLibCarAddressSpace *) malloc(sizeof(InsLibCarAddressSpace));
				if (!ncaras) {
					prnterr("Cant allocate memory");
					InsLibFreeHost(hostd);
					return NULL;
				}

				bzero((void *) ncaras, sizeof(InsLibCarAddressSpace));

				if (carma->CarAddressSpace == NULL)
					carma->CarAddressSpace = ncaras;
				else
					caras->Next = ncaras;
				caras = ncaras;

				prop = cur_node->properties;
				while (prop) {
					prop = GetNextProp(prop,&pname,&pvalu,pflag);
					pky = LookUp((char *) pname);

					if (pky == DATA_WIDTH) {
						caras->DataWidth = GetNumber(pvalu);
						if ((caras->DataWidth !=16) && (caras->DataWidth !=32))
							prnterr("Bad DATA_WIDTH in VME_SPACE clause");


					} else if (pky == WINDOW_SIZE) {
						caras->WindowSize = GetNumber(pvalu);

					} else if (pky == COMMENT) {
						strncpy(carma->Comment,pvalu,InsLibCOMMENT_SIZE);

					} else break;
				}

				if (caras->DataWidth  == 0) prnterr("Data WIDTH missing from CARRIER ADDRESS SPACE");
				if (caras->WindowSize == 0) prnterr("WINDOW SIZE  missing from CARRIER ADDRESS SPACE");

				if (pflag) printf("\n");
				break;


			default:
				prnterr("Symbol out of context in XML file");
			}
		}
		DoElement(cur_node->children, pflag);
	}
	if (pflag)
		indent(BACKWARDS);

	return hostd;
}


/* ================================================================== */
/* Exported installer library routines                                */
/* ================================================================== */

/**
 * @brief Build the driver descriptors from the installation XML description
 *
 * @param fname -- XML install file name to parse
 * @param pflag -- Debug print flag to print the tree
 *
 * @return NULL                     - failed
 * @return host description pointer - OK
 */
InsLibHostDesc *InsLibParseInstallFile(char *fname, int pflag)
{
	xmlDoc *doc;
	xmlNode *root_element;
	InsLibHostDesc *hostd;

	xmlInitParser();

	doc = xmlReadFile(fname, NULL, 0);
	if (!doc)
		return NULL;

	root_element = xmlDocGetRootElement(doc);

	hostd = DoElement(root_element, pflag);

	if (pflag)
		printf("\n");

	xmlFreeDoc(doc);
	xmlCleanupParser();
	return hostd;
}


/**
 * @brief Get a driver by name from host installation list
 *
 * @param hostd -- host description
 * @param dname -- driver name to search for. If NULL - then get all declared
 *                 drivers in subsequent calls.
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
		if (strncmp(drvrd->DrvrName, dname, InsLibNAME_SIZE) == 0)
			return drvrd;
		drvrd = drvrd->Next;
	}
	return NULL;
}


/**
 * @brief Get module descriptions of the driver.
 *
 * @param dd -- Driver description
 * @param mn -- module number (starting from 1!). If 0 - then get all declared
 *              driver modules in subsequent calls.
 *
 * @return NULL             - if fails
 * @return module descr ptr - if success
 */
InsLibModlDesc *InsLibGetModule(InsLibDrvrDesc *dd, int mn)
{
	InsLibModlDesc *modd        = NULL;
	static InsLibModlDesc *last = NULL;

	if (!dd)
		return NULL;

	modd = dd->Modules;

	if (!mn) {
		if (last)
			last = last->Next;
		else
			last = modd;
		return last;
	}

	if (!WITHIN_RANGE(1, mn, dd->ModuleCount))
		return NULL; /* out-of-range */

	while (--mn)
		modd = modd->Next;

	return modd;
}

void InsLibFreeCarSpace(InsLibCarAddressSpace *caras)
{
	if (caras) {
		InsLibFreeCarSpace(caras->Next);
		free(caras);
	}
}

void InsLibFreeVmeSpace(InsLibVmeAddressSpace *vmeas)
{
	if (vmeas) {
		InsLibFreeVmeSpace(vmeas->Next);
		free(vmeas);
	}
}

void InsLibFreePciSpace(InsLibPciAddressSpace *pcias)
{
	if (pcias) {
		InsLibFreePciSpace(pcias->Next);
		free(pcias);
	}
}

void InsLibFreeCar(InsLibCarModuleAddress *carma)
{
	if (carma) {
		InsLibFreeCarSpace(carma->CarAddressSpace);
		free(carma);
	}
}

void InsLibFreeVme(InsLibVmeModuleAddress *vmema)
{
	if (vmema) {
		InsLibFreeVmeSpace(vmema->VmeAddressSpace);
		free(vmema);
	}
}

void InsLibFreePci(InsLibPciModuleAddress *pcima)
{
	if (pcima) {
		InsLibFreePciSpace(pcima->PciAddressSpace);
		free(pcima);
	}
}

/**
 * @brief Free all memory for a module
 *
 * @param modld --
 *
 * <long-description>
 */
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
	      free(modld->Extra);
      if (modld->Isr)
	      free(modld->Isr);
   }
}

/**
 * @brief Free all memory for a driver
 *
 * @param drvrd --
 *
 * <long-description>
 */
void InsLibFreeDriver(InsLibDrvrDesc *drvrd)
{
	if (drvrd) {
		InsLibFreeDriver(drvrd->Next);
		InsLibFreeModule(drvrd->Modules);
		free(drvrd);
	}
}

/**
 * @brief Free all memory from a host driver description
 *
 * @param hostd --
 *
 * <long-description>
 */
void InsLibFreeHost(InsLibHostDesc *hostd)
{
	if (hostd) {
		InsLibFreeDriver(hostd->Drivers);
		free(hostd);
	}
}

void InsLibPrintCarSpace(InsLibCarAddressSpace *caras)
{
	if (caras) {
		printf("[CAR:Dw:0x%X:Ws:%d]",
		       caras->DataWidth,
		       caras->WindowSize);
		InsLibPrintCarSpace(caras->Next);
	}
}

void InsLibPrintVmeSpace(InsLibVmeAddressSpace *vmeas)
{
	if (vmeas) {
		printf("[VME:Am:0x%X:Dw:%d:Ws:0x%X:Ba:0x%X:Fm:%d]",
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
		printf("[PCI:Bar:%d:En:%d]",
		       pcias->BaseAddressRegister,
		       pcias->Endian);
		InsLibPrintPciSpace(pcias->Next);
	}
}

void InsLibPrintCar(InsLibCarModuleAddress *carma)
{
	if (carma) {
		printf("[CAR:Bn:%d:Bp:%d]",
		       carma->BoardNumber,
		       carma->BoardPosition);
		InsLibPrintCarSpace(carma->CarAddressSpace);
	}
}

void InsLibPrintVme(InsLibVmeModuleAddress *vmema)
{
	if (vmema)
		InsLibPrintVmeSpace(vmema->VmeAddressSpace);
}

void InsLibPrintPci(InsLibPciModuleAddress *pcima)
{
	if (pcima) {
		printf("[PCI:Bn:%d:Sn:%d:VId:0x%X:DId:0x%X:SVId:"
		       "0x%X:SDId:0x%X]",
		       pcima->BusNumber,
		       pcima->SlotNumber,
		       pcima->VendorId,
		       pcima->DeviceId,
		       pcima->SubVendorId,
		       pcima->SubDeviceId);
		InsLibPrintPciSpace(pcima->PciAddressSpace);
	}
}

/**
 * @brief Print all memory for a module
 *
 * @param modld --
 *
 * <long-description>
 *
 */
void InsLibPrintModule(InsLibModlDesc *modld)
{
	if (modld) {
		printf("[MOD:%s Bt:%d:Mn:%d",
		       modld->ModuleName,
		       modld->BusType,
		       modld->ModuleNumber);
		if (modld->Isr) {
			printf(":Fl:%d:Vec:0x%X:Lvl:%d",
			       modld->Isr->IsrFlag,
			       modld->Isr->Vector,
			       modld->Isr->Level);
		}
		printf("]");
		if (modld->BusType == InsLibBusTypeVME)
			InsLibPrintVme(modld->ModuleAddress);
		else if (modld->BusType == InsLibBusTypeCARRIER)
			InsLibPrintCar(modld->ModuleAddress);
		else
			InsLibPrintPci(modld->ModuleAddress);
		printf("\n");
		InsLibPrintModule(modld->Next);
	}
}

/**
 * @brief Print all memory for a driver
 *
 * @param drvrd --
 *
 * <long-description>
 *
 */
void InsLibPrintDriver(InsLibDrvrDesc *drvrd)
{
	if (drvrd) {
		printf("[DRV:%s Dg:%d Df:0x%X Ef:0x%X Mc:%d]\n",
		       drvrd->DrvrName,
		       drvrd->isdg,
		       drvrd->DebugFlags,
		       drvrd->EmulationFlags,
		       drvrd->ModuleCount);

		InsLibPrintModule(drvrd->Modules);
		InsLibPrintDriver(drvrd->Next);
	}
}

/**
 * @brief Print all memory from a host driver description
 *
 * @param hostd --
 *
 * <long-description>
 *
 */
void InsLibPrintHost(InsLibHostDesc *hostd)
{
	if (hostd) {
		printf("[HST:%s]\n",
		       hostd->HostName);
		InsLibPrintDriver(hostd->Drivers);
	}
}

/**
 * @brief Parse PCI module description
 *
 * @param cur_node -- current XML node
 * @param pflag    -- debug print flag to print the tree
 * @param md       -- module description ptr
 *
 * @return NULL                       - failure
 * @return Pci module address pointer - if OK
 */
static InsLibPciModuleAddress *parse_pci_address_tag(xmlNode *cur_node,
						     int pflag,
						     InsLibModlDesc *md)
{
	KeyWord pky;
	InsLibPciModuleAddress *pcima;
	xmlAttr *prop = cur_node->properties;
	char *pname, *pvalu;

	if (!md) {
		prnterr("Encountered PCI clause with no previous MODULE"
			   " clause");
		return NULL;
	}

	if ( (md->BusType != InsLibBusTypePMC) &&
	     (md->BusType != InsLibBusTypePCI) ) {
		prnterr("Encountered PCI/PMC clause on a non PCI/PMC bus");
		return NULL;
	}

	pcima = (InsLibPciModuleAddress *)
		malloc(sizeof(InsLibPciModuleAddress));

	if (!pcima) {
		prnterr("Cant allocate memory for PCI");
		return NULL;
	}

	bzero((void *) pcima, sizeof(InsLibPciModuleAddress));

	if (!md->ModuleAddress)
		md->ModuleAddress = pcima;
	else {
		prnterr("More than one Module Address description"
			   " Encountered in PCI/PMC module description");
		free(pcima);
		return NULL;
	}

	while (prop) {
		prop = GetNextProp(prop, &pname, &pvalu, pflag);
		pky = LookUp((char *) pname);

		if (pky == BUS_NUMBER) {
			pcima->BusNumber = GetNumber(pvalu);
			if ( (pcima->BusNumber > 16) &&
			     (pcima->BusNumber != _NOT_DEF_) ) {
				prnterr("Bad PCI BUS number [%d]",
					   pcima->BusNumber);
				free(pcima);
				return NULL;
			}
		} else if (pky == SLOT_NUMBER) {
			pcima->SlotNumber = GetNumber(pvalu);
			if ( (pcima->SlotNumber > 16) &&
			     (pcima->SlotNumber != _NOT_DEF_) ) {
				prnterr("Bad PCI SLOT number [%d]",
					   pcima->SlotNumber);
				free(pcima);
				return NULL;
			}
		} else if (pky == VENDOR_ID) {
			pcima->VendorId = GetNumber(pvalu);
		} else if (pky == DEVICE_ID) {
			pcima->DeviceId = GetNumber(pvalu);
		} else if (pky == SUBVENDOR_ID) {
			pcima->SubVendorId = GetNumber(pvalu);
		} else if (pky == SUBDEVICE_ID) {
			pcima->SubDeviceId = GetNumber(pvalu);
		} else if (pky == COMMENT) {
			strncpy(pcima->Comment, pvalu, InsLibCOMMENT_SIZE);
		} else
			return pcima;
	}

	if (!pcima->BusNumber) {
		prnterr("BUS number missing from PCI clause");
		free(pcima);
		return NULL;
	}
	if (!pcima->SlotNumber) {
		prnterr("SLOT number missing from PCI clause");
		free(pcima);
		return NULL;
	}
	if (!pcima->VendorId) {
		prnterr("VENDOR ID missing from PCI clause");
		free(pcima);
		return NULL;
	}
	if (!pcima->DeviceId) {
		prnterr("DEVICE ID missing from PCI clause");
		free(pcima);
		return NULL;
	}

	if (pflag)
		printf("\n");

	return pcima;
}

/**
 * @brief Parse VME module description
 *
 * @param cur_node -- current XML node
 * @param pflag    -- debug print flag to print the tree
 * @param md       -- module description ptr
 *
 * @return NULL                       - failure
 * @return Pci module address pointer - if OK
 */
static InsLibVmeModuleAddress *parse_vme_address_tag(xmlNode *cur_node,
						     int pflag,
						     InsLibModlDesc *md)
{
	KeyWord pky;
	InsLibVmeModuleAddress *vmema;
	xmlAttr *prop;
	char *pname, *pvalu;

	if (!md) {
		prnterr("Encountered VME clause with no previous MODULE"
			   " clause");
		return NULL;
	}

	if (md->BusType != InsLibBusTypeVME) {
		prnterr("Encountered VME clause on a non VME bus");
		return NULL;
	}

	vmema = (InsLibVmeModuleAddress *)
		calloc(1, sizeof(InsLibVmeModuleAddress));

	if (!vmema) {
		prnterr("Cant allocate memory for VME");
		return NULL;
	}

	if (md->ModuleAddress == NULL)
		md->ModuleAddress = vmema;
	else {
		prnterr("More than one Module Address description"
			   " Encountered in VME module description");
		free(vmema);
		return NULL;
	}

	prop = cur_node->properties;
	while (prop) {
		prop = GetNextProp(prop, &pname, &pvalu, pflag);
		pky = LookUp(pname);

		if (pky == COMMENT)
			strncpy(vmema->Comment, pvalu, InsLibCOMMENT_SIZE);
		else
			return vmema;
	}

	if (pflag)
		printf("\n");

	return vmema;
}

/**
 * @brief Parse CARRIER module description
 *
 * @param cur_node -- current XML node
 * @param pflag    -- debug print flag to print the tree
 * @param md       -- module description ptr
 *
 * @return NULL                       - failure
 * @return Pci module address pointer - if OK
 */
static InsLibCarModuleAddress *parse_car_address_tag(xmlNode *cur_node,
						     int pflag,
						     InsLibModlDesc *md)
{
	KeyWord pky;
	InsLibCarModuleAddress *carma;
	xmlAttr *prop = cur_node->properties;
	char *pname, *pvalu;

	if (!md)
		prnterr("Encountered CARRIER clause with no previous MODULE"
			   " clause");
	if (md->BusType != InsLibBusTypeCARRIER)
		prnterr("Encountered CARRIER clause on a non CARRIER bus");

	carma = (InsLibCarModuleAddress *)
		malloc(sizeof(InsLibCarModuleAddress));

	if (!carma) {
		prnterr("Cant allocate memory for CARRIER");
		return NULL;
	}

	bzero((void *) carma, sizeof(InsLibCarModuleAddress));

	if (md->ModuleAddress == NULL)
		md->ModuleAddress = carma;
	else {
		prnterr("More than one Module Address description"
			   " Encountered in CARRIER module description");
		free(carma);
		return NULL;
	}

	while (prop) {
		prop = GetNextProp(prop, &pname, &pvalu, pflag);
		pky = LookUp((char *) pname);

		if (pky == BOARD_NUMBER) {
			carma->BoardNumber = GetNumber(pvalu);
			if ( (carma->BoardNumber < 1) ||
			     (carma->BoardNumber > 16) ) {
				prnterr("Bad BOARD NUMBER in CARRIER"
					   " clause");
				free(carma);
				return NULL;
			}


		} else if (pky == BOARD_POSITION) {
			carma->BoardPosition = GetNumber(pvalu);
			if ( (carma->BoardPosition < 1) ||
			     (carma->BoardPosition > 8) ) {
				prnterr("Bad BOARD POSITION in CARRIER"
					   " clause");
				free(carma);
				return NULL;
			}

		} else if (pky == COMMENT) {
			strncpy(carma->Comment, pvalu, InsLibCOMMENT_SIZE);

		} else
			return carma;
	}

	if (carma->BoardNumber == 0) {
		prnterr("BOARD NUMBER missing from CARRIER clause");
		free(carma);
		return NULL;
	}
	if (carma->BoardPosition == 0) {
		prnterr("BOARD POSITION missing from CARRIER clause");
		free(carma);
		return NULL;
	}

	if (pflag)
		printf("\n");

	return carma;
}

/**
 * @brief Add new VME address description in the linked list
 *
 * @param vmema  -- VME module description
 * @param nvmeas -- new Address Space description to add
 *
 * Configuration ROM/Control and Status Register.
 * CR/CSR space should come first in the list of Address Spaces because
 * they must be mapped first.
 */
static void add_vme_addr_space(InsLibVmeModuleAddress *vmema,
			       InsLibVmeAddressSpace *nvmeas)
{
	InsLibVmeAddressSpace *ptr;

	if (!vmema->VmeAddressSpace) {
		/* first one -- just add */
		vmema->VmeAddressSpace = nvmeas;
		return;
	}

	if (nvmeas->AddressModifier != AM_A24_CR_CSR) {
		/* add to the end of the list */
		ptr = vmema->VmeAddressSpace;
		while (ptr->Next) /* get to the last one */
			++ptr;
		ptr->Next = nvmeas;
		return;
	}

	/* This is CR/CSR -- add to the beginning of the list */
	nvmeas->Next = vmema->VmeAddressSpace;
	vmema->VmeAddressSpace = nvmeas;
	return;
}

static InsLibHostDesc* handle_install_node(xmlNode *cur_node, int pflag)
{
	xmlAttr *prop;
	char *pname, *pvalu;
	KeyWord pky;
	InsLibHostDesc *hostd = (InsLibHostDesc *)
		calloc(1, sizeof(InsLibHostDesc));

	if (!hostd) {
		prnterr("Cant allocate memory for Host description");
		return NULL;
	}

	prop = cur_node->properties;

	while (prop) {
		prop = GetNextProp(prop, &pname, &pvalu, pflag);
		pky = LookUp(pname);

		if (pky == HOST)
			strncpy(hostd->HostName, pvalu, InsLibNAME_SIZE);
		else if (pky == COMMENT)
			strncpy(hostd->Comment, pvalu, InsLibCOMMENT_SIZE);
		else
			break;
	}

	if (!strlen(hostd->HostName))
		prnterr("Host NAME missing from <install> clause");

	if (pflag)
		printf("\n");

	return hostd;
}

static void add_driver_node(InsLibHostDesc *hd, InsLibDrvrDesc *dd)
{
	InsLibDrvrDesc *ptr;

	if (!hd->Drivers) {
		/* first description */
		hd->Drivers = dd;
		return;
	}

	ptr = hd->Drivers;
	while (ptr->Next)
		ptr = ptr->Next;

	ptr->Next = dd;
}

static InsLibDrvrDesc* handle_driver_node(xmlNode *cur_node, int pflag,
					  InsLibHostDesc *hostd)
{
	xmlAttr *prop;
	char *pname, *pvalu;
	KeyWord pky;
	InsLibDrvrDesc *drvrd = (InsLibDrvrDesc *)
		calloc(1, sizeof(InsLibDrvrDesc));

	if (!drvrd) {
		prnterr("Cant allocate memory for Driver description");
		return NULL;
	}

	drvrd->isdg = -1; /* set default driverGen flag to 'not provided' */
	prop = cur_node->properties;
	while (prop) {
		prop = GetNextProp(prop, &pname, &pvalu, pflag);
		pky = LookUp(pname);

		if (pky == NAME)
			strncpy(drvrd->DrvrName, pvalu, InsLibNAME_SIZE);
		else if (pky == COMMENT)
			strncpy(drvrd->Comment, pvalu, InsLibCOMMENT_SIZE);
		else if (pky == INSTALL_DEBUG_LEVEL)
			drvrd->DebugFlags = GetNumber(pvalu);
		else if (pky == EMULATION_FLAG)
			drvrd->EmulationFlags = GetNumber(pvalu);
		else if (pky == DRIVERGEN)
			drvrd->isdg = (*pvalu == 'y') ? 1 : 0;
		else
			break;
	}
	if (strlen(drvrd->DrvrName) < 1)
		prnterr("Driver name missing from driver clause");
	if (pflag)
		printf("\n");

	add_driver_node(hostd, drvrd);
	return drvrd;
}

static void add_module_node(InsLibDrvrDesc *dd, InsLibModlDesc *nmd)
{
	InsLibModlDesc *ptr;

	if (!dd->Modules) {
		/* first */
		dd->Modules = nmd;
		return;
	}

	ptr = dd->Modules;
	while (ptr->Next)
		ptr = ptr->Next;

	ptr->Next = nmd;
	dd->ModuleCount++;
}

static InsLibModlDesc* handle_module_node(xmlNode *cur_node, int pflag,
					  InsLibDrvrDesc *drvrd)
{
	xmlAttr *prop;
	char *pname, *pvalu;
	KeyWord pky;
	InsLibModlDesc *modld = (InsLibModlDesc *)
		calloc(1, sizeof(InsLibModlDesc));

	if (!modld) {
		prnterr("Cant allocate memory for Module description");
		return NULL;
	}

	prop = cur_node->properties;
	while (prop) {
		prop = GetNextProp(prop, &pname, &pvalu, pflag);
		pky = LookUp(pname);

		if (pky == BUS_TYPE) {
			if (!strcmp(pvalu,"VME"))
				modld->BusType = InsLibBusTypeVME;
			else if (!strcmp(pvalu,"PCI"))
				modld->BusType = InsLibBusTypePCI;
			else if (!strcmp(pvalu,"PMC"))
				modld->BusType = InsLibBusTypePMC;
			else if (!strcmp(pvalu,"CAR"))
				modld->BusType = InsLibBusTypeCARRIER;
			else
				prnterr("Bad BUS_TYPE in MODULE clause");
		} else if (pky == LOGICAL_MODULE_NUMBER) {
			modld->ModuleNumber = GetNumber(pvalu);
			if ( ((modld->ModuleNumber < 1) ||
			      (modld->ModuleNumber > InsLibMAX_MODULES)) &&
			     (modld->ModuleNumber != _NOT_DEF_) )
				prnterr("Bad MODULE_NUMBER [ %d ] in MODULE"
					" clause", modld->ModuleNumber);

		} else if (pky == EXTRA) {
			modld->Extra = (char *) calloc(1, strlen(pvalu));
			if (!modld->Extra) {
				prnterr("Cant allocate memory for extra"
					" Module parameters");
				free(modld);
				return NULL;
			}
			strcpy(modld->Extra, pvalu);
		} else if (pky == COMMENT) {
			strncpy(modld->Comment, pvalu, InsLibCOMMENT_SIZE);

		} else if (pky == IGNORE_INSTALL_ERRORS) {
			modld->IgnoreErrors = 1;
		} else if (pky == NAME) {
			strncpy(modld->ModuleName, pvalu, InsLibNAME_SIZE);
		} else
			break;
	}

	if (modld->BusType == InsLibBusTypeNOT_SET)
		prnterr("BUS type missing from MODULE clause");
	if (modld->ModuleNumber == 0)
		prnterr("Logical Module Number is missing from MODULE clause");

	if (pflag) printf("\n");

	add_module_node(drvrd, modld);
	return modld;
}

static InsLibVmeAddressSpace*
handle_vme_space_node(xmlNode *cur_node,
		      int pflag,
		      InsLibVmeModuleAddress *vmema,
		      InsLibModlDesc *modld)
{
	xmlAttr *prop;
	char *pname, *pvalu;
	KeyWord pky;
	InsLibVmeAddressSpace *vmeas = (InsLibVmeAddressSpace *)
		calloc(1, sizeof(InsLibVmeAddressSpace));

	if (!vmeas) {
		prnterr("Cant allocate memory for VME Space description");
		return NULL;
	}

	prop = cur_node->properties;
	while (prop) {
		prop = GetNextProp(prop, &pname, &pvalu, pflag);
		pky = LookUp(pname);

		if (pky == MODIFIER) {
			vmeas->AddressModifier = GetNumber(pvalu);
			if (vmeas->AddressModifier > 0xFF)
				prnterr("%s: Bad address MODIFIER in "
					"<vme_space> clause",
					strlen(modld->ModuleName)?
					modld->ModuleName:
					"UNKNOWN MODULE");

		} else if (pky == DATA_WIDTH) {
			vmeas->DataWidth = GetNumber(pvalu);
			if ( (vmeas->DataWidth !=16) &&
			     (vmeas->DataWidth !=32) )
				prnterr("%s: Bad DATA_WIDTH in <vme_space>"
					" clause", strlen(modld->ModuleName)?
					modld->ModuleName:
					"UNKNOWN MODULE");
		} else if (pky == WINDOW_SIZE) {
			vmeas->WindowSize = GetNumber(pvalu);
		} else if (pky == ADDRESS) {
			vmeas->BaseAddress = GetNumber(pvalu);
		} else if (pky == FREE_FLAG) {
			vmeas->FreeModifierFlag = GetNumber(pvalu);
		} else if (pky == COMMENT) {
			strncpy(vmeas->Comment, pvalu, InsLibCOMMENT_SIZE);
		} else
			break;
	}

	if (!vmeas->AddressModifier)
		prnterr("%s: Address MODIFIER missing from <vme_space>",
			strlen(modld->ModuleName)?modld->ModuleName:
			"UNKNOWN MODULE");
	if (!vmeas->DataWidth)
		prnterr("%s: Data WIDTH missing from <vme_space>",
			strlen(modld->ModuleName)?modld->ModuleName:
			"UNKNOWN MODULE");
	if (!vmeas->WindowSize)
		prnterr("%s: WINDOW SIZE missing from <vme_space>",
			strlen(modld->ModuleName)?modld->ModuleName:
			"UNKNOWN MODULE");
	if (!vmeas->BaseAddress)
		prnterr("%s: VME BASE ADDRESS missing from <vme_space>",
			strlen(modld->ModuleName)?modld->ModuleName:
			"UNKNOWN MODULE");

	if (pflag)
		printf("\n");

	add_vme_addr_space(vmema, vmeas);
	return vmeas;
}
