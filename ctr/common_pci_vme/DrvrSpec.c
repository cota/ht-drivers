/* ===================================================== */
/* Driver specific routines and data areas.              */
/* Julian Lewis May 2004                                 */
/* ===================================================== */

#include <linux/errno.h>
#include <EmulateLynxOs.h>
#include <DrvrSpec.h>

/* ===================================================== */

static CtrDrvrInfoTable info_table;
void *LynxOsInfoTable = (void *) &info_table;

/* ===================================================== */
/* Initialize info table, returns 0 if ok                */
/* else return a Linux 2.4 Error code                    */
/* To be filled with driver specific initialization      */
/* ===================================================== */

int LynxOsInitInfoTable(int recover) {
   info_table.RecoverMode = recover;
   return 0;
}

/* ===================================================== */
/* Returns the size and direction of Ioctl IO transfers. */
/* Fill in an entry for each driver Ioctl                */
/* ===================================================== */

#define READ_FLAG 0x01 /* As per LynxOsMemoryREAD_FLAG   */
#define WRIT_FLAG 0x03 /* defined in EmulateLynxOs.h     */

int LynxOsMemoryGetSize(int cmd, int *dirp) {
int cnt, dir;

   switch ((CtrDrvrControlFunction) cmd) {

   case CtrDrvrSET_SW_DEBUG:           /* Set driver debug mode */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_SW_DEBUG:
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrGET_VERSION:            /* Get version date */
      cnt = sizeof(CtrDrvrVersion);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrSET_TIMEOUT:            /* Set the read timeout value */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_TIMEOUT:            /* Get the read timeout value */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrSET_QUEUE_FLAG:         /* Set queuing capabiulities on off */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_QUEUE_FLAG:         /* 1=Q_off 0=Q_on */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrGET_QUEUE_SIZE:         /* Number of events on queue */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrGET_QUEUE_OVERFLOW:     /* Number of missed events */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrGET_MODULE_DESCRIPTOR:  /* Get the current Module descriptor */
      cnt = sizeof(CtrDrvrModuleAddress);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrSET_MODULE:             /* Select the module to work with */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_MODULE:             /* Which module am I working with */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrGET_MODULE_COUNT:       /* The number of installed modules */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrRESET:                  /* Reset the module, re-establish connections */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrENABLE:                 /* Enable CTR module event reception */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_STATUS:             /* Read module status */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrGET_INPUT_DELAY:        /* Get input delay in 25ns ticks */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrSET_INPUT_DELAY:        /* Set input delay in 25ns ticks */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_CLIENT_LIST:        /* Get the list of driver clients */
      cnt = sizeof(CtrDrvrClientList);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrCONNECT:                /* Connect to an object interrupt */
      cnt = sizeof(CtrDrvrConnection);
      dir = READ_FLAG;
      break;

   case CtrDrvrDISCONNECT:             /* Disconnect from an object interrupt */
      cnt = sizeof(CtrDrvrConnection);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_CLIENT_CONNECTIONS: /* Get the list of a client connections on module */
      cnt = sizeof(CtrDrvrClientConnections);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrSET_UTC:                /* Set Universal Coordinated Time for next PPS tick */
      cnt = sizeof(CtrDrvrCTime);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_UTC:                /* Latch and read the current UTC time */
      cnt = sizeof(CtrDrvrCTime);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrGET_CABLE_ID:           /* Cables telegram ID */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrGET_ACTION:             /* Low level direct access to CTR RAM tables */
      cnt = sizeof(CtrDrvrAction);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrSET_ACTION:             /* Set may not modify the bus interrupt settings */
      cnt = sizeof(CtrDrvrAction);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrCREATE_CTIM_OBJECT:     /* Create a new CTIM timing object */
      cnt = sizeof(CtrDrvrCtimBinding);
      dir = READ_FLAG;
      break;

   case CtrDrvrDESTROY_CTIM_OBJECT:    /* Destroy a CTIM timing object */
      cnt = sizeof(CtrDrvrCtimBinding);
      dir = READ_FLAG;
      break;

   case CtrDrvrLIST_CTIM_OBJECTS:      /* Returns a list of created CTIM objects */
      cnt = sizeof(CtrDrvrCtimObjects);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrCHANGE_CTIM_FRAME:      /* Change the frame of an existing CTIM object */
      cnt = sizeof(CtrDrvrCtimBinding);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrCREATE_PTIM_OBJECT:     /* Create a new PTIM timing object */
      cnt = sizeof(CtrDrvrPtimBinding);
      dir = READ_FLAG;
      break;

   case CtrDrvrDESTROY_PTIM_OBJECT:    /* Destroy a PTIM timing object */
      cnt = sizeof(CtrDrvrPtimBinding);
      dir = READ_FLAG;
      break;

   case CtrDrvrLIST_PTIM_OBJECTS:      /* List PTIM timing objects */
      cnt = sizeof(CtrDrvrPtimObjects);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_PTIM_BINDING:       /* Search for a PTIM object binding */
      cnt = sizeof(CtrDrvrPtimBinding);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_OUT_MASK:           /* Counter output routing mask */
      cnt = sizeof(CtrDrvrCounterMaskBuf);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrSET_OUT_MASK:           /* Counter output routing mask */
      cnt = sizeof(CtrDrvrCounterMaskBuf);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_COUNTER_HISTORY:    /* One deep history of counter */
     cnt = sizeof(CtrDrvrCounterHistoryBuf);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_REMOTE:             /* Counter Remote/Local status */
      cnt = sizeof(CtrdrvrRemoteCommandBuf);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrSET_REMOTE:             /* Counter Remote/Local status */
      cnt = sizeof(CtrdrvrRemoteCommandBuf);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrREMOTE:                 /* Remote control counter */
      cnt = sizeof(CtrdrvrRemoteCommandBuf);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_CONFIG:             /* Get a counter configuration */
      cnt = sizeof(CtrDrvrCounterConfigurationBuf);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrSET_CONFIG:             /* Set a counter configuration */
      cnt = sizeof(CtrDrvrCounterConfigurationBuf);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_PLL:                /* Get phase locked loop parameters */
      cnt = sizeof(CtrDrvrPll);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrSET_PLL:                /* Set phase locked loop parameters */
      cnt = sizeof(CtrDrvrPll);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrSET_PLL_ASYNC_PERIOD:   /* Set PLL asynchronous period */
      cnt = sizeof(CtrDrvrPllAsyncPeriodNs);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_PLL_ASYNC_PERIOD:   /* Get PLL asynchronous period */
      cnt = sizeof(CtrDrvrPllAsyncPeriodNs);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrREAD_TELEGRAM:          /* Read telegrams from CTR */
      cnt = sizeof(CtrDrvrTgmBuf);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrREAD_EVENT_HISTORY:     /* Read incomming event history */
      cnt = sizeof(CtrDrvrEventHistory);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   /* ============================================================ */
   /* Hardware specialists IOCTL Commands to maintain and diagnose */
   /* the chips on the CTR board. Not for normal timing users.     */

   case CtrDrvrJTAG_OPEN:              /* Open JTAG interface to the Xilinx FPGA */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrJTAG_READ_BYTE:         /* Read back uploaded VHDL bit stream byte */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrJTAG_WRITE_BYTE:        /* Write compiled VHDL bit stream byte */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrJTAG_CLOSE:             /* Close JTAG interface */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrHPTDC_OPEN:             /* Open HPTDC JTAG interface */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrHPTDC_IO:               /* Perform HPTDC IO operation */
      cnt = sizeof(CtrDrvrHptdcIoBuf);
      dir = READ_FLAG | WRIT_FLAG;
      break;

   case CtrDrvrHPTDC_CLOSE:            /* Close HPTDC JTAG interface */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrRAW_READ:
      cnt = sizeof(CtrDrvrRawIoBlock);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrRAW_WRITE:
      cnt = sizeof(CtrDrvrRawIoBlock);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_RECEPTION_ERRORS:
      cnt = sizeof(CtrDrvrReceptionErrors);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_IO_STATUS:
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrGET_IDENTITY:
      cnt = sizeof(CtrDrvrBoardId);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrSET_DEBUG_HISTORY:
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrSET_BRUTAL_PLL:
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_MODULE_STATS:
      cnt = sizeof(CtrDrvrModuleStats);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrSET_CABLE_ID:
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrIOCTL_64:
   case CtrDrvrIOCTL_65:
   case CtrDrvrIOCTL_66:
   case CtrDrvrIOCTL_67:
   case CtrDrvrIOCTL_68:
   case CtrDrvrIOCTL_69:
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

#ifdef CTR_VME
   case CtrDrvrGET_OUTPUT_BYTE:        /* VME P2 output byte number */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrSET_OUTPUT_BYTE:        /* VME P2 output byte number */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;
#endif

#ifdef CTR_PCI
   case CtrDrvrSET_MODULE_BY_SLOT:     /* Select the module to work with by slot ID */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrGET_MODULE_SLOT:        /* Get the slot ID of the selected module */
      cnt = sizeof(unsigned long);
      dir = WRIT_FLAG;
      break;

   case CtrDrvrREMAP:                  /* Remap BAR2 after a config change */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvr93LC56B_EEPROM_OPEN:    /* Open the PLX9030 configuration EEPROM 93LC56B for write */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvr93LC56B_EEPROM_READ:    /* Read from the EEPROM 93LC56B the PLX9030 configuration */
      cnt = sizeof(CtrDrvrRawIoBlock);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvr93LC56B_EEPROM_WRITE:   /* Write to the EEPROM 93LC56B a new PLX9030 configuration */
      cnt = sizeof(CtrDrvrRawIoBlock);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvr93LC56B_EEPROM_ERASE:   /* Erase the EEPROM 93LC56B, deletes PLX9030 configuration */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvr93LC56B_EEPROM_CLOSE:   /* Close EEPROM 93LC56B and load new PLX9030 configuration */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrPLX9030_RECONFIGURE:    /* Load EEPROM configuration into the PLX9030 */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrPLX9030_CONFIG_OPEN:    /* Open the PLX9030 configuration */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrPLX9030_CONFIG_READ:    /* Read the PLX9030 configuration registers */
      cnt = sizeof(CtrDrvrRawIoBlock);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrPLX9030_CONFIG_WRITE:   /* Write to PLX9030 configuration registers (Experts only) */
      cnt = sizeof(CtrDrvrRawIoBlock);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrPLX9030_CONFIG_CLOSE:   /* Close the PLX9030 configuration */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrPLX9030_LOCAL_OPEN:     /* Open the PLX9030 local configuration */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

   case CtrDrvrPLX9030_LOCAL_READ:     /* Read the PLX9030 local configuration registers */
      cnt = sizeof(CtrDrvrRawIoBlock);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrPLX9030_LOCAL_WRITE:    /* Write the PLX9030 local configuration registers (Experts only) */
      cnt = sizeof(CtrDrvrRawIoBlock);
      dir = WRIT_FLAG | READ_FLAG;
      break;

   case CtrDrvrPLX9030_LOCAL_CLOSE:    /* Close the PLX9030 local configuration */
      cnt = sizeof(unsigned long);
      dir = READ_FLAG;
      break;

#endif

   default:
      cnt = -EFAULT;
      dir = 0;
   }
   *dirp = dir;
   return cnt;
}
