/**
 * @file mtt-skel.c
 *
 * @brief MTT skel-based driver
 *
 * @author Copyright (c) 2006-2008 CERN: Julian Lewis <julian.lewis@cern.ch>
 * @author Copyright (c) 2009 CERN; Emilio G. Cota <cota@braap.org>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#include <cdcm/cdcm.h>

#include <mtthard.h>
#include <mttdrvr.h>
#include <mtt_priv.h>

#include <skeldrvr.h>
#include <skeldrvrP.h>
#include <skeluser_ioctl.h>

static char *mtt_ioctl_names[] = {
	[_IOC_NR(SkelUserIoctlFIRST)]	= "MTT First IOCTL",
	[_IOC_NR(MTT_IOCSCABLEID)]	= "Set module cable id",
	[_IOC_NR(MTT_IOCGCABLEID)]	= "Get module cable id",
	[_IOC_NR(MTT_IOCSUTC_SENDING)]	= "Enable/Disable UTC sending",
	[_IOC_NR(MTT_IOCSEXT_1KHZ)]	= "Select external 1kHz clock",
	[_IOC_NR(MTT_IOCSOUT_DELAY)]	= "Set output delay (in 40MHz ticks)",
	[_IOC_NR(MTT_IOCGOUT_DELAY)]	= "Get output delay (in 40MHz ticks)",
	[_IOC_NR(MTT_IOCSOUT_ENABLE)]	= "Enable/Disable output",
	[_IOC_NR(MTT_IOCSSYNC_PERIOD)]	= "Set sync period (in ms)",
	[_IOC_NR(MTT_IOCGSYNC_PERIOD)]	= "Get sync period (in ms)",
	[_IOC_NR(MTT_IOCSUTC)]		= "Set UTC for next PPS tick",
	[_IOC_NR(MTT_IOCGUTC)]		= "Get the current UTC",
	[_IOC_NR(MTT_IOCSSEND_EVENT)]	= "Send an event out now",
	[_IOC_NR(MTT_IOCSTASKS_START)]	= "Start tasks specified by bitmask",
	[_IOC_NR(MTT_IOCSTASKS_STOP)]	= "Stop tasks specificied by bitmask",
	[_IOC_NR(MTT_IOCSTASKS_CONT)]	= "Continue from current tasks PC",
	[_IOC_NR(MTT_IOCGTASKS_CURR)]	= "Get bit mask for all running tasks",
	[_IOC_NR(MTT_IOCSTCB)]		= "Set Task Control Block",
	[_IOC_NR(MTT_IOCGTCB)]		= "Get Task Control Block",
	[_IOC_NR(MTT_IOCSGRVAL)]	= "Set Global Register Value",
	[_IOC_NR(MTT_IOCGGRVAL)]	= "Get Global Register Value",
	[_IOC_NR(MTT_IOCSTRVAL)]	= "Set Task Register Value",
	[_IOC_NR(MTT_IOCGTRVAL)]	= "Get Task Register Value",
	[_IOC_NR(MTT_IOCSPROGRAM)]	= "Set program code",
	[_IOC_NR(MTT_IOCGPROGRAM)]	= "Read program code",
	[_IOC_NR(MTT_IOCGSTATUS)]	= "Get status",
	[_IOC_NR(MTT_IOCRAW_WRITE)]	= "Raw I/O Write",
	[_IOC_NR(MTT_IOCRAW_READ)]	= "Raw I/O Read",
	[_IOC_NR(MTT_IOCGVERSION)]	= "Get version",
	[_IOC_NR(SkelUserIoctlLAST)]	= "MTT Last IOCTL"
};

char *SkelUserGetIoctlName(int cmd)
{
	return mtt_ioctl_names[_IOC_NR(cmd)];
}

static inline void
mtt_writes(SkelDrvrModuleContext *mcon, ssize_t offset, uint16_t value)
{
	struct udata *udata = mcon->UserData;

	cdcm_iowrite16be(value, udata->jtag + offset);
}

static inline unsigned int mtt_reads(SkelDrvrModuleContext *mcon, ssize_t offs)
{
	struct udata *udata = mcon->UserData;

	return cdcm_ioread16be(udata->jtag + offs);
}

static inline void
mtt_writew(SkelDrvrModuleContext *mcon, ssize_t offset, uint32_t value)
{
	struct udata *udata = mcon->UserData;

	cdcm_iowrite32be(value, udata->iomap + offset);
}

static inline unsigned int mtt_readw(SkelDrvrModuleContext *mcon, ssize_t offs)
{
	struct udata *udata = mcon->UserData;

	return cdcm_ioread32be(udata->iomap + offs);
}

static inline void
__mtt_orw(SkelDrvrModuleContext *mcon, ssize_t offset, uint32_t value)
{
	uint32_t regval;

	regval = mtt_readw(mcon, offset);
	cdcm_mb();
	mtt_writew(mcon, offset, regval | value);
}

/*
 * Update (OR) a register atomically by taking the module's iolock
 */
