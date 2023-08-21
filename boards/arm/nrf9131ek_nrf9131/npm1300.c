/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

LOG_MODULE_REGISTER(board_secure, CONFIG_BOARD_LOG_LEVEL);

#define CHECKERR if (err) {LOG_ERR("I2C error: %d", err); return err;}

static const struct i2c_dt_spec pmic = I2C_DT_SPEC_GET(DT_NODELABEL(pmic_main));

static int pmic_write_reg(uint16_t address, uint8_t value)
{
	uint8_t buf[] = {
		address >> 8,
		address & 0xFF,
		value,
	};

	return i2c_write_dt(&pmic, buf, ARRAY_SIZE(buf));
}

static int thingy91x_board_init(void)
{
	int err = 0;

	if (!device_is_ready(pmic.bus)) {
		LOG_ERR("cannot reach PMIC!");
		return -ENODEV;
	}

	// disable charger for config
	err = pmic_write_reg(0x0305, 0x03); CHECKERR;

	// set VBUS current limit 500mA 
	err = pmic_write_reg(0x0201, 0x00); CHECKERR;
	err = pmic_write_reg(0x0202, 0x00); CHECKERR;
	err = pmic_write_reg(0x0200, 0x01); CHECKERR;

	// enable charger
	err = pmic_write_reg(0x0304, 0x03); CHECKERR;

	return err;
}

SYS_INIT(thingy91x_board_init, POST_KERNEL, CONFIG_BOARD_INIT_PRIORITY);
