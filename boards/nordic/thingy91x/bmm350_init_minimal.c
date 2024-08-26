/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bmm350_init_minimal, CONFIG_BOARD_LOG_LEVEL);

#if	DT_NODE_HAS_STATUS(DT_NODELABEL(ldsw_sensors), okay) && \
	DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)

#define LDSW_SENSORS DEVICE_DT_GET(DT_NODELABEL(ldsw_sensors))
#define BMM350_I2C_DEVICE DEVICE_DT_GET(DT_NODELABEL(i2c2))
#define BMM350_I2C_ADDRESS 0x14

#define BMM350_REG_OTP_CMD_REG 0x50
#define BMM350_OTP_CMD_PWR_OFF_OTP 0x80
#define BMM350_START_UP_TIME_FROM_POR 3000

static int bmm350_init_minimal(void)
{
	if (regulator_is_enabled(LDSW_SENSORS)) {
		k_sleep(K_USEC(BMM350_START_UP_TIME_FROM_POR));

		/* Turn off the OTP memory on BMM350 to save power */
		return i2c_reg_write_byte(BMM350_I2C_DEVICE, BMM350_I2C_ADDRESS,
					  BMM350_REG_OTP_CMD_REG, BMM350_OTP_CMD_PWR_OFF_OTP);
	}
	return 0;
}

SYS_INIT(bmm350_init_minimal, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ldsw_sensors), okay) && \
	* DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
	*/
