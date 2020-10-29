/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <pm_config.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>

#define STORAGE_LAST_WORD (PM_SETTINGS_STORAGE_END_ADDRESS - 4)
#define IMAGE_LAST_WORD (PM_APP_END_ADDRESS - 4)


static void test_writing_to_app_image(void)
{
	/* Prepare for flash writes */
	int err;
	const struct device *flash_dev;
	static uint8_t val[] = {0xba, 0x53, 0xba, 0x11};

	flash_dev = device_get_binding(PM_APP_DEV_NAME);
	zassert_not_null(flash_dev, "Could not load flash driver");

	printf("Perform a legal flash write to show that it is supported\n");
	flash_write_protection_set(flash_dev, false);
	err = flash_write(flash_dev, STORAGE_LAST_WORD, val, sizeof(val));
	zassert_equal(err, 0, "Failed when writing to legal flash area");

	printf("Next we write inside the image and expect a failure\n");
	printf("The next lines should be a BUS FAULT\n");
	flash_write_protection_set(flash_dev, false);
	err = flash_write(flash_dev, IMAGE_LAST_WORD, val, sizeof(val));
	zassert_equal(err, 0, "Failed when writing to legal flash area");

}


void test_main(void)
{
	ztest_test_suite(fprotect_test,
			 ztest_unit_test(test_writing_to_app_image)
			 );

	ztest_run_test_suite(fprotect_test);
}
