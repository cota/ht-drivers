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
#include  	<sys/stat.h>
#include	<sys/types.h>
#include	<stdlib.h>
#include 	<unistd.h>
#include 	<pthread.h>

#define 	MAX_THREADS 20

#include	"libvmodttl.h"

int device;

void *annoying_thread(void *thd)
{
	int val = 0;
	int i = 0;
	int thread = *(int *)thd;

	while(i < 20){
		vmodttl_write(device, 1, thread);
		vmodttl_read(device, 0, &val);
		printf("Thread %d -- Channel 0, value %d\n", thread, val);
		i++;
	}
	return NULL;	
}

int main (int argc, char *argv[])
{        
	struct vmodttl_config conf;
	int val;
	int i;
	pthread_t thread[MAX_THREADS];


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


	for(i = 0; i < MAX_THREADS; i++){
		pthread_create((pthread_t *)&thread[i], NULL, annoying_thread, (void *)&i);
	}
	
	for(i = 0; i < MAX_THREADS; i++){
		pthread_join(thread[i], NULL);
	}
	
	vmodttl_close(device); 

	return 0;
}


