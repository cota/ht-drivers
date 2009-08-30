/**
 * @file mtt_priv.h
 *
 * @brief private header file for the MTT driver
 *
 * @author Copyright (c) 2006-2008 CERN: Julian Lewis <julian.lewis@cern.ch>
 * @author Copyright (c) 2009 CERN; Emilio G. Cota <cota@braap.org>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */

#ifndef _MTT_PRIV_H_
#define _MTT_PRIV_H_

#include <cdcm/cdcmBoth.h>

/**
 * struct mtt_task - keep the state of an MTT task
 * @lock: protect access to the struct
 * @LoadAddress: Where it is loaded in memory
 * @InstructionCount: size in instructions
 * @PcStart: Start PC value
 * @PcOffset: Relocator PC offset
 * @Pc: PC value
 * @Name: Task name
 */
struct mtt_task {
	struct cdcm_mutex	lock;
	unsigned long		LoadAddress;
	unsigned long		InstructionCount;
	unsigned long		PcStart;
	unsigned long		PcOffset;
	unsigned long		Pc;
	char			Name[MttDrvrNameSIZE];
};

/**
 * struct udata - module's private data, to be hooked onto the module context
 * @EnabledOutput: set to 1 when output enabled
 * @CableId: cable id
 * @External1KHZ: set to 1 when an external 1KHz signal is connected
 * @UtcSending: set to 1 to enable UTC sending
 * @OutputDelay: output delay
 * @SyncPeriod: sync period
 * @FlashOpen: ongoing JTAG operation
 * @Tasks: MTT tasks
 * @iolock: spinlock to protect I/O accesses
 * @iomap: kernel address of the mapped VME space
 * @jtag: kernel address of the mapped JTAG space
 */
struct udata {
	int			EnabledOutput;
	int			CableId;
	int			External1KHZ;
	int			UtcSending;
	int			OutputDelay;
	int			SyncPeriod;
	int			BusError;
	int			FlashOpen;
	struct mtt_task		Tasks[MttDrvrTASKS];
	cdcm_spinlock_t		iolock;
	void			*iomap;
	void			*jtag;
};

#endif /* _MTT_PRIV_H_ */
