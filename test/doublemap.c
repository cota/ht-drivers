/*
 * doublemap.c
 *
 * Test that several mappings can be done (and removed) successfully from
 * a single process.
 */
#include <unistd.h>
#include <libvmebus.h>
#include <stdio.h>
#include <string.h>

struct vme_mapping	map[2];
void			*virt[2];

static int do_mapping(int am, int dw, int size, int vme_addr, int i)
{
	map[i].am		= am;
	map[i].data_width	= dw;
	map[i].sizel		= size;
	map[i].vme_addrl	= vme_addr;

	virt[i] = vme_map(&map[i], 1);

	if (virt[i] == NULL) {
		perror("vme_map");
		printf("Mapping %d failed\n", i);
		return -1;
	}
	printf("Mapping %d okay\n", i);

	return 0;
}

int main(int argc, char *argv[])
{
	int ret;

	ret = do_mapping(VME_A16_USER, VME_D16, 0x1000, 0x0, 0);
	if (ret)
		return ret;

	ret = do_mapping(VME_A24_USER_DATA_SCT, VME_D16, 0x20000, 0x900000, 1);
	if (ret)
		return ret;

	vme_unmap(&map[0], 0);
	vme_unmap(&map[1], 0);

	return 0;
}
