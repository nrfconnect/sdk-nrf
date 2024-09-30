/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kernel.h>

#define TEST_AREA_OFFSET	0xff000
#define EXPECTED_SIZE	512
uint8_t buf[EXPECTED_SIZE];

static const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, jedec_spi_nor));


void thread_definition(void)
{
	int ret;

	while (1) {
		ret = flash_read(flash_dev, TEST_AREA_OFFSET, buf, EXPECTED_SIZE);
		if (ret < 0) {
			printk("Failure in reading byte %d", ret);
			return;
		}
	};
};
