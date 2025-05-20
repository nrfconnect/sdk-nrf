/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

struct test_partition_t {
	int slot;
	uint32_t start_address;
	uint32_t size;
};

/*
 * List of partitions to check, this must be static values which correspond to the static PM file
 * without using macros for the start address or size fields, to ensure values are correct
 */
static struct test_partition_t partitions[] = {
	{
		.slot = FIXED_PARTITION_ID(dummy_1),
		.start_address = 0x98000,
		.size = 0x1000,
	},
	{
		.slot = FIXED_PARTITION_ID(dummy_2),
		.start_address = 0x99000,
		.size = 0x1000,
	},
	{
		.slot = FIXED_PARTITION_ID(dummy_3),
		.start_address = 0x9a000,
		.size = 0x66000,
	},
};

ZTEST(static_pm_file, test_partitions)
{
	const struct flash_area *fa;
	int rc;
	uint8_t i = 0;

	while (i < ARRAY_SIZE(partitions)) {
		fa = NULL;
		rc = flash_area_open(partitions[i].slot, &fa);
		flash_area_close(fa);

		zassert_equal(rc, 0, "Expected partition open to be successful");
		zassert_equal(fa->fa_off, partitions[i].start_address,
			      "Expected partition offset mismatch");
		zassert_equal(fa->fa_size, partitions[i].size, "Expected partition size mismatch");


		++i;
	}

	zassert_equal(i, 3, "Expected partition count mismatch");
}

ZTEST_SUITE(static_pm_file, NULL, NULL, NULL, NULL, NULL);