static void mtt_orw(SkelDrvrModuleContext *mcon, ssize_t offset, uint32_t value)
{
	struct udata	*udata = mcon->UserData;
	unsigned long	flags;

	cdcm_spin_lock_irqsave(&udata->iolock, flags);
	__mtt_orw(mcon, offset, value);
	cdcm_spin_unlock_irqrestore(&udata->iolock, flags);
}

static inline struct udata *get_udata_ccon(SkelDrvrClientContext *ccon)
{
	SkelDrvrModuleContext *mcon = get_mcon(ccon->ModuleNumber);

	return mcon->UserData;
}

static MttDrvrMap *get_iomap_ccon(SkelDrvrClientContext *ccon)
{
	SkelDrvrModuleContext	*mcon = get_mcon(ccon->ModuleNumber);
	struct udata		*udata = mcon->UserData;

	return udata->iomap;
}

static void mtt_iowritew_r(void *to, void *from, ssize_t size)
{
	int count = size / sizeof(uint32_t);

	if (count <= 0) {
		return;
	} else {
		uint32_t *io	= (uint32_t *)to;
		uint32_t *mem	= (uint32_t *)from;

		while (count-- != 0)
			cdcm_iowrite32be(*mem++, io++);
	}
}

static void mtt_ioreadw_r(void *to, void *from, ssize_t size)
{
	int count = size / sizeof(uint32_t);

	if (count <= 0) {
		return;
	} else {
		uint32_t *io	= (uint32_t *)from;
		uint32_t *mem	= (uint32_t *)to;

		while (count-- != 0)
			*mem++ = cdcm_ioread32be(io++);
	}
}

static uint32_t mtt_get_status(SkelDrvrModuleContext *mcon)
{
	struct udata	*udata = mcon->UserData;
	uint32_t	ret;

	ret = mtt_readw(mcon, MTT_STATUS);

	if (udata->FlashOpen)
		ret |= MttDrvrStatusFLASH_OPEN;
	/* yes, this is racy with reco/noreco & friends, but who cares.. */
	if (udata->BusError) {
		ret |= MttDrvrStatusBUS_FAULT;
		udata->BusError = 0;
	}

	return ret;
}

static int mtt_ping(SkelDrvrModuleContext *mcon)
{
	struct udata	*udata = mcon->UserData;
	int		ret = OK;

	/* read the version and check for any bus errors */
	if (!recoset()) {
		mtt_readw(mcon, MTT_VHDL);
		cdcm_mb();
	} else {
		report_module(mcon, SkelDrvrDebugFlagASSERTION,	"Bus Error");
		udata->BusError = 1;
		pseterr(ENXIO);
		ret = SYSERR;
	}
	noreco();
	return ret;
}

