#include <time.h>
#include <file.h>
#include <stdio.h>
#include <ctype.h>
#include <smem.h>
#include <icvioctl.h>
#include <icv196vmelib.h>
/*#include "icv196vmeP.h"*/
/*#include "/u/dscps/rtfclty/gpsynchrolib.h" */

#define SIZE 50
#define addr = 0x00500000
#define Evtsrce_icv196 2
#define ICV_nln 16
#define ICV_nboards 4

extern int errno;
static char path[] =  "/dev/icv196service";
static int  service_fd = 0;
static int  synchro_fd = 0;

static int  module = 0;
static int  old_module = 0;
static char choice[10];

extern int gpevtconnect();
extern int gpevtdisconnect();

void get_input(char *ch, int *inp)
{
    char *p;

    printf("   ----------------------------------------------------------------\n");
    printf("   |  1: change the ICV board being accessed (current board: %d)   |\n", module);
    printf("   |  2: read all interrupt counters                              |\n");
    printf("   |  3  read all reenable flags                                  |\n");
    printf("   |  4: read interrupt enable mask                               |\n");
    printf("   |  5: read module info (VME address, interr. vect.  etc.)      |\n");
    printf("   |  6: read handle info (lines connected to by a user)          |\n");
    printf("   |  7: set I/O direction  (does not concern interrupts)         |\n");
    printf("   |  8: read I/O direction (does not concern interrupts)         |\n");
    printf("   |  9/10 set/clear reenable flag                                |\n");
    printf("   | 11/12: enable/disable line                                   |\n");
    printf("   | 13: read an event (enable interrupt line first)              |\n");
    printf("   | 14/15: configure blocking (default)/nonblocking read         |\n");
    printf("   |  0: exit                                                     |\n");
    printf("   |  Nb: functions  9-15 automatically connect to the interr-line|\n");
    printf("   ----------------------------------------------------------------\n");
    scanf("%s", ch);
    for (p = ch; *p != '\0'; p++)
	if (!isdigit((int)*p)) {
	    *inp = -1;
	    return;
	}
    *inp =  atoi(ch);
    if ((*inp < 0) || (*inp > 16))
	printf("input not correct\n");
}

int connect(int *fd, short group, short index, short mode)
{

	union U_short {
		short wordshort;
		struct {
			char w1;
			char w2;
		} elt;
	};

    int  L_type;
    int  L_val;
    int  L_dev;
    union U_short L_ws;
    short arg[2];

    L_type = Evtsrce_icv196;
    L_val = 0;
    L_ws.elt.w1 = group;
    L_ws.elt.w2 = index;

    arg[0] = L_ws.wordshort;
    arg[1] = mode;

    L_dev = gpevtconnect(L_type, L_val, &arg[0]);
    if ( (L_dev <=0))
      return(-1);
    *fd = L_dev;
    return (0);
}


int disconnect(short group, short index)
{
	union U_short {
		short wordshort;
		struct {
			char w1;
			char w2;
		} elt;
	};

    int  L_type;
    int  L_dev;
    union U_short L_ws;
    short arg[2];

    L_type = Evtsrce_icv196;
    L_ws.elt.w1 = group;
    L_ws.elt.w2 = index;

    arg[0] = L_ws.wordshort;
    arg[1] = 0;

    L_dev = gpevtdisconnect(L_type, &arg[0]);
    if ( (L_dev <=0))
      return(-1);
    return (0);
}

