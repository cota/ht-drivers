#include <hptdc.h>

#define HptdcNAME_LENGTH 64

typedef char HptdcName[HptdcNAME_LENGTH];

static HptdcRegisterVlaue hptdcSetup       [HptdcSetupREGISTERS];
static HptdcRegisterVlaue hptdcSetupRbk    [HptdcSetupREGISTERS];
static HptdcName          hptdcSetupNames  [HptdcSetupREGISTERS];
static int hptdc_setup_loaded = 0;

static HptdcRegisterVlaue hptdcControl     [HptdcControlREGISTERS];
static HptdcRegisterVlaue hptdcControlRbk  [HptdcControlREGISTERS];
static HptdcName          hptdcControlNames[HptdcControlREGISTERS];
static int hptdc_control_loaded = 0;

static HptdcRegisterVlaue hptdcStatus[HptdcStatusREGISTERS] =
{  {0,10,0 },
   {0,11,11},
   {0,19,12},
   {0,20,20},
   {0,21,21},
   {0,29,22},
   {0,37,30},
   {0,45,38},
   {0,53,46},
   {0,57,54},
   {0,58,58},
   {0,59,59},
   {0,60,60},
   {0,61,61} };

static HptdcName hptdcStatusNames[HptdcStatusREGISTERS] =
{  "error",
   "have_token",
   "readout_fifo_occupancy",
   "readout_fifo_full",
   "readout_fifo_empty",
   "group3_l1_occupancy",
   "group2_l1_occupancy",
   "group1_l1_occupancy",
   "group0_l1_occupancy",
   "trigger_fifo_occupancy",
   "trigger_fifo_full",
   "trigger_fifo_empty",
   "dll_lock",
   "not_used" };
