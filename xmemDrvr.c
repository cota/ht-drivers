/**
 * @file xmemDrvr.c
 *
 * @brief Device driver for the PCI (PMC/PCI) VMIC Reflective memory
 *
 * @author Julian Lewis AB/CO/HT CERN
 *
 * @date Created on 02/09/2004
 *
 * \mainpage Xmem
 *
 * \section warning_sec Warning
 *
 * NOTE. The driver assumes that the size of a long
 * in both user and kernel space is 4 bytes; this is only true for 32-bit
 * machines. Therefore \b 64-bit \b platforms \b are \b NOT \b supported.
 * This was a design mistake from the early beginning and has not been fixed
 * yet because we have other priorities.
 *
 * \section install_sec Driver Description
 *
 * The driver can handle multiple VMIC modules.
 * This is useful for making "bridges" between distributed memory networks.
 * Bridged networks are connected together by the PCI DMA engines
 * on the Plx9656 chips.
 *
 * The driver can handle multiple clients on one platform accessing
 * the reflective memory.
 * Each Client has a unique client context where current working
 * settings are kept.
 *
 * Support functions needed to implement a distributed shared memory
 * have been provided.
 * See my paper "Distributed Memory in a Heterogeneous Network" ICALEPCS
 * http://www-bd.fnal.gov/icalepcs/abstracts/PDF/fpo30.pdf
 *
 * The driver partitions the memory in the VMIC SDRAM into
 * areas called segments.
 * The segment table is initialized from file at run time.
 * Each segment has an Id 1..32, encoded as a single bit in a 32-Bit long.
 * Each segment has a size and is located in SDRAM at a fixed location.
 * The segment descriptor contains a 32-Bit field which indicates
 * which of the first 32 Nodes are allowed write access.
 *
 * Node ID ZERO is illegal and must never be used, the first valid Node ID is 1.
 * The first 1..32 VMIC node Ids are encoded in the same way as a single
 * bit in a 32-bit long.
 * Nodes with higher than 32 Id numbers cannot use the segment interface,
 * they use direct access.
 * Read/Write segment access is via the READ/WRITE_SEGMENT IOCTLS.
 * (This uses the DMA engines)
 *
 * The general purpose interrupt 3 is used by the driver to know when segments
 * have been updated.
 * General purpose interrupt number 3 is not available for client use,
 * they can use 1 and 2.
 * The 32-bit data word of Int-3 indicates which of the 32 segments
 * have been written.
 * Connecting to the SEGMENT_UPDATE interrupt (INT-3) and reading
 * the 32-Bit value permits a task to synchronise on segment updates.
 *
 * Each time a segment is updated, the fact is latched for each client
 * in a bit for each segment.
 * The CHECK_SEGMENT IOCTL returns the value of the updated Id's
 * in a 32-Bit long. This is for polling.
 * Each READ_SEGMENT IOCTL Clears the segments Id bit in the updated bits mask.
 *
 * Direct access to the VMIC SDRAM is permitted, the layout of the
 * segment table is available by calling the GET_SEGMENT_TABLE IOCTL.
 * This is the only interface allowed for node Id > 32
 *
 * The driver is transparent to the host machine's endianness; this is achieved
 * by using the CDCM functions to convert data from/to the host computer
 * to/from the device.
 * Note that the PLX does not swap any data (i.e. BIGEND set to little endian),
 * except for the case of a DMA transfer (where the CPU is not involved).
 * Therefore we can only transfer to the VMIC's SDRAM chunks of 4 bytes -- to
 * be consistent since otherwise the PLX we would be performing data-invariance,
 * and what we want to is address invariance, which is simpler.
 * See the paper below for data and address invariance:
 * "Data Movement Between Big-Endian and Little-Endian Devices",
 * by Kyle Aubrey and Ashan Kabir.
 * http://www.freescale.com/files/32bit/doc/app_note/AN2285.pdf
 *
 * @version 2.0 Emilio G. Cota 15/10/2008, CDCM-compliant.
 * @version 1.0 Julian Lewis 02/09/2004, Lynx driver.
 */

#include <cdcm/cdcm.h>
#include <cdcm/cdcmPciDma.h>

#include <plx9656_layout.h>
#include <vmic5565_layout.h>
#include <xmemDrvr.h>
#include <xmemDrvrP.h>

#ifdef __powerpc__
extern void iointunmask(); /* needed to register an interrupt */
#endif

static XmemDrvrWorkingArea *Wa; //!< Global pointer to Working Area

static const char *ioc_names[] = {
	[_IOC_NR(XmemDrvrILLEGAL_IOCTL)]	= "First IOCTL",
	[_IOC_NR(XmemDrvrSET_SW_DEBUG)]		= "Set Debug Mode",
	[_IOC_NR(XmemDrvrGET_SW_DEBUG)]		= "Get Debug Mode",
	[_IOC_NR(XmemDrvrGET_VERSION)]		= "Get Version",
	[_IOC_NR(XmemDrvrSET_TIMEOUT)]		= "Set Timeout",
	[_IOC_NR(XmemDrvrGET_TIMEOUT)]		= "Get Timeout",
	[_IOC_NR(XmemDrvrSET_QUEUE_FLAG)]	= "Set Queue Flag",
	[_IOC_NR(XmemDrvrGET_QUEUE_FLAG)]	= "Get Queue Flag",
	[_IOC_NR(XmemDrvrGET_QUEUE_SIZE)]	= "Get Queue Size",
	[_IOC_NR(XmemDrvrGET_QUEUE_OVERFLOW)]	= "Get Queue Overflow",
	[_IOC_NR(XmemDrvrGET_MODULE_DESCRIPTOR)] = "Get Module Descriptor",
	[_IOC_NR(XmemDrvrSET_MODULE)]		= "Set Module",
	[_IOC_NR(XmemDrvrGET_MODULE)]		= "Get Current Module",
	[_IOC_NR(XmemDrvrGET_MODULE_COUNT)]	= "Get Module Count",
	[_IOC_NR(XmemDrvrSET_MODULE_BY_SLOT)]	= "Get Module by Slot",
	[_IOC_NR(XmemDrvrGET_MODULE_SLOT)]	= "Get Module Slot",
	[_IOC_NR(XmemDrvrRESET)]		= "Reset",
	[_IOC_NR(XmemDrvrSET_COMMAND)]		= "Set Command",
	[_IOC_NR(XmemDrvrGET_STATUS)]		= "Get Status",
	[_IOC_NR(XmemDrvrGET_NODES)]		= "Get Nodes",
	[_IOC_NR(XmemDrvrGET_CLIENT_LIST)]	= "Get Client List",
	[_IOC_NR(XmemDrvrCONNECT)]		= "Connect to Interrupt",
	[_IOC_NR(XmemDrvrDISCONNECT)]		= "Disconnect from Interrupt",
	[_IOC_NR(XmemDrvrGET_CLIENT_CONNECTIONS)] = "Get Client Connections",
	[_IOC_NR(XmemDrvrSEND_INTERRUPT)]	= "Send Interrupt",
	[_IOC_NR(XmemDrvrGET_XMEM_ADDRESS)]	= "Get Xmem Address",
	[_IOC_NR(XmemDrvrSET_SEGMENT_TABLE)]	= "Set Segment Table",
	[_IOC_NR(XmemDrvrGET_SEGMENT_TABLE)]	= "Get Segment Table",
	[_IOC_NR(XmemDrvrREAD_SEGMENT)]		= "Read Segment",
	[_IOC_NR(XmemDrvrWRITE_SEGMENT)]	= "Write Segment",
	[_IOC_NR(XmemDrvrCHECK_SEGMENT)]	= "Check Segment",
	[_IOC_NR(XmemDrvrFLUSH_SEGMENTS)]	= "Flush Segments",
	[_IOC_NR(XmemDrvrCONFIG_OPEN)]		= "Open Configuration Regs.",
	[_IOC_NR(XmemDrvrLOCAL_OPEN)]		= "Open Local Regs.",
	[_IOC_NR(XmemDrvrRFM_OPEN)]		= "Open RFM Regs.",
	[_IOC_NR(XmemDrvrRAW_OPEN)]		= "Open VMIC SDRAM",
	[_IOC_NR(XmemDrvrCONFIG_READ)]		= "Read from Config. Regs.",
	[_IOC_NR(XmemDrvrLOCAL_READ)]		= "Read from Local Regs.",
	[_IOC_NR(XmemDrvrRFM_READ)]		= "Read from RFM Regs.",
	[_IOC_NR(XmemDrvrRAW_READ)]		= "Read from VMIC SDRAM",
	[_IOC_NR(XmemDrvrCONFIG_WRITE)]		= "Write to Config. Regs.",
	[_IOC_NR(XmemDrvrLOCAL_WRITE)]		= "Write to Local Regs.",
	[_IOC_NR(XmemDrvrRFM_WRITE)]		= "Write to RFM Regs.",
	[_IOC_NR(XmemDrvrRAW_WRITE)]		= "Write to VMIC SDRAM",
	[_IOC_NR(XmemDrvrCONFIG_CLOSE)]		= "Close Conf. Regs.",
	[_IOC_NR(XmemDrvrLOCAL_CLOSE)]		= "Close Local Regs.",
	[_IOC_NR(XmemDrvrRFM_CLOSE)]		= "Close RFM Regs.",
	[_IOC_NR(XmemDrvrRAW_CLOSE)]		= "Close VMIC SDRAM",
	[_IOC_NR(XmemDrvrSET_DMA_THRESHOLD)]	= "Set Dma Threshold",
	[_IOC_NR(XmemDrvrGET_DMA_THRESHOLD)]	= "Get Dma Threshold",
	[_IOC_NR(XmemDrvrLAST_IOCTL)]		= "Last IOCTL"
};

/**
 * LongCopy - copies size bytes in chunks of longs from src to dst.
 *
 * @param dst: copy to
 * @param src: copy from
 * @param size: size of the transfer
 *
 * Note that if the size to be copied is smaller than sizeof(long),
 * no copy will be performed.
 *
 */
static void LongCopy(unsigned long *dst, unsigned long *src, unsigned long size)
{
	int sb;
	int i;

	sb = size/sizeof(unsigned long);
	for (i = 0; i < sb; i++) dst[i] = src[i];
}

/**
 * LongCopyToXmem - copies size bytes in chunks of longs to the Xmem.
 *
 * @param vmic_to: copy to
 * @param v_from: copy from
 * @param size: size of the transfer
 *
 * Note that if the size to be copied is smaller than sizeof(long),
 * no copy will be performed.
 *
 */
static void LongCopyToXmem(void *vmic_to, void *v_from, unsigned long size)
{
	int count;
	u_int32_t *tbuf = (u_int32_t *)v_from;
	u_int32_t *ioaddr = (u_int32_t *)vmic_to;

	count = size / sizeof(u_int32_t);
	if (count <= 0) return;
	do {
		cdcm_iowrite32le(*tbuf, ioaddr);
		tbuf++;
		ioaddr++;
	} while (--count != 0);
}

/**
 * LongCopyFromXmem - copies size bytes in chunks of longs from the Xmem.
 *
 * @param vmic_from: copy from
 * @param v_to: copy to
 * @param size: size of the transfer
 *
 * Note that if the size to be copied is smaller than sizeof(long),
 * no copy will be performed.
 *
 */
static void LongCopyFromXmem(void *vmic_from, void *v_to, unsigned long size)
{
	int count;
	u_int32_t *tbuf = (u_int32_t *)v_to;
	u_int32_t *ioaddr = (u_int32_t *)vmic_from;

	count = size / sizeof(u_int32_t);
	if (count <= 0) return;
	do {
		*tbuf++ = cdcm_ioread32le(ioaddr);
		ioaddr++;
	} while (--count != 0);
}

/**
 * CancelTimeout - cancels a given timeout
 *
 * @param t: pointer to the ID of the timeout to be cancelled
 *
 * Care has been taken to avoid cancelling an already cancelled (or expired)
 * timeout by protecting the critical region properly.
 *
 */
static void CancelTimeout(int *t)
{
	unsigned long ps;
	int v;

	disable(ps);
	{
		if ((v = *t)) {
			*t = 0;
			cancel_timeout(v);
		}
	}
	restore(ps);
}

/**
 * ReadTimeout - handler function for ccon->Timer.
 *
 * @param c: pointer to the ID of the timeout
 *
 * @return 0 on success
 */
static int ReadTimeout(void *c)
{
	XmemDrvrClientContext *ccon = (XmemDrvrClientContext*)c;
	ccon->Timer = 0;
	ccon->Queue.Size   = 0;
	ccon->Queue.Missed = 0;
	sreset(&ccon->Semaphore);
	return 0;
}

/**
 * WrDmaTimeout - handler function for mcon->WrDmaSemaphore
 *
 * @param m: pointer to the ID of the timeout
 *
 * WrDmaSemaphore is signalled when the timeout expires.
 *
 * @return 0 on success
 */
static int WrDmaTimeout(void *m)
{
	XmemDrvrModuleContext *mcon = (XmemDrvrModuleContext*)m;
	mcon->WrTimer = 0;
	ssignal(&mcon->WrDmaSemaphore);
	return 0;
}

/**
 * RdDmaTimeout - handler function for mcon->RdDmaSemaphore
 *
 * @param m: pointer to the ID of the timeout
 *
 * RdDmaSemaphore is signalled when the timeout expires.
 *
 * @return 0 on success
 */
static int RdDmaTimeout(void *m)
{
	XmemDrvrModuleContext *mcon = (XmemDrvrModuleContext*)m;
	mcon->RdTimer = 0;
	ssignal(&mcon->RdDmaSemaphore);
	return 0;
}

static int ioc_nr_ok(int nr)
{
	return WITHIN_RANGE(_IOC_NR(XmemDrvrILLEGAL_IOCTL), _IOC_NR(nr),
			    _IOC_NR(XmemDrvrLAST_IOCTL)) &&
		_IOC_TYPE(nr) == XMEM_IOCTL_MAGIC;
}

/**
 * TraceIoctl - tracks the IOCTL requests by printing on the kernel's log.
 *
 * @param nr: IOCTL command number
 * @param arg: argument of the IOCTL call
 * @param ccon: pointer to client context
 *
 * This function will work only if XmemDrvrDebugTRACE and ccon->Debug are set.
 *
 */
