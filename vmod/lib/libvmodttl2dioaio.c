#include <stdio.h>
#include <gm/moduletypes.h>
#include "libvmodttl2dioaio.h"
#include <dioaiolib.h>


#define VMODTTL_MAX_BOARDS 64          /* max. # of boards supported   */

static void print_error(int error);

int vmodttl_open(int lun)
{
	if(lun < 0 || lun >= VMODTTL_MAX_BOARDS){
		fprintf(stderr, "libvmodttl2dioaio : Invalid lun %d\n", lun);
		return -1;
	}
	return lun;
}

int vmodttl_close(int lun)
{
	return 0;
}

int vmodttl_io_config(int lun, struct vmodttl_config conf)
{
	int ret;

	ret = DioGlobalReset(IocVMODTTL, lun);
	if (ret < 0){
          print_error(ret);
          return ret;
     	}

	ret = DioChannelSet(IocVMODTTL, lun, 0, 1, conf.dir_a, conf.inverting_logic, conf.inverting_logic);
	if (ret < 0){
		print_error(ret);
		return ret;
	}

	ret = DioChannelSet(IocVMODTTL, lun, 1, 1, conf.dir_b, conf.inverting_logic, conf.inverting_logic);
	if (ret < 0){
		print_error(ret);
		return ret;
	}

	return 0;
}

int vmodttl_io_chan_config(int lun, enum vmodttl_channel chan, struct vmodttl_config conf)
{
	int ret = 0;

	switch(chan) {

	case VMOD_TTL_CHANNEL_A:
		ret = DioChannelSet(IocVMODTTL, lun, 0, 1, conf.dir_a, conf.inverting_logic, conf.inverting_logic);
		break;
	case VMOD_TTL_CHANNEL_B:
		ret = DioChannelSet(IocVMODTTL, lun, 1, 1, conf.dir_b, conf.inverting_logic, conf.inverting_logic);
		break;
	case VMOD_TTL_CHANNELS_AB:
		ret = vmodttl_io_config(lun, conf);
		break;
	default:
		fprintf(stderr, "libvmodttl2dioaio : Invalid channel (%d) to configure\n", chan);
	}

	if (ret < 0){
		print_error(ret);
		return ret;
	}
	return 0;
}

int vmodttl_write(int lun, enum vmodttl_channel chan, int val)
{
	int ret;
	
	if (chan != VMOD_TTL_CHANNELS_AB){
		ret = DioChannelWrite(IocVMODTTL, lun, chan, 1, val);
	} else {
		ret = DioChannelWrite(IocVMODTTL, lun, 0, 1, val & 0xff);
		ret = DioChannelWrite(IocVMODTTL, lun, 1, 1, (val >> 8) & 0xff);
	}

	if (ret < 0)
	{
		print_error(ret);
	}	

	return ret;
}

int vmodttl_read(int lun, enum vmodttl_channel chan, int *val)
{
	int ret;

	if (chan != VMOD_TTL_CHANNELS_AB){
		ret = DioChannelRead(IocVMODTTL, lun, chan, 1, (int *)val);
	} else {
		int tmp = 0;
		int tmp2 = 0;

		ret = DioChannelRead(IocVMODTTL, lun, 0, 1, (int *)&tmp);
		ret = DioChannelRead(IocVMODTTL, lun, 1, 1, (int *)&tmp2);
		*val = tmp + (tmp2 << 8);
		 
	}
	return ret;
}

int vmodttl_pattern(int lun, enum vmodttl_channel chan, int pos, enum vmodttl_conf_pattern bit_pattern)
{
	/* Not implemented */
	return 0;
}

int vmodttl_read_config(int lun, struct vmodttl_config *conf)
{
	/* Not implemented */
	return 0;
}

int vmodttl_read_device(int lun, unsigned char buffer[2])
{
	/* Not implemented */
	return 0;
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

