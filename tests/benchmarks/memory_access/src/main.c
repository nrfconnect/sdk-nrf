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

#if DT_NODE_EXISTS(DT_PATH(soc))
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

static bool region_is_peripherals(const struct reserved_mem_region *region)
{
	const char *label = region->labels[0];

	if (strstr(label, "peripherals") != NULL) {
		return true;
	}

	if (strstr(label, "bellboard") != NULL) {
		return true;
	}

	if (strstr(label, "tcm") != NULL) {
		return true;
	}

	if (strstr(label, "cram") != NULL) {
		return true;
	}

	return false;
}

static bool region_is_write_forbidden(const struct reserved_mem_region *region)
{
	const char *label = region->labels[0];

	if (strstr(label, "write_forbidden") != NULL) {
		return true;
	}

	return false;
}

static bool region_allows_write(const struct reserved_mem_region *region)
{
	if (IS_ENABLED(CONFIG_TEST_WRITE_FROBIDDEN_MEMORY_AREA)) {
		return true;
	}

	return !region_is_write_forbidden(region);
}

static bool region_should_attempt_write(const struct reserved_mem_region *region,
					uint32_t *test_variable_pointer)
{
	bool in_active_cpu_ram =
		(test_variable_pointer > (uint32_t *)region->addr) &&
		(test_variable_pointer < (uint32_t *)(region->addr + region->size));

	return !in_active_cpu_ram && region_allows_write(region);
}

static bool read_test_region(const struct reserved_mem_region *region)
{
	uint32_t test_buffer = 0xAB;
	bool region_ok = true;

	printk("[R] Access given memory area\n");

	for (uint32_t *mem_region = (uint32_t *)region->addr;
	     mem_region < (uint32_t *)(region->addr + region->size); mem_region++) {

		if (memcpy((void *)&test_buffer, (void *)mem_region, sizeof(test_buffer)) == NULL) {
			printk("Given memory region cannot be read: %p\n", mem_region);
			region_ok = false;
		}
	}

	return region_ok;
}

static bool write_test_region(const struct reserved_mem_region *region)
{
	printk("[W] Access given memory area\n");

	if (memset((void *)region->addr, 0xAB, region->size) == NULL) {
		printk("Not able to set the given memory area\n");
		return false;
	}

	printk("Given area written successfully\n");
	return true;
}

int main(void)
{
	bool test_status = true;
	uint32_t test_variable = 0x15;
	uint32_t *test_variable_pointer = &test_variable;

	printk("Hello %s\n", CONFIG_BOARD_TARGET);

	for (size_t i = 0; i < ARRAY_SIZE(reserved_regions); i++) {

		if (!region_has_tested_cpu_label(&reserved_regions[i])) {
			continue;
		}

		if (region_is_peripherals(&reserved_regions[i])) {
			continue;
		}

		printk("[%s] address=0x%08x, size=0x%x (%u bytes)\n", reserved_regions[i].labels[0],
		       reserved_regions[i].addr, reserved_regions[i].size,
		       reserved_regions[i].size);

		bool attempt_write =
			region_should_attempt_write(&reserved_regions[i], test_variable_pointer);

		if (!attempt_write) {
			/* Active CPU data RAM must not be written (would crash).
			 * Write-forbidden regions are read-only unless
			 * CONFIG_TEST_WRITE_FROBIDDEN_MEMORY_AREA is enabled.
			 */
			if (!read_test_region(&reserved_regions[i])) {
				test_status = false;
			}

		} else {
			if (!write_test_region(&reserved_regions[i])) {
				test_status = false;
			}

			if (!read_test_region(&reserved_regions[i])) {
				test_status = false;
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