/* function to be called with the module's iolock held */
static void __mtt_cmd(SkelDrvrModuleContext *mcon, uint32_t cmd, uint32_t val)
{
	mtt_writew(mcon, MTT_COMMAND_VAL, val);
	cdcm_wmb();
	mtt_writew(mcon, MTT_COMMAND, cmd);
	cdcm_wmb();
}

/* Ensure the command is executed atomically by acquiring the module's iolock */
static inline void
mtt_cmd(SkelDrvrModuleContext *mcon, uint32_t cmd, uint32_t val)
{
	struct udata	*udata = mcon->UserData;
	unsigned long	flags;

	cdcm_spin_lock_irqsave(&udata->iolock, flags);
	__mtt_cmd(mcon, cmd, val);
	cdcm_spin_unlock_irqrestore(&udata->iolock, flags);
}

/* Note the specific MTT version can also be used */
SkelUserReturn SkelUserGetModuleVersion(SkelDrvrModuleContext *mcon, char *mver)
{
	if (mtt_ping(mcon))
		return SkelUserReturnFAILED;
	ksprintf(mver, "%d", mtt_readw(mcon, MTT_VHDL));
	return SkelUserReturnOK;
}

/*
 * Do not use the skel function for retrieving the status; use the specific one.
 */
SkelUserReturn
SkelUserGetHardwareStatus(SkelDrvrModuleContext *mcon, uint32_t *hsts)
{
	*hsts = mcon->StandardStatus;
	return SkelUserReturnNOT_IMPLEMENTED;
}

static void mtt_gs_utc(SkelDrvrModuleContext *mcon, MttDrvrTime *utc, int set)
{
	struct udata	*udata = mcon->UserData;
	unsigned long	flags;

	cdcm_spin_lock_irqsave(&udata->iolock, flags);
	if (set) {
		__mtt_cmd(mcon, MttDrvrCommandSET_UTC, utc->Second);
	} else {
		/* reading the ms register causes the seconds to be latched */
		utc->MilliSecond = mtt_readw(mcon, MTT_MSEC);
		cdcm_wmb();
		utc->Second = mtt_readw(mcon, MTT_SEC);
	}
	cdcm_spin_unlock_irqrestore(&udata->iolock, flags);
}

static void
mtt_gs_utc_skel(SkelDrvrModuleContext *mcon, SkelDrvrTime *sktime, int set)
{
	MttDrvrTime utc;

	if (set) {
		utc.Second	= sktime->Second;
		utc.MilliSecond	= sktime->NanoSecond / 1000;
		mtt_gs_utc(mcon, &utc, 1);
	} else {
		mtt_gs_utc(mcon, &utc, 0);
		sktime->Second		= utc.Second;
		sktime->NanoSecond	= utc.MilliSecond * 1000;
	}
}

SkelUserReturn SkelUserGetUtc(SkelDrvrModuleContext *mcon, SkelDrvrTime *sktime)
{
	mtt_gs_utc_skel(mcon, sktime, 0);
	return SkelUserReturnOK;
}

static inline void __mtt_quiesce(SkelDrvrModuleContext *mcon)
{
	/* disable all interrupts */
	mtt_writew(mcon, MTT_INTEN, 0);

	/* clear any previous interrupt (Clear-on-read) */
	mtt_readw(mcon, MTT_INTSR);
}

static inline uint32_t mtt_shift_intr_level(uint32_t level)
{
	return level == 3 ? MttDrvrIrqLevel_3 : MttDrvrIrqLevel_2;
}

static void __mtt_reset(SkelDrvrModuleContext *mcon)
{
	InsLibIntrDesc *isr = mcon->Modld->Isr;

	/* quiesce the module */
	__mtt_quiesce(mcon);

	/* reset the module */
	__mtt_cmd(mcon, MttDrvrCommandRESET, 1);
	usec_sleep(5);

	/*
	 * re-configure the interrupt level
	 * Note that the vector and level share the register
	 */
	if (isr) {
		mtt_writew(mcon, MTT_INTR, isr->Vector |
			mtt_shift_intr_level(isr->Level));
	}
}

