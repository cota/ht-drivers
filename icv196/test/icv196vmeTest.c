#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/file.h>
#include <stdio.h>
#include <ctype.h>
#include <skeluser_ioctl.h>
#include <icv196vmelib.h>
#include <general_both.h>
#include <gp.h>

/* #include "icv196vmeP.h"*/
/* #include "/u/dscps/rtfclty/gpsynchrolib.h" */
//#define addr = 0x00500000
//#define Evtsrce_icv196 2

#define ICV_nln     16
#define ICV_nboards 8  /* MAX allowed modules */

extern int errno;
static int service_fd = 0;
static int synchro_fd = 0;

#ifdef __Lynx__
extern int snprintf(char *str, size_t size, const char *format, ...);
#endif

static int  module     = 0;
static int  old_module = 0;

static char choice[10];

static void transmit_data(int fd, struct gpfd *gpfd);

void get_input(char *ch, int *inp)
{
	char *p;

	printf("   +--------------------------------------------------------------+\n");
	printf("   |  0: exit                                                     |\n");
	printf("   |  1: test                                                     |\n");
	printf("   |  2: change the ICV board being accessed (current board: %d)   |\n", module);
	printf("   |  3: read interrupt enable mask                               |\n");
	printf("   |  4: read module info (VME address, interr. vect.  etc.)      |\n");
	printf("   |  5: read handle info (lines connected to by a user)          |\n");
	printf("   |  6: read group                                               |\n");
	printf("   |  7: write group                                              |\n");
	printf("   |  8: set I/O direction  (does not concern interrupts)         |\n");
	printf("   |  9: read I/O direction (does not concern interrupts)         |\n");
	printf("   |  10:    read all interrupt counters                          |\n");
	printf("   |  11:    read all reenable flags                              |\n");
	printf("   |  12/13: set/clear reenable flag                              |\n");
	printf("   |  14/15: enable/disable line                                  |\n");
	printf("   |  16:    read an event (enable interrupt line first)          |\n");
	printf("   |  17/18: configure blocking (default)/nonblocking read        |\n");
	printf("   |                                                              |\n");
	printf("   |  Nb: functions 12-18 automatically connect to interrupt-line |\n");
	printf("   +--------------------------------------------------------------+\n\n");
	printf("--> ");
	scanf("%s", ch);
	getchar();			/* eat up \n */

	for (p = ch; *p != '\0'; p++)
		if (!isdigit((int)*p)) {
			*inp = -1;
			return;
		}

	*inp =  atoi(ch);
	if ((*inp < 0) || (*inp > 16))
		printf("input not correct\n");
}

