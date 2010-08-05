#ifndef _SIS33_LIB_H_
#define _SIS33_LIB_H_

#include <sis33.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @cond INTERNAL */
struct sis33lib {
	int fd;
	int index;
	int n_clkfreqs;
	unsigned int *clkfreqs;
	int n_ev_lengths;
	unsigned int *ev_lengths;
	int n_bits;
	int ev_tstamp_supported;
};
/** @endcond */

extern const char *libsis33_version;

typedef struct sis33lib sis33_t; /**< opaque libsis33 handle struct */

sis33_t *sis33_open(int index);
int sis33_close(sis33_t *device);

int sis33_loglevel(int loglevel);
int sis33_errno(void);
char *sis33_strerror(int errnum);
void sis33_perror(const char *string);

int sis33_acq(sis33_t *device, unsigned int segment, unsigned int nr_events, unsigned int ev_length);
int sis33_acq_wait(sis33_t *device, unsigned int segment, unsigned int nr_events, unsigned int ev_length);
int sis33_acq_timeout(sis33_t *device, unsigned int segment, unsigned int nr_events, unsigned int ev_length, const struct timespec *timeout);
int sis33_acq_cancel(sis33_t *device);

struct sis33_acq *sis33_acqs_zalloc(unsigned int nr_events, unsigned int ev_length);
void sis33_acqs_free(struct sis33_acq *acqs, unsigned int n_acqs);

int sis33_fetch(sis33_t *device, unsigned int segment, unsigned int channel,
		struct sis33_acq *acqs, unsigned int n_acqs, struct timeval *endtime);
int sis33_fetch_wait(sis33_t *device, unsigned int segment, unsigned int channel,
		struct sis33_acq *acqs, unsigned int n_acqs, struct timeval *endtime);
int sis33_fetch_timeout(sis33_t *device, unsigned int segment, unsigned int channel,
			struct sis33_acq *acqs, unsigned int n_acqs,
			struct timeval *endtime, struct timespec *timeout);

int sis33_set_clock_source(sis33_t *device, enum sis33_clksrc clksrc);
int sis33_get_clock_source(sis33_t *device, enum sis33_clksrc *clksrc);
int sis33_get_nr_clock_frequencies(sis33_t *device);
int sis33_get_available_clock_frequencies(sis33_t *device, unsigned int *clkfreqs, int n);
unsigned int sis33_round_clock_frequency(sis33_t *device, unsigned int clkfreq, enum sis33_round round);
int sis33_set_clock_frequency(sis33_t *device, unsigned int hz);
int sis33_get_clock_frequency(sis33_t *device, unsigned int *hz);

int sis33_get_nr_event_lengths(sis33_t *device);
int sis33_get_available_event_lengths(sis33_t *device, unsigned int *lengths, int n);
unsigned int sis33_round_event_length(sis33_t *device, unsigned int ev_length, enum sis33_round round);

int sis33_trigger(sis33_t *device, enum sis33_trigger trigger);
int sis33_set_enable_external_trigger(sis33_t *device, int value);
int sis33_get_enable_external_trigger(sis33_t *device, int *value);

int sis33_set_start_auto(sis33_t *device, int value);
int sis33_get_start_auto(sis33_t *device, int *value);
int sis33_set_start_delay(sis33_t *device, int delay);
int sis33_get_start_delay(sis33_t *device, int *delay);

int sis33_set_stop_auto(sis33_t *device, int value);
int sis33_get_stop_auto(sis33_t *device, int *value);
int sis33_set_stop_delay(sis33_t *device, int delay);
int sis33_get_stop_delay(sis33_t *device, int *delay);

int sis33_get_nr_channels(sis33_t *device);
int sis33_channel_set_offset(sis33_t *device, int channel_nr, unsigned int offset);
int sis33_channel_set_offset_all(sis33_t *device, unsigned int offset);
int sis33_channel_get_offset(sis33_t *device, int channel_nr, unsigned int *offset);

int sis33_set_nr_segments(sis33_t *device, unsigned int segments);
int sis33_get_nr_segments(sis33_t *device, unsigned int *segments);
int sis33_get_max_nr_segments(sis33_t *device, unsigned int *segments);
int sis33_get_min_nr_segments(sis33_t *device, unsigned int *segments);

int sis33_get_max_nr_events(sis33_t *device, unsigned int *events);
int sis33_get_nr_bits(sis33_t *device);

int sis33_event_timestamping_is_supported(sis33_t *device);
int sis33_get_max_event_timestamping_divider(sis33_t *device, unsigned int *max);
int sis33_get_event_timestamping_divider(sis33_t *device, unsigned int *divider);
int sis33_set_event_timestamping_divider(sis33_t *device, unsigned int divider);
int sis33_get_max_event_timestamping_clock_ticks(sis33_t *device, unsigned int *log2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SIS33_LIB_H_ */
