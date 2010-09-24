#include        <string.h> 
#include        <stdio.h>
#include        <unistd.h>
#include        <fcntl.h>
#include        <sys/ioctl.h>
#include        <sys/types.h>
#include        <stdlib.h>
#include        <unistd.h>

#ifdef ppc4
	#include "libvmoddor2dioaio.h"
#else
	#include "libvmoddor.h"
#endif

int main (int argc, char *argv[])
{
	int lun = -1;
	int ret = -1;
	struct vmoddor_warg val;

	if(argc != 5){
		printf("test_vmoddor <lun> <offset> <size> <value>\n");
		printf("<lun> Logical Unit Number. \n"
			"<offset> value of the offset applied. \n"
			"<size> size of the data (4, 8, 16). \n"
			"<value> integer value to be written.\n");

		exit(-1);
	} 
	else
	{
		lun = atoi(argv[1]);
		val.offset = atoi(argv[2]);
		val.size = atoi(argv[3]);
		val.data = atoi(argv[4]);
	}

	ret = vmoddor_open(lun);
	if(ret < 0){
		perror("open failed");
		exit(-1);
	}

	ret = vmoddor_write(lun, val);
	if (ret <0){
		perror("Error on put data to channel");
		exit(-1);
	}

	return 0;
}
