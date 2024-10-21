/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kernel.h>

#if DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_spi_nor)
#elif DT_HAS_COMPAT_STATUS_OKAY(jedec_mspi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_mspi_nor)
#else
#error Unsupported flash driver
#define FLASH_NODE DT_INVALID_NODE
#endif

#define TEST_AREA_OFFSET	0xff000
#define EXPECTED_SIZE	512
uint8_t buf[EXPECTED_SIZE];

static const struct device *flash_dev = DEVICE_DT_GET(FLASH_NODE);


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
