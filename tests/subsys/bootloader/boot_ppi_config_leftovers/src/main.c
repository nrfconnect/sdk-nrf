/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>

#define SUB_REGION_START_OFFSET 0x80U
#define SUB_REGION_END_OFFSET   0x100U

#define PUB_REGION_START_OFFSET 0x180U
#define PUB_REGION_END_OFFSET   0x200U

#define EARLY_CHECK_PRIORITY 1

struct periph_test_info {
	bool is_zeroed;
	void *base_address;
};

#define GET_PERIPH(node_id, prop, idx) \
	{ 0, (void *)DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)) }

static struct periph_test_info peripherals[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_PATH(zephyr_user), test_periphs, GET_PERIPH, (,))
};

static bool is_region_zeros(void *start, void *end)
{
	for (uint32_t *addr = (uint32_t *)start; addr < (uint32_t *)end; addr++) {
		if (*addr != 0U) {
			return false;
		}
	}
	return true;
}

static bool is_publish_subscribe_region_cleared(void *peripheral_address)
{
	void *sub_start_addr = (uint8_t *)peripheral_address + SUB_REGION_START_OFFSET;
	void *sub_end_addr   = (uint8_t *)peripheral_address + SUB_REGION_END_OFFSET;

	void *pub_start_addr = (uint8_t *)peripheral_address + PUB_REGION_START_OFFSET;
	void *pub_end_addr   = (uint8_t *)peripheral_address + PUB_REGION_END_OFFSET;

	return is_region_zeros(sub_start_addr, sub_end_addr) &&
	       is_region_zeros(pub_start_addr, pub_end_addr);
}

static int check_publish_subscribe_registers(void)
{
	for (int i = 0; i < ARRAY_SIZE(peripherals); i++) {
		peripherals[i].is_zeroed =
			is_publish_subscribe_region_cleared(peripherals[i].base_address);
	}
	return 0;
}

ZTEST(boot_ppi_config_leftovers, test_publish_subscribe_leftovers)
{
	for (int i = 0; i < ARRAY_SIZE(peripherals); i++) {
		zexpect_true(peripherals[i].is_zeroed,
			     "Publish/subscribe register is not 0, peripheral base address: %p "
			     "(index %d)",
			     peripherals[i].base_address, i);
	}
}

SYS_INIT(check_publish_subscribe_registers, EARLY, EARLY_CHECK_PRIORITY);
ZTEST_SUITE(boot_ppi_config_leftovers, NULL, NULL, NULL, NULL, NULL);