static void udata_set_defaults(SkelDrvrModuleContext *mcon)
{
	struct udata *udata = mcon->UserData;

	/* clear the user data struct */
	udata->FlashOpen = 0;
	udata->UtcSending = 0;
	udata->BusError = 0;

	/* make sure the hardware matches the information in udata */
	if (udata->EnabledOutput)
		mtt_cmd(mcon, MttDrvrCommandENABLE, 1);
	if (udata->External1KHZ)
		mtt_cmd(mcon, MttDrvrCommandEXTERNAL_1KHZ, 1);
	if (udata->SyncPeriod)
		mtt_cmd(mcon, MttDrvrCommandSET_SYNC_PERIOD, udata->SyncPeriod);
	if (udata->OutputDelay)
		mtt_cmd(mcon, MttDrvrCommandSET_OUT_DELAY, udata->OutputDelay);
}

SkelUserReturn SkelUserHardwareReset(SkelDrvrModuleContext *mcon)
{
	struct udata	*udata = mcon->UserData;
	unsigned long	flags;

	cdcm_spin_lock_irqsave(&udata->iolock, flags);
	__mtt_reset(mcon);
	cdcm_spin_unlock_irqrestore(&udata->iolock, flags);

	udata_set_defaults(mcon);
	return SkelUserReturnOK;
}

SkelUserReturn SkelUserGetInterruptSource(SkelDrvrModuleContext *mcon,
					  SkelDrvrTime *sktime, uint32_t *src)
{
	*src = mtt_readw(mcon, MTT_INTSR);
	cdcm_wmb();
	mtt_gs_utc_skel(mcon, sktime, 0);

	return SkelUserReturnOK;
}

/*
 * Note: this function may be called in atomic context
 */
SkelUserReturn
SkelUserEnableInterrupts(SkelDrvrModuleContext *mcon, uint32_t mask)
{
	mtt_orw(mcon, MTT_INTEN, mask & MttDrvrIntMASK);
	return SkelUserReturnOK;
}

SkelUserReturn
SkelUserHardwareEnable(SkelDrvrModuleContext *mcon, uint32_t flag)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

SkelUserReturn SkelUserJtagReadByte(SkelDrvrModuleContext *mcon, uint32_t *val)
{
	*val = mtt_reads(mcon, 0);
	return SkelUserReturnOK;
}

SkelUserReturn SkelUserJtagWriteByte(SkelDrvrModuleContext *mcon, uint32_t val)
{
	mtt_writes(mcon, 0, val);
	return SkelUserReturnOK;
}

SkelUserReturn SkelUserModuleInit(SkelDrvrModuleContext *mcon)
{
	struct udata *udata;

	udata = (void *)sysbrk(sizeof(struct udata));
	if (udata == NULL) {
		report_module(mcon, SkelDrvrDebugFlagASSERTION,
			"Not enough memory for the module's data");
		pseterr(ENOMEM);
		return SkelUserReturnFAILED;
	}

	bzero((void *)udata, sizeof(struct udata));
	mcon->UserData = udata;

	/* initialise the iomap address */
	udata->iomap = get_vmemap_addr(mcon, 0x39, 32);

	/* initialise the jtag address */
	udata->jtag = get_vmemap_addr(mcon, 0x29, 16);

	/* initialise locking fields */
	cdcm_spin_lock_init(&udata->iolock);

	/* module software reset */
	SkelUserHardwareReset(mcon);

	return SkelUserReturnOK;
}

void SkelUserModuleRelease(SkelDrvrModuleContext *mcon)
{
	/* quiesce the module before it gets uninstalled */
	__mtt_quiesce(mcon);

	/* free userdata */
	sysfree(mcon->UserData, sizeof(struct udata));
}

