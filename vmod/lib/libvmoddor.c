
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vmoddor.h"
#include "libvmoddor.h"

static int luns_to_fd [VMODDOR_MAX_DEVICES];

int vmoddor_open(int lun)
{
	char device[VMODDOR_SIZE_CARRIER_NAME] = "";
	int ret;

	if(luns_to_fd[lun] != 0)
		return luns_to_fd[lun];

	sprintf(device, "/dev/vmoddor.%d", lun);

	ret = open(device, O_RDWR, 0);
	if (ret < 0){
		fprintf(stderr, "Failed to open the file: %s\n", strerror(errno));
		return -1;
	}
	
	luns_to_fd[lun] = ret;
	return 0;
}

int vmoddor_close(int lun)
{
	int ret;

	ret = close(luns_to_fd[lun]);
	luns_to_fd[lun] = 0; 
	return ret;
}

int vmoddor_write(int lun , struct vmoddor_warg val)
{
	int ret;
	int fd;
	struct vmoddor_arg argument;

	argument.data = val.data;
	argument.offset = val.offset;
	argument.size = val.size;
	
	fd = luns_to_fd[lun];
	ret = ioctl(fd, VMODDOR_WRITE, (char *)&argument);
	
	if(ret < 0){
		fprintf(stderr, "Failed to write data to the channel: %s\n", strerror(errno));
		return ret;
	}

	return 0;
}





