/*
 * CVORG library header file
 */

#ifndef _CVORG_LIB_H_
#define _CVORG_LIB_H_

#include <cvorg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct cvorglib {
	int fd;
};

typedef struct cvorglib cvorg_t;

/* version string of the current libcvorg */
extern const char *libcvorg_version;

cvorg_t *cvorg_open(int devicenr, enum cvorg_channelnr channelnr);
int cvorg_close(cvorg_t *device);

int cvorg_loglevel(int loglevel);
int cvorg_errno(void);
char *cvorg_strerror(int errnum);
void cvorg_perror(const char *string);

int cvorg_lock(cvorg_t *device);
int cvorg_unlock(cvorg_t *device);

int cvorg_set_sampfreq(cvorg_t *device, unsigned int freq);
int cvorg_get_sampfreq(cvorg_t *device, unsigned int *freq);

int cvorg_channel_set_outoff(cvorg_t *device, enum cvorg_outoff outoff);
int cvorg_channel_get_outoff(cvorg_t *device, enum cvorg_outoff *outoff);

int cvorg_channel_set_outgain(cvorg_t *device, int32_t *outgain);
int cvorg_channel_get_outgain(cvorg_t *device, int32_t *outgain);

int cvorg_channel_set_inpol(cvorg_t *device, enum cvorg_inpol inpol);
int cvorg_channel_get_inpol(cvorg_t *device, enum cvorg_inpol *inpol);

int cvorg_channel_get_status(cvorg_t *device, unsigned int *status);

int cvorg_channel_set_trigger(cvorg_t *device, enum cvorg_trigger trigger);

int cvorg_channel_set_sequence(cvorg_t *device, struct cvorg_seq *sequence);

int cvorg_channel_set_waveform(cvorg_t *device, struct cvorg_wv *waveform);

char *cvorg_get_hw_version(cvorg_t *device);

int cvorg_channel_disable_output(cvorg_t *device);
int cvorg_channel_enable_output(cvorg_t *device);

int cvorg_dac_get_conf(cvorg_t *device, struct cvorg_dac *conf);
int cvorg_dac_set_conf(cvorg_t *device, struct cvorg_dac conf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CVORG_LIB_H_ */