int main(int argc, char *argv[], char *envp[])
{
	int retval, i, j, dir, stat, grp_nr, grp_mask = 1;
	char buff[50] = { 0 } , line[10];
	int status[icv_ModuleNb][icv_LineNb] = { { 0 } };
	int input, count;
	char board[2];
	long *evt;
	struct icv196T_Service arg;
	struct icv196T_UserLine arg2;
	struct icv196T_ModuleInfo arg3[icv_ModuleNb];
	struct icv196T_HandleInfo arg4;
	struct icv196T_ModuleInfo *Info_P;
	struct icv196T_HandleLines *Info_H;

	service_fd = icv196_get_handle();
	if (service_fd < 0) {
		fprintf(stderr, "Can't get library handle...\n");
		exit(EXIT_FAILURE);
	}

	synchro_fd = service_fd; /* TODO */

	do {
		get_input(choice, &input);

		switch (input) {
		case 0:
			break;
		case 1:
			{
				struct gpfd *gpfd = open_gpf(NULL);
				if (!gpfd) {
					printf("\n\n<enter> to continue"); getchar();
					break;
				}

				if (!load_gpf(gpfd)) {
					printf("No data loaded from gnuplot file\n");
					printf("\n\n<enter> to continue"); getchar();
					break;
				}

				if (!do_conv_gp_file(gpfd, 1))
					printf("Failed to create converted file\n");
				else
					printf("Converted file ready\n");

				transmit_data(service_fd, gpfd);
				printf("\n\n<enter> to continue"); getchar();
				break;
			}
		case 2:
			old_module = module;
			printf("number of module to be accessed ?\n" );
			scanf("%s", board);
			module = atoi(board);
			if ((module < 0) || (module >= ICV_nboards)) {
				printf("board number out of range\n");
				module = old_module;
				break;
			}
			break;
		case 3:
			arg.module = (unsigned char) module;
			if ((retval = ioctl(service_fd, ICVVME_intenmask, &arg)) < 0) {
				perror("could not do ioctl\n");
				break;
			}
			stat = (unsigned short)arg.data[0];
			printf("STATUS OF INTERRUPT LINES ON BOARD NR. %d: ( 0 = disabled, 1 = enabled )\n", module);
			for (i = 0; i <= 15; i++) {
				printf("line %2d: %2d   ", i, stat & grp_mask);
				stat >>= 1;
				if (((i+1) % 4) == 0 && i != 0)
					printf("\n");
			}
			printf("\n");
			break;
		case 4:
			Info_P = arg3;
			if ((retval = ioctl(service_fd, ICVVME_getmoduleinfo, Info_P)) < 0) {
				perror("could not do ioctl\n");
				break;
			}

			for (i = 0; i < icv_ModuleNb; i++, Info_P++) {
				printf("MODULE INFO FOR MODULE NR. %d:\n", i);
				if (!Info_P->ModuleFlag) {
					printf("module not installed\n\n");
					continue;
				}

				printf("module base address: 0x%lx\n", Info_P->ModuleInfo.base);
				printf("module address space size in bytes: %ld\n",
				       Info_P->ModuleInfo.size);
				printf("module interrupt vectors:\n");

				for (j = 0; j < icv_LineNb; j++) {
					printf("line %2d:%d  ", j, Info_P->ModuleInfo.vector);

					if (((j+1) % 6) == 0 && j != 0)
						printf("\n");
				}
				printf("\n");

				printf("module interrupt levels:\n");

				for (j = 0; j < icv_LineNb; j++) {
					printf("line %2d:%d  ", j, Info_P->ModuleInfo.level);
					if (((j+1) % 6) == 0 && j != 0)
						printf("\n");
				}
				printf("\n");
				printf("\n");
			}
			break;
		case 5:
			Info_H = (struct icv196T_HandleLines *)&(arg4.handle[0]);
			for (i = 0; i < SkelDrvrCLIENT_CONTEXTS; i++, Info_H++)  {
				Info_H -> pid = 0;
				for (j = 0; j < ICV_LogLineNb; j++)  {
					Info_H -> lines[j].group = 0xff;
					Info_H -> lines[j].index = 0xff;
				}
			}

			if ((retval = ioctl(service_fd, ICVVME_gethandleinfo, &arg4)) < 0) {
				perror("could not do ioctl\n");
				break;
			}

			Info_H = (struct icv196T_HandleLines *)&(arg4.handle[0]);
			Info_H++;
			for (i = 1; i < SkelDrvrCLIENT_CONTEXTS; i++, Info_H++) {
				printf("HANDLE INFO FOR HANDLE NR. %d:   ", i);
				if (!Info_H->pid) {
					printf("handle not used\n\n");
					continue;
				}
				printf("used by process pid: %d\n", Info_H -> pid);
				printf("lines connected :");

				for (j = 0; Info_H -> lines[j].group != 0xff; j++) {
					printf("[modul %2d, line %2d]  ", Info_H -> lines[j].group,
					       Info_H -> lines[j].index);
					if (((j+1) % 3) == 0 && j != 0)
						printf("\n");
				}
				printf("\n");
				printf("\n");
			}
			break;
		case 6: /* read from the group */
			{
				short *val = (short*)buff;
			memset(buff, 0, sizeof(buff));
			printf("Group index to read [0 - 11] --> ");
			scanf("%d", &grp_nr);  getchar();

			if (grp_nr & 1) {
				printf("Data size in bytes to read [1 - 2] --> ");
				scanf("%ld", (long*)buff); getchar();
			} else
				*buff = 1;

			retval = icv196_read_channel(service_fd, module, grp_nr, buff);
			if (retval < 0)
                                printf("Failed\n");
                        else
                                printf("0x%hx\n", *val);

                        printf("\n\n<enter> to continue"); getchar();
			break;
			}
		case 7: /* write into the group */
			memset(buff, 0, sizeof(buff));
			printf("Group index to write [0 - 11] --> ");
			scanf("%d", &grp_nr);  getchar();

			if (grp_nr&1) {
				printf("Data size (in bytes) to write [1 - 2] --> ");
				scanf("%ld", (long*)&buff[3]); getchar();
			} else
				buff[3] = 1;

			if ((long)buff[3] == 1)
				printf("Data to write in hex (byte) --> ");
			else
				printf("Data to write in hex (word) --> ");

			scanf("%hx", (short*)buff); getchar();

			if (icv196_write_channel(service_fd, module, grp_nr, buff))
				printf("Failed\n");
                        else
                                printf("Done\n");
                        printf("\n\n<enter> to continue"); getchar();

			break;
		case 8:
			printf("Group index to be set [0 - 11] -> ");
			scanf("%d", &grp_nr);  getchar();

			printf("Direction [0 -- input, 1 -- output] -> ");
			scanf("%d", &dir); getchar();

			if (icv196_init_channel(service_fd, module, grp_nr, 1, dir))
				printf("Failed\n");
			else
				printf("Done\n");
			printf("\n\n<enter> to continue"); getchar();
			break;
		case 9:
			memset(&arg, 0, sizeof(arg));
			arg.module = module;

			if (ioctl(service_fd, ICVVME_readio, &arg) < 0) {
				perror("could not do ioctl\n");
				break;
			}
			dir = arg.data[0];
			printf("I/O groups direction on module#%d"
			       " (0:input, 1:output)\n", module);
			for (i = 0; i <= 11; i++) {
				printf("group[%2d] %2d\n", i, dir & 1);
				dir >>= 1;
			}
			printf("\n\n<enter> to continue"); getchar();
			break;
		case 10:
			arg.module = (unsigned char) module;
			if ((retval = ioctl(service_fd, ICVVME_intcount, &arg)) < 0) {
				printf("retval = %d \n ", retval);
				perror("could not do ioctl\n");
				break;
			}
			printf("INTERRUPT COUNTER VALUES ON BOARD NR. %d:\n", module);
			for (i = 0; i < ICV_nln; i++) {
				printf("line %2d: %3ld ", i, arg.data[i]);
				if (((i+1) % 4) == 0 && i != 0)
					printf("\n");
			}
			printf("\n");
			break;
		case 11:
			arg.module = (unsigned char) module;
			if ((retval = ioctl(service_fd, ICVVME_reenflags, &arg)) < 0) {
				perror("could not do ioctl\n");
				break;
			}
			printf("REENABLE INTERRUPT FLAGS ON BOARD NR. %d: ( 1 = reenable on,  2 = reenable off )\n", module);
			for (i = 0; i < ICV_nln; i++) {
				printf("line %2d: %2ld", i, arg.data[i]);
				if (((i+1) % 4) == 0 && i != 0)
					printf("\n");
			}
			printf("\n");
			break;
		case 12:
			printf("line index to access\n");
			printf("on module#%d\n", module);
			printf("[0 -- 15] --> ");
			scanf("%s", line); getchar();

			i = atoi(line);
			if (!WITHIN_RANGE(0, i, icv_LineNb-1)) {
				printf("line number out of range\n");
				break;
			}
			if (!status[module][i]) {
				icv196_connect(synchro_fd, module, i, 0);
				status[module][i] = synchro_fd;
			}
			synchro_fd = status[module][i];
			arg2.group = module;
			arg2.index = i;
			if ((retval = ioctl(synchro_fd, ICVVME_setreenable , &arg2)) < 0) {
				perror("could not do ioctl\n");
				break;
			}
			break;
		case 13:
			printf("number of line to access \n" );
			printf("on module nr. %d ? \n", module );
			scanf("%s", line);

			i = atoi(line);
			if ((i < 0) || (i >= icv_LineNb)) {
				printf("line number out of range\n");
				break;
			}
			if (status[module][i] == 0) {
				icv196_connect(synchro_fd, module, i, 0);
				status[module][i] = synchro_fd;
			}
			synchro_fd = status[module][i];
			arg2.group = module;
			arg2.index = i;
			if ((retval = ioctl(synchro_fd, ICVVME_clearreenable , &arg2)) < 0) {
				perror("could not do ioctl\n");
				break;
			}
			break;
		case 14:
			printf("Line index to access on module#%d\n", module);
			printf("[0 -- 15] --> ");
			scanf("%s", line); getchar();

			i = atoi(line);
			if (!WITHIN_RANGE(0, i, icv_LineNb-1)) {
				printf("idx out of range\n");
				break;
			}

#if 0
			if (status[module][i] == 0) {
				icv196_connect(synchro_fd, module, i, 0);
				status[module][i] = synchro_fd;
			}
			synchro_fd = status[module][i];
#endif

			arg2.group = module;
			arg2.index = i;
			if (ioctl(synchro_fd, ICVVME_enable , &arg2) < 0)
				perror("ICVVME_enable ioctl failed");
			break;
		case 15:
			printf("number of line to access \n" );
			printf("on module nr. %d ? \n", module );
			scanf("%s", line);

			i = atoi(line);
			if ((i < 0) || (i >= icv_LineNb)) {
				printf("line number out of range\n");
				break;
			}
			if (status[module][i] == 0) {
				icv196_connect(synchro_fd, module, i, 0);
				status[module][i] = synchro_fd;
			}
			synchro_fd = status[module][i];
			arg2.group = module;
			arg2.index = i;
			if ((retval = ioctl(synchro_fd, ICVVME_disable , &arg2)) < 0) {
				perror("could not do ioctl\n");
				break;
			}
			break;
		case 16:
			if (!synchro_fd) {
				printf("no line to read from\n");
				break;
			}
			memset(buff, 0, sizeof(buff));
			count = 32;
			if ((retval = read(synchro_fd, buff, count)) < 0) {
				perror("could not read\n");
				break;
			}
			printf("EVENTS READ:\n");
			evt = (long *)buff;
			if (*evt == 0) {
				printf("no events read\n");
				printf("\n");
				break;
			}
			for (i = 0; i < 8; i++, evt++) {
				if (*evt != 0) {
					printf("evt. nr. %2d: 0x%lx    ", i, *evt);
					if (((i+1) % 2) == 0 && i != 0)
						printf("\n");
				}
			}
			printf("\n");
			break;
		case 17:
			printf("number of line to access \n" );
			printf("on module nr. %d ? \n", module );
			scanf("%s", line);

			i = atoi(line);
			if ((i < 0) || (i >= icv_LineNb)) {
				printf("line number out of range\n");
				break;
			}
			if (status[module][i] == 0) {
				icv196_connect(synchro_fd, module, i, 0);
				status[module][i] = synchro_fd;
			}
			synchro_fd = status[module][i];
			arg2.group = module;
			arg2.index = i;
			if ((retval = ioctl(synchro_fd, ICVVME_wait, &arg2)) < 0) {
				perror("could not do ioctl\n");
				break;
			}
			break;
		case 18:
			printf("number of line to access \n" );
			printf("on module nr. %d ? \n", module );
			scanf("%s", line);

			i = atoi(line);
			if ((i < 0) || (i >= icv_LineNb)) {
				printf("line number out of range\n");
				break;
			}
			if (status[module][i] == 0) {
				icv196_connect(synchro_fd, module, i, 0);
				status[module][i] = synchro_fd;
			}
			synchro_fd = status[module][i];
			arg2.group = module;
			arg2.index = i;
			if ((retval = ioctl(synchro_fd, ICVVME_nowait, &arg2)) < 0) {
				perror("could not do ioctl\n");
				break;
			}
			break;
		default:
			printf("input not correct\n");
			break;
		}
	} while (input != 0);

	for (i = 0; i < icv_ModuleNb; i++)
		for (j = 0; j < icv_LineNb; j++)
			if (status[i][j] != 0) icv196_disconnect(synchro_fd, i, j);

	if (icv196_put_handle(service_fd))
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}

