/*-
 * Copyright (c) 2010 Samuel I. Gonsalvez <siglesia@cern.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "cio8536.h"
#include  "vmodttl.h"
#include "libvmodttl.h"

#define DEV_NAME 20

static int luns_to_fd [VMODTTL_MAX_BOARDS];

static void vmodttl_get_from_bit_pattern(enum vmodttl_conf_pattern *bit_pattern, unsigned char pmr, 
				    unsigned char ptr, unsigned char ppr); 

int vmodttl_open(int lun)
{
	char device[DEV_NAME] = "";
	int ret;
	
	if (luns_to_fd[lun] != 0)
		return luns_to_fd[lun];

	sprintf(device, "/dev/vmodttl.%d", lun);

	ret = open(device, O_RDWR, 0);

	if(ret < 0){
		fprintf(stderr, "libvmodttl: Failed to open the file: %s\n", strerror(errno));
		fprintf(stderr, "libvmodttl: Device /dev/vmodttl%d\n", lun);
		return -1;
	}

	luns_to_fd[lun] = ret;
	return 0;
}

int vmodttl_close(int lun)
{
	int ret;

	ret = close(luns_to_fd[lun]);
	luns_to_fd[lun] = 0;
	return ret;
}

int vmodttl_write(int lun, enum vmodttl_channel chan, int val)
{
	struct vmodttlarg buf;
	int ret = 0;

	buf.dev = chan; 	
	buf.val = val;

	ret = ioctl(luns_to_fd[lun], VMODTTL_WRITE_CHAN, (char *)&buf);

	if(ret){
		fprintf(stderr,"libvmodttl: Failed to put data to channel %d: %s", chan, strerror(errno));
		return -1;
	}

	return 0;
}

int vmodttl_read(int lun, enum vmodttl_channel chan, int *val)
{
	struct vmodttlarg buf;
	int ret = 0;

	buf.dev = chan;
	buf.val = 0;
	
	ret = ioctl(luns_to_fd[lun], VMODTTL_READ_CHAN, (char *)&buf);

	if(ret){
		fprintf(stderr, "libvmodttl: Failed to get data from channel %d: %s", chan, strerror(errno));
		return -1;
	}
	if (chan == VMOD_TTL_CHANNELS_AB)
		*val = buf.val & 0xffff;
	else
		*val = buf.val & 0x00ff;

	return 0;
}

int vmodttl_io_config(int lun, struct vmodttl_config conf)
{
	int i;
	struct vmodttlconfig ttl_conf;
	int fd, ret = 0;
	int io_config = 0;

	fd = luns_to_fd[lun];
	io_config = conf.dir_a & 0x01;
	io_config += (conf.dir_b & 0x01) << 1;

	ttl_conf.io_flag = io_config;
	ttl_conf.us_pulse = conf.us_pulse;
	ttl_conf.invert = conf.inverting_logic;

	ttl_conf.open_collector = conf.mode_a;
	ttl_conf.open_collector += (conf.mode_b & 0x01) << 1;
	ttl_conf.open_collector += (conf.mode_c & 0x01) << 2;

	ttl_conf.pattern_mode_a = conf.pattern_mode_a;
	ttl_conf.pattern_mode_b = conf.pattern_mode_b;

	ret = ioctl(fd, VMODTTL_CONFIG, (char *)&ttl_conf);
	if(ret < 0){
		fprintf(stderr, "libvmodttl: Failed to change the I/O configuration of the channels: %s\n", strerror(errno));
		return ret;
	}

	/* After configured the device, we configure the pattern */
	for(i = 0; i < NR_BITS; i++) {
		if (conf.dir_a == CHAN_IN)
			vmodttl_pattern(lun, CHAN_A, i, conf.conf_pattern_a[i]);
		if (conf.dir_b == CHAN_IN)
			vmodttl_pattern(lun, CHAN_B, i, conf.conf_pattern_a[i]);
	}

	return 0;
}

int vmodttl_io_chan_config(int lun, enum vmodttl_channel chan, struct vmodttl_config conf)
{
	struct vmodttl_config ttl_conf;
	int fd, i;

	vmodttl_read_config(lun, &ttl_conf);
	fd = luns_to_fd[lun];
	
	switch(chan) {
	case VMOD_TTL_CHANNEL_A:
		ttl_conf.dir_a = conf.dir_a;
		ttl_conf.mode_a = conf.mode_a;
		ttl_conf.pattern_mode_a = conf.pattern_mode_a;	
		for(i = 0; i < NR_BITS; i++) 
			ttl_conf.conf_pattern_a[i] = conf.conf_pattern_a[i];
		break;
	case VMOD_TTL_CHANNEL_B:
		ttl_conf.dir_b = conf.dir_b;
		ttl_conf.mode_b = conf.mode_b;
		ttl_conf.pattern_mode_b = conf.pattern_mode_b;
		for(i = 0; i < NR_BITS; i++) 
			ttl_conf.conf_pattern_b[i] = conf.conf_pattern_b[i];
		
		break;
	case VMOD_TTL_CHANNELS_AB:
		ttl_conf = conf;
		break;
	default:
		
		fprintf(stderr, "libvmodttl: Invalid channel (%d) to configure.\n", chan);
		return -1;
	
	}

	return vmodttl_io_config(lun, ttl_conf);
}

