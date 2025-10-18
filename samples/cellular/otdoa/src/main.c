/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <otdoa_al/otdoa_little_fs.h>

#include "otdoa_gpio.h"
#include "otdoa_sample_app.h"

LOG_MODULE_REGISTER(otdoa_sample, LOG_LEVEL_DBG);

int main(void)
{
	LOG_INF("OTDOA Sample Application started");

	LOG_DBG("Mounting littleFS");
	if (0 == mount_little_fs()) {
#ifdef FS_TESTS
		test_little_fs();
#endif
	}

	init_button();
	set_blink_mode(MED);

	return otdoa_sample_main();
}
