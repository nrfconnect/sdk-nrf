/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include <string.h>

#if DT_NODE_EXISTS(DT_PATH(reserved_memory))
#define CPU_MEMORY DT_PATH(reserved_memory)
#elif DT_NODE_EXISTS(DT_PATH(soc))
#define CPU_MEMORY DT_PATH(soc)
#else
#error "Cpu memory not defined in DTS"
#endif

struct reserved_mem_region {
	uint32_t addr;
	size_t size;
	const char *labels[5];
};

#define MEMORY_REGION_INIT(node_id)                                                                \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, reg),                                                 \
		   ({                                                                              \
			    .labels = DT_NODELABEL_STRING_ARRAY(node_id),                          \
			    .addr = (uint32_t)DT_REG_ADDR(node_id),                                \
			    .size = (size_t)DT_REG_SIZE(node_id),                                  \
		    },))

static const struct reserved_mem_region reserved_regions[] = {
	DT_FOREACH_CHILD(CPU_MEMORY, MEMORY_REGION_INIT)};

static bool region_has_tested_cpu_label(const struct reserved_mem_region *region)
{
	char *cpu_name = strrchr(CONFIG_BOARD_QUALIFIERS, '/') + 1;

	if (strstr(region->labels[0], cpu_name) != NULL) {
		return true;
	} else {
		return false;
	}
}

int main(void)
{
	bool test_status = true;
	uint32_t test_variable = 0x15;
	uint32_t *test_variable_pointer = &test_variable;
	uint32_t test_buffer;

	printk("Hello %s\n", CONFIG_BOARD_TARGET);

	for (size_t i = 0; i < ARRAY_SIZE(reserved_regions); i++) {

		if (!region_has_tested_cpu_label(&reserved_regions[i])) {
			continue;
		}

		printk("[%s] address=0x%08x, size=0x%x (%u bytes)\n", reserved_regions[i].labels[0],
		       reserved_regions[i].addr, reserved_regions[i].size,
		       reserved_regions[i].size);

		if ((test_variable_pointer > (uint32_t *)reserved_regions[i].addr) &&
		    (test_variable_pointer <
		     (uint32_t *)(reserved_regions[i].addr + reserved_regions[i].size))) {
			/* If current memory area is the active CPU data RAM - do not write it.
			 * Such action will casue a crash.
			 * Instead just test read access
			 */
			printk("[R] Access given memory area\n");

			for (uint32_t *mem_region = (uint32_t *)reserved_regions[i].addr;
			     mem_region <
			     (uint32_t *)(reserved_regions[i].addr + reserved_regions[i].size);
			     mem_region++) {

				if (memcpy((void *)&test_buffer, (void *)mem_region,
					   sizeof(uint32_t)) == NULL) {
					printk("Given memory region cannot be read: %p\n",
					       mem_region);
					test_status = false;
				}
			}

		} else {
			printk("[W] Access given memory area\n");

			if (memset((void *)reserved_regions[i].addr, 0xAB,
				   reserved_regions[i].size) == NULL) {
				printk("Not able to set the given memory area\n");
				test_status = false;
			} else {
				printk("Given area written successfully\n");
			}
		}
	}

	if (test_status) {
		printk("Test PASS\n");
	} else {
		printk("Test FAIL\n");
	}

	return 0;
}