static void TraceIoctl(int nr, char *arg, XmemDrvrClientContext *ccon)
{
	const char *iocname;

	if (!(ccon->Debug & XmemDrvrDebugTRACE))
		return;

	if (!ioc_nr_ok(nr))
		return;

	iocname = ioc_names[_IOC_NR(nr)];

	if (arg)
		XMEM_INFO("IOCTL '%s' Arg: %d", iocname, *((int *)arg));
	else
		XMEM_INFO("IOCTL '%s' Arg: [Null]", iocname);

	return;
}

/**
 * SetEndian - sets the endianness of the machine in the working area.
 *
 * This function uses the CDCM_{BIG,LITTLE}_ENDIAN macros, which
 * are platform-independent.
 *
 */
static void SetEndian(void)
{
#ifdef CDCM_LITTLE_ENDIAN
	Wa->Endian = XmemDrvrEndianLITTLE;
#else
	Wa->Endian = XmemDrvrEndianBIG;
#endif
}

/**
 * DrmLocalReadWrite - for PLX Local Configuration
 *
 * @param mcon: module context
 * @param addr: offset within the PLX Local Registers
 * @param value: pointer to where the data is read/written
 * @param size: size of the data to be transferred
 * @param flag: XmemDrvr READ/WRITE flag
 *
 * This function reads/writes from/to the PLX9656 local config registers.
 *
 * @return OK on success
 */
static int DrmLocalReadWrite(XmemDrvrModuleContext *mcon, unsigned long addr,
			unsigned long *value, unsigned long size,
			XmemDrvrIoDir flag)
{
	void *lmap;
	void *ioaddr;
	unsigned long ps;

	lmap = (void *)mcon->Local;
	if (!lmap) return SYSERR;
	ioaddr = lmap + addr;
	if (!recoset()) {
		if (flag == XmemDrvrWRITE) {
			switch (size) {
			case XmemDrvrBYTE:
				cdcm_iowrite8(*value, ioaddr);
				break;
			case XmemDrvrWORD:
				cdcm_iowrite16le(*value, ioaddr);
				break;
			case XmemDrvrLONG:
				cdcm_iowrite32le(*value, ioaddr);
				break;
			}
		} else {
			switch (size) {
			case XmemDrvrBYTE:
				*value = cdcm_ioread8(ioaddr);
				break;
			case XmemDrvrWORD:
				*value = cdcm_ioread16le(ioaddr);
				break;
			case XmemDrvrLONG:
				*value = cdcm_ioread32le(ioaddr);
				break;
			}
		}
		return OK;
	}
	else {
		disable(ps);
		kkprintf("xmemDrvr: BUS-ERROR: Module:%d PciSlot:%d \n",
			(int) mcon->ModuleIndex+1,
			(int) mcon->PciSlot);
		noreco();
		enable(ps);
		pseterr(ENXIO); /* No such device or address */
		return SYSERR;
	}
	noreco();
	return OK;
}

/**
 * DrmConfigReadWrite - Read/Write PLX' config registers
 *
 * @param mcon: module context
 * @param addr: offset within the config registers
 * @param value: pointer to where the data is read/written
 * @param size: size of the data to be transferred
 * @param flag: XmemDrvr READ/WRITE flag
 *
 * Reads/writes from the PLX9656 configuration registers.
 * Since drm_device_{read,write} can only access 4-byte aligned addresses,
 * we only read/write in chunks of 4 bytes.
 *
 * @return OK on success
 */
static int DrmConfigReadWrite(XmemDrvrModuleContext *mcon, unsigned long addr,
			unsigned long *value, unsigned long size,
			XmemDrvrIoDir flag)
{
	int cc;
	unsigned long regval;
	unsigned long regnum;
	unsigned char *cptr;
	unsigned short *sptr;
	int i=0;

	regnum = addr >> 2;
	cptr = (char *)&regval;
	sptr = (short *)&regval;
	/* First read the long register value (forced because of drm lib) */
	cc = drm_device_read(mcon->Handle, PCI_RESID_REGS, regnum, 0, &regval);
	if (cc) {
		cprintf("xmemDrvr: DrmConfigReadWrite:errno %d in drm_device_read\n", cc);
		return cc;
	}
	switch (size) {
	case 1: /* byte: i = {0,1,2,3} */
#if defined(CDCM_BIG_ENDIAN)
		i = (~addr) & 0x3;
#else
		i = addr & 0x3;
#endif
		break;
	case 2: /* word: i = {0,1} */
#if defined(CDCM_BIG_ENDIAN)
		i = (~addr >> 1) & 0x1;
#else
		i = (addr >> 1) & 0x1;
#endif
		break;
	default: /* double word: no swapping needed here */
		break;
	}
	if (flag == XmemDrvrWRITE) {
		switch(size) {
		case 1:
			cptr[i] = (unsigned char)*value;
			break;
		case 2:
			sptr[i] = (unsigned short)*value;
			break;
		default:
			regval = (unsigned long)*value;
		}
		/* Write back modified configuration register */
		cc = drm_device_write(mcon->Handle, PCI_RESID_REGS, regnum, 0, &regval);
		if (cc) {
			cprintf("xmemDrvr:DrmConfigReadWrite:errno %d@drm_device_write\n", cc);
			return cc;
		}
	}
	else { /* XmemDrvrREAD */
		switch(size) {
		case 1:
			*value = (unsigned long)cptr[i];
			break;
		case 2:
			*value = (unsigned long)sptr[i];
			break;
		default:
			*value = (unsigned long)regval;
		}
	}
	return OK;
}

#ifndef COMPILE_TIME /* the makefile should set it */
#define COMPILE_TIME 0
#endif

/**
 * RfmIo - read/writes VMIC RFM registers
 *
 * @param mcon: module context
 * @param riob: description of the block on which we want to perform I/O
 * @param flag: read/write flag
 *
 * This function uses the CDCM functions to access the I/O mapped memory.
 * Note that the memory barriers are not necessary since cdcm_{read,write}X
 * already include them.
 * Pay attention as well to the endianness: cdcm_cpu_to_{le,be}X ensure
 * portable code. This is _address invariance_, which requires the
 * PLX to be set to little endian (i.e. the PLX doesn't swap anything).
 *
 * @return OK on success
 */
static int RfmIo(XmemDrvrModuleContext *mcon, XmemDrvrRawIoBlock *riob,
		XmemDrvrIoDir flag)
{

	unsigned long *vmap;
	unsigned long *uary;
	void *ioaddr;
	char *iod;

	int rval; /* Return value */
	int i = 0;
	int itms, offs;
	unsigned long ps;
	XmemDrvrSize size;

	rval = OK;

	/* shorter names for the riob variables */
	uary = riob->UserArray;
	size = riob->Size;
	offs = riob->Offset;
	itms = riob->Items;

	vmap = (unsigned long *)mcon->Map;
	ioaddr = (void *)vmap;

	if (itms == 0) itms = 1;

	if (!recoset()) {         /* Catch bus errors */

		for (i = 0; i < itms; i++) {

			if (flag == XmemDrvrWRITE) {
				switch (size) {
				case XmemDrvrBYTE:
					cdcm_iowrite8(uary[i], ioaddr + offs);
					break;
				case XmemDrvrWORD:
					cdcm_iowrite16le(uary[i], ioaddr + offs);
					break;
				case XmemDrvrLONG:
					cdcm_iowrite32le(uary[i], ioaddr + offs);
					break;
				}
			} else {
				switch (riob->Size) {
				case XmemDrvrBYTE:
					uary[i] = cdcm_ioread8(ioaddr + offs);
					break;
				case XmemDrvrWORD:
					uary[i] = cdcm_ioread16le(ioaddr + offs);
					break;
				case XmemDrvrLONG:
					uary[i] = cdcm_ioread32le(ioaddr + offs);
					break;
				}
			}
			offs += (int) size;
		}
	} else {
		disable(ps);

		if (flag == XmemDrvrWRITE)
			iod = "Write";
		else
			iod = "Read";
		kkprintf("xmemDrvr: BUS-ERROR: Module:%d Addr:%x Dir:%s Data:%d\n",
			(int)mcon->ModuleIndex + 1,(int)ioaddr + offs, iod, (int)uary[i]);

		pseterr(ENXIO); /* No such device or address */
		rval = SYSERR;
		restore(ps);
	}
	noreco(); /* Remove local bus trap */
	return rval;
}

/**
 * GetRfmReg - reads a register from the VMIC's RFM
 *
 * @param mcon: module context
 * @param reg: register offset, from the VmicRfm enum
 * @param size: bytes to be read
 *
 * This is just a wrapper for a single read call through RfmIo().
 *
 * @return the register's value (unsigned long format)
 */
static unsigned long GetRfmReg(XmemDrvrModuleContext *mcon, VmicRfm reg, int size)
{
	XmemDrvrRawIoBlock ioblk;
	unsigned long res;

	ioblk.Items = 1;
	ioblk.Size = size;
	ioblk.Offset = reg;
	ioblk.UserArray = &res;
	RfmIo(mcon, &ioblk, XmemDrvrREAD);
	return res;
}

/**
 * SetRfmReg - sets the value of a VMIC's RFM's register
 *
 * @param mcon: module context
 * @param reg: register offset, from the VmicRfm enum
 * @param size: bytes to be written
 * @param data: value to be written into the register
 *
 * This is just a wrapper for a single write operation through RfmIo().
 *
 */
static void SetRfmReg(XmemDrvrModuleContext *mcon, VmicRfm reg, int size,
		      unsigned long data)
{
	XmemDrvrRawIoBlock ioblk;

	ioblk.Items = 1;
	ioblk.Size = size;
	ioblk.Offset = reg;
	ioblk.UserArray = &data;
	RfmIo(mcon,&ioblk,XmemDrvrWRITE);
}


/**
 * GetVersion - gets the version (compile time) and checks for bus errors.
 *
 * @param mcon: module context
 * @param ver: pointer to struct XmemDrvrVersion, which will be written to.
 *
 * Apart from getting the version, this function is also used by PingModule() to
 * as a way of checking if the device is alive -- hence the bus error recovery.
 *
 * @return OK on success
 */
static int GetVersion(XmemDrvrModuleContext *mcon, XmemDrvrVersion *ver)
{
	unsigned long ps;

	if (!recoset()) { /* Catch bus errors  */
		ver->DriverVersion = COMPILE_TIME;                   /* Driver version */
		ver->BoardRevision = GetRfmReg(mcon,VmicRfmBRV,1);   /* Board Revision RO */
		ver->BoardId       = GetRfmReg(mcon,VmicRfmBID,1);   /* Board ID = 65  RO */
	} else {
		disable(ps);
		kkprintf("xmemDrvr: BUS-ERROR: Module:%d PciSlot:%d \n",
			(int) mcon->ModuleIndex+1, (int) mcon->PciSlot);
		noreco();
		enable(ps);
		pseterr(ENXIO); /* No such device or address */
		return SYSERR;
	}
	noreco();
	return OK;
}

/**
 * PingModule - ping module using GetVersion()
 *
 * @param mcon: module context
 *
 * GetVersion() is called by this function; if the device is alive then
 * PingModule() will return OK.
 *
 * @return OK if the device pointed by mcon is alive.
 */
static int PingModule(XmemDrvrModuleContext *mcon)
{
	XmemDrvrVersion ver;
	return GetVersion(mcon,&ver);
}

/**
 * EnableInterrupts - Enables interrupts given by msk on module mcon.
 *
 * @param mcon: module context
 * @param msk: bitmask that contains the interrupts to be enabled
 *
 * This function operates on the Rfm register area of the VMIC5565.
 * Registers are set to trigger the interrupts given through the bitmask msk.
 *
 * @return OK on success
 */
static int EnableInterrupts(XmemDrvrModuleContext *mcon, VmicLier msk)
{
	void *vmap;
	unsigned int lisr_tmp;
	unsigned long ps;
	unsigned long intcsr;

	vmap = mcon->Map;
	msk &= VmicLierMASK; /* Clear out any crap bits (e.g. IntSOFTWAKEUP) */

	disable(ps); /* acquire spinlock */
	if (msk & VmicLierINT1 && !(mcon->InterruptEnable & VmicLierINT1)) {
		SetRfmReg(mcon, VmicRfmSID1, 1, 0);
		cdcm_iowrite32le(0, vmap + VmicRfmISD1);
	}
	if (msk & VmicLierINT2 && !(mcon->InterruptEnable & VmicLierINT2)) {
		SetRfmReg(mcon, VmicRfmSID2, 1, 0);
		cdcm_iowrite32le(0, vmap + VmicRfmISD2);
	}
	if (msk & VmicLierINT3 && !(mcon->InterruptEnable & VmicLierINT3)) {
		SetRfmReg(mcon, VmicRfmSID3, 1, 0);
		cdcm_iowrite32le(0, vmap + VmicRfmISD3);
	}
	if (msk & VmicLierPENDING_INIT &&
		!(mcon->InterruptEnable & VmicLierPENDING_INIT)) {
		SetRfmReg(mcon, VmicRfmINITN, 1, 0);
		cdcm_iowrite32le(0, vmap + VmicRfmINITD);
	}
	if (msk & VmicLierPARITY_ERROR &&
		!(mcon->InterruptEnable & VmicLierPARITY_ERROR)) {
		mcon->Command |= VmicLcsrPARITY_ON;
		cdcm_iowrite32le(mcon->Command, vmap + VmicRfmLCSR1);
	}
	if (msk) {
		mcon->InterruptEnable |= msk;
		cdcm_iowrite32le(mcon->InterruptEnable, vmap + VmicRfmLIER);

		lisr_tmp = cdcm_ioread32le(vmap + VmicRfmLISR);

		cdcm_iowrite32le(lisr_tmp | VmicLisrENABLE, vmap + VmicRfmLISR);
		lisr_tmp = cdcm_ioread32le(vmap + VmicRfmLISR);

		DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrREAD);
		intcsr |= PlxIntcsrENABLE_PCI
			| PlxIntcsrENABLE_LOCAL
			| PlxIntcsrENABLE_DMA_CHAN_0
			| PlxIntcsrENABLE_DMA_CHAN_1;
		DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrWRITE);
