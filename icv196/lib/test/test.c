#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <icv196vmelib.h>

#define LUN 0

enum channel_dir {
	INPUT  = 0,
	OUTPUT = 1
};

int main(int argc, char *argv[], char *envp[])
{
	int fd;
	char wbuff, rbuff;

	fd = icv196_get_handle();
	if (fd < 0) {
		printf("Can't get library handle...\n");
		exit(EXIT_FAILURE);
	}
	if (icv196_init_channel(fd, LUN, 1, 1, OUTPUT)) {
		printf("icv196_init_channel() failed\n");
		return -1;
	}
	if (icv196_init_channel(fd, LUN, 5, 1, INPUT)) {
		printf("icv196_init_channel() failed\n");
		return -1;
	}

	wbuff = 56;
	rbuff = 0;

	icv196_write_channel(fd, LUN, 1, 1, &wbuff);
	icv196_read_channel(fd, LUN, 5, 1,  &rbuff);
	printf("Write %hd, read back %hd\n", wbuff, rbuff);
	exit(EXIT_SUCCESS);
}
