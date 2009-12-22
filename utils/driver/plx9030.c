/**
 * @file plx9030.c
 *
 * @brief PCI SMARTarget I/O PCI Accelerator.
 *
 * @author Julian Lewis
 *
 * @date Mar, 2008
 *
 * @version 1.0  ygeorgie  27/03/2008  Initial version.
 */
#include "drvr_dbg_prnt.h"
#include "plx9030.h"
#include "general_drvr.h"
#include "general_both.h"

#define YGEORGIE_VERS
//#define LEWIS_VERS

static void plx9030_open(plx_info_t *card);
static void plx9030_close(plx_info_t *card);
static int rwPlx9030lcr(plx_info_t *card, unsigned long addr, unsigned long *value, unsigned long size, plxrw_t rwflg);
static void send_eprom_cmd(plx_info_t *card, EpCmd cmd, unsigned long adr);
static int write_eprom_word(plx_info_t *card, unsigned long word);
static void read_eprom_word(plx_info_t *card, unsigned long *word);


#if 0
/**
 * @brief Read PLX9030 Local Configuration Registers (see 10-3)
 *
 * @param
 *
 * @return
 */
int readPlx9030lcr(plx_info_t *card, unsigned long addr, unsigned long size)
{}


/**
 * @brief Write PLX9030 Local Configuration Registers (see 10-3)
 *
 * @param
 *
 * @return
 */
int writePlx9030lcr(plx_info_t *card, unsigned long addr, unsigned long *value, unsigned long size)
{}
#endif

/**
 * @brief Read EEPROM memory
 *
 * @param card   - plx description
 * @param reg    - EEPROM register (starting from 1!) to read
 *                 (0 for all the registers)
 * @param e_data - data will go here
 *
 * @return how many registers were read
 */
int plx9030_read_eeprom(plx_info_t *card, int reg, Plx9030Eprom_t* e_data)
{
  int cntr = 0;
  unsigned long word;
  unsigned short *ptr = (unsigned short*)e_data;

  if (!reg) { /* user wants to read all the registers */
    for (cntr = 0; cntr < Plx9030EPROM_REGS; cntr++) {
      send_eprom_cmd(card, EpCmdREAD, cntr/*word to read*/);
      read_eprom_word(card, &word);
      ptr[cntr] = (unsigned short) word;
    }
  } else { /* user wants specific register [1 - 68] (indexes [0 - 67]) */
    if (WITHIN_RANGE(Plx9030DeviceId + 1, reg, Plx9030EPROM_REGS)) {
      send_eprom_cmd(card, EpCmdREAD, reg - 1/*word to read*/);
      read_eprom_word(card, &word);
      ptr[reg - 1] = word;
      cntr = 1;
    }
  }

  return(cntr);
}

/**
 * @brief Write EEPROM memory
 *
 * @param card   - plx description
 * @param reg    - EEPROM register (starting from 1) to write
 *                 (0 for all the registers)
 * @param e_data - data will be taken from here
 *
 * @return how many registers were written
 */
int plx9030_write_eeprom(plx_info_t *card, int reg, Plx9030Eprom_t* e_data)
{
  int cntr = 0;
  unsigned short *ptr = (unsigned short*)e_data;
  unsigned long word;

  plx9030_open(card);
  if (!reg) { /* user wants to write all the registers */
    for (cntr = 0; cntr < Plx9030EPROM_REGS; cntr++) {
      send_eprom_cmd(card, EpCmdWRITE, cntr/*word to write*/);
      if (write_eprom_word(card, ptr[cntr]) != PLX_OK) {
	cntr = 0;
	break;
      }
    }
  } else { /* user wants specific register [1 - 68] (indexes [0 - 67]) */
    if (WITHIN_RANGE(Plx9030DeviceId + 1, reg, Plx9030EPROM_REGS)) {
      send_eprom_cmd(card, EpCmdWRITE, reg - 1/*word index to write*/);
      word = ptr[reg - 1];
      if (write_eprom_word(card, word) == PLX_OK)
	cntr = 1;
    }
  }

  plx9030_close(card);
  return(cntr);
}


