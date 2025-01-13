/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

static const struct device *i2c = DEVICE_DT_GET(DT_ALIAS(sensor_bme688));

static bool suspend_req;

bool self_suspend_req(void)
{
	suspend_req = true;
	return true;
}

void thread_definition(void)
{
	int ret;
	uint8_t value;

	while (1) {
		ret = i2c_reg_read_byte(i2c, 0x76, 0x75, &value);
		if (suspend_req) {
			suspend_req = false;
			k_thread_suspend(k_current_get());
		}
		if (ret < 0) {
			printk("Failure in reading byte %d", ret);
			return;
		}
	};
};