#ifdef __powerpc__
		iointunmask(mcon->IVector);
#endif
	}
	restore(ps); /* release spinlock */
	return OK;
}

/**
 * GetSetStatus - Get or Set the the value of LCSR1: Local Control and Status 1.
 *
 * @param mcon: module context
 * @param cmd: command (32bits) to be set
 * @param iod: XmemDrvrREAD or XmemDrvrWRITE
 *
 * Commands are sent through cmd, which is a 32 bits long bitmask.
 *
 * @return resulting value of register LCSR1
 */
static XmemDrvrScr GetSetStatus(XmemDrvrModuleContext *mcon, XmemDrvrScr cmd,
				XmemDrvrIoDir iod)
{
	void *vmap;
	XmemDrvrScr stat;

	vmap = mcon->Map;

	if (iod == XmemDrvrWRITE) {
		cmd &= XmemDrvrScrCMD_MASK;
		mcon->Command = cmd;
		cdcm_iowrite32le(mcon->Command, vmap + VmicRfmLCSR1);
	}

	stat = (XmemDrvrScr)cdcm_ioread32le(vmap + VmicRfmLCSR1);

	return stat;
}

/**
 * InterruptSelf - 'Simulate' an interrupt to the module itself.
 *
 * @param sbuf: pointer to struct XmemDrvrSendBuf
 * @param mcon: module context
 *
 * This is what we mean by 'simulating' an interrupt:
 * For the client, the result of an interrupt is that ccon->Semaphore
 * is signalled, and then the client reads from ccon->Queue. So for each of
 * the clients connected to module mcon, a queue entry is filled according
 * to sbuf, and then ccon->Semaphore is signalled.
 *
 */
static void InterruptSelf(XmemDrvrSendBuf *sbuf, XmemDrvrModuleContext *mcon) {

	XmemDrvrNic            ntype;
	XmemDrvrReadBuf        rbf;
	XmemDrvrClientContext *ccon;
	XmemDrvrQueue         *queue;
	unsigned long usegs, msk;
	int i;

	bzero((void *) &rbf, sizeof(XmemDrvrReadBuf));
	usegs = 0;
	rbf.Module = sbuf->Module;
	ntype = sbuf->InterruptType & XmemDrvrNicTYPE;
	switch (ntype) {
	case XmemDrvrNicREQUEST_RESET:
		rbf.Mask = XmemDrvrIntrREQUEST_RESET;
		break;
	case XmemDrvrNicINT_1:
		rbf.Mask = XmemDrvrIntrINT_1;
		rbf.NodeId[XmemDrvrIntIdxINT_1] = mcon->NodeId;
		rbf.NdData[XmemDrvrIntIdxINT_1] = sbuf->Data;
		break;
	case XmemDrvrNicINT_2:
		rbf.Mask = XmemDrvrIntrINT_2;
		rbf.NodeId[XmemDrvrIntIdxINT_2] = mcon->NodeId;
		rbf.NdData[XmemDrvrIntIdxINT_2] = sbuf->Data;
		break;
	case XmemDrvrNicSEGMENT_UPDATE:
		rbf.Mask = XmemDrvrIntrSEGMENT_UPDATE;
		rbf.NodeId[XmemDrvrIntIdxSEGMENT_UPDATE] = mcon->NodeId;
		rbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE] = sbuf->Data;
		usegs = sbuf->Data;
		break;
	case XmemDrvrNicINITIALIZED:
		rbf.Mask = XmemDrvrIntrPENDING_INIT;
		rbf.NodeId[XmemDrvrIntIdxPENDING_INIT] = mcon->NodeId;
		rbf.NdData[XmemDrvrIntIdxPENDING_INIT] = sbuf->Data;
		break;
	default:
		break;
	}

	for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
		ccon = &Wa->ClientContexts[i];
		if (usegs)
			ccon->UpdatedSegments |= usegs;
		msk = mcon->Clients[i];

		if (msk & rbf.Mask) {
			queue = &ccon->Queue;
			if (queue->Size < XmemDrvrQUEUE_SIZE) {
				queue->Entries[queue->Size++] = rbf;
				ssignal(&ccon->Semaphore); /* Wakeup client */
			}
			else {
				queue->Size = XmemDrvrQUEUE_SIZE;
				queue->Missed++;
			}
		}

	}
}

/**
 * SendInterrupt - Send an interrupt to other node(s)
 *
 * @param sbuf: pointer to the XmemDrvrSendBuf struct
 *
 * @return OK on success
 */
static int SendInterrupt(XmemDrvrSendBuf *sbuf)
{
	XmemDrvrModuleContext *mcon;
	XmemDrvrNic ntype, ncast;
	int i, msk, midx;
	void *vmap;

	midx = sbuf->Module - 1;
	if (midx < 0 || midx >= Wa->Modules) {
		pseterr(ENXIO);
		return SYSERR;
	}

	mcon = &Wa->ModuleContexts[midx];
	vmap = mcon->Map;
	ntype = sbuf->InterruptType & XmemDrvrNicTYPE;
	ncast = sbuf->InterruptType & XmemDrvrNicCAST;

	cdcm_iowrite32le(sbuf->Data, vmap + VmicRfmNTD);

	switch (ncast) {

	case XmemDrvrNicBROADCAST:

		cdcm_iowrite8(XmemDrvrNicBROADCAST | ntype, vmap + VmicRfmNIC);
		InterruptSelf(sbuf, mcon);
		return OK;

	case XmemDrvrNicMULTICAST:

		for (i = 0; i < XmemDrvrNODES; i++) {
			if (i == mcon->NodeId - 1)
				InterruptSelf(sbuf, mcon);
			else {
				msk = 1 << i;
				if (msk & sbuf->MulticastMask) {
					cdcm_iowrite8(i + 1, vmap + VmicRfmNTN);
					cdcm_iowrite8(ntype, vmap + VmicRfmNIC);
				}
			}
		}
		return OK;

	case XmemDrvrNicUNICAST:

		if (sbuf->UnicastNodeId == mcon->NodeId)
			InterruptSelf(sbuf, mcon);
		else {
			cdcm_iowrite8(sbuf->UnicastNodeId, vmap + VmicRfmNTN);
			cdcm_iowrite8(ntype, vmap + VmicRfmNIC);
		}
		return OK;

	default:

		pseterr(EINVAL);
		return SYSERR;

	}
}

/**
 * Reset - Resets a module to a default state.
 *
 * @param mcon: module context
 *
 * If the device is not alive, SYSERR is returned.
 *
 * @return OK on success
 */
static int Reset(XmemDrvrModuleContext *mcon)
{
	XmemDrvrSendBuf sbuf;

	if (PingModule(mcon) == OK) {
		EnableInterrupts(mcon, mcon->InterruptEnable);
		GetSetStatus(mcon, mcon->Command | XmemDrvrScrDARK_ON, XmemDrvrWRITE);
		mcon->NodeId = GetRfmReg(mcon, VmicRfmNID, 1);
		Wa->Nodes = 1 << (mcon->NodeId - 1);
		/* Tell the network that I need to be initialised in case I'm connected
		   to PENDING_INIT. */
		if (mcon->InterruptEnable & XmemDrvrIntrPENDING_INIT) {
			sbuf.Module        = mcon->ModuleIndex + 1;
			sbuf.UnicastNodeId = 0;
			sbuf.MulticastMask = 0;
			sbuf.Data          = 1 << (mcon->NodeId -1);
			sbuf.InterruptType = XmemDrvrNicINITIALIZED | XmemDrvrNicBROADCAST;
			return SendInterrupt(&sbuf);
		} else return OK;
	}
	pseterr(ENXIO);
	return SYSERR;
}

/**
 * GetSeg - returns 1 if the segment is valid.
 *
 * @param segid: segment id (hex)
 * @param sgx: index number of the segment [0... n-1].
 *
 * Checks that the segment is valid, and also returns by reference the segment
 * index number (i.e. [0 ... n-1], where n is the number of nodes). With sgx
 * the called can access the segment matrix directly.
 *
 * @return 1 if the segment is valid; 0 otherwise.
 */
static int GetSeg(unsigned long segid, unsigned int *sgx)
{
	int i;
	int nodemsk;
	for (i = 0; i < XmemDrvrSEGMENTS; i++) {
		nodemsk = 0x1 << i;
		if (nodemsk & segid) {
			*sgx = i;
			break;
		}
	}
	if (! (Wa->SegTable.Used & nodemsk)) return 0;
	return 1;
}

/**
 * WrPermSeg - checks if the node has write access to the segment.
 *
 * @param segid: segment id
 * @param node: node [1...n]
 * @param nodeid: node id (bitmask)
 *
 * First it checks that the segment is valid; then checks if the node has
 * write access to the segment given by segid. It also returns by reference
 * the node id, which is a bitmask of the given node.
 *
 * @return 1 if node has been granted write permission, 0 otherwise.
 */
static int WrPermSeg(unsigned long segid, unsigned long node, int *nodeid)
{
	int sgx;
	XmemDrvrSegDesc *segment;

	if (! GetSeg(segid, &sgx)) return 0;
	segment = &Wa->SegTable.Descriptors[sgx];
	*nodeid = 0x1 << (node - 1);
	if (! (*nodeid & segment->Nodes)) return 0;
	return 1;
}

/* FIXME: BUG on PageCopy: if after settin up a DMA mapping the function fails,
 * we don't call unmap() before returning SYSERR.
 */
/**
 * PageCopy - Copies a page to/from VMIC's SDRAM using DMA.
 *
 * @param siod: contains the size and addresses of the buffers involved
 * @param iod: direction of the transfer (XmemDrvr{READ,WRITE})
 * @param mapped: 1 if the buffer has already been DMA-mapped. 0 otherwise.
 *
 * This function has to be called from a region protected by
 * mcon->BusySemaphore, since it accesses the struct mcon->DmaOp. Therefore
 * BusySemaphore will be held at least by those regions willing to perform
 * DMA to/from the VMIC.
 * if(!mapped), the host machine will set up a DMA mapping that will return
 * the physical address of the page given in siod.
 * if(mapped), that means DmaOp has been set by the caller and therefore
 * mcon->DmaOp contains all the necessary information to perform the DMA.
 *
 * @return OK on success
 */
static int PageCopy(XmemDrvrSegIoDesc *siod, XmemDrvrIoDir iod,
		const int mapped)
{
	XmemDrvrModuleContext *mcon;
	XmemDrvrSegDesc *sgds;
	unsigned int midx, sgx, myid;
	char *sram;
	unsigned long dmamode, dptr, cmdma, intcsr;

	sgx = 0;
	midx = siod->Module - 1;
	if (siod->UserArray == NULL && !mapped) goto address_err;
	if (midx < 0 || midx >= Wa->Modules) goto device_err;
	if (! siod->Id) goto device_err;
	if (! GetSeg(siod->Id, &sgx)) goto nosegment;

	mcon = &Wa->ModuleContexts[midx];
	DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrREAD);
	intcsr |= PlxIntcsrENABLE_PCI
		| PlxIntcsrENABLE_LOCAL
		| PlxIntcsrENABLE_DMA_CHAN_0
		| PlxIntcsrENABLE_DMA_CHAN_1;
	DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrWRITE);

	sgds = &Wa->SegTable.Descriptors[sgx];
	sram = sgds->Address + siod->Offset;
	if (! mapped) {
		mcon->DmaOp.Flag = iod == XmemDrvrWRITE;
		mcon->DmaOp.Len = siod->Size;
		mcon->DmaOp.Buffer = siod->UserArray;
		mcon->DmaOp.Dma = cdcm_pci_map(mcon, siod->UserArray,
					mcon->DmaOp.Len, mcon->DmaOp.Flag);
	}
	if (mcon->DmaOp.Dma == 0)
		goto nomapping;

	/* FIXME: code is repeated below. Do it properly! */
	switch (iod) {
	case XmemDrvrWRITE:
		/* Write to the VMIC's SDRAM from host machine
		 * We use the PLX' DMACHANNEL1 for writing
		 */
		if (! WrPermSeg(siod->Id, mcon->NodeId, &myid)) goto access_err;
		dmamode = PlxDmaMode16BIT
			| PlxDmaMode32BIT
			| PlxDmaModeINP_ENB
			| PlxDmaModeCONT_BURST_ENB
			| PlxDmaModeLOCL_BURST_ENB
			| PlxDmaModeINT_PIN_SELECT
			| PlxDmaModeDONE_INT_ENB;
		DrmLocalReadWrite(mcon, PlxLocalDMAMODE1, &dmamode, 4, XmemDrvrWRITE);
		DrmLocalReadWrite(mcon, PlxLocalDMAPADR1, (unsigned long *)&mcon->DmaOp.Dma,
				4, XmemDrvrWRITE);
		DrmLocalReadWrite(mcon, PlxLocalDMALADR1, (unsigned long *)&sram, 4,
				XmemDrvrWRITE);
		DrmLocalReadWrite(mcon, PlxLocalDMASIZ1, (unsigned long *)&mcon->DmaOp.Len,
				4, XmemDrvrWRITE);
		/* Set transfer direction and termination interrupt */
		dptr = PlxDmaDprTERM_INT_ENB & ~PlxDmaDprDIR_LOC_PCI;
		DrmLocalReadWrite(mcon, PlxLocalDMADPR1, &dptr, 4, XmemDrvrWRITE);

		mcon->WrTimer = timeout(WrDmaTimeout, (char *) mcon, XmemDrvrDMA_TIMEOUT);
		if (mcon->WrTimer < 0) {
			mcon->WrTimer = 0;
			pseterr(EBUSY);
			return SYSERR;
		}

		sreset(&mcon->WrDmaSemaphore);

		cmdma = PlxDmaCsrENABLE | PlxDmaCsrSTART; /* Command: Start transfer */
		DrmLocalReadWrite(mcon, PlxLocalDMACSR1, &cmdma, 1, XmemDrvrWRITE);

		if (swait(&mcon->WrDmaSemaphore, SEM_SIGABORT)) {
			sreset(&mcon->WrDmaSemaphore);
			pseterr(EINTR);
			return SYSERR;
		}
		if (mcon->WrTimer) {
			CancelTimeout(&mcon->WrTimer);
		}
		else {
			cmdma = 0;
			DrmLocalReadWrite(mcon, PlxLocalDMACSR1, &cmdma, 1, XmemDrvrREAD);
			if ((cmdma & PlxDmaCsrDONE) == 0) {
				pseterr(EBUSY);
				return SYSERR;
			}
		}
		if (! mapped)  /* mapping has to be cleared */
			cdcm_pci_unmap(mcon, mcon->DmaOp.Dma, mcon->DmaOp.Len, mcon->DmaOp.Flag);

		return OK;
		break;

	case XmemDrvrREAD:
		/* Read from the VMIC's SDRAM to host machine
		 * We use the PLX' DMACHANNEL0 for writing
		 */
		dmamode = PlxDmaMode16BIT
			| PlxDmaMode32BIT
			| PlxDmaModeINP_ENB
			| PlxDmaModeCONT_BURST_ENB
			| PlxDmaModeLOCL_BURST_ENB
			| PlxDmaModeINT_PIN_SELECT
			| PlxDmaModeDONE_INT_ENB;

		DrmLocalReadWrite(mcon, PlxLocalDMAMODE0, &dmamode, 4, XmemDrvrWRITE);
		DrmLocalReadWrite(mcon, PlxLocalDMAPADR0, (unsigned long *)&mcon->DmaOp.Dma,
				4, XmemDrvrWRITE);
		DrmLocalReadWrite(mcon, PlxLocalDMALADR0, (unsigned long *)&sram, 4,
				XmemDrvrWRITE);
		DrmLocalReadWrite(mcon, PlxLocalDMASIZ0, (unsigned long *)&mcon->DmaOp.Len,
				4, XmemDrvrWRITE);
		/* Set transfer direction and termination interrupt */
		dptr = PlxDmaDprTERM_INT_ENB | PlxDmaDprDIR_LOC_PCI;
		DrmLocalReadWrite(mcon,PlxLocalDMADPR0 ,&dptr, 4, XmemDrvrWRITE);

		mcon->RdTimer = timeout(RdDmaTimeout, (char *)mcon, XmemDrvrDMA_TIMEOUT);
		if (mcon->RdTimer < 0) {
			mcon->RdTimer = 0;
			pseterr(EBUSY);
			return SYSERR;
		}
		sreset(&mcon->RdDmaSemaphore);

		cmdma = PlxDmaCsrENABLE | PlxDmaCsrSTART; /* Command: Start transfer */
		DrmLocalReadWrite(mcon, PlxLocalDMACSR0, &cmdma, 1, XmemDrvrWRITE);

		if (swait(&mcon->RdDmaSemaphore, SEM_SIGABORT)) {
			sreset(&mcon->RdDmaSemaphore);
			pseterr(EINTR);
			return SYSERR;
		}
		if (mcon->RdTimer)
			CancelTimeout(&mcon->RdTimer);
		else {
			cmdma = 0;
			DrmLocalReadWrite(mcon, PlxLocalDMACSR0, &cmdma, 1, XmemDrvrREAD);
			if ((cmdma & PlxDmaCsrDONE) == 0) {
				pseterr(EBUSY);
				return SYSERR;
			}
		}
		if (! mapped)  /* mapping has to be cleared */
			cdcm_pci_unmap(mcon, mcon->DmaOp.Dma, mcon->DmaOp.Len, mcon->DmaOp.Flag);

		return OK;
		break;

	default:
		goto arg_err;
	}
