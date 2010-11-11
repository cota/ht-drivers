#include <stdio.h>
#include <gm/moduletypes.h>
#include <dioaiolib.h>
#include "libvmoddor2dioaio.h"

#define VMODDOR_MAX_BOARDS 64
#define MAX_SIZE 5
#define MAX_OFFSET 6
static short int offset_to_group [MAX_OFFSET][MAX_SIZE] =  /* Translate from offset to groups */
	{{-1, -1, 2, -1, 0}, 				   /* that are used in dioaiolib */	
	{-1, -1, 0, -1, -1},
	{-1, 2, -1, -1, -1},
	{-1, 0, -1, -1, -1},
	{-1, 3, -1, -1, -1},
	{-1, 1, -1, -1, -1}};

static void print_error(int error);

int vmoddor_open(int lun)
{
	if(lun < 0 || lun >= VMODDOR_MAX_BOARDS){
		fprintf(stderr, "libvmoddor2dioaio : Invalid lun %d\n", lun);
		return -1;
	}	
	
	return lun;
}

int vmoddor_close(int lun)
{
	return 0;
}

int vmoddor_write(int lun , struct vmoddor_warg val)
{
	int ret;
	int group_no;
	int size = val.size / 4;

	group_no = offset_to_group[val.offset][size];

	/* The size in dioaiolib means 4 bits in VMODDOR case. */
	ret = DioStrobeWrite(IocVMODDOR, group_no, size, 0, 20, 80, val.data);	
	
	if (ret < 0)
		print_error(ret);

	return ret;
}

static void print_error(int error)
{
	switch(error){
		case -dio_system_errno:
			fprintf(stderr, dio_err1msg);
			break;
		case -dio_bad_key:
			fprintf(stderr, dio_err8msg);
			break;
		case -dio_cannot_find_shared_table:
			fprintf(stderr, dio_err7msg);
			break;
		case -dio_cannot_map_shared_table:
			fprintf(stderr, dio_err6msg);
			break;
		case -dio_odd_group_no_and_even_size:
			fprintf(stderr, dio_err5msg);
			break;
		case -dio_bad_size:
			fprintf(stderr, dio_err4msg);
			break;
		case -dio_bad_group:
			fprintf(stderr, dio_err3msg);
			break;
		case -dio_no_direct_access_allowed:
			fprintf(stderr, dio_err2msg);
			break;
		case -dio_lun_out_of_range:
			fprintf(stderr, dio_err9msg);
			break;
		case -dio_no_module_of_this_type:
			fprintf(stderr, dio_err10msg);
			break;
		default:
			break;
	}

}
