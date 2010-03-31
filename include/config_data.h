/**
 * @file config_data.h
 *
 * @brief Common part of libinst used in kernel and user space
 *
 * @author Copyright (C) 2009 CERN CO/HT Julian Lewis
 *
 * @date Created on 02/02/2009
 *
 * @section license_sec License
 *          Released under the GPL
 */
#ifndef _CONFIG_DATA_H_INCLUDE_
#define _CONFIG_DATA_H_INCLUDE_

#define InsLibMAX_MODULES  16
#define InsLibNAME_SIZE    32
#define InsLibCOMMENT_SIZE 128

/** @brief Different bus types supported by skeleton and other drivers */
typedef enum {
	InsLibBusTypeNOT_SET,
	InsLibBusTypeCARRIER,
	InsLibBusTypeVME,
	InsLibBusTypePMC,
	InsLibBusTypePCI
} InsLibBusType;

/** @brief Endianity */
typedef enum {
	InsLibEndianNOT_SET,
	InsLibEndianBIG,
	InsLibEndianLITTLE
} InsLibEndian;


/** @brief Descriptor for installing an ISR */
typedef struct {
	unsigned int IsrFlag; /**< If 0 then no ISR is required */
	unsigned int Vector;
	unsigned int Level;
	char         Comment[InsLibCOMMENT_SIZE];
} InsLibIntrDesc;


/** @brief Start of a module description linked list
 *
 * @param BusType       --
 * @param ModuleNumber  -- Logical Unit Number. aka [L U N]
 *                         Must start at '1', zero is illegal.
 * @param IgnoreErrors  -- Tolerate install errors
 * @param Isr           --
 * @param ModuleAddress -- Any module type [PCI/VME/CARR] hardware description.
 * @param Extra         -- user can use it to pass extra params
 * @param Comment       --
 * @param Next          -- next module description
 * @param ModuleName    -- DB module name
 */
typedef struct {
	InsLibBusType   BusType;
	unsigned int    ModuleNumber;
	unsigned int	IgnoreErrors;
	InsLibIntrDesc *Isr;
	void           *ModuleAddress;
	char           *Extra;
	char            Comment[InsLibCOMMENT_SIZE];
	void           *Next;
	char            ModuleName[InsLibNAME_SIZE];
} InsLibModlDesc;

/** @brief The bus types PCI/VME/CAR are dealtwith in the driver anonymously
 *
 * So with these structures a driver can work with any bus type.
 */
typedef struct {
	void         *Mapped;
	void         *Next;
	unsigned int  SpaceNumber;
	unsigned int  WindowSize;
	unsigned int  DataWidth;
	InsLibEndian  Endian;
} InsLibAnyAddressSpace;


/** @brief */
typedef struct {
	InsLibAnyAddressSpace *AnyAddressSpace;
} InsLibAnyModuleAddress;


/** @brief Each PCI address space is a BAR and is mapped to kernel memory
 *
 * @param Mapped              -- Mapped kernel memory pointer or NULL
 * @param Next                -- Next BAR (if any)
 * @param BaseAddressRegister -- which BAR [0 - 5]
 * @param WindowSize          --
 * @param DataWidth           --
 * @param Endian              --
 * @param Comment             --
 */
typedef struct {
	void         *Mapped;
	void         *Next;
	unsigned int  BaseAddressRegister;
	unsigned int  WindowSize;
	unsigned int  DataWidth;
	InsLibEndian  Endian;
	char          Comment[InsLibCOMMENT_SIZE];
} InsLibPciAddressSpace;


/** @brief  A basic PCI module hardware address */
typedef struct {
	InsLibPciAddressSpace *PciAddressSpace; /**< Points to all the BAR
						   registers */
	unsigned int           BusNumber;
	unsigned int           SlotNumber;
	unsigned int           VendorId;
	unsigned int           DeviceId;
	unsigned int           SubVendorId;
	unsigned int           SubDeviceId;
	char                   Comment[InsLibCOMMENT_SIZE];
} InsLibPciModuleAddress;

/** @brief Each VME address space is a VME base address mapped to kernel
 *         memory.
 *
 * @param Mapped           -- Virtual Base Address
 * @param Next             -- next VME addr space (if any)
 * @param AddressModifier  -- SpaceNumber is the VME address modifier
 * @param WindowSize       --
 * @param DataWidth        --
 * @param Endian           --
 * @param BaseAddress      -- VME base address
 * @param FreeModifierFlag --
 * @param Comment          --
 */
typedef struct {
	void         *Mapped;
	void         *Next;
	unsigned int  AddressModifier;
	unsigned int  WindowSize;
	unsigned int  DataWidth;
	InsLibEndian  Endian;
	unsigned int  BaseAddress;
	unsigned int  FreeModifierFlag;
	char          Comment[InsLibCOMMENT_SIZE];
} InsLibVmeAddressSpace;


/** @brief The VME module hardware address is just its address spaces */
typedef struct {
	InsLibVmeAddressSpace *VmeAddressSpace; /**<  */
	char                   Comment[InsLibCOMMENT_SIZE]; /**<  */
} InsLibVmeModuleAddress;


/** @brief Carrier boards provide a bus on which piggy back boards ride */
typedef struct {
	void         *Mapped; /**< Points to the piggy backs mapped kernel
				 memory */
	void         *Next;	/**< next carrier addr space (if any) */
	unsigned int  SpaceNumber; /**< SpaceNumber for carrier boards its just
				      a number */
	unsigned int  DataWidth;
	unsigned int  WindowSize;
	InsLibEndian  Endian;
	char          Comment[InsLibCOMMENT_SIZE];
 } InsLibCarAddressSpace;


/** @brief The hardware address is the carrier board number and position
 *         on that board
 */
typedef struct {
	InsLibCarAddressSpace *CarAddressSpace;
	char                   DriverName[InsLibNAME_SIZE];
	unsigned int           MotherboardNumber;
	unsigned int           SlotNumber;
	char                   Comment[InsLibCOMMENT_SIZE];
} InsLibCarModuleAddress;


/** @brief A driver resides on a host computer and has a name and a list
 *         of modules
 */
typedef struct {
	InsLibModlDesc *Modules; /**< all controlled modules of the driver */
	void           *Next; /**< next driver description */
	char            DrvrName[InsLibNAME_SIZE]; /**< driver name */
	unsigned int    DebugFlags;
	unsigned int    EmulationFlags;
	unsigned int    ModuleCount;	/**< how many modules driver controls */
	char            Comment[InsLibCOMMENT_SIZE]; /**< comment string */
	int             isdg; /**< if driver was generated by the DriverGen?
				  0 -- no
				  1 -- yes
				 -1 -- not specified */
} InsLibDrvrDesc;

/** @brief The root node for the host */
typedef struct {
	InsLibDrvrDesc *Drivers; /**< drivers list */
	char            HostName[InsLibNAME_SIZE];
	char            Comment[InsLibCOMMENT_SIZE];
} InsLibHostDesc;



#endif	/* _CONFIG_DATA_H_INCLUDE_ */
