/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

/* Note: logging is normally disabled for this test
 * Enable only for debugging purposes
 */
LOG_MODULE_REGISTER(idle_twim);

#define SENSOR_NODE    DT_COMPAT_GET_ANY_STATUS_OKAY(bosch_bme680)
#define I2C_TEST_NODE  DT_PARENT(SENSOR_NODE)
#define DEVICE_ADDRESS (uint8_t) DT_REG_ADDR(SENSOR_NODE)

#define CHIP_ID_REGISTER_ADDRESS    0xD0
#define VARIANT_ID_REGISTER_ADDRESS 0xF0

static const struct device *const i2c_device = DEVICE_DT_GET(I2C_TEST_NODE);

/*
 * Helper method for reading the BME688 I2C registers
 */
static uint8_t read_sensor_register(uint8_t register_address)
{
	int rc;
	uint8_t response;

	rc = i2c_reg_read_byte(i2c_device, DEVICE_ADDRESS, register_address, &response);
	__ASSERT_NO_MSG(rc == 0);
	printk("I2C read reg, addr: 0x%x, val: 0x%x\n", register_address, response);
	return response;
}

int main(void)
{
	uint8_t response;
	uint32_t i2c_config = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

	printk("Device address 0x%x\n", DEVICE_ADDRESS);
	printk("I2C speed setting: %d\n", I2C_SPEED_STANDARD);

	response = i2c_configure(i2c_device, i2c_config);
	if (response != 0) {
		printk("I2C configuration failed%d\n", response);
		return -1;
	}

	while (1) {
		response = read_sensor_register(CHIP_ID_REGISTER_ADDRESS);
		printk("Chip_Id: %d\n", response);

		response = read_sensor_register(VARIANT_ID_REGISTER_ADDRESS);
		printk("Variant_Id: %d\n", response);
		k_msleep(2000);
	}

	return 0;
}