SkelUserReturn SkelUserClientInit(SkelDrvrClientContext *ccon)
{
	return SkelUserReturnNOT_IMPLEMENTED;
}

void SkelUserClientRelease(SkelDrvrClientContext *ccon)
{
}

static SkelUserReturn
mtt_gs_cableid_ioctl(SkelDrvrClientContext *ccon, void *arg, int set)
{
	struct udata	*udata = get_udata_ccon(ccon);
	uint32_t	*valp = arg;

	if (set)
		udata->CableId = *valp;
	else
		*valp = udata->CableId;

	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_utc_sending_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
	struct udata	*udata = get_udata_ccon(ccon);
	uint32_t	*valp = arg;

	udata->UtcSending = !!*valp;
	mtt_cmd(get_mcon(ccon->ModuleNumber), MttDrvrCommandUTC_SENDING_ON,
		udata->UtcSending);
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_ext_1khz_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
	struct udata	*udata = get_udata_ccon(ccon);
	uint32_t	*valp = arg;

	udata->External1KHZ = !!*valp;
	mtt_cmd(get_mcon(ccon->ModuleNumber), MttDrvrCommandEXTERNAL_1KHZ,
		udata->External1KHZ);
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_gs_out_delay_ioctl(SkelDrvrClientContext *ccon, void *arg, int set)
{
	struct udata	*udata = get_udata_ccon(ccon);
	uint32_t	*valp = arg;

	if (set) {
		udata->OutputDelay = *valp;
		mtt_cmd(get_mcon(ccon->ModuleNumber),
			MttDrvrCommandSET_OUT_DELAY, *valp);
	} else {
		*valp = udata->OutputDelay;
	}
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_out_enable_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
	struct udata	*udata = get_udata_ccon(ccon);
	uint32_t	*valp = arg;

	udata->EnabledOutput = !!*valp;
	mtt_cmd(get_mcon(ccon->ModuleNumber), MttDrvrCommandENABLE,
		udata->EnabledOutput);
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_gs_sync_period_ioctl(SkelDrvrClientContext *ccon, void *arg, int set)
{
	struct udata	*udata = get_udata_ccon(ccon);
	uint32_t	*valp = arg;

	if (set) {
		udata->SyncPeriod = *valp;
		mtt_cmd(get_mcon(ccon->ModuleNumber),
			MttDrvrCommandSET_SYNC_PERIOD, *valp);
	} else {
		*valp = udata->SyncPeriod;
	}
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_gs_utc_ioctl(SkelDrvrClientContext *ccon, void *arg, int set)
{
	MttDrvrTime	*utc = arg;

	mtt_gs_utc(get_mcon(ccon->ModuleNumber), utc, set);
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_send_event_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
	uint32_t *valp = arg;

	mtt_cmd(get_mcon(ccon->ModuleNumber), MttDrvrCommandSEND_EVENT, *valp);
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_tasks_start_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
	SkelDrvrModuleContext	*mcon = get_mcon(ccon->ModuleNumber);
	struct udata		*udata = mcon->UserData;
	uint32_t		*valp = arg;
	uint32_t		tmask, mask;
	int			i;

	mask = *valp & MttDrvrTASK_MASK;
	if (!mask) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	for (i = 0; i < MttDrvrTASKS; i++) {
		tmask = 1 << i;
		if (!(*valp & tmask)) {
			continue;
		} else {
			MttDrvrMap		*map = get_iomap_ccon(ccon);
			MttDrvrTaskBlock	*task = &map->Tasks[i];

			mtt_writew(mcon, MTT_TASKS_STOP, tmask);
			cdcm_iowrite32be(udata->Tasks[i].PcStart, &task->Pc);
			mtt_writew(mcon, MTT_TASKS_START, tmask);
		}
	}
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_gs_tasks_ioctl(SkelDrvrClientContext *ccon, void *arg, int reg, int set)
{
	SkelDrvrModuleContext	*mcon = get_mcon(ccon->ModuleNumber);
	uint32_t		*valp = arg;
	uint32_t		mask;

	if (set) {
		mask = *valp & MttDrvrTASK_MASK;
		if (!mask) {
			pseterr(EINVAL);
			return SkelUserReturnFAILED;
		}
		mtt_writew(mcon, reg, mask);
	} else {
		*valp = mtt_readw(mcon, reg) & MttDrvrTASK_MASK;
	}
	return SkelUserReturnOK;
}

static void
mtt_set_tcb(struct udata *udata, MttDrvrTaskBuf *taskbuf, uint32_t action)
{
	MttDrvrMap	*map = udata->iomap;
	int		i = taskbuf->Task - 1;
	struct mtt_task	*localtask = &udata->Tasks[i];

	switch (action) {
	case MttDrvrTBFLoadAddress:
		localtask->LoadAddress = taskbuf->LoadAddress;
		break;
	case MttDrvrTBFInstructionCount:
		localtask->InstructionCount = taskbuf->InstructionCount;
		break;
	case MttDrvrTBFPcStart:
		localtask->PcStart = taskbuf->PcStart;
		break;
	case MttDrvrTBFPcOffset:
		localtask->PcOffset = taskbuf->ControlBlock.PcOffset;
		cdcm_iowrite32be(taskbuf->ControlBlock.PcOffset,
				&map->Tasks[i].PcOffset);
		break;
	case MttDrvrTBFPc:
		localtask->Pc = taskbuf->ControlBlock.Pc;
		cdcm_iowrite32be(taskbuf->ControlBlock.Pc, &map->Tasks[i].Pc);
		break;
	case MttDrvrTBFName:
		strncpy(localtask->Name, taskbuf->Name, MttDrvrNameSIZE);
	default:
		break;
	}

}

static void mtt_get_tcb(struct udata *udata, MttDrvrTaskBuf *taskbuf)
{
	MttDrvrMap	*map = udata->iomap;
	int		i = taskbuf->Task - 1;
	struct mtt_task	*localtask = &udata->Tasks[i];

	taskbuf->LoadAddress		= localtask->LoadAddress;
	taskbuf->InstructionCount	= localtask->InstructionCount;
	taskbuf->PcStart		= localtask->PcStart;
	taskbuf->Fields			= MttDrvrTBFAll;
	strncpy(taskbuf->Name, localtask->Name, MttDrvrNameSIZE);
	mtt_ioreadw_r(&taskbuf->ControlBlock, &map->Tasks[i],
		sizeof(MttDrvrTaskBlock));
}

static SkelUserReturn
mtt_gs_tcb_ioctl(SkelDrvrClientContext *ccon, void *arg, int set)
{
	struct udata	*udata = get_udata_ccon(ccon);
	MttDrvrTaskBuf	*taskbuf = arg;
	uint32_t	mask;
	int		i;

	/* sanity check */
	if (!WITHIN_RANGE(1, taskbuf->Task, MttDrvrTASKS)) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	if (set) {
		for (i = 0; i < MttDrvrTBFbits; i++) {
			mask = 1 << i;
			if (!(mask & taskbuf->Fields))
				continue;
			mtt_set_tcb(udata, taskbuf, mask);
		}
	} else {
		mtt_get_tcb(udata, taskbuf);
	}

	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_gs_grval_ioctl(SkelDrvrClientContext *ccon, void *arg, int set)
{
	struct udata		*udata = get_udata_ccon(ccon);
	MttDrvrMap		*map = udata->iomap;
	MttDrvrGlobalRegBuf	*regbuf = arg;
	int			regnum = regbuf->RegNum;

	if (regnum >= MttDrvrGRAM_SIZE) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	if (set)
		cdcm_iowrite32be(regbuf->RegVal, &map->GlobalMem[regnum]);
	else
		regbuf->RegVal = cdcm_ioread32be(&map->GlobalMem[regnum]);
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_gs_trval_ioctl(SkelDrvrClientContext *ccon, void *arg, int set)
{
	struct udata		*udata = get_udata_ccon(ccon);
	MttDrvrMap		*map = udata->iomap;
	MttDrvrTaskRegBuf	*trbuf = arg;
	int			tasknum = trbuf->Task;
	unsigned long		*mttmem = map->LocalMem[tasknum - 1];
	int			i;

	/* sanity check */
	if (!WITHIN_RANGE(1, tasknum, MttDrvrTASKS)) {
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	if (set) {
		int regmask;

		for (i = 0; i < MttDrvrLRAM_SIZE; i++) {
			regmask = 1 << i;
			if (!(regmask & trbuf->RegMask))
				continue;
			cdcm_iowrite32be(trbuf->RegVals[i], &mttmem[i]);
			return OK;
		}
		pseterr(EINVAL);
		return SkelUserReturnFAILED;
	}

	for (i = 0; i < MttDrvrLRAM_SIZE; i++)
		trbuf->RegVals[i] = cdcm_ioread32be(&mttmem[i]);
	trbuf->RegMask = 0xffffffff;
	return OK;
}

static SkelUserReturn
__mtt_io(SkelDrvrModuleContext *mcon, void *u_addr, void *ioaddr, ssize_t size,
	 int write)
{
	void *bounce;

	bounce = (void *)sysbrk(size);
	if (bounce == NULL) {
		report_module(mcon, SkelDrvrDebugFlagASSERTION,
			"%s: -ENOMEM", __FUNCTION__);
		pseterr(ENOMEM);
		goto out_err;
	}

	if (write) {
		if (cdcm_copy_from_user(bounce, u_addr, size)) {
			pseterr(EFAULT);
			goto out_err;
		}
		mtt_iowritew_r(ioaddr, bounce, size);
	} else {
		mtt_ioreadw_r(bounce, ioaddr, size);
		if (cdcm_copy_to_user(u_addr, bounce, size)) {
			pseterr(EFAULT);
			goto out_err;
		}
	}
	sysfree(bounce, size);
	return SkelUserReturnOK;
out_err:
	if (bounce)
		sysfree(bounce, size);
	return SkelUserReturnFAILED;
}

static SkelUserReturn
mtt_gs_program_ioctl(SkelDrvrClientContext *ccon, void *arg, int set)
{
	struct udata		*udata = get_udata_ccon(ccon);
	MttDrvrMap		*map = udata->iomap;
	MttDrvrProgramBuf	*prog = arg;
	MttDrvrInstruction	*ioaddr = &map->ProgramMem[prog->LoadAddress];
	ssize_t	size = prog->InstructionCount * sizeof(MttDrvrInstruction);

	return __mtt_io(get_mcon(ccon->ModuleNumber), prog->Program,
			ioaddr, size, set);
}

static SkelUserReturn mtt_status_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
	SkelDrvrModuleContext	*mcon = get_mcon(ccon->ModuleNumber);
	uint32_t		*valp = arg;

	if (mtt_ping(mcon))
		return SkelUserReturnFAILED;
	*valp = mtt_get_status(mcon);
	return SkelUserReturnOK;
}

static SkelUserReturn
mtt_rawio_ioctl(SkelDrvrClientContext *ccon, void *arg, int write)
{
	MttDrvrRawIoBlock	*io = arg;
	struct udata		*udata = get_udata_ccon(ccon);
	uint32_t		*iomap = udata->iomap;

	return __mtt_io(get_mcon(ccon->ModuleNumber), io->UserArray,
			iomap + io->Offset, io->Size * sizeof(uint32_t), write);
}

static SkelUserReturn mtt_version_ioctl(SkelDrvrClientContext *ccon, void *arg)
{
	SkelDrvrModuleContext	*mcon = get_mcon(ccon->ModuleNumber);
	MttDrvrVersion		*version = arg;

	if (mtt_ping(mcon))
		return SkelUserReturnFAILED;
	version->VhdlVersion = mtt_readw(mcon, MTT_VHDL);
	version->DrvrVersion = COMPILE_TIME;
	return SkelUserReturnOK;
}

SkelUserReturn
SkelUserIoctls(SkelDrvrClientContext *ccon, SkelDrvrModuleContext *mcon,
	       int cm, char *u_arg)
{
	void *arg = u_arg;

	switch (cm) {
	case MTT_IOCSCABLEID:
		return mtt_gs_cableid_ioctl(ccon, arg, 1);
	case MTT_IOCGCABLEID:
		return mtt_gs_cableid_ioctl(ccon, arg, 0);
	case MTT_IOCSUTC_SENDING:
		return mtt_utc_sending_ioctl(ccon, arg);
	case MTT_IOCSEXT_1KHZ:
		return mtt_ext_1khz_ioctl(ccon, arg);
	case MTT_IOCSOUT_DELAY:
		return mtt_gs_out_delay_ioctl(ccon, arg, 1);
	case MTT_IOCGOUT_DELAY:
		return mtt_gs_out_delay_ioctl(ccon, arg, 0);
	case MTT_IOCSOUT_ENABLE:
		return mtt_out_enable_ioctl(ccon, arg);
	case MTT_IOCSSYNC_PERIOD:
		return mtt_gs_sync_period_ioctl(ccon, arg, 1);
	case MTT_IOCGSYNC_PERIOD:
		return mtt_gs_sync_period_ioctl(ccon, arg, 0);
	case MTT_IOCSUTC:
		return mtt_gs_utc_ioctl(ccon, arg, 1);
	case MTT_IOCGUTC:
		return mtt_gs_utc_ioctl(ccon, arg, 0);
	case MTT_IOCSSEND_EVENT:
		return mtt_send_event_ioctl(ccon, arg);
	case MTT_IOCSTASKS_START:
		return mtt_tasks_start_ioctl(ccon, arg);
	case MTT_IOCSTASKS_STOP:
		return mtt_gs_tasks_ioctl(ccon, arg, MTT_TASKS_STOP, 1);
	case MTT_IOCSTASKS_CONT:
		return mtt_gs_tasks_ioctl(ccon, arg, MTT_TASKS_START, 1);
	case MTT_IOCGTASKS_CURR:
		return mtt_gs_tasks_ioctl(ccon, arg, MTT_TASKS_CURR, 0);
	case MTT_IOCSTCB:
		return mtt_gs_tcb_ioctl(ccon, arg, 1);
	case MTT_IOCGTCB:
		return mtt_gs_tcb_ioctl(ccon, arg, 0);
	case MTT_IOCSGRVAL:
		return mtt_gs_grval_ioctl(ccon, arg, 1);
	case MTT_IOCGGRVAL:
		return mtt_gs_grval_ioctl(ccon, arg, 0);
	case MTT_IOCSTRVAL:
		return mtt_gs_trval_ioctl(ccon, arg, 1);
	case MTT_IOCGTRVAL:
		return mtt_gs_trval_ioctl(ccon, arg, 0);
	case MTT_IOCSPROGRAM:
		return mtt_gs_program_ioctl(ccon, arg, 1);
	case MTT_IOCGPROGRAM:
		return mtt_gs_program_ioctl(ccon, arg, 0);
	case MTT_IOCGSTATUS:
		return mtt_status_ioctl(ccon, arg);
	case MTT_IOCRAW_WRITE:
		return mtt_rawio_ioctl(ccon, arg, 1);
	case MTT_IOCRAW_READ:
		return mtt_rawio_ioctl(ccon, arg, 0);
	case MTT_IOCGVERSION:
		return mtt_version_ioctl(ccon, arg);
	default:
		break;
	}

	return SkelUserReturnNOT_IMPLEMENTED;
}

/* no special configuration needed */
struct skel_conf SkelConf;