/**
 * plx9030_open - Enables EEPROM for reading/writing.
 *
 * @card: PLX9030 data
 */
static void plx9030_open(plx_info_t *card)
{
  unsigned long cntrl; /* PLX9030 serial EEPROM control register */

  if (card->plx_flash_open == 0) { /* check if not already open */
    /* Assert the chip select for the EEPROM and the target retry delay
       clocks. See PLX PCI 9030 Data Book 10-35 */
    cntrl  = Plx9030CntrlCHIP_UNSELECT; /* Chip un-select/reset 93lc56b EEPROM */
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
    cntrl |= Plx9030CntrlCHIP_SELECT;   /* Chip select 93lc56b EEPROM */
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
    send_eprom_cmd(card, EpCmdEWEN, 0);
    card->plx_flash_open = 1;
  }
}


/**
 * plx9030_close - Disable EEPROM for reading/writing.
 *
 * @card: PLX9030 data
 */
static void plx9030_close(plx_info_t *card)
{
  unsigned long cntrl; /* PLX9030 serial EEPROM control register */

  if (card->plx_flash_open == 1) { /* check if open */
    send_eprom_cmd(card, EpCmdEWDS, 0);
    cntrl = Plx9030CntrlCHIP_UNSELECT; /* Chip un-select 93lc56b EEPROM */
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
    card->plx_flash_open = 0;
  }
}



/**
 * @brief Read/Write PLX9030 Local Configuration Registers (see 10-3)
 *
 * @card:  plx data
 * @addr:  address to read/write
 * @value: value is [taken from/placed here], depending on the @e flag
 * @size:  size of the @e value to read/write
 * @rwflg: r/w flag. if PLX_LCR_WR (1) - write, if PLX_LCR_RD (0) - read
 *
 * The local config space is always Little Endian. This routine always uses
 * Long access, for smaller sizes the Long is first read, modified, and
 * written back.
 *
 * @return  PLX_OK              - we cool
 * @return -PLX_NOT_INITIALIZED - local config space address not set
 * @return -PLX_NOT_SUPPORTED   - value size is not supported
 */
static int rwPlx9030lcr(plx_info_t *card, unsigned long addr, unsigned long *value, unsigned long size, plxrw_t rwflg)
{
  int i;
  unsigned long regnum, regval;
  unsigned char *cp;
  unsigned short *sp;

  if (card->plx_remap) {

    cp = (char  *) &regval; /* For size 1, byte */
    sp = (short *) &regval; /* For size 2, short */
    regnum = addr>>2;       /* Long address is the regnum */

    /* get current value */
    //regval = le32_to_cpu(ioread32(card->plx_remap[regnum]));
    regval = le32_to_cpu(ioread32(card->plx_remap + addr));

    if (rwflg == PLX_LCR_WR) { /* Write */
      switch (size) {
      case 1: /* Byte */
	i = 3 - (addr & 3);              /* Address of Byte in regval */
	cp[i] = (unsigned char) *value;  /* Overwrite the Byte */
	break;

      case 2: /* Short */
	i = 1 - ((addr>>1) & 1);         /* Address of Short in regval */
	sp[i] = (unsigned short) *value; /* Overwrute the short */
	break;

      case 4: /* Long */
	regval = *value;                 /* Overwrite the Long */
	break;
      default:
	return(-PLX_NOT_SUPPORTED); /* ERROR. Not supported */
      }

      /* write back data */
      //card->plx_remap[regnum] = cpu_to_le32(regval);
      iowrite32(cpu_to_le32(regval), card->plx_remap + addr);
    } else { /* it's a Read */
      switch (size) {
      case 1:/* Byte */
	i = 3 - (addr & 3);              /* Address of Byte in regval */
	*value = (unsigned long) cp[i];  /* Return Byte value */
	break;
      case 2: /* Short */
	i = 1 - ((addr>>1) & 1);         /* Address of Short in regval */
	*value = (unsigned long) sp[i];  /* Return Short value */
	break;
      case 4: /* Long */
	*value = regval;
	break;
      default:
	return(-PLX_NOT_SUPPORTED); /* ERROR. Not supported */
      }
    }
    return(PLX_OK);
  }

  PRNT_ABS_ERR("PLX_NOT_INITIALIZED!");
  return(-PLX_NOT_INITIALIZED);
}


