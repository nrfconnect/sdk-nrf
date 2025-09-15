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
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_qspi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_qspi_nor)
#elif DT_HAS_COMPAT_STATUS_OKAY(jedec_mspi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_mspi_nor)
#else
#error Unsupported flash driver
#define FLASH_NODE DT_INVALID_NODE
#endif

#define TEST_AREA_OFFSET 0xff000
#define EXPECTED_SIZE	 512
#define FLASH_PAGE_SIZE	 4096

uint8_t buf[EXPECTED_SIZE];

static const struct device *flash_dev = DEVICE_DT_GET(FLASH_NODE);

static bool suspend_req;

bool self_suspend_req(void)
{
	suspend_req = true;
	return true;
}

void thread_definition(void)
{
	int rc;
	uint8_t fill_value = 0x00;

	while (1) {
		if (suspend_req) {
			suspend_req = false;
			k_thread_suspend(k_current_get());
		}

		rc = flash_erase(flash_dev, TEST_AREA_OFFSET, FLASH_PAGE_SIZE);
		__ASSERT(rc == 0, "flash_erase %d\n", rc);
		rc = flash_fill(flash_dev, fill_value, TEST_AREA_OFFSET, EXPECTED_SIZE);
		__ASSERT(rc == 0, "flash_fill %d\n", rc);
		rc = flash_read(flash_dev, TEST_AREA_OFFSET, buf, EXPECTED_SIZE);
		__ASSERT(rc == 0, "flash_read %d\n", rc);
		for (size_t i = 0; i < EXPECTED_SIZE; i++) {
			__ASSERT(buf[i] == fill_value, "flash value %d but expected %d\n", buf[i],
				 fill_value);
			buf[i] = (uint8_t)i;
		}
		fill_value++;
	};
};