int vmodttl_read_config(int lun, struct vmodttl_config *conf)
{
	int ret;
	int fd;
	int i;
	struct vmodttlconfig ttl_conf;

	fd = luns_to_fd[lun];
	ret = ioctl(fd, VMODTTL_READ_CONFIG, (char *)&ttl_conf);

	if(ret < 0)
		fprintf(stderr, "libvmodttl: Failed to copy the I/O configuration of the channels: %s\n", strerror(errno));

	conf->dir_a = ttl_conf.io_flag & 0x01;	
	conf->dir_b = (ttl_conf.io_flag >> 1) & 0x01;
	conf->mode_a = ttl_conf.open_collector & 0x01;	
	conf->mode_b = (ttl_conf.open_collector >> 1) & 0x01;	
	conf->mode_c = (ttl_conf.open_collector >> 2) & 0x01;	
	conf->inverting_logic = ttl_conf.invert;
	conf->us_pulse = ttl_conf.us_pulse;
	conf->pattern_mode_a = ttl_conf.pattern_mode_a;
	conf->pattern_mode_b = ttl_conf.pattern_mode_b;

	for(i = 0; i < NR_BITS; i++) {
		struct vmodttl_pattern *pattern = &ttl_conf.bit_pattern_a[i];
		vmodttl_get_from_bit_pattern(&conf->conf_pattern_a[i], 
						pattern->pmr, pattern->ptr, pattern->ppr);
		pattern = &ttl_conf.bit_pattern_b[i];
		vmodttl_get_from_bit_pattern(&conf->conf_pattern_b[i], 
						pattern->pmr, pattern->ptr, pattern->ppr);
	}
	return ret;
}

int vmodttl_read_device(int lun, unsigned char buffer[2])
{
	return 0;
}

static void vmodttl_get_from_bit_pattern(enum vmodttl_conf_pattern *bit_pattern, unsigned char pmr, 
				    unsigned char ptr, unsigned char ppr) 
{

	if (pmr == 0 && ptr == 1 && ppr == 0)
		*bit_pattern = ANY;
	else if (pmr == 1 && ptr == 0 && ppr == 0)
		*bit_pattern = ZERO;
	else if (pmr == 1 && ptr == 0 && ppr == 1)
		*bit_pattern = ONE;
	else if (pmr == 1 && ptr == 1 && ppr == 0)
		*bit_pattern = ONE_TO_ZERO;
	else if (pmr == 1 && ptr == 1 && ppr == 1)
		*bit_pattern = ZERO_TO_ONE;
	else 
		*bit_pattern = OFF;


}
static void vmodttl_get_bit_pattern(enum vmodttl_conf_pattern bit_pattern, unsigned char *pmr, 
				    unsigned char *ptr, unsigned char *ppr)
{
	switch(bit_pattern) {
		
	case ANY:
		*pmr = 0;
		*ptr = 1;
		*ppr = 0;
		break;
	case ZERO: 
		*pmr = 1;
		*ptr = 0;
		*ppr = 0;
		break;

	case ONE:
		*pmr = 1;
		*ptr = 0;
		*ppr = 1;
		break;

	case ONE_TO_ZERO: 
		*pmr = 1;
		*ptr = 1;
		*ppr = 0;
		break;

	case ZERO_TO_ONE:
		*pmr = 1;
		*ptr = 1;
		*ppr = 1;
		break;

	case OFF:
	default:
		*pmr = 0;
		*ptr = 0;
		*ppr = 0;
	}

}
int vmodttl_pattern(int lun, enum vmodttl_channel port, int pos, enum vmodttl_conf_pattern bit_pattern)
{	
	struct vmodttl_pattern buf;
	int ret = 0;

	if(port < VMOD_TTL_CHANNEL_A || port > VMOD_TTL_CHANNEL_B) {
		fprintf(stderr, "libvmodttl: Wrong channel to configure the pattern\n");
		return -1;
	}

	if(pos < 0 || pos >= NUM_BITS) {
		fprintf(stderr, "libvmodttl: Wrong bit position to configure the pattern\n");
		return -1;
	}
	
	vmodttl_get_bit_pattern(bit_pattern, &buf.pmr, &buf.ptr, &buf.ppr);
	buf.port = port;
	buf.pos = pos;
	
	ret = ioctl(luns_to_fd[lun], VMODTTL_PATTERN, (char *)&buf);

	if(ret){
		fprintf(stderr, "libvmodttl: Failed to configure the pattern to the channel %d bit %d: %s", 
							port, pos, strerror(errno));
		return -1;
	}

	return 0;
}

