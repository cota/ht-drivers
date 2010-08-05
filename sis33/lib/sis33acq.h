#ifndef _SIS33_ACQ_H_
#define _SIS33_ACQ_H_

#include <sis33.h>

#ifdef __linux__
#include <byteswap.h>
#define myswap32 bswap_32
#else /* We won't use this on BE platforms, but anyway let's define something */
static inline unsigned short myswap32(unsigned short val)
{
	return (((val & 0xff000000) >> 24) | ((val & 0xff0000) >> 8) |
		((val & 0xff00) << 8) | ((val & 0xff) << 24));
}
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 0
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 1
#endif

int sis33acq_list_normalize(struct sis33_acq_list *list, int elems);
struct sis33_acq *sis33acq_zalloc(unsigned int nr_events, unsigned int ev_length);
void sis33acq_free(struct sis33_acq *acqs, unsigned int n_acqs);

#endif /* _SIS33_ACQ_H_ */
