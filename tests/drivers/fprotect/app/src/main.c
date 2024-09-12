/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <pm_config.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>

#define STORAGE_LAST_WORD (PM_SETTINGS_STORAGE_END_ADDRESS - 4)
#define IMAGE_LAST_WORD (PM_APP_END_ADDRESS - 4)

static uint32_t expected_fatal;
static uint32_t actual_fatal;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	actual_fatal++;
}

ZTEST(fprotect_test, test_writing_to_app_image)
{
	/* Prepare for flash writes */
	int err;
	const struct device *flash_dev;
	static uint8_t val[] = {0xba, 0x53, 0xba, 0x11};

	flash_dev = DEVICE_DT_GET(DT_NODELABEL(PM_APP_DEV));
	zassert_true(device_is_ready(flash_dev), "Flash device not ready");

	printf("Perform a legal flash write to show that it is supported\n");
	err = flash_write(flash_dev, STORAGE_LAST_WORD, val, sizeof(val));
	zassert_equal(err, 0, "Failed when writing to legal flash area");

	zassert_equal(expected_fatal, actual_fatal, "An unexpected fatal error has occurred.\n");
	expected_fatal++;

	printf("Next we write inside the image and expect a failure\n");
	printf("The next lines should be a BUS FAULT\n");
	flash_write(flash_dev, IMAGE_LAST_WORD, val, sizeof(val));
	zassert_unreachable("Should have BUS FAULTed before coming here.");
}

static void check_fatal(void *unused)
{
	zassert_equal(expected_fatal, actual_fatal,
			"The wrong number of fatal error has occurred (e:%d != a:%d).\n",
			expected_fatal, actual_fatal);
}

ZTEST_SUITE(fprotect_test, NULL, NULL, NULL, check_fatal, NULL);