/**
 * @brief Send a command to the 93lc56b EEPROM
 *
 * @param card - plx card description
 * @param cmd  -
 * @param adr  - eeprom word to read/write (68 16-bit words. See 3-3)
 *
 * @return
 */
static void send_eprom_cmd(plx_info_t *card, EpCmd cmd, unsigned long adr)
{
  unsigned long cntrl;
  unsigned short cmnd, msk;
  int i;

  /* Reset flash memory state machine */

  cntrl = Plx9030CntrlCHIP_UNSELECT;
  /* Chip select off */
  rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);

  cntrl |= Plx9030CntrlDATA_CLOCK;

  /* Strobe Clk up */
  rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);

  cntrl = Plx9030CntrlCHIP_UNSELECT;

  /* Strobe Clk down */
  rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);

  cmnd = ((0x7F & (unsigned short) adr) << 5) | (unsigned short) cmd;
  msk = 0x8000;
  for (i = 0; i < EpClkCycles; i++) {
    if (msk & cmnd)
      cntrl = Plx9030CntrlDATA_OUT_ONE;
    else
      cntrl = Plx9030CntrlDATA_OUT_ZERO;

    /* Write 1 or 0 */
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
    cntrl |= Plx9030CntrlDATA_CLOCK;

    /* Clk Up */
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
    msk = msk >> 1;
  }

}


/**
 * plx9030_erase_eeprom -
 *
 * @card: PLX9030 data
 */
void plx9030_erase_eeprom(plx_info_t *card)
{
  plx9030_open(card);
  /* Erase the EEPROM 93LC56B, deletes PLX9030 configuration */
  send_eprom_cmd(card, EpCmdERAL, 0);
  plx9030_close(card);
}


/**
 * plx9030_software_reset - reset PCI 9030 and assert LRESET#
 *
 * @card: PLX9030 data
 *
 * After reset, the PCI9030 attempts to read the serial EEPROM to determine
 * its presence.
 */
void plx9030_software_reset(plx_info_t *card)
{
  unsigned long cntrl = 0; /* Plx control word */

  plx9030_open(card);

  rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_RD); /* read current */
  cntrl |= Plx9030CntrlPCI_SOFTWARE_RESET; /* set software reset */
  rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR); /* write it back */
  cntrl &= ~Plx9030CntrlPCI_SOFTWARE_RESET; /* disable software reset bit */
  rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR); /* reset is done */

  plx9030_close(card);
}


/**
 * @brief
 *
 * @param pdev - pci device description
 * @param card - PLX info table
 *
 * @return
 */
