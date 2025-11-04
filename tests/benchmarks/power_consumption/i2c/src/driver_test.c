/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define I2C_DEV_ADR (0x76)
#define REG_CHIP_ID_ADR (0xd0)
#define REG_CHIP_ID_VAL (0x61)

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
		if (suspend_req) {
			suspend_req = false;
			k_thread_suspend(k_current_get());
		}

		ret = i2c_reg_read_byte(i2c, I2C_DEV_ADR, REG_CHIP_ID_ADR, &value);
		__ASSERT(ret == 0, "i2c_reg_read_byte() returned %d", ret);

		/* Check if communication with bme688 was successful. */
		__ASSERT(value == REG_CHIP_ID_VAL,
			"Invalid CHIP_ID, got %u, expected %u\n", value, REG_CHIP_ID_VAL);
		value = 0;
	};
};
