/* ================================================================================= */
/* Collect soruces to make a single module.                                          */
/* This frig is needed because of the brain dead way Kernel 2.6 Version magic works. */
/* Julian 29th April 2005                                                            */
/* ================================================================================= */

#include <linux/module.h>   /* Can only be included once */
#include <linux/version.h>

#include <ModuleLynxOs.c>
#include <EmulateLynxOs.c>
#include <cdcmSem.c>
#include <DrvrSpec.c>

#include <ctrdrvr.c>

/* ===================================================== */
