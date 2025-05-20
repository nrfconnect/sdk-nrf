/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <fprotect.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#define STORAGE_PARTITION storage_partition

#define STORAGE_AREA_DEVICE FIXED_PARTITION_DEVICE(STORAGE_PARTITION)
#define STORAGE_AREA_ADDRESS FIXED_PARTITION_OFFSET(STORAGE_PARTITION)

BUILD_ASSERT(STORAGE_AREA_ADDRESS % CONFIG_NRF_RRAM_REGION_ADDRESS_RESOLUTION == 0,
	"storage area isn't properly aligned with protection region");

static uint32_t expected_fatal;
static uint32_t actual_fatal;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	actual_fatal++;
}

ZTEST(fprotect_test, test_writing_to_app_image)
{
	static uint8_t val[] = {0xba, 0x53, 0xba, 0x11};

	printf("Perform a legal flash write to show that it is supported\n");
	int err = flash_write(STORAGE_AREA_DEVICE, STORAGE_AREA_ADDRESS, val, sizeof(val));

	zassert_equal(err, 0, "Failed when writing to legal flash area");
	zassert_equal(expected_fatal, actual_fatal, "An unexpected fatal error has occurred.\n");
	expected_fatal++;
	printf("Enable protection over single storage area block and attempt writing into it\n");
	err = fprotect_area(STORAGE_AREA_ADDRESS, CONFIG_FPROTECT_BLOCK_SIZE);
	zassert_equal(err, 0, "fprotect_area() exited with error");
	printf("The next lines should be a BUS FAULT\n");
	flash_write(STORAGE_AREA_DEVICE, STORAGE_AREA_ADDRESS, val, sizeof(val));
	zassert_unreachable("Should have BUS FAULTed before coming here.");
}

static void check_fatal(void *unused)
{
	zassert_equal(expected_fatal, actual_fatal,
			"The wrong number of fatal error has occurred (e:%d != a:%d).\n",
			expected_fatal, actual_fatal);
}

ZTEST_SUITE(fprotect_test, NULL, NULL, NULL, check_fatal, NULL);
