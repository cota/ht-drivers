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

#include	<string.h> 
#include 	<stdio.h>
#include	<unistd.h>
#include	<fcntl.h>
#include  <sys/stat.h>
#include	<sys/types.h>
#include	<stdlib.h>
#include 	<unistd.h>

#ifdef ppc4
	#include "libvmodttl2dioaio.h"
#else
	#include "libvmodttl.h"
#endif

int device;

void write_channel(int pd)
{	
	int chan = 0;
	int val = 0;

	printf("Please, give the number of the channel [0 -> A, 1 -> B, 2 -> AB]: ");
	scanf("%d", &chan);

	printf("Please, give the value to be written to the channel (hex): 0x");
	scanf("%x", &val);
	
	if(vmodttl_write(pd, chan, val) < 0)
		printf("test_vmodttl: Write error");
}

void read_channel(int pd)
{	
	int chan = 0;
	int val = 0;

	printf("Please, give the number of the channel [0 -> A, 1 ->B]: ");
	scanf("%d", &chan);

	if(vmodttl_read(pd, chan, &val) < 0)
		printf("test_vmodttl: Write error");
		
	printf("Channel %d, value %x\n", chan, val);
}

int main (int argc, char *argv[])
{        
	struct vmodttl_config conf;
	int val;


	printf("  ______________________________________  \n");
	printf(" |                                      | \n");
	printf(" |       VMOD-TTL Testing program       | \n");
	printf(" |______________________________________| \n");
	printf(" |                                      | \n");
	printf(" | Created by: Samuel I. Gonsalvez      | \n");
	printf(" | Email: siglesia@cern.ch              | \n");
	printf(" |______________________________________| \n");

	if (argc == 2)
	{
		device = atoi(argv[1]);
	}
	else
	{
		printf("\n Please use: test_read_write <lun_of_the_device>.\n");
		exit(-1);
	}

	if(vmodttl_open (device) < 0) {
		perror("open failed");	
		return -1;
	}

	printf("\n Port configuration \n");
	printf("\n Channel A [ (0) Input - (1) Output]: \n");
	scanf("%d", &val);
	conf.dir_a = val;
	if (!conf.dir_a){
		conf.mode_a = 0;
	}else{
		printf("\n Channel A [ (0) Open Drain - (1) Open Collector ]\n");
		scanf("%d", &val);
		conf.mode_a = val;
	}

	printf("\n Channel B [ (0) Input - (1) Output]: \n");
	scanf("%d", &val);
	conf.dir_b = val;

	if (!conf.dir_b){
		conf.mode_b = 0;

	}else{
		printf("\n Channel B [ (0) Open Drain - (1) Open Collector ]\n");
		scanf("%d", &val);
		conf.mode_b = val;
	}

	printf("\n Channel C [ (0) Open Drain - (1) Open Collector ]\n");
	scanf("%d", &val);
	conf.mode_c = val;

	printf("\n Up time of the pulse in the data strobe (useg): \n");
	scanf("%d", &val);
	conf.us_pulse = val;

	printf("\n In all the channels: [ (0) Non-inverting Logic - (1) Inverting logic]: \n");
	scanf("%d", &val);
	conf.inverting_logic = val;

	if (vmodttl_io_config(device, conf) < 0)
		exit(-1);

	while(1){
		vmodttl_read(device, 0, &val);
		printf("Value: 0x%x\n", val);

	}

	vmodttl_close(device); 

	return 0;
}
