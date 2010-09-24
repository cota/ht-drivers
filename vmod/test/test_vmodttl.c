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

	printf("Please, give the number of the channel [0 -> A, 1 ->B, 2 -> AB]: ");
	scanf("%d", &chan);

	if(vmodttl_read(pd, chan, &val) < 0)
		printf("test_vmodttl: Write error");
		
	printf("Channel %d, value 0x%x\n", chan, val);
}

int main (int argc, char *argv[])
{        
        char tmp;
	struct vmodttl_config conf;
	int val;
	int i;


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

	conf.pattern_mode_a = AND;
	conf.pattern_mode_b = OR;
	
	for(i = 0; i< NUM_BITS; i++) {
			conf.conf_pattern_a[i] = OFF;
			conf.conf_pattern_b[i] = OFF;
	}

	if (vmodttl_io_config(device, conf) < 0)
		exit(-1);


	do{
		if(tmp != '\n'){	
			printf("\n **** OPTIONS ****\n\n");
			printf(" a) Write channel.\n");
			printf(" b) Read channel.\n");
			printf(" Press \'q\' to exit...\n");
		}

		scanf("%c", &tmp);

		switch(tmp){
		case 'a':
			write_channel(device);
			break;
		case 'b':
			read_channel(device);
			break;
		case 'q':
			printf(" Exiting...\n");
			break;
		default:
			break;	
		}

	}while(tmp != 'q');

	conf.pattern_mode_a = 1;
	conf.mode_a = 1;
	conf.dir_a = 1;

	if(vmodttl_io_chan_config(device, 0, conf))
		exit(-1);

	if(vmodttl_read_config(device, &conf))
		exit(-1);
	printf("\n\n ------------------------------- \n");
	printf("\n\n 	I/O Config used!             \n");
	printf(" ------------------------------- \n");
	printf(" dir a: %d mode a: %d \n", conf.dir_a, conf.mode_a);	
	printf(" dir b: %d mode b: %d \n", conf.dir_b, conf.mode_b);	
	printf(" mode c: %d \n", conf.mode_c);	
	printf(" us_pulse: %d  inverting_logic: %d\n", conf.us_pulse, conf.inverting_logic);	
	printf(" pattern mode A: %d  pattern mode B: %d\n", conf.pattern_mode_a, conf.pattern_mode_b);	
	printf(" port A: %d/%d/%d/%d/%d/%d/%d/%d  port B: %d/%d/%d/%d/%d/%d/%d/%d \n", 
			conf.conf_pattern_a[0],
			conf.conf_pattern_a[1],
			conf.conf_pattern_a[2],
			conf.conf_pattern_a[3],
			conf.conf_pattern_a[4],
			conf.conf_pattern_a[5],
			conf.conf_pattern_a[6],
			conf.conf_pattern_a[7],
			conf.conf_pattern_b[0],
			conf.conf_pattern_b[1],
			conf.conf_pattern_b[2],
			conf.conf_pattern_b[3],
			conf.conf_pattern_b[4],
			conf.conf_pattern_b[5],
			conf.conf_pattern_b[6],
			conf.conf_pattern_b[7]
			);	
	printf("\n\n ------------------------------- \n");
	vmodttl_close(device); 

	return 0;
}