access_err:
	cprintf("xmemDrvr:PageCopy: Write permission denied for segment ID=0x%X.\n",
		(unsigned int)siod->Id);
	pseterr(EACCES);
	return SYSERR;
address_err:
	cprintf("xmemDrvr:PageCopy: Illegal address.\n");
	pseterr(EFAULT);
	return SYSERR;
arg_err:
	cprintf("xmemDrvr:PageCopy: Wrong argument.\n");
	pseterr(EINVAL);
	return SYSERR;
device_err:
	cprintf("xmemDrvr:PageCopy: No such device.");
	pseterr(ENODEV);
	return SYSERR;
nosegment:
	cprintf("xmemDrvr:PageCopy:No such segment defined:%d.\n", sgx);
	pseterr(ENXIO);
	return SYSERR;
nomapping:
	cprintf("xmemDrvr:PageCopy: unable to perform DMA mapping.\n");
	pseterr(EIO);
	return SYSERR;
}

/**
 * SegmentCopy - copies a segment to/from the VMIC's SDRAM.
 *
 * @param siod: contains the information of the transfer to be done
 * @param iod: direction: {XmemDrvrREAD,XmemDrvrWRITE}
 * @param ccon: client context
 * @param mcon: module context
 *
 * If the size of the transfer is >= the DMA threshold, the physical pages
 * of the user's buffer are found and passed to PageCopy().
 * If the size is smaller than the threshold, I/O is CPU-based.
 *
 * @return OK on success
 */
static int SegmentCopy(XmemDrvrSegIoDesc *siod, XmemDrvrIoDir iod,
		XmemDrvrClientContext *ccon, XmemDrvrModuleContext *mcon)
{
	XmemDrvrSendBuf  sbuf;
	XmemDrvrSegIoDesc sio;
	XmemDrvrSegDesc *sgds;
	int err;
	unsigned int pg, pgs, myid, sgx, sram;

	err = OK;

	swait(&mcon->BusySemaphore, SEM_SIGIGNORE); /* acquire the mutex */
	/* Inside protected region ("Busy" state). */

	if (siod->UserArray == NULL) goto address_err;
	if (! siod->Id) goto nodevice;
	if (! GetSeg(siod->Id, &sgx)) goto nosegment;

	bzero((void *)&sio, sizeof(XmemDrvrSegIoDesc));
	sio.Module = siod->Module;
	sio.Id     = siod->Id;
	sio.Offset = siod->Offset;

	if (siod->Size >= Wa->DmaThreshold) {
		/* |--> then siod->UserArray is a pointer to user space. */
		/* Map and lock the pages of the user's buffer. */
		pgs = cdcm_pci_mmchain_lock(mcon->Handle, &mcon->Dma, iod == XmemDrvrWRITE,
					ccon->Pid, siod->UserArray, siod->Size, mcon->dmachain);
		if (ccon->Debug)
			cprintf("xmemDrvr: Page count from cdcm_pci_mmchain_lock: %d.\n", pgs);
		if (pgs <= 0 || pgs > XmemDrvrMAX_DMA_CHAIN) goto badchain;

		mcon->DmaOp.Flag = iod == XmemDrvrWRITE;
		mcon->DmaOp.Buffer = siod->UserArray;

		for (pg = 0; pg < pgs; pg++) {

			mcon->DmaOp.Dma = mcon->dmachain[pg].address; /* this is a portable type */
			mcon->DmaOp.Len = mcon->dmachain[pg].count;

			if (ccon->Debug) {
				cprintf("xmemDrvr:SegmentCopy:%d Addr:0x%X Size:%d Offset:0x%x\n",
					pg + 1, (int)mcon->DmaOp.Dma, (int)mcon->DmaOp.Len,
					(int)sio.Offset);
			}

			err = PageCopy(&sio, iod, 1); /* mapped = 1 */

			if (err < 0)
				break;
			sio.Offset += mcon->dmachain[pg].count;
		}
		/* clear SG mapping (which also unlocks the pages) */
		cdcm_pci_mem_unlock(mcon->Handle, &mcon->Dma, ccon->Pid, iod == XmemDrvrREAD);
	}
	else { /* siod->Size < Wa->DmaThreshold */
		/* Here, siod->UserArray is a pointer to kernel space. */
		sgds = &Wa->SegTable.Descriptors[sgx];
		sram = (unsigned int) sgds->Address + siod->Offset;

		if (iod == XmemDrvrWRITE) {
			if (! WrPermSeg(siod->Id, mcon->NodeId, &myid)) goto access_err;
			LongCopyToXmem(mcon->SDRam + sram, siod->UserArray, siod->Size);
		}
		else
			LongCopyFromXmem(mcon->SDRam + sram, siod->UserArray, siod->Size);
	}

	if (err == OK && iod == XmemDrvrWRITE && siod->UpdateFlg) {
		sbuf.Module        = siod->Module;
		sbuf.UnicastNodeId = 0;
		sbuf.MulticastMask = 0;
		sbuf.Data          = siod->Id;
		sbuf.InterruptType = XmemDrvrNicSEGMENT_UPDATE | XmemDrvrNicBROADCAST;
		err = SendInterrupt(&sbuf);
	}

	ssignal(&mcon->BusySemaphore); /* release the mutex */

	return err;

access_err:
	cprintf("xmemDrvr:SegmentCopy: Write permission denied, segment ID=0x%X.\n",
		(unsigned int)siod->Id);
	ssignal(&mcon->BusySemaphore);
	pseterr(EACCES);
	return SYSERR;
address_err:
	cprintf("xmemDrvr:SegmentCopy: illegal address.\n");
	ssignal(&mcon->BusySemaphore);
	pseterr(EFAULT);
	return SYSERR;
badchain:
	cprintf("xmemDrvr:SegmentCopy:Bad DMA chain list length:%d [1..%lu]\n",
		pgs, XmemDrvrMAX_DMA_CHAIN);
	ssignal(&mcon->BusySemaphore);
	pseterr(ERANGE);
	return SYSERR;
nodevice:
	cprintf("xmemDrvr:SegmentCopy:No such device.\n");
	ssignal(&mcon->BusySemaphore);
	pseterr(ENODEV);
	return SYSERR;
nosegment:
	cprintf("xmemDrvr:SegmentCopy:No such segment defined:%d.\n", (int)sgx);
	ssignal(&mcon->BusySemaphore);
	pseterr(ENXIO);
	return SYSERR;
}

/**
 * FlushSegments - flushes segments to network
 *
 * @param mcon: module context
 * @param mask: bitmask which contains which segments need to be flushed
 *
 * two mutexes are acquired in this functions: mcon->TempbufSemaphore and
 * mcon->BusySemaphore. The former provides  mutually exclusive access to
 * mcon->Tempbuf (which is used as a cache to send the segments in pieces)
 * and the latter has to be acquired before any call to PageCopy().
 *
 * @return OK on success
 */
static int FlushSegments(XmemDrvrModuleContext *mcon, unsigned long mask)
{
	XmemDrvrSegIoDesc  sio;
	XmemDrvrSegDesc   *sgds;
	XmemDrvrSendBuf    sbuf;
	unsigned long msk, umsk;
	int i, pg, err, rsze;

	swait(&mcon->TempbufSemaphore, SEM_SIGIGNORE); /* acquire the mutex */
	/* Tempbuf has been acquired */
	swait(&mcon->BusySemaphore, SEM_SIGIGNORE); /* acquire the mutex */
	/* Inside protected region ("Busy" state). */


	/* FIXME: So much unnecessary nesting. */
	umsk = 0;
	for (i = 0; i < XmemDrvrSEGMENTS; i++) {
		msk = 1 << i;
		if ((msk & mask) && (msk & Wa->SegTable.Used)) {
			umsk |= msk;
			sgds = &Wa->SegTable.Descriptors[i];

			bzero((void *) &sio, sizeof(XmemDrvrSegIoDesc));
			sio.Module = mcon->ModuleIndex + 1;
			sio.Id = msk;
			sio.UserArray = mcon->Tempbuf;
			sio.Offset = 0;

			/* chop up the segment in pagesized chunks and send them out */
			for (pg = 0; pg < sgds->Size; pg += PAGESIZE) {
				if (sgds->Size > pg) rsze = sgds->Size - pg;
				else                 rsze = sgds->Size;
				if (rsze > PAGESIZE) rsze = PAGESIZE;
				sio.Offset = pg;
				sio.Size = rsze;

				err = PageCopy(&sio, XmemDrvrREAD, 0); /* mapped = 0 */
				if (err < 0) {
					ssignal(&mcon->BusySemaphore);
					ssignal(&mcon->TempbufSemaphore);
					return err;
				}
				err = PageCopy(&sio, XmemDrvrWRITE, 0); /* mapped = 0 */
				if (err < 0) {
					ssignal(&mcon->BusySemaphore);
					ssignal(&mcon->TempbufSemaphore);
					return err;
				}
			}
		}
	}

	/* release mutexes */
	ssignal(&mcon->BusySemaphore);
	ssignal(&mcon->TempbufSemaphore);

	if (umsk) {
		sbuf.Module        = mcon->ModuleIndex + 1;
		sbuf.UnicastNodeId = 0;
		sbuf.MulticastMask = 0;
		sbuf.Data          = umsk;
		sbuf.InterruptType = XmemDrvrNicSEGMENT_UPDATE | XmemDrvrNicBROADCAST;
		return SendInterrupt(&sbuf);
	}
	return OK;
}

/**
 * RawIo - Raw IO operations to the VMIC SDRam, in chunks of 4 bytes only.
 *
 * @param mcon: module context
 * @param riob: raw IO operation struct
 * @param flag: write to module if flag==XmemDrvrWRITE; read otherwise.
 *
 * Note that since this function is not meant to be called by a client,
 * access rights to the segment are not checked -- in other words, this
 * function is only meant to be used by the driver's test program.
 *
 * @return OK on success
 */