int plx9030_init(struct pci_dev *pdev, plx_info_t *card)
{
  unsigned long las0brd;	/* Local Address Space 0 */

#ifdef YGEORGIE_VERS
  unsigned short plx_command_reg; /* ygeorgie version */
#elif LEWIS_VERS
  unsigned int dword;		/* Julian's version */
#endif /* YGEORGIE_VERS */

  /* Ensure using memory mapped I/O */
#ifdef YGEORGIE_VERS
  pci_read_config_word(pdev, PLX_PCI_CMD, &plx_command_reg);
  //printk("-------> COMMAND1 is %#x (%s)\n", plx_command_reg, bitprint(plx_command_reg));
  plx_command_reg |= 2;
  pci_write_config_word(pdev, PLX_PCI_CMD, plx_command_reg);
  //pci_read_config_word(pdev, PLX_PCI_CMD, &plx_command_reg);
  //printk("-------> COMMAND2 is %#x (%s)\n", plx_command_reg, bitprint(plx_command_reg));
#elif LEWIS_VERS
  pci_read_config_dword(pdev, 1, &dword);
  dword |= 2;
  pci_write_config_dword(pdev, 1, dword);
#endif /* YGEORGIE_VERS */


  if (map_bars(&card->plx_bar_list, pdev, (1<<0)/*we need to map only BAR0*/, "PLX9030") != 1) {
    PRNT_ABS_ERR("Can't map plx9030 (BAR0) local configuration");
    return(-PLX_FAILED);
  }

  /* set mapped memory */
  card->plx_remap = container_of(list_idx(&card->plx_bar_list, 0), bar_map_t, mem_list)->mem_remap;

  /* Set up the LAS0BRD local configuration register to do appropriate
     endian mapping so that board operates in CPU endian, and enable the
     address space.
     See PLX PCI 9030 Data Book Chapter 10-2 && 10-21 for more details */

  rwPlx9030lcr(card, PLX9030_LAS0BRD, &las0brd, sizeof(las0brd), PLX_LCR_RD);
  //printk("-------> PLX9030_LAS0BRD#1 is %#lx (%s)\n", las0brd, bitprint(las0brd));

#ifdef __BIG_ENDIAN
  las0brd = Plx9030Las0brdBIG_ENDIAN; /* Big endian mapping */
#elif __LITTLE_ENDIAN
  las0brd = Plx9030Las0brdLITTLE_ENDIAN; /* Little endian mapping */
#else
#error "__BIG_ENDIAN or __LITTLE_ENDIAN must be defined!"
#endif

  /* enable space */
  rwPlx9030lcr(card, PLX9030_LAS0BRD, &las0brd, sizeof(las0brd), PLX_LCR_WR);


  /*
  pci_read_config_dword(pdev, 1, &dword);
  pci_read_config_word(pdev, PLX_PCI_CMD, &plx_command_reg);
  printk("-------> COMMAND1 is %#x (%s)\n", plx_command_reg, bitprint(plx_command_reg));

  printk("-------> DWORD1 is %#x (%s)\n", dword, bitprint(dword));
  dword |= 2;
  pci_write_config_dword(pdev, 1, dword);
  pci_read_config_dword(pdev, 1, &dword);

  pci_read_config_word(pdev, PLX_PCI_CMD, &plx_command_reg);
  printk("-------> COMMAND2 is %#x (%s)\n", plx_command_reg, bitprint(plx_command_reg));
  printk("-------> DWORD2 is %#x (%s)\n", dword, bitprint(dword));
  */

  /*
  pci_read_config_word(pdev, 2, &plx_command_reg);
  printk("-------> DEVICE ID is %#x (%s)\n", plx_command_reg, bitprint(plx_command_reg));
  pci_read_config_word(pdev, 0, &plx_command_reg);
  printk("-------> VENDOR data is %#x (%s)\n", plx_command_reg, bitprint(plx_command_reg));
  */

  return(PLX_OK);
}


/**
 * @brief Write a data word to the 93lc56b EEPROM
 *
 * @param card - plx card description
 * @param word - data to write
 *
 * @return  PLX_OK      - word written
 * @return -PLX_TIMEOUT - Timout from 93lc56B
 */
