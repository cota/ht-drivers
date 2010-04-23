/**
 * @file cdcmMem.h
 *
 * @brief All CDCM memory handling definitions are located here.
 *
 * @author Georgievskiy Yury, Alain Gagnaire. CERN AB/CO.
 *
 * @date Created on 17/02/2007
 *
 * Many thanks to Julian Lewis and Nicolas de Metz-Noblat.
 *
 * @version $Id: cdcmMem.h,v 1.4 2009/01/09 10:26:03 ygeorgie Exp $
 */
#ifndef _CDCM_MEM_H_INCLUDE_
#define _CDCM_MEM_H_INCLUDE_

/* The size and flags of a buffer are prepended to it */
struct cdcm_mem_header {
	unsigned int size;
	unsigned int flags;
};

void cdcm_mem_free(void *addr);
void *cdcm_mem_alloc(ssize_t size, int flags);

#endif /* _CDCM_MEM_H_INCLUDE_ */