static int RawIo(XmemDrvrModuleContext *mcon, XmemDrvrRawIoBlock *riob,
		XmemDrvrIoDir flag)
{
	unsigned long *vmap; /* Module Memory map */
	unsigned long *uary;

	int rval; /* Return value */
	int i = 0;
	int itms, offs, lidx = 0;
	unsigned long ps;   /* Processor status word */
	char *iod;

	rval = OK;

	vmap = (unsigned long *) mcon->SDRam;
	uary = riob->UserArray;
	offs = riob->Offset / sizeof(u_int32_t);
	itms = riob->Items;

	if (itms == 0) itms = 1;
	if (flag == XmemDrvrWRITE)
		iod = "Write";
	else
		iod = "Read";

	if (!recoset()) { /* Catch bus errors */

		if (flag == XmemDrvrWRITE)
			LongCopyToXmem(  vmap + offs, riob->UserArray, itms*sizeof(u_int32_t));
		else
			LongCopyFromXmem(vmap + offs, riob->UserArray, itms*sizeof(u_int32_t));

	} else {
		disable(ps);

		kkprintf("xmemDrvr: BUS-ERROR: Module:%d Addr:%x Dir:%s Data:%d\n",
			(int)mcon->ModuleIndex+1, (int)&vmap[lidx], iod,(int) uary[i]);

		pseterr(ENXIO); /* No such device or address */
		rval = SYSERR;
		restore(ps);
	}
	noreco(); /* Remove local bus trap */
	return rval;
}

/**
 * Connect - Connect to a VMIC interrupt
 *
 * @param conx: contains the interrupt bitmask to connect to.
 * @param ccon: client context.
 *
 * The client is connect to the VMIC interrupt given by the bitmask
 * conx->Mask.
 *
 * @return EnableInterrupt(), OK on success.
 */
static int Connect(XmemDrvrConnection *conx, XmemDrvrClientContext *ccon)
{
	int midx;
	unsigned long ps;
	XmemDrvrModuleContext *mcon;

	/* Get the module to make the connection on */
	midx = conx->Module - 1;
	if (midx < 0 || midx >= Wa->Modules)
		midx = ccon->ModuleIndex;

	mcon = &Wa->ModuleContexts[midx];
	disable(ps); /* acquire spinlock */
	mcon->Clients[ccon->ClientIndex] |= conx->Mask;
	restore(ps); /* release spinlock */
	return EnableInterrupts(mcon, conx->Mask);
}

/**
 * DisConnect - Disconnect from a VMIC interrupt
 *
 * @param conx: contains the interrupt bitmask to disconnect from.
 * @param ccon: client context.
 *
 * Disconnects a client from a VMIC interrupt, and if it was the only one
 * connected to it, disables that interrupts in the module.
 *
 * @return OK on success.
 */
static int DisConnect(XmemDrvrConnection *conx, XmemDrvrClientContext *ccon)
{
	int i, midx, disall;
	unsigned long msk;
	unsigned long ps;
	void *vmap;
	XmemDrvrModuleContext *mcon;

	/* Get the module to disconnect from */
	midx = conx->Module - 1;
	if (midx < 0 || midx >= Wa->Modules)
		midx = ccon->ModuleIndex;
	mcon  = &Wa->ModuleContexts[midx];


	disable(ps); /* acquire spinlock */
	mcon->Clients[ccon->ClientIndex] &= ~conx->Mask;
	/* Check to see if others are connected */
	disall = 1;
	for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
		if (conx->Mask & mcon->Clients[i]) {
			disall = 0;
			break;
		}
	}
	if (disall) {
		vmap = mcon->Map;
		msk = conx->Mask & VmicLierMASK; /* filter out non-hardware interrupts */
		if (msk) {
			mcon->InterruptEnable &= ~msk;
			cdcm_iowrite32le(mcon->InterruptEnable,
					 vmap + VmicRfmLIER);
		}
	}
	restore(ps); /* release spinlock */


	return OK;
}

/**
 * DisConnectAll - Disconnect a client from all VMIC interrupts from all the
 * modules.
 *
 * @param ccon: client context
 *
 */
static void DisConnectAll(XmemDrvrClientContext *ccon) {

	XmemDrvrConnection     conx;
	XmemDrvrModuleContext *mcon;
	int i, cidx;

	cidx = ccon->ClientIndex;
	for (i = 0; i < Wa->Modules; i++) {
		mcon = &Wa->ModuleContexts[i];
		if (mcon->Clients[cidx]) {
			conx.Module = i + 1;
			conx.Mask = mcon->Clients[cidx];
			DisConnect(&conx, ccon);
		}
	}
}

/**
 * IntrHandler - Interrupt handler
 *
 * @param m: module context
 *
 * The interrupt handler checks the source of the interrupt and interacts
 * with the clients accordingly.
 * In order to serve interrupts that are coming at a fast rate, the interrupt
 * status register (LISR) is checked and cleared in a while loop -- i.e. the
 * driver might serve more than one interrupt without exiting IntrHandler().
 * This approach has a serious drawback: LISR is read and cleared in a
 * non-atomic way!
 *
 */
void IntrHandler(void *m)
{
	uint32_t		isrc, msk, i;
	XmemDrvrModuleContext	*mcon = (XmemDrvrModuleContext*)m;
	XmemDrvrQueue		*queue;
	XmemDrvrClientContext	*ccon;
	XmemDrvrReadBuf		rbf;
	unsigned long		intcsr, usegs, node, data, dma;
	void			*vmap = mcon->Map;

	DrmLocalReadWrite(mcon, PlxLocalINTCSR, &intcsr, 4, XmemDrvrREAD);

	if (intcsr & PlxIntcsrSTATUS_LOCAL) {
		/* read the interrupt status register (LISR) */
		isrc = cdcm_ioread32le(vmap + VmicRfmLISR);
		isrc &= VmicLisrSOURCE_MASK; /* clean the bitmask */
		if (!isrc)
			goto out;

		bzero((void *) &rbf, sizeof(XmemDrvrReadBuf));
		rbf.Module = mcon->ModuleIndex + 1;
		usegs = 0;

		for (i = 0; i < VmicLisrSOURCES; i++) {
			msk = 1 << i;

			if (! (msk & isrc))
				continue;

			rbf.Mask |= msk;

			switch (msk) {
			case VmicLisrPARITY_ERROR:
			case VmicLisrWRITE_ERROR:
			case VmicLisrLOST_SYNC:
			case VmicLisrRX_OVERFLOW:
			case VmicLisrRX_ALMOST_FULL:
			case VmicLisrDATA_ERROR:
			case VmicLisrROGUE_CLOBBER:
			case VmicLisrRESET_RQ:
				/* do nothing */
				break;

				/*
				 * When a node comes up it sends out a pending-init interrupt
				 * with a non zero data field. Any client on a node connected
				 * to the pending-init interrupt will send back another init
				 * interrupt but with a zero data field. In this way, we will
				 * establish a node map of nodes connected to pending-init,
				 * and hence an algorithm on the client side to handle these
				 * interrupts can be established based on a knowledge of all
				 * nodes present and interested in pending-init interrupts.
				 */
			case VmicLisrPENDING_INIT:

				data = cdcm_ioread32le(vmap + VmicRfmINITD);
				rbf.NdData[XmemDrvrIntIdxPENDING_INIT] = data;

				node = GetRfmReg(mcon, VmicRfmINITN, 1);
				rbf.NodeId[XmemDrvrIntIdxPENDING_INIT] = node;

				Wa->Nodes |= (1 << (node - 1)); /* update known nodes */

				/*
				 * If someone needs initializing, I broadcast back a pending init
				 * with zero data. Proceeding this way all nodes will know who is
				 * on-line. Everyone else will do the same, so eventually all the
				 * nodes will have introduced themselves.
				 * Note that 'data' is tested to be not zero -- otherwise the
				 * protocol would never end.
				 */

				if (! data)
					break;

				/* reset known nodes to the only two I know for the time being */
				Wa->Nodes = (1 << (mcon->NodeId - 1)) | (1 << (node - 1));
				break;

				/* INT3 is SEGMENT_UPDATE, it's not for general use. */
			case VmicLisrINT3:

				rbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE] =
					cdcm_ioread32le(vmap + VmicRfmISD3);

				node = GetRfmReg(mcon, VmicRfmSID3, 1);
				rbf.NodeId[XmemDrvrIntIdxSEGMENT_UPDATE] = node;

				/* pick up updated segments */
				usegs = rbf.NdData[XmemDrvrIntIdxSEGMENT_UPDATE];

				Wa->Nodes |= 1 << (node - 1); /* update known nodes */

				break;


			case VmicLisrINT2:

				rbf.NdData[XmemDrvrIntIdxINT_2] =
					cdcm_ioread32le(vmap + VmicRfmISD2);

				node = GetRfmReg(mcon, VmicRfmSID2, 1);
				rbf.NodeId[XmemDrvrIntIdxINT_2] = node;

				Wa->Nodes |= 1 << (node - 1); /* update known nodes */
				break;

			case VmicLisrINT1:

				rbf.NdData[XmemDrvrIntIdxINT_1] =
					cdcm_ioread32le(vmap + VmicRfmISD1);

				node = GetRfmReg(mcon, VmicRfmSID1, 1);
				rbf.NodeId[XmemDrvrIntIdxINT_1] = node;

				Wa->Nodes |= 1 << (node -1); /* update known nodes */
				break;

			default:
				rbf.Mask = 0;
				break;
			}
		}

		for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {

			ccon = &Wa->ClientContexts[i];

			if (usegs) { /* Or in updated segments in clients */
				ccon->UpdatedSegments |= usegs;
			}

			msk = mcon->Clients[i];

			if (!(msk & isrc))
				continue;

			queue = &ccon->Queue;
			if (queue->Size < XmemDrvrQUEUE_SIZE) {
				queue->Entries[queue->Size++] = rbf;
				ssignal(&ccon->Semaphore); /* Wakeup client */
			} else {
				queue->Size = XmemDrvrQUEUE_SIZE;
				queue->Missed++;
			}

		} /* gone through all the interrupt sources */

	}

	if (intcsr & PlxIntcsrSTATUS_DMA_CHAN_0) {   /* DMA Channel 0 for reading */

		dma = PlxDmaCsrCLEAR_INTERRUPT;
		DrmLocalReadWrite(mcon, PlxLocalDMACSR0, &dma, 1, XmemDrvrWRITE);

		ssignal(&mcon->RdDmaSemaphore); /* wake up caller */

	}

	if (intcsr & PlxIntcsrSTATUS_DMA_CHAN_1) {   /* DMA Channel 1 for writing */

		dma = PlxDmaCsrCLEAR_INTERRUPT;
		DrmLocalReadWrite(mcon, PlxLocalDMACSR1, &dma, 1, XmemDrvrWRITE);

		ssignal(&mcon->WrDmaSemaphore); /* wake up caller */
	}

out:
	/* under Lynx + PowerPC, this is necessary to trigger the interrupts */
#if (!defined(__linux__) && defined(__powerpc__))
	iointunmask(mcon->IVector);
#endif
	/* re-enable interrupts */
	cdcm_iowrite32le(VmicLisrENABLE, vmap + VmicRfmLISR);
}


/**
 * XmemDrvrOpen - open service access point for the user
 *
 * @param s: working area
 * @param dnm: device number
 * @param flp: file pointer
 *
 * @return OK on success
 */
int XmemDrvrOpen(void *s, int dnm, struct cdcm_file *flp)
{
	int cnum;                       /* Client number */
	XmemDrvrClientContext *ccon;    /* Client context */
	XmemDrvrQueue         *queue;   /* Client queue */

	/*
	 * We allow one client per minor device, we use the minor device
	 * number as an index into the client contexts array.
	 */

	cnum = minor(flp->dev) - 1;
	if (cnum < 0 || cnum >= XmemDrvrCLIENT_CONTEXTS) {
		pseterr(EFAULT);
		return SYSERR;
	}

	ccon = &Wa->ClientContexts[cnum];

	if (ccon->InUse) {
		pseterr(EBUSY);
		return SYSERR;
	}

	/* Initialize a client context */
	bzero((void *)ccon, sizeof(XmemDrvrClientContext));
	ccon->ClientIndex     = cnum;
	ccon->Timeout         = XmemDrvrDEFAULT_TIMEOUT;
	ccon->InUse           = 1;
	ccon->Pid             = getpid();
	ccon->UpdatedSegments = 0;
	ccon->Debug           = XmemDrvrDebugNONE;

	queue = &ccon->Queue;
	queue->Size   = 0;
	queue->Missed = 0;
	sreset(&ccon->Semaphore);

#if 0
	cprintf("xmemDrvr:Open:%d ClientSem:0x%X:%d PID:%d\n",
		cnum,&ccon->Semaphore,ccon->Semaphore,ccon->Pid);
#endif

	return OK;
}

/**
 * XmemDrvrClose - routine called to close the device's access point
 *
 * @param s: working area
 * @param flp: file pointernn
 *
 * @return OK on success
 */
int XmemDrvrClose(void *s, struct cdcm_file *flp)
{
	int cnum;                    /* Client number */
	XmemDrvrClientContext *ccon; /* Client context */
	XmemDrvrWorkingArea *wa __attribute__((__unused__)) = (XmemDrvrWorkingArea*)s;

	/*
	 * We allow one client per minor device, we use the minor device
	 * number as an index into the client contexts array.
	 */

	cnum = minor(flp->dev) - 1;

	if (cnum < 0 || cnum >= XmemDrvrCLIENT_CONTEXTS) {
		cprintf("xmemDrvrClose: Error bad client context: %d\n",cnum);
		pseterr(EFAULT);
		return SYSERR;
	}

	ccon = &Wa->ClientContexts[cnum];

	if (ccon->Debug & XmemDrvrDebugTRACE) {
		cprintf("xmemDrvrClose: Close client: %d for Pid: %d\n",
			(int)ccon->ClientIndex, (int)ccon->Pid);
	}
	if (ccon->InUse == 0) {
		cprintf("xmemDrvrClose:Error:bad client context: %d Pid: %d Not in use\n",
			(int)ccon->ClientIndex, (int)ccon->Pid);
		pseterr(EBADF); /* Bad file number */
		return SYSERR;
	}

	/* Cancel any pending timeouts */
	CancelTimeout(&ccon->Timer);

	/* Disconnect this client from events */
	DisConnectAll(ccon);

	/* Wipe out the client context */
	bzero((void *) ccon, sizeof(XmemDrvrClientContext));

	return(OK);
}

/**
 * XmemDrvrRead - Read entry point
 *
 * @param s: working area
 * @param flp: file pointer
 * @param u_buf: user buffer. results will go here
 * @param cnt: number of bytes to read
 *
 * @return number of bytes read
 */