int main(int argc, char *argv[], char *envp[])
{
    int retval, i, j, dir, stat, grp_nr, grp_mask = 1;
    char buff[SIZE], line[10];
    int status[icv_ModuleNb][icv_LineNb];
    int input, count;
    char board[2];
    long *evt;
    struct icv196T_Service arg;
    struct icv196T_UserLine arg2;
    struct icv196T_ModuleInfo arg3[icv_ModuleNb];
    struct icv196T_HandleInfo arg4;
    struct icv196T_ModuleInfo *Info_P;
    struct icv196T_HandleLines *Info_H;

    for (i = 0; i < SIZE; i++)
	buff[i] = 0;

    for (i = 0; i < icv_ModuleNb; i++)
	for (j = 0; j < icv_LineNb; j++)
	    status[i][j] = 0;

    if ((service_fd = open(path, O_RDONLY)) < 0) {
	perror("could not open file %s\n");
	return;
    }

    do {
	get_input(&choice, &input);

	switch (input) {

	case 1:

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


	case 2:

	    arg.module = (unsigned char) module;
	    if ((retval = ioctl(service_fd, ICVVME_intcount, &arg)) < 0) {
		printf("retval = %d \n ", retval);
		perror("could not do ioctl\n");
		break;
	    }
	    printf("INTERRUPT COUNTER VALUES ON BOARD NR. %d:\n", module);
	    for (i = 0; i < ICV_nln; i++) {
		printf("line %2d: %3d    ", i, arg.data[i]);
		if (((i+1) % 4) == 0 && i != 0)
		    printf("\n");
	    }
	    printf("\n");
	    break;


	case 3:

	    arg.module = (unsigned char) module;
	    if ((retval = ioctl(service_fd, ICVVME_reenflags, &arg)) < 0) {
		perror("could not do ioctl\n");
		break;
	    }
	    printf("REENABLE INTERRUPT FLAGS ON BOARD NR. %d: ( 1 = reenable on,  2 = reenable off )\n", module);

	    for (i = 0; i < ICV_nln; i++) {
		printf("line %2d: %2d   ", i, arg.data[i]);
		if (((i+1) % 4) == 0 && i != 0)
		    printf("\n");
	    }
	    printf("\n");
	    break;

	case 4:

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

	case 5:

	    Info_P = arg3;
	    if ((retval = ioctl(service_fd, ICVVME_getmoduleinfo, Info_P)) < 0) {
		perror("could not do ioctl\n");
		break;
	    }

	    for (i = 0; i < icv_ModuleNb; i++, Info_P++) {
		printf("MODULE INFO FOR MODULE NR. %d:\n", i);
		if (Info_P -> ModuleFlag == NULL) {
		    printf("module not installed\n\n");
		    continue;
		}
		printf("module base address: 0x%x\n", Info_P -> ModuleInfo.base);

		printf("module address space size in bytes: %d\n", Info_P ->
					     ModuleInfo.size);

		printf("module interrupt vectors:\n");

		for (j = 0; j < icv_LineNb; j++) {
		    printf("line %2d:%d  ", j, Info_P -> ModuleInfo.vector[j]);

		    if (((j+1) % 6) == 0 && j != 0)
			printf("\n");
		}
		printf("\n");

		printf("module interrupt levels:\n");

		for (j = 0; j < icv_LineNb; j++) {
		    printf("line %2d:%d  ", j, Info_P -> ModuleInfo.level[j]);
		    if (((j+1) % 6) == 0 && j != 0)
			printf("\n");
		}
		printf("\n");
		printf("\n");
	    }
	    break;

	case 6:

	    Info_H = (struct icv196T_HandleLines *)&(arg4.handle[0]);
	    for (i = 0; i < ICVVME_MaxChan; i++, Info_H++)  {
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
	    for (i = 1;i < ICVVME_MaxChan; i++, Info_H++) {
		printf("HANDLE INFO FOR HANDLE NR. %d:   ", i);
		if (Info_H -> pid == NULL) {
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

	case 7:

	    printf("number of the group to be set ?\n" );
	    scanf("%d", &grp_nr);
	    arg.data[0] = grp_nr;

	    printf("direction of the group ? (0: input, 1: output)\n" );
	    scanf("%d", &dir);
	    arg.data[1] = dir;

	    arg.module = module;
	    arg.line = i;
	    if ((retval = ioctl(service_fd, ICVVME_setio, &arg)) < 0) {
		perror("could not do ioctl\n");
		break;
	    }
	    break;

	case 8:

	    arg.module = (unsigned char) module;
	    arg.line = i;
	    if ((retval = ioctl(service_fd, ICVVME_readio, &arg)) < 0) {
		perror("could not do ioctl\n");
		break;
	    }
	    dir = arg.data[0];
	    printf("I/O GROUPS (DIRECTION) ON BOARD NR. %d:
		   (0:input, 1:output) \n", module);
	    for (i = 0; i <= 11; i++) {
		printf("group%2d:%2d  ", i, dir & grp_mask);
		dir >>= 1;
		if (((i+1) % 6) == 0 && i != 0)
		    printf("\n");
	    }
	    printf("\n");
	    break;

	case 9:

	    printf("number of line to access \n" );
	    printf("on module nr. %d ? \n", module );

	    scanf("%s", line);

	    i = atoi(line);
	    if ((i < 0) || (i >= icv_LineNb)) {
		printf("line number out of range\n");
		break;
	    }
	    if (status[module][i] == 0) {
		connect(&synchro_fd, module, i, 0);
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


	case 10:

	    printf("number of line to access \n" );
	    printf("on module nr. %d ? \n", module );

	    scanf("%s", line);

	    i = atoi(line);
	    if ((i < 0) || (i >= icv_LineNb)) {
		printf("line number out of range\n");
		break;
	    }
	    if (status[module][i] == 0) {
		connect(&synchro_fd, module, i, 0);
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

	case 11:

	    printf("number of line to access \n" );
	    printf("on module nr. %d ? \n", module );

	    scanf("%s", line);

	    i = atoi(line);
	    if ((i < 0) || (i >= icv_LineNb)) {
		printf("line number out of range\n");
		break;
	    }
	    if (status[module][i] == 0) {
		connect(&synchro_fd, module, i, 0);
		status[module][i] = synchro_fd;
	    }
	    synchro_fd = status[module][i];
	    arg2.group = module;
	    arg2.index = i;
	    if ((retval = ioctl(synchro_fd, ICVVME_enable , &arg2)) < 0) {
		perror("could not do ioctl\n");
		break;
	    }
	    break;

	case 12:

	    printf("number of line to access \n" );
	    printf("on module nr. %d ? \n", module );

	    scanf("%s", line);

	    i = atoi(line);
	    if ((i < 0) || (i >= icv_LineNb)) {
		printf("line number out of range\n");
		break;
	    }
	    if (status[module][i] == 0) {
		connect(&synchro_fd, module, i, 0);
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


	case 13:

	    if (synchro_fd == 0) {
		printf("no line to read from\n");
		break;
	    }
	    for (i=0; i < SIZE; i++) buff[i] = 0;
	    count = 32;
	    if ((retval = read (synchro_fd, buff, count)) < 0) {
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
	    for (i = 0; i < 8; i++, evt++)
	    {
		if (*evt != 0)
		{
		    printf("evt. nr. %2d: 0x%x    ", i, *evt);
		    if (((i+1) % 2) == 0 && i != 0)
			printf("\n");
		}
	    }
	    printf("\n");

	    break;

	case 14:

	    printf("number of line to access \n" );
	    printf("on module nr. %d ? \n", module );

	    scanf("%s", line);

	    i = atoi(line);
	    if ((i < 0) || (i >= icv_LineNb)) {
		printf("line number out of range\n");
		break;
	    }
	    if (status[module][i] == 0) {
		connect(&synchro_fd, module, i, 0);
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
		connect(&synchro_fd, module, i, 0);
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


	case 0:
	    break;


	default:
	    printf("input not correct\n");
	    break;

	}
    } while (input != 0);

    for (i = 0; i < icv_ModuleNb; i++)
	for (j = 0; j < icv_LineNb; j++)
	    if (status[i][j] != 0) disconnect(i,j);

    if ((retval = close(service_fd)) < 0) {
	perror("could not close file %s\n");
	return;
    }
}