static int init_group(int fd, int *grp, int dir)
{
	int mt[6][2] = {
		[0] {
			[0] = 0,
			[1] = 1
		},

		[1] {
			[0] = 2,
			[1] = 3
		},

		[2] {
			[0] = 4,
			[1] = 5
		},

		[3] {
			[0] = 6,
			[1] = 7
		},

		[4] {
			[0] = 8,
			[1] = 9
		},

		[5] {
			[0] = 10,
			[1] = 11
		}
	};

#if 0
	printf("Will initialize 2 subgroups of group %d: %d && %d\n",
	       *grp, mt[*grp-1][0], mt[*grp-1][1]);
#endif

	if (icv196_init_channel(fd, module, mt[*grp-1][0], 1, dir) ||
	    icv196_init_channel(fd, module, mt[*grp-1][1], 1, dir)) {
		printf("Failed to init group %d\n", *grp);
		return 0;
	}

	*grp = mt[*grp-1][1];

	return 1;
}

/**
 * @brief Test to transmit 2byte long data
 *
 * @param fd   --
 * @param gpfd --
 *
 * <long-description>
 *
 */
static void transmit_data(int fd, struct gpfd *gpfd)

{
	char buff[50] = { 0 }, *ptr;
	short *bptr = (short*) buff;
	int wr_grp, rd_grp, rval[2], i;
	short wv;		/* saved written value */

 get_write:
	printf("Group index to write into [1 -- 6] (0 -- abort) --> ");
	scanf("%d", &wr_grp);  getchar();

	if (!wr_grp)
		return;

	if (!(WITHIN_RANGE(1, wr_grp, 6))) {
		printf("Wrong write group\n");
		goto get_write;
	}

	switch (wr_grp) {
	case 1:
		rval[0] = 3;
		rval[1] = 5;
		break;
	case 2:
		rval[0] = 4;
		rval[1] = 6;
		break;
	case 3:

		rval[0] = 1;
		rval[1] = 5;
		break;
	case 4:
		rval[0] = 2;
		rval[1] = 6;
		break;
	case 5:
		rval[0] = 1;
		rval[1] = 3;
		break;
	case 6:
		rval[0] = 2;
		rval[1] = 4;
		break;
	}

 get_read:
	printf("Group index to read from [%d, %d] (0 -- abort) --> ",
	       rval[0], rval[1]);
	scanf("%d", &rd_grp);  getchar();

	if (!rd_grp)
		return;

	if (rd_grp != rval[0] && rd_grp != rval[1]) {
		printf("Wrong read group\n");
		goto get_read;
	}

	/* set direction */
	if (!init_group(fd, &wr_grp, 1) ||
	    !init_group(fd, &rd_grp, 0))
		return;

	//printf("Actual Groups: write %d   read %d\n", wr_grp, rd_grp);

	/* set new file name for read data */
	ptr = rindex(gpfd->gpfn, '.');
	snprintf(ptr, 4, ".rd");

	for (i = 0; i < gpfd->elam; i++) {
		wv = *bptr = gpfd->gpv[i].conv;
		buff[3] = 2;

		if (icv196_write_channel(fd, module, wr_grp, buff)) {
			printf("icv196_write_channel() failed\n");
			continue;
		}

		usleep(500); /* delay for HW to do the job */

		buff[0] = 2;
		if (icv196_read_channel(fd, module, rd_grp, buff) < 0) {
			printf("icv196_read_channel() failed\n");
			continue;
		}
		usleep(500); /* delay for HW to do the job */

		/* safe received values and put them in the file */
		gpfd->gpv[i].conv = *bptr;
		//printf("Write 0x%hx    Read back 0x%hx\n", wv, *bptr);
	}

	do_conv_gp_file(gpfd, 0);
	printf("Received data saved in %s file\n", gpfd->gpfn);
}