int XmemDrvrRead(void *s, struct cdcm_file *flp, char *u_buf, int cnt)
{
	XmemDrvrClientContext *ccon;    /* Client context */
	XmemDrvrQueue         *queue;
	XmemDrvrReadBuf       *rb;
	XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea*)s;
	int           cnum; /* Client number */
	int           wcnt; /* Writable byte counts at arg address */
	int           pid;
	unsigned int  iq;
	unsigned long ps;

	cnum = minor(flp->dev) - 1;

	if (u_buf) {
		wcnt = wbounds((int)u_buf); /* Number of writeable bytes without error */
		if (wcnt < sizeof(XmemDrvrReadBuf)) {
			cprintf("xmemDrvr:Read: Target buffer too small: Context:%d Not Open\n",
				cnum);
			pseterr(EINVAL);
			return SYSERR;
		}
	}

	ccon = &wa->ClientContexts[cnum];

	if (ccon->InUse != 1) {
		cprintf("xmemDrvr:Read: Bad FileDescriptor: Context:%d Not Open\n", cnum);
		pseterr(EBADF); /* Bad file number */
		return SYSERR;
	}

	/*
	 * Block any calls coming from unknown PIDs.
	 * Only the PID that opened the driver is allowed to read.
	 */

	pid = getpid();
	if (pid != ccon->Pid) {
		cprintf("xmemDrvrRead: Spurious read PID:%d for PID:%d on FD:%d\n",
			(int)pid, (int)ccon->Pid, (int)cnum);
		pseterr(EBADF); /* Bad file number */
		return SYSERR;
	}

	queue = &ccon->Queue;

	if (queue->QueueOff) {
		disable(ps);
		{
			queue->Size   = 0;
			queue->Missed = 0;
			sreset(&ccon->Semaphore);
		}
		restore(ps);
	}

	if (ccon->Timeout) {
		ccon->Timer = timeout(ReadTimeout, (char *)ccon, ccon->Timeout);
		if (ccon->Timer < 0) {
			ccon->Timer = 0;
			queue->Size   = 0;
			queue->Missed = 0;
			sreset(&ccon->Semaphore);
			cprintf("xmemDrvr:Read: No timers available: Context:%d\n", cnum);
			pseterr(EBUSY); /* No available timers */
			return 0;
		}
	}

	if (swait(&ccon->Semaphore, SEM_SIGABORT)) {
		queue->Size   = 0;
		queue->Missed = 0;
		sreset(&ccon->Semaphore);
		if (ccon->Debug)
			cprintf("xmemDrvr:Read:Error ret from swait:Context:%d\n", cnum);
		pseterr(EINTR); /* We have been signaled */
		return 0;
	}

	if (ccon->Timeout) {
		if (ccon->Timer) {
			CancelTimeout(&ccon->Timer);
		}
		else {
			queue->Size   = 0;
			queue->Missed = 0;
			sreset(&ccon->Semaphore);
			if (ccon->Debug)
				cprintf("xmemDrvr:Read: TimeOut: Context:%d\n", cnum);
			pseterr(ETIME);
			return 0;
		}
	}

	rb = (XmemDrvrReadBuf *)u_buf;
	if (queue->Size) {
		disable(ps);
		{
			iq = --queue->Size;
			*rb = queue->Entries[iq];
		}
		restore(ps);
		return sizeof(XmemDrvrReadBuf);
	}

	queue->Size   = 0;
	queue->Missed = 0;
	sreset(&ccon->Semaphore);
	if (ccon->Debug) cprintf("xmemDrvr:Read: Queue Empty: Context:%d\n", cnum);
	pseterr(EINTR);
	return 0;
}

/**
 * XmemDrvrWrite - Write entry point
 *
 * @param s: working area
 * @param flp: file pointer
 * @param u_buf: user's buffer
 * @param cnt: byte count in buffer
 *
 * This entry point is used to wake up clients connected to a 'software
 * interrupt' called IntrSOFTWAKEUP.
 * For some event it is advisable that certain clients (say clients A) subscribe
 * to its correspondent hardware interrupt, and then when they're called
 * they do some pre-processing before issuing a write() which will wake-up
 * the rest of the clients (say, clients B).
 * Therefore for a certain event we can set a hierarchy.
 *
 * An example of this would be SEGMENT_UPDATE: a client (client A) would
 * subscribe to IntrSEGMENT_UPDATE, and then when it's triggered, he would
 * copy the segment to RAM memory and issue a write() announcing it.
 * Then the clients connected to IntrSOFTWAKEUP (clients B) (NB: they're not
 * subscribed to IntrSEGMENT_UPDATE) would be woken up -- then they could
 * read safely the updated segment from RAM memory.
 *
 * @return number of bytes written on success.
 */
int XmemDrvrWrite(void *s, struct cdcm_file *flp, char *u_buf, int cnt)
{
	XmemDrvrModuleContext *mcon;
	XmemDrvrClientContext *ccon;
	XmemDrvrQueue         *queue;
	XmemDrvrReadBuf        rbf; /* read buffer (for the clients) */
	XmemDrvrWriteBuf      *wbf; /* write buffer */
	XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea*)s;
	int           cnum;
	int           pid;
	int           midx;
	unsigned int  i;
	unsigned long ps;
	unsigned long msk;

	/*
	 * Note: The user buffer does not have to be checked with {r,w}bounds
	 * because the read() and write() system calls have already done this.
	 * {r,w}bounds are only meant to be used inside the ioctl() entry point.
	 * Source: LynxOs man pages.
	 */

	wbf = (XmemDrvrWriteBuf *)u_buf;

	/* Get client context of the writer */
	cnum = minor(flp->dev) - 1;
	ccon = &wa->ClientContexts[cnum];

	if (ccon->InUse != 1) {
		cprintf("xmemDrvr:Write: Bad FileDescriptor: Context:%d Not Open\n", cnum);
		pseterr(EBADF); /* Bad file number */
		return SYSERR;
	}

	/* Get the module's context */
	midx = wbf->Module - 1;
	if (midx < 0 || midx >= Wa->Modules) {
		pseterr(ENXIO); /* No such device or address */
		return SYSERR;
	}
	mcon = &wa->ModuleContexts[midx];

	/*
	 * Block any calls coming from unknown PIDs.
	 * Only the PID that opened the driver is allowed to read.
	 */
	pid = getpid();
	if (pid != ccon->Pid) {
		cprintf("xmemDrvr:Write: Spurious read PID:%d for PID:%d on FD:%d\n",
			(unsigned int)pid, (unsigned int)ccon->Pid, (unsigned int)cnum);
		pseterr(EBADF); /* Bad file number */
		return SYSERR;
	}

	/* Prepare the read buffer for suscribed clients */
	bzero((void *)&rbf, sizeof(XmemDrvrWriteBuf));
	rbf.Module = wbf->Module;
	rbf.NdData[XmemDrvrIntIdxSOFTWAKEUP] = wbf->Data;
	rbf.NodeId[XmemDrvrIntIdxSOFTWAKEUP] = wbf->NodeId;
	rbf.Mask = XmemDrvrIntrSOFTWAKEUP;

	/* then put it in the client's queue */
	for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {

		ccon = &Wa->ClientContexts[i];
		queue = &ccon->Queue;

		msk = mcon->Clients[i];

		if (! (msk & XmemDrvrIntrSOFTWAKEUP))
			continue; /* this client is not connected to IntrSOFTWAKEUP */


		/* the client's connected to IntrSOFTWAKEUP --> fill in his queue */

		disable(ps); /* acquire spinlock */

		if (queue->Size < XmemDrvrQUEUE_SIZE) {
			queue->Entries[queue->Size++] = rbf;
			ssignal(&ccon->Semaphore); /* Wake-up client */
		}
		else {
			queue->Size = XmemDrvrQUEUE_SIZE;
			queue->Missed++;
		}

		restore(ps); /* release spinlock */

	}

	return sizeof(XmemDrvrWriteBuf);
}

/**
 * XmemDrvrSelect - select service subroutine of the driver.
 *        [!!!!!! LINUX NOTE !!!!!!] ---> Will NOT work under Linux!
 *        Third parameter (condition to monitor) is void, i.e.
 *        it's not known. Never rely on it!.
 *
 * @param s: working area
 * @param flp: file pointer
 * @param wch: Read/Write direction
 * @param ffs: Selection structures
 *
 * Although there's some code below, this entry point has not been reviewed and
 * should not be used.
 *
 * @return OK on success
 */
int XmemDrvrSelect(void *s, struct cdcm_file *flp, int wch, struct sel *ffs)
{
	XmemDrvrClientContext * ccon;
	XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea*)s;
	int cnum;

	cnum = minor(flp->dev) - 1;
	ccon = &wa->ClientContexts[cnum];

	if (wch == SREAD) {
		ffs->iosem = (int *)&ccon->Semaphore; /* Watch out here I hope   */
		return OK;                               /* the system dosn't swait */
	}                                           /* the read does it too !! */
	pseterr(EACCES);			      /* Permission denied */
	return SYSERR;
}


/**
 * XmemDrvrInstall - This routine is called at driver install with a
 * parameter pointer, addressing a table initialised with the info table
 *
 * @param info: device info table
 *
 * @return char pointer to 'wa' (working area) on success.
 */
char* XmemDrvrInstall(void *info)
{
	XmemDrvrWorkingArea *wa;
	struct drm_node_s *handle;
	XmemDrvrModuleContext *mcon;
	unsigned int vadr;
	unsigned long ivec;
	unsigned long bigend;
	int midx, mid, cc;

	/* Allocate the driver working area. */
	wa = (XmemDrvrWorkingArea *)sysbrk(sizeof(XmemDrvrWorkingArea));
	if (!wa) {
		cprintf("xmemDrvr: NOT ENOUGH MEMORY(WorkingArea)\n");
		pseterr(ENOMEM);
		return (char *)SYSERR;
	}
	bzero((void *)wa, sizeof(XmemDrvrWorkingArea)); /* Clear working area */
	Wa = wa; /* Global working area pointer */

	Wa->DmaThreshold = XmemDrvrDEFAULT_DMA_THRESHOLD;

	SetEndian(); /* Set Big/Little endian flag in driver working area */

	for (midx = 0; midx < XmemDrvrMODULE_CONTEXTS; midx++) {
		cc = drm_get_handle(PCI_BUSLAYER, VMIC_VENDOR_ID, VMIC_DEVICE_ID, &handle);
		if (cc) {
			if (midx == 0) {
				sysfree((void *) wa, sizeof(XmemDrvrWorkingArea));
				Wa = NULL;
				cprintf("xmemDrvr: No VMIC card found: No hardware: Fatal\n");
				pseterr(ENODEV);
				return (char *) SYSERR;
			} else
				break; /* no more devices */
		}

		mcon = &wa->ModuleContexts[midx];
		if (mcon->InUse == 0) {
			drm_device_read(handle, PCI_RESID_DEVNO, 0, 0, &mid);
			mcon->PciSlot     = mid;
			mcon->ModuleIndex = midx;
			mcon->Handle      = handle;
			mcon->InUse       = 1;

			/* Allocate space for the temporary buffer mcon->Tempbuf.
			 * This buffer is used for copying up to PAGESIZE bytes of data from user
			 * space, and is also used by FlushSegments() as a cache.
			 * If the user wants to transfer more than PAGESIZE bytes, DMA is used.
			 * This means the DMA threshold mustn't be set beyond PAGESIZE.
			 */
			mcon->Tempbuf = (void *)sysbrk(PAGESIZE);
			if (!mcon->Tempbuf) {
				cprintf("xmemDrvr: NOT ENOUGH MEMORY(mod[%d]->Tempbuf)\n", midx);
				pseterr(ENOMEM);
				return ((char *)SYSERR);
			}

			/* initialise mutexes */
			sreset( &mcon->BusySemaphore);
			ssignal(&mcon->BusySemaphore);

			sreset( &mcon->TempbufSemaphore);
			ssignal(&mcon->TempbufSemaphore);

			/* initialise countdown semaphores */
			sreset( &mcon->RdDmaSemaphore);
			sreset( &mcon->WrDmaSemaphore);

			/* register ISR */
			cc = drm_register_isr(handle, IntrHandler, (void *)mcon);
			if (cc == SYSERR) {
				cprintf("xmemDrvr: Can't register ISR for timing module: %d\n",midx+1);
				return((char *)SYSERR);
			}
			else {

				ivec = 0;
				DrmConfigReadWrite(mcon, PlxConfigPCIILR, &ivec, 1, XmemDrvrREAD);
				mcon->IVector = 0xFF & ivec;
#ifdef __powerpc__
				iointunmask(mcon->IVector);
#endif

			}
		}

		vadr = (int)NULL;
		cc = drm_map_resource(handle, PCI_RESID_BAR2, &vadr);
		if (cc || !vadr) {
			cprintf("xmemDrvr: Can't map memBAR2 for module: %d cc=%d, vadr=0x%x\n",
				midx + 1, cc, vadr);
			return((char *)SYSERR);
		}
		mcon->Map = (VmicRfmMap *)vadr;

		vadr = (int)NULL;
		cc = drm_map_resource(handle, PCI_RESID_BAR3, &vadr);
		if (cc || !vadr) {
			cprintf("xmemDrvr: Can't map memory (BAR3) for module: %d\n",midx + 1);
			return((char *)SYSERR);
		}
		mcon->SDRam = (unsigned char *) vadr;

		vadr = (int) NULL;
		cc = drm_map_resource(handle, PCI_RESID_BAR0, &vadr);
		if (cc || !vadr) {
			cprintf("xmemDrvr: Can't map plx9656 (BAR0) local configuration: %d\n",
				midx + 1);
			return((char *)SYSERR);
		}
		mcon->Local = (PlxLocalMap *)vadr;


		/*
		 * Set up the BIGEND  local configuration register to do appropriate
		 * endian mapping
		 */
		if (Wa->Endian == XmemDrvrEndianBIG) {
			bigend = 0xC0; /* Only set big endian for DMA Channels: BIGEND=11000000 */
			DrmLocalReadWrite(mcon, PlxLocalBIGEND, &bigend, 1, XmemDrvrWRITE);
		}
		wa->Modules = midx + 1;
		Reset(mcon); /* Soft reset and initialize module */
	}
	return (char *)wa;
}

