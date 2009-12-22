/**
 * @file skeluser.h
 *
 * @brief Skel definitions for the driver's working area
 *
 * Copyright (C) 2009 CERN
 * @author Name Surname	<email@cern.ch>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _SKEL_USER_H_
#define _SKEL_USER_H_

#define SkelDrvrCLIENT_CONTEXTS 16
#define SkelDrvrMODULE_CONTEXTS 2

/*
 * How much space to reserve in the module context to keep copies of
 * registers. These are useful in emulation and to restore settings
 * after a hardware reset.
 */
#define SkelDrvrREGISTERS 256

/*
 * How many interrupt sources you want and the size of
 * the queue you need to store them for each client.
 */
#define SkelDrvrINTERRUPTS 32
#define SkelDrvrQUEUE_SIZE 32
#define SkelDrvrCONNECTIONS 16

/* Default timeout values, in 10ms ticks */
#define SkelDrvrDEFAULT_CLIENT_TIMEOUT 2000
#define SkelDrvrDEFAULT_MODULE_TIMEOUT 10

#endif /* _SKEL_USER_H_ */
