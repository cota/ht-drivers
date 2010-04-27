/**
 * @file skeluser.h
 *
 * @brief Skel definitions for driver working area
 *
 * Copyright (c) 2009 CERN
 * @author Emilio G. Cota <cota@braap.org>
 *
 * @section license_sec License
 * Released under the GPL v2. (and only v2, not any later version)
 */
#ifndef _CVORG_SKEL_USER_H_
#define _CVORG_SKEL_USER_H_

#define SkelDrvrCLIENT_CONTEXTS 16
#define SkelDrvrMODULE_CONTEXTS 8

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
#define SkelDrvrINTERRUPTS 2
#define SkelDrvrQUEUE_SIZE 8
#define SkelDrvrCONNECTIONS 16

/* Default timeout values, in 10ms ticks */
#define SkelDrvrDEFAULT_CLIENT_TIMEOUT 2000
#define SkelDrvrDEFAULT_MODULE_TIMEOUT 10

#endif /* _CVORG_SKEL_USER_H_ */