/**
 * XmemDrvrUninstall - routine called to uninstall the driver
 *
 * @param s: working area
 *
 * @return OK on success
 */
int XmemDrvrUninstall(void *s)
{
	XmemDrvrClientContext *ccon;
	XmemDrvrWorkingArea *wa = (XmemDrvrWorkingArea*)s;
	XmemDrvrModuleContext *mcon;
	int i;

	for (i = 0; i < Wa->Modules; i++) {
		mcon = &wa->ModuleContexts[i];
		if (mcon->InUse) {
			if (mcon->Local) {
				drm_unmap_resource(mcon->Handle, PCI_RESID_BAR0);
				mcon->LocalOpen = 0;
				mcon->Local = NULL;
			}
			if (mcon->Map) {
				drm_unmap_resource(mcon->Handle, PCI_RESID_BAR2);
				mcon->Map = NULL;
			}
			if (mcon->SDRam) {
				drm_unmap_resource(mcon->Handle, PCI_RESID_BAR3);
				mcon->SDRam = NULL;
			}
			drm_unregister_isr(mcon->Handle);
			drm_free_handle(mcon->Handle);
			bzero((void *) mcon, sizeof(XmemDrvrModuleContext));
		}
	}

	for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
		ccon = &wa->ClientContexts[i];
		if (ccon->InUse) {
			if (ccon->Timer) {
				CancelTimeout(&ccon->Timer);
			}
			ssignal(&ccon->Semaphore); /* Wakeup client */
			ccon->InUse = 0;
		}
	}

	sysfree((void *)wa, sizeof(XmemDrvrWorkingArea));
	Wa = NULL;
	cprintf("xmemDrvr: Driver: Uninstalled\n");
	return OK;
}

static int rwbounds(int nr, int arg)
{
	int r = rbounds(arg);
	int w = wbounds(arg);

	switch (_IOC_DIR(nr)) {
	case _IOC_READ:
		if (w < _IOC_SIZE(nr))
			return -1;
		break;
	case _IOC_WRITE:
		if (r < _IOC_SIZE(nr))
			return -1;
		break;
	case (_IOC_READ | _IOC_WRITE):
		if (w < _IOC_SIZE(nr) || r < _IOC_SIZE(nr))
			return -1;
		break;
	case _IOC_NONE:
	default:
		break;
	}
	return 0;
}

/*
 * FIXME: it would be _much_ cleaner to have the IOCTL entry point like a switch
 * that instead of having everything inside, calls for each case a different
 * function. It's just a huge waste the way it's done now.
 */


/**
 * XmemDrvrIoctl - routine called to perform ioctl function
 *
 * @param s: working area
 * @param flp: file pointer
 * @param cm: IOCTL command
 * @param arg: data for the IOCTL
 *
 * @return OK on success
 */