static int write_eprom_word(plx_info_t *card, unsigned long word)
{
  unsigned long cntrl = 0;
  unsigned short bits, msk;
  int i, plxtmo;

  bits = (unsigned short) word;
  msk = 0x8000;

  for (i = 0; i < 16; i++) {
    if (msk & bits)
      cntrl = Plx9030CntrlDATA_OUT_ONE;
    else
      cntrl = Plx9030CntrlDATA_OUT_ZERO;

    /* Write 1 or 0 */
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
    cntrl |= 0x01000000;
    /* Strobe Clk */
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
    msk = msk >> 1;
  }

  cntrl = Plx9030CntrlCHIP_UNSELECT;
  /* Chip select off */
  rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
  cntrl = Plx9030CntrlCHIP_SELECT;
  /* Chip select on */
  rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, 4, PLX_LCR_WR);

  plxtmo = 6000; /* At 1us per cycle = 6ms */
  while (((Plx9030CntrlDONE_FLAG & cntrl) == 0) && (plxtmo-- > 0)) {
    /* Read Busy Status */
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_RD);
  }

  cntrl = Plx9030CntrlCHIP_UNSELECT;
  /* Chip select off */
  rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);

  if (plxtmo <= 0) {
    PRNT_ABS_DBG_MSG("Timeout from 93lc56B");
    return(-PLX_TIMEOUT);
  }
  return(PLX_OK);
}


/**
 * @brief Read a data word from the 93lc56b EEPROM
 *
 * @param card - plx card description
 * @param word - eeprom data will go here
 *
 * @return void
 */
static void read_eprom_word(plx_info_t *card, unsigned long *word)
{
  unsigned long cntrl = 0;
  unsigned short bits, msk;
  int i;

  bits = 0;
  msk = 0x8000;
  for (i = 0; i < 16; i++) {
    cntrl = Plx9030CntrlCHIP_SELECT;
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
    cntrl |= Plx9030CntrlDATA_CLOCK;
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_WR);
    rwPlx9030lcr(card, PLX9030_CNTRL, &cntrl, sizeof(cntrl), PLX_LCR_RD);
    if (cntrl & Plx9030CntrlDATA_IN_ONE) bits |= msk;
    msk = msk >> 1;
  }

  *word = (unsigned long) bits;
}


#if 0
/**
 * @brief
 *
 * @param
 *
 * @return
 */
/* ========================================================== */
/* Read/Write PLX9030 config registers.                       */
/* This is needed because of the brain dead definition of the */
/* offset field in drm_read/write as long addresses.          */
/* ========================================================== */
static int DrmConfigReadWrite(CtrDrvrModuleContext *mcon,unsigned long addr, unsigned long *value, unsigned long size, unsigned long flag) {
int i, cc;
unsigned long regnum, regval;
unsigned char *cp;
unsigned short *sp;

   cp = (char  *) &regval;  /* For size 1, byte */
   sp = (short *) &regval;  /* For size 2, short */
   regnum = addr>>2;        /* The regnum turns out to be the long address */

   /* First read the long register value (Forced because of drm lib) */

   cc = drm_device_read(mcon->Handle,PCI_RESID_REGS,regnum,0,&regval);
   if (cc) {
      printk("%s@%d: Error (%d) From drm_device_read\n", __func__, __LINE__, cc);
      return cc;
   }

   /* For read/write access the byte or short or long in regval */

   if (flag) {                                  /* Write */

      /* Modify regval according to address and size */

      switch (size) {
	 case 1:                                /* Byte */
	    i = 3 - (addr & 3);                 /* Byte in regval */
	    cp[i] = (unsigned char) *value;
	 break;

	 case 2:                                /* Short */
	   i = 1 - ((addr>>1) & 1);             /* Shorts in regval */
	   sp[i] = (unsigned short) *value;
	 break;

	 default: regval = *value;              /* Long */
      }

      /* Write back modified configuration register */

      cc = drm_device_write(mcon->Handle,PCI_RESID_REGS,regnum,0,&regval);
      if (cc) {
	 printk("%s()@%d: Error (%d) From drm_device_write\n", __func__, __LINE__, cc);
	 return cc;
      }

   } else {                                     /* Read */

      /* Extract the data from regval according to address and size */

      switch (size) {
	 case 1:
	    i = 3 - (addr & 3);
	    *value = (unsigned long) cp[i];
	 break;

	 case 2:
	   i = 1 - ((addr>>1) & 1);
	   *value = (unsigned long) sp[i];
	 break;

	 default: *value = regval;
      }
   }
   return OK;
}


