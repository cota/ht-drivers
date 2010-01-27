/*
 * berrtest.c - Bus Error Test
 *
 * Generate a few bus errors and see if they are properly handled.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <libvmebus.h>

/* Number of (faulty) accesses to be done */
#define ACCESSES	4

#define AM		0x9
#define DW		VME_D32
#define VME_ADDR	0x1000000
#define MAP_SIZE	0x1000

int main(int argc, char *argv[])
{
	struct vme_mapping mapping;
	uint64_t vme_addr;
	uint32_t *virt;
	int berr;
	int i;

	memset(&mapping, 0, sizeof(struct vme_mapping));

	mapping.sizel		= MAP_SIZE;
	mapping.vme_addrl	= VME_ADDR;
	mapping.am		= AM;
	mapping.data_width	= DW;

	virt = vme_map(&mapping, 1);
	if (virt == NULL) {
		perror("vme_map");
		printf("Mapping failed\n");
		exit(EXIT_FAILURE);
	}

	vme_addr = mapping.vme_addrl;

	for (i = 0; i < ACCESSES; i++, virt++, vme_addr += sizeof(uint32_t)) {
		/* cause a VME bus error */
		*virt = i;

		/* sleep so that the bus error gets picked up by the driver */
		sleep(1);

		berr = vme_bus_error_check_clear(&mapping, vme_addr);
		if (berr < 0) {
			printf("vmebus: check_clear_berr failed\n");
			exit(EXIT_FAILURE);
		}
		if (berr) {
			printf("vmebus: bus error @ 0x%llx\n",
				(unsigned long long)vme_addr);
		}

		berr = vme_bus_error_check_clear(&mapping, vme_addr);
		if (berr < 0) {
			printf("vmebus: check_clear_berr failed\n");
			exit(EXIT_FAILURE);
		}
		if (berr) {
			printf("BUG: bus error reported twice.\n");
			exit(EXIT_FAILURE);
		}
	}
	vme_unmap(&mapping, 1);
	return 0;
}