int XmemDrvrIoctl(void *s, struct cdcm_file * flp, int cm, char * arg)
{
	XmemDrvrModuleContext           *mcon;   /* Module context */
	XmemDrvrClientContext           *ccon;   /* Client context */
	XmemDrvrConnection              *conx;
	XmemDrvrClientList              *cls;
	XmemDrvrClientConnections       *ccn;
	XmemDrvrRawIoBlock              *riob;
	XmemDrvrVersion                 *ver;
	XmemDrvrModuleDescriptor        *mdesc;
	XmemDrvrSendBuf                 *sbuf;
	XmemDrvrRamAddress              *radr;
	XmemDrvrSegTable                *stab;
	XmemDrvrSegIoDesc               *siod;
	void				*argp = (void *)arg;
	int i, j, size;
	int cnum;                 /* Client number */
	int32_t lav, *lap;           /* Long Value pointed to by Arg */
	unsigned short sav;       /* Short argument and for Jtag IO */
	int rcnt, wcnt;           /* Readable, Writable byte counts at arg address */
	unsigned long ps;
	int err;
	unsigned long lptr[50];
	XmemDrvrConnection tempcon[XmemDrvrMODULE_CONTEXTS];
	void *temptr;

	/*
	 * Check argument contains a valid address for reading or writing.
	 * We can not allow bus errors to occur inside the driver due to
	 * the caller providing a garbage address in "arg". So if arg is
	 * not null set "rcnt" and "wcnt" to contain the byte counts which
	 * can be read or written to without error.
	 */
	if (arg != NULL) {

		rcnt = rbounds((int)arg); /* Number of readable bytes without error */
		wcnt = wbounds((int)arg); /* Number of writable bytes without error */

		if (rwbounds(cm, (int)arg)) {
			XMEM_WARN("IOCTL: Illegal Non-Null Argument. "
				"rcnt=%d/%d, wcnt=%d/%d, cmd=%d", rcnt,
				_IOC_SIZE(cm), wcnt, _IOC_SIZE(cm), cm);
			pseterr(EINVAL);
			return SYSERR;
		}

		lav = *((uint32_t *)arg); /* Long argument value */
		lap =   (uint32_t *)arg ; /* Long argument pointer */

	}
	else {

		rcnt = 0;
		wcnt = 0;
		lav = 0;
		lap = NULL; /* Null arg = zero read/write counts */

	}

	sav = lav; /* Short argument value */

	/*
	 * We allow one client per minor device, we use the minor device
	 * number as an index into the client contexts array.
	 */

	cnum = minor(flp->dev) - 1;

	if ( cnum < 0 || cnum >= XmemDrvrCLIENT_CONTEXTS) {
		pseterr(ENODEV); /* No such device */
		return SYSERR;
	}

	/* We can't control a file which is not open. */

	ccon = &Wa->ClientContexts[cnum];
	if (ccon->InUse == 0) {
		cprintf("xmemDrvrIoctl: DEVICE %2d IS NOT OPEN\n",cnum + 1);
		pseterr(EBADF); /* Bad file number */
		return SYSERR;
	}

	/*
	 * Block any calls coming from unknown PIDs.
	 * Only the PID that opened the driver is allowed to call IOCTLs
	 */

	if (getpid() != ccon->Pid) {
		cprintf("xmemDrvrIoctl: Spurious IOCTL:%d by PID:%d for PID:%d on FD:%d\n",
			cm, (int)getpid(), (int)ccon->Pid, (int)cnum);
		pseterr(EBADF);           /* Bad file number */
		return SYSERR;
	}

	/* Set up some useful module pointers */
	mcon = &Wa->ModuleContexts[ccon->ModuleIndex]; /* Default module selected */

	TraceIoctl(cm,arg,ccon);   /* Print debug message */



	/*
	 * Decode callers command and do it.
	 */

	switch (cm) {

	case XmemDrvrSET_SW_DEBUG: /* Set driver debug mode */
		ccon->Debug = (XmemDrvrDebug) lav;
		return OK;

	case XmemDrvrGET_SW_DEBUG:
		*lap = (long)ccon->Debug;
		return OK;

	case XmemDrvrGET_VERSION: /* Get version date */
		ver = argp;
		return GetVersion(mcon,ver);

	case XmemDrvrSET_TIMEOUT: /* Set the read timeout value */
		if (lav < XmemDrvrMINIMUM_TIMEOUT && lav) {
			lav = XmemDrvrMINIMUM_TIMEOUT;
		}
		ccon->Timeout = lav; /* Note that if (!lav), this is properly handled */
		if (ccon->Debug) {
			if (lav)
				cprintf("xmemDrvr: TicksPerSec:%d Timeout:%d\n", (int)tickspersec,
					(int)lav);
			else
				cprintf("xmemDrvr: Disabled Client's Timeout.\n");
		}
		return OK;

	case XmemDrvrGET_TIMEOUT: /* Get the read timeout value */
		*lap = ccon->Timeout;
		return OK;

	case XmemDrvrSET_QUEUE_FLAG: /* Set queueing capabilities on/off */
		if (lav)
			ccon->Queue.QueueOff = 1;
		else
			ccon->Queue.QueueOff = 0;

		disable(ps);
		{
			ccon->Queue.Size   = 0;
			ccon->Queue.Missed = 0;
			sreset(&ccon->Semaphore);
		}
		restore(ps);
		return OK;

	case XmemDrvrGET_QUEUE_FLAG: /* 1-->Q_off, 0-->Q_on */
		*lap = ccon->Queue.QueueOff;
		return OK;

	case XmemDrvrGET_QUEUE_SIZE: /* Number of events on queue */
		*lap = ccon->Queue.Size;
		return OK;

	case XmemDrvrGET_QUEUE_OVERFLOW: /* Number of missed events */
		disable(ps);
		*lap = ccon->Queue.Missed;
		ccon->Queue.Missed = 0;
		restore(ps);

		return OK;

	case XmemDrvrGET_MODULE_DESCRIPTOR:
		mdesc = argp;
		mdesc->Module  = mcon->ModuleIndex + 1;
		mdesc->PciSlot = mcon->PciSlot;
		mdesc->NodeId  = mcon->NodeId;
		mdesc->Map     = (unsigned char *)mcon->Map;
		mdesc->SDRam   = mcon->SDRam;

		return OK;

	case XmemDrvrSET_MODULE: /* Select the module to work with */
		if (lav >= 1 &&  lav <= Wa->Modules) {
			ccon->ModuleIndex = lav - 1;
			return OK;
		}
		pseterr(ENODEV);
		return SYSERR;

	case XmemDrvrGET_MODULE: /* Which module am I working with */
		*lap = ccon->ModuleIndex + 1;
		return OK;

	case XmemDrvrGET_MODULE_COUNT: /* The number of installed modules */
		*lap = Wa->Modules;
		return OK;

	case XmemDrvrSET_MODULE_BY_SLOT: /* Select the module to work with by ID */
		for (i = 0; i < Wa->Modules; i++) {
			mcon = &Wa->ModuleContexts[i];
			if (mcon->PciSlot == lav) {
				ccon->ModuleIndex = i;
				return OK;
			}
		}
		pseterr(ENODEV);
		return SYSERR;

	case XmemDrvrGET_MODULE_SLOT: /* Get the ID of the selected module */
		*lap = mcon->PciSlot;
		return OK;

	case XmemDrvrRESET: /* Reset the module, re-establish connections */
		return Reset(mcon);

	case XmemDrvrSET_COMMAND: /* Send a command to the VMIC module */
		GetSetStatus(mcon, (XmemDrvrScr)lav, XmemDrvrWRITE);
		return OK;

	case XmemDrvrGET_STATUS: /* Read module status */
		*lap = (unsigned long)GetSetStatus(mcon, mcon->Command, XmemDrvrREAD);
		return OK;

	case XmemDrvrGET_NODES:
		*lap = Wa->Nodes;
		return OK;

	case XmemDrvrGET_CLIENT_LIST: /* Get the list of driver clients */
		cls = argp;
		bzero((void *)cls, sizeof(XmemDrvrClientList));

		for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
			ccon = &Wa->ClientContexts[i];
			if (ccon->InUse)
				cls->Pid[cls->Size++] = ccon->Pid;
		}

		return OK;

	case XmemDrvrCONNECT: /* Connect to an object interrupt */
		conx = argp;
		return Connect(conx,ccon);

	case XmemDrvrDISCONNECT: /* Disconnect from an object interrupt */
		conx = argp;

		if (conx->Mask)
			return DisConnect(conx,ccon);
		else {
			DisConnectAll(ccon);
			return OK;
		}

	case XmemDrvrGET_CLIENT_CONNECTIONS:
		/* Get the list of a client connections on module */
		ccn = argp;
		temptr = ccn->Connections; /* store the address */

		ccn->Size = 0;
		size = 0;
		for (i = 0; i < XmemDrvrCLIENT_CONTEXTS; i++) {
			ccon = &Wa->ClientContexts[i];
			if (ccon->InUse && ccon->Pid == ccn->Pid) {
				for (j = 0; j < Wa->Modules; j++) {
					mcon = &Wa->ModuleContexts[j];
					if (mcon->Clients[i]) {
						tempcon[size].Module = j + 1;
						tempcon[size].Mask   = mcon->Clients[i];
						ccn->Size = ++size;
					}
				}
			}
		}
		if (!cdcm_copy_to_user(temptr, tempcon, ccn->Size))
			return OK;
		else pseterr(EFAULT);
		return SYSERR;

	case XmemDrvrSEND_INTERRUPT: /* Send an interrupt to other nodes */
		sbuf = argp;
		return SendInterrupt(sbuf);

	case XmemDrvrGET_XMEM_ADDRESS:
		/* Get physical address of reflective memory SDRAM */
		radr = argp;
		i = radr->Module - 1;
		if (i >= 0 && i < Wa->Modules) {
			mcon = &Wa->ModuleContexts[i];
			radr->Address = mcon->SDRam;
			return OK;
		}
		pseterr(ENODEV);
		return SYSERR;

	case XmemDrvrSET_SEGMENT_TABLE:
		/* Set the list of all defined xmem memory segments */
		stab = argp;
		LongCopy((unsigned long *)&Wa->SegTable, (unsigned long *)stab,
			sizeof(XmemDrvrSegTable));
		return OK;

	case XmemDrvrGET_SEGMENT_TABLE: /* List of all defined xmem memory segments */
		stab = argp;
		LongCopy((unsigned long *)stab, (unsigned long *)&Wa->SegTable,
			sizeof(XmemDrvrSegTable));
		return OK;

	case XmemDrvrREAD_SEGMENT:   /* Copy from xmem segment to local memory */
		siod = argp;
		if (siod->UserArray == NULL) {
			cprintf("xmemDrvr: UserArray is NULL!\n");
			pseterr(EINVAL);
			return SYSERR;
		}

		if (ccon->Debug) {
			cprintf("xmemDrvr: Rd: Mod:%d Seg:0x%x Off:%d Sze:%d Uad:0x%x Udf:%d\n",
				(int)siod->Module, (int)siod->Id, (int)siod->Offset,
				(int)siod->Size, (int)siod->UserArray, (int)siod->UpdateFlg);
		}

		if (siod->Size >= Wa->DmaThreshold) { /* use DMA engines */

			return SegmentCopy(siod, XmemDrvrREAD, ccon, mcon);

		}
		else { /* CPU-driven copy */

			temptr = siod->UserArray; /* Save the user space address */


			swait(&mcon->TempbufSemaphore, SEM_SIGIGNORE); /* acquire mutex */

			err = cdcm_copy_from_user(mcon->Tempbuf, siod->UserArray, siod->Size);
			if (err) {
				cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
				ssignal(&mcon->TempbufSemaphore);
				pseterr(EFAULT);
				return (SYSERR);
			}

			siod->UserArray = mcon->Tempbuf; /* fill with the kernel address */
			err = SegmentCopy(siod, XmemDrvrREAD, ccon, mcon);
			siod->UserArray = temptr; /* Restore the user space address */

			if (err != OK) {
				ssignal(&mcon->TempbufSemaphore); /* release mutex */
				return err;
			}

			err = cdcm_copy_to_user(siod->UserArray, mcon->Tempbuf, siod->Size);
			ssignal(&mcon->TempbufSemaphore); /* release mutex */


			if (err) {
				cprintf("xmemDrvr: Copy to user failed. Returned error: %d.\n", err);
				pseterr(EFAULT);
				return (SYSERR);
			}

			return OK;

		}
		pseterr(EINVAL); /* will never get to this point, anyway */
		return SYSERR;


	case XmemDrvrWRITE_SEGMENT:  /* Update the segment with new contents */
		siod = argp;
		if (siod->UserArray == NULL) {
			cprintf("xmemDrvr: UserArray is NULL!\n");
			pseterr(EINVAL);
			return SYSERR;
		}

		if (ccon->Debug) {
			cprintf("xmemDrvr: Wr: Mod:%d Seg:0x%x Off:%d Sze:%d Uad:0x%x Udf:%d\n",
				(int)siod->Module, (int)siod->Id, (int)siod->Offset,
				(int)siod->Size, (int)siod->UserArray, (int)siod->UpdateFlg);
		}

		if (siod->Size >= Wa->DmaThreshold) { /* use DMA engines */

			err = SegmentCopy(siod, XmemDrvrWRITE, ccon, mcon);

			return err;

		}
		else { /* CPU-driven copy */

			temptr = siod->UserArray; /* Save the user space address */


			swait(&mcon->TempbufSemaphore, SEM_SIGIGNORE); /* acquire mutex */

			err = cdcm_copy_from_user(mcon->Tempbuf, siod->UserArray, siod->Size);
			if (err) {
				cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
				ssignal(&mcon->TempbufSemaphore);
				pseterr(EFAULT);
				return (SYSERR);
			}

			siod->UserArray = mcon->Tempbuf; /* fill with the kernel address */
			err = SegmentCopy(siod, XmemDrvrWRITE, ccon, mcon);
			siod->UserArray = temptr; /* Restore the user space address */

			ssignal(&mcon->TempbufSemaphore); /* release mutex */
			/* Note that here we don't need to copy_to_user() */

			return err; /* propagate OK or SYSERR from SegmentCopy() */

		}
		pseterr(EINVAL); /* will never get to this point, anyway */
		return SYSERR;

	case XmemDrvrCHECK_SEGMENT:
		/* Check to see if a segment has been updated since last read */
		disable(ps);
		*lap = ccon->UpdatedSegments;
		ccon->UpdatedSegments = 0;
		restore(ps);
		return OK;

	case XmemDrvrFLUSH_SEGMENTS:
		/* Flush segments out to other nodes after PendingInit */
		return FlushSegments(mcon, *lap);

	case XmemDrvrCONFIG_OPEN: /* Open the PLX9656 configuration */
		mcon->ConfigOpen = 1;
		return OK;

	case XmemDrvrLOCAL_OPEN: /* Open the PLX9656 local configuration */
		mcon->LocalOpen = 1;
		return OK;

	case XmemDrvrRFM_OPEN: /* Open the VMIC RFM configuration */
		mcon->RfmOpen = 1;
		return OK;

	case XmemDrvrRAW_OPEN: /* Open the VMIC SDRAM */
		mcon->RawOpen = 1;
		return OK;

	case XmemDrvrCONFIG_READ: /* Read the PLX9656 configuration registers */
		if (mcon->ConfigOpen) {

			riob = argp;
			if (riob->UserArray == NULL) {
				cprintf("xmemDrvr: UserArray is NULL!\n");
				pseterr(EINVAL);
				return SYSERR;
			}

			temptr = riob->UserArray; /* Save the user space address */
			err = cdcm_copy_from_user(lptr, riob->UserArray,
						sizeof(long) * riob->Items);
			if (err) {
				cprintf("xmemDrvr: Copy from user failed. Returned error:%d.\n", err);
				pseterr(EFAULT);
				return SYSERR;
			}

			riob->UserArray = lptr; /* fill with the kernel address */
			err = DrmConfigReadWrite(mcon, riob->Offset, riob->UserArray,
						riob->Size, XmemDrvrREAD);
			riob->UserArray = temptr; /* Restore user space address */

			if (err != OK) {
				pseterr(EIO);
				return SYSERR;
			}

			err = cdcm_copy_to_user(riob->UserArray, lptr,
						sizeof(long) * riob->Items);
			if (err) {
				pseterr(EFAULT);
				return SYSERR;
			}
			return OK;

		}
		pseterr(EACCES);
		return SYSERR;

	case XmemDrvrLOCAL_READ:  /* Read the PLX9656 local configuration registers */
		if (mcon->LocalOpen) {

			riob = argp;
			if (riob->UserArray == NULL) {
				cprintf("xmemDrvr: UserArray is NULL!\n");
				pseterr(EINVAL);
				return SYSERR;
			}

			temptr = riob->UserArray; /* Save the user space address */

			err = cdcm_copy_from_user(lptr, riob->UserArray,
						sizeof(long) * riob->Items);
			if (err) {
				cprintf("xmemDrvr: Copy from user failed. returned error:%d.\n", err);
				pseterr(EFAULT);
				return SYSERR;
			}

			riob->UserArray = lptr; /* fill with the kernel address */
			err = DrmLocalReadWrite(mcon, riob->Offset, riob->UserArray,
						riob->Size, XmemDrvrREAD);
			riob->UserArray = temptr; /* Restore user space address */
			if (err != OK) {
				pseterr(EIO);
				return SYSERR;
			}

			err = cdcm_copy_to_user(riob->UserArray, lptr,
						sizeof(long) * riob->Items);
			if (err) {
				pseterr(EFAULT);
				return SYSERR;
			}
			return OK;

		}
		pseterr(EACCES);
		return SYSERR;

	case XmemDrvrRFM_READ: /* Read from VMIC 5565 FPGA Register block RFM */
		riob = argp;
		if (riob->UserArray == NULL) {
			cprintf("xmemDrvr: UserArray is NULL!\n");
			pseterr(EINVAL);
			return SYSERR;
		}


		temptr = riob->UserArray; /* Save the user space address */
		err = cdcm_copy_from_user(lptr, riob->UserArray,
					sizeof(long) * riob->Items);
		if (err) {
			cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
			pseterr(EFAULT);
			return SYSERR;
		}

		riob->UserArray = lptr; /* fill with the kernel address */
		err = RfmIo(mcon, riob, XmemDrvrREAD);
		riob->UserArray = temptr; /* Restore user space address */

		if (err != OK) {
			pseterr(EIO);
			return SYSERR;
		}

		err = cdcm_copy_to_user(riob->UserArray, lptr,
					sizeof(long) * riob->Items);
		if (err) {
			pseterr(EFAULT);
			return SYSERR;
		}
		return OK;

	case XmemDrvrRAW_READ: /* Read from VMIC 5565 SDRAM */
		riob = argp;
		if (riob->UserArray == NULL) {
			cprintf("xmemDrvr: UserArray is NULL!\n");
			pseterr(EINVAL);
			return SYSERR;
		}

		temptr = riob->UserArray; /* Store the user space address */
		err = cdcm_copy_from_user(lptr, riob->UserArray,
					sizeof(long) * riob->Items);
		if (err) {
			cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
			pseterr(EFAULT);
			return SYSERR;

		}

		riob->UserArray = lptr; /* fill with the kernel address */
		err = RawIo(mcon, riob, XmemDrvrREAD);
		riob->UserArray = temptr; /* Restore user space address */

		if (err != OK) {
			pseterr(EIO);
			return SYSERR;
		}

		err = cdcm_copy_to_user(riob->UserArray, lptr,
					sizeof(long) * riob->Items);
		if (err) {
			pseterr(EFAULT);
			return SYSERR;
		}

		return OK;

	case XmemDrvrCONFIG_WRITE:
		/* Write to PLX9656 configuration registers (Experts only) */
		if (mcon->ConfigOpen) {

			riob = argp;
			if (riob->UserArray == NULL) {
				cprintf("xmemDrvr: UserArray is NULL!\n");
				pseterr(EINVAL);
				return SYSERR;
			}

			temptr = riob->UserArray; /* Store the user space address */
			err = cdcm_copy_from_user(lptr, riob->UserArray,
						sizeof(long) * riob->Items);
			if (err) {
				cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
				pseterr(EFAULT);
				return SYSERR;
			}

			riob->UserArray = lptr; /* fill with the kernel address */
			err = DrmConfigReadWrite(mcon, riob->Offset, riob->UserArray,
						riob->Size, XmemDrvrWRITE);
			riob->UserArray = temptr; /* Restore user space address */

			if (err != OK) {
				pseterr(EIO);
				return SYSERR;
			}

			/* No copy_to_user here */
			return OK;

		}
		pseterr(EACCES);
		return SYSERR;

	case XmemDrvrLOCAL_WRITE:
		/* Write the PLX9656 local configuration registers (Experts only) */
		if (mcon->LocalOpen) {

			riob = argp;
			if (riob->UserArray == NULL) {
				cprintf("xmemDrvr: UserArray is NULL!\n");
				pseterr(EINVAL);
				return SYSERR;
			}
			temptr = riob->UserArray; /* Store the user space address */
			err = cdcm_copy_from_user(lptr, riob->UserArray,
						sizeof(long) * riob->Items);
			if (err) {
				cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
				pseterr(EFAULT);
				return SYSERR;
			}


			riob->UserArray = lptr; /* fill with the kernel address */
			err = DrmConfigReadWrite(mcon, riob->Offset, riob->UserArray,
						riob->Size, XmemDrvrWRITE);
			riob->UserArray = temptr; /* Restore user space address */

			if (err != OK) {
				pseterr(EIO);
				return SYSERR;
			}

			/* No copy_to_user here */
			return OK;

		}
		pseterr(EACCES);
		return SYSERR;

	case XmemDrvrRFM_WRITE: /* Write to VMIC 5565 FPGA Register block RFM */
		riob = argp;
		if (riob->UserArray == NULL) {
			cprintf("xmemDrvr: UserArray is NULL!\n");
			pseterr(EINVAL);
			return SYSERR;
		}

		temptr = riob->UserArray; /* Store the user space address */
		err = cdcm_copy_from_user(lptr, riob->UserArray,
					sizeof(long) * riob->Items);

		if (err) {
			cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
			pseterr(EFAULT);
			return SYSERR;
		}

		riob->UserArray = lptr; /* fill with the kernel address */
		err = RfmIo(mcon, riob, XmemDrvrWRITE);
		riob->UserArray = temptr; /* Restore user space address */

		if (err != OK) {
			pseterr(EIO);
			return SYSERR;
		}

		/* No copy_to_user here */
		return OK;

	case XmemDrvrRAW_WRITE: /* Write to VMIC 5565 SDRAM */
		riob = argp;
		if (riob->UserArray == NULL) {
			cprintf("xmemDrvr: UserArray is NULL!\n");
			pseterr(EINVAL);
			return SYSERR;
		}

		temptr = riob->UserArray; /* Store the user space address */
		err = cdcm_copy_from_user(lptr, riob->UserArray,
					sizeof(long) * riob->Items);
		if (err) {
			cprintf("xmemDrvr: Copy from user failed. returned error: %d.\n", err);
			pseterr(EFAULT);
			return SYSERR;
		}

		riob->UserArray = lptr; /* fill with the kernel address */
		err = RawIo(mcon, riob, XmemDrvrWRITE);
		riob->UserArray = temptr; /* Restore user space address */

		if (err != OK) {
			pseterr(EIO);
			return SYSERR;
		}

		err = cdcm_copy_to_user(riob->UserArray, lptr,
					sizeof(long) * riob->Items);
		if (err) {
			pseterr(EFAULT);
			return SYSERR;
		}

		return OK;

	case XmemDrvrSET_DMA_THRESHOLD: /* Set Drivers DMA threshold */
		if (lav < PAGESIZE)
			Wa->DmaThreshold = lav;
		else
			cprintf("xmemDrvr: DMA_THRESHOLD should be less than PAGESIZE=%ld\n",
				PAGESIZE);
		return OK;

	case XmemDrvrGET_DMA_THRESHOLD: /* Get Drivers DMA threshold */
		*lap = Wa->DmaThreshold;
		return OK;

	case XmemDrvrCONFIG_CLOSE: /* Close the PLX9656 configuration */
		mcon->ConfigOpen = 0;
		return OK;

	case XmemDrvrLOCAL_CLOSE: /* Close the PLX9656 local configuration */
		mcon->LocalOpen = 0;
		return OK;

	case XmemDrvrRFM_CLOSE: /* Close VMIC 5565 FPGA Register block RFM */
		mcon->RfmOpen = 0;
		return OK;

	case XmemDrvrRAW_CLOSE: /* Close VMIC 5565 SDRAM */
		mcon->RfmOpen = 0;
		return OK;

	default: break;
	}

	pseterr(ENOTTY);
	return SYSERR;
}


/*! CDCM-like driver's entry points
 */
struct dldd entry_points = {
	XmemDrvrOpen,         /* open      */
	XmemDrvrClose,        /* close     */
	XmemDrvrRead,         /* read      */
	XmemDrvrWrite,        /* write     */
	XmemDrvrSelect,       /* select    */
	XmemDrvrIoctl,        /* ioctl     */
	XmemDrvrInstall,      /* install   */
	XmemDrvrUninstall,    /* uninstall */
	NULL			/* memory-mapped devices access */
};