/**
 * @brief
 *
 * @param
 *
 * @return
 */
/* ========================================================== */
/* Rebuild PCI device tree, uninstall devices and reinstall.  */
/* Needed if device descriptions (ie IDs)  are modified.      */
/* This should be done after the Plx9030 EEPROM has been      */
/* reloaded, or simply do a reboot of the DSC.                */
/* ========================================================== */
static int RemapFlag = 0;

static int Remap(CtrDrvrModuleContext *mcon) {

   drm_locate(mcon->Handle);    /* Rebuild PCI tree */

   RemapFlag = 1;               /* Don't release memory */
   CtrDrvrUninstall(Wa);        /* Uninstall all */
   CtrDrvrInstall(Wa);          /* Re-install all */
   RemapFlag = 0;               /* Normal uninstall */

   return OK;
}


/*========================================================================*/
/* IOCTL                                                                  */
/*========================================================================*/
/**
 * @brief
 *
 * @param wa  - Working area
 * @param flp - File pointer
 * @param cm  - IOCTL command
 * @param arg -
 *
 * @return
 */
static int CtrDrvrIoctl(CtrDrvrWorkingArea *wa, struct file *flp, CtrDrvrControlFunction cm, char *arg)
{
  unsigned long cntrl;      /* PLX9030 serial EEPROM control register */


  /*************************************/
  /* Decode callers command and do it. */
  /*************************************/

  switch (cm) {
  case CtrDrvrGET_MODULE_DESCRIPTOR:
    if (wcnt >= sizeof(CtrDrvrModuleAddress)) {
      moad = (CtrDrvrModuleAddress *) arg;

      if (mcon->DeviceId == NBCI1553_DEVICE_ID) {
	moad->ModuleType = CtrDrvrModuleTypeCTR;
	moad->DeviceId   = NBCI1553_DEVICE_ID;
	moad->VendorId   = CERN_VENDOR_ID;
	moad->MemoryMap  = (unsigned long) mcon->Map;
	moad->LocalMap   = (unsigned long) mcon->Local;
      } else {
	moad->ModuleType = CtrDrvrModuleTypePLX;
	moad->DeviceId   = PLX9030_DEVICE_ID;
	moad->VendorId   = PLX9030_VENDER_ID;
	moad->MemoryMap  = 0;
	moad->LocalMap   = (unsigned long) mcon->Local;
      }
      moad->ModuleNumber  = mcon->ModuleIndex +1;
      moad->PciSlot       = mcon->PciSlot;


      return OK;
    }
    break;
  case CtrDrvrJTAG_OPEN:              /* Open JTAG interface */

    lval = Plx9030IntcsrLINT_HIGH;
    /* Kill interrupts */
    rwPlx9030lcr(mcon, PLX9030_INTCSR, &lval, 2, PLX_LCR_WR);

    lval = CtrDrvrJTAG_PIN_TMS_OUT
      | CtrDrvrJTAG_PIN_TCK_OUT
      | CtrDrvrJTAG_PIN_TDI_OUT; /* I/O Direction out */

    rwPlx9030lcr(mcon, PLX9030_GPIOC, &lval, 4, PLX_LCR_WR);
    mcon->FlashOpen = 1;
    return OK;

  case CtrDrvrJTAG_READ_BYTE:         /* Read back uploaded VHDL bit stream */

    if (lap) {
      if (mcon->FlashOpen) {
	rwPlx9030lcr(mcon, PLX9030_GPIOC, &lval, 4, PLX_LCR_RD);
	if (lval & CtrDrvrJTAG_PIN_TDO_ONE) *lap = CtrDrvrJTAG_TDO;
	else                                *lap = 0;
	return OK;
      }
      pseterr(EBUSY);                  /* Device busy, not opened */
      return SYSERR;
    }
    break;
  case CtrDrvrJTAG_WRITE_BYTE:        /* Upload a new compiled VHDL bit stream */

    if (lap) {
      if (mcon->FlashOpen) {

	lval = CtrDrvrJTAG_PIN_TMS_OUT
	  | CtrDrvrJTAG_PIN_TCK_OUT
	  | CtrDrvrJTAG_PIN_TDI_OUT; /* I/O Direction out */

	if (lav & CtrDrvrJTAG_TMS) lval |= CtrDrvrJTAG_PIN_TMS_ONE;
	if (lav & CtrDrvrJTAG_TCK) lval |= CtrDrvrJTAG_PIN_TCK_ONE;
	if (lav & CtrDrvrJTAG_TDI) lval |= CtrDrvrJTAG_PIN_TDI_ONE;

	rwPlx9030lcr(mcon, PLX9030_GPIOC, &lval, 4, PLX_LCR_WR);
	return OK;
      }
      pseterr(EBUSY);                  /* Device busy, not opened */
      return SYSERR;
    }
    break;
  case CtrDrvr93LC56B_EEPROM_OPEN:    /* Open the PLX9030 configuration EEPROM 93LC56B for Write */
    if (mcon->FlashOpen == 0) {

      /* Assert the chip select for the EEPROM and the target retry delay clocks */
      /* See PLX PCI 9030 Data Book 10-35 */

      cntrl  = Plx9030CntrlCHIP_UNSELECT; /* Chip un-select/reset 93lc56b EEPROM */
      rwPlx9030lcr(mcon, PLX9030_CNTRL, &cntrl, 4, PLX_LCR_WR);
      cntrl |= Plx9030CntrlCHIP_SELECT;   /* Chip select 93lc56b EEPROM */
      rwPlx9030lcr(mcon, PLX9030_CNTRL, &cntrl, 4, PLX_LCR_WR);
      send_eprom_cmd(mcon,EpCmdEWEN,0);
      mcon->FlashOpen = 1;
    }
    return OK;
  case CtrDrvr93LC56B_EEPROM_READ:    /* Read from the EEPROM 93LC56B the PLX9030 configuration */
    if (mcon->FlashOpen) {
      if (wcnt >= sizeof(CtrDrvrRawIoBlock)) {
	riob = (CtrDrvrRawIoBlock *) arg;
	if (riob->UserArray != NULL) {
	  send_eprom_cmd(mcon,EpCmdREAD,(riob->Offset)>>1);
	  read_eprom_word(mcon,&(riob->UserArray[0]));
	  return OK;
	}
      }
    }
    break;
  case CtrDrvr93LC56B_EEPROM_WRITE:   /* Write to the EEPROM 93LC56B a new PLX9030 configuration */
    if (mcon->FlashOpen) {
      if (rcnt >= sizeof(CtrDrvrRawIoBlock)) {
	riob = (CtrDrvrRawIoBlock *) arg;
	if (riob->UserArray != NULL) {
	  send_eprom_cmd(mcon, EpCmdWRITE, (riob->Offset)>>1/*in words*/);
	  write_eprom_word(mcon, riob->UserArray[0]);
	  return OK;
	}
      }
    }
    break;
  case CtrDrvr93LC56B_EEPROM_ERASE:   /* Erase the EEPROM 93LC56B, deletes PLX9030 configuration */
    if (mcon->FlashOpen) {
      send_eprom_cmd(mcon,EpCmdERAL,0);
      return OK;
    }
    break;
  case CtrDrvr93LC56B_EEPROM_CLOSE:   /* Close EEPROM 93LC56B and load new PLX9030 configuration */
    send_eprom_cmd(mcon,EpCmdEWDS,0);
    cntrl = Plx9030CntrlCHIP_UNSELECT; /* Chip un-select 93lc56b EEPROM */
    rwPlx9030lcr(mcon, PLX9030_CNTRL, &cntrl, 4, PLX_LCR_WR);
    mcon->FlashOpen = 0;
    return OK;
  case CtrDrvrPLX9030_RECONFIGURE:    /* Load EEPROM configuration into the PLX9030 */
    if (mcon->FlashOpen == 0) {
      cntrl  = Plx9030CntrlRECONFIGURE;   /* Reload PLX9030 from 93lc56b EEPROM */
      rwPlx9030lcr(mcon, PLX9030_CNTRL, &cntrl, 4, PLX_LCR_WR);
      return OK;
    }
    break;                              /* EEPROM must be closed */
  case CtrDrvrPLX9030_CONFIG_OPEN:    /* Open the PLX9030 configuration for read */
    mcon->ConfigOpen = 1;
    return OK;
  case CtrDrvrPLX9030_CONFIG_READ:    /* Read the PLX9030 configuration registers */
    if (mcon->ConfigOpen) {
      if (wcnt >= sizeof(CtrDrvrRawIoBlock)) {
	riob = (CtrDrvrRawIoBlock *) arg;
	if ((riob->UserArray != NULL) &&  (wcnt > riob->Size)) {
	  return DrmConfigReadWrite(mcon,riob->Offset,riob->UserArray,riob->Size,0);
	}
      }
    }
    break;

  case CtrDrvrPLX9030_CONFIG_WRITE:   /* Write to PLX9030 configuration registers (Experts only) */
    if (mcon->ConfigOpen) {
      if (rcnt >= sizeof(CtrDrvrRawIoBlock)) {
	riob = (CtrDrvrRawIoBlock *) arg;
	if ((riob->UserArray != NULL) &&  (rcnt >= riob->Size)) {
	  return DrmConfigReadWrite(mcon,riob->Offset,riob->UserArray,riob->Size,1);
	}
      }
    }
    break;
  case CtrDrvrPLX9030_CONFIG_CLOSE:   /* Close the PLX9030 configuration */
    mcon->ConfigOpen = 0;
    return OK;
  case CtrDrvrPLX9030_LOCAL_OPEN:     /* Open the PLX9030 local configuration for read */
    mcon->LocalOpen = 1;
    return OK;
  case CtrDrvrPLX9030_LOCAL_READ:     /* Read the PLX9030 local configuration registers */
    if (mcon->LocalOpen) {
      if (wcnt >= sizeof(CtrDrvrRawIoBlock)) {
	riob = (CtrDrvrRawIoBlock *) arg;
	if ((riob->UserArray != NULL) &&  (wcnt >= riob->Size)) {
	  return rwPlx9030lcr(mcon, riob->Offset, riob->UserArray, riob->Size, PLX_LCR_RD);
	}
      }
    }
    break;

  case CtrDrvrPLX9030_LOCAL_WRITE:    /* Write the PLX9030 local configuration registers (Experts only) */
    if (mcon->LocalOpen) {
      if (rcnt >= sizeof(CtrDrvrRawIoBlock)) {
	riob = (CtrDrvrRawIoBlock *) arg;
	if ((riob->UserArray != NULL) &&  (rcnt > riob->Size)) {
	  return rwPlx9030lcr(mcon, riob->Offset, riob->UserArray, riob->Size, PLX_LCR_WR);
	}
      }
    }
    break;

  case CtrDrvrPLX9030_LOCAL_CLOSE:    /* Close the PLX9030 local configuration */
    mcon->LocalOpen = 0;
    return OK;
  }
}


/*
   Functions, where rwPlx9030lcr() is also used:

   1. GetVersion
   2. EnableInterrupts
   3. Reset
   4. RawIo
   5. CtrDrvrInstall
 */
#endif
