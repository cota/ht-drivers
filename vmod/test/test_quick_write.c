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

int main (int argc, char *argv[])
{        
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

	val = 0;
	while(1){
		
		if(val) {
			vmodttl_write(device, 1, 0x43);
			val = 0;
		}else{
			vmodttl_write(device, 1, 0xe1);
			val = 1;
		}
		usleep(250);
		//sleep(1);
	}

	vmodttl_close(device); 

	return 0;
}
