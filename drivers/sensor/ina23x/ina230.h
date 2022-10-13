/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright (c) 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA23X_INA230_H_
#define ZEPHYR_DRIVERS_SENSOR_INA23X_INA230_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define INA230_REG_CONFIG     0x00
#define INA230_REG_SHUNT_VOLT 0x01
#define INA230_REG_BUS_VOLT   0x02
#define INA230_REG_POWER      0x03
#define INA230_REG_CURRENT    0x04
#define INA230_REG_CALIB      0x05
#define INA230_REG_MASK       0x06
#define INA230_REG_ALERT      0x07

struct ina230_data {
	const struct device *dev;
	uint16_t current;
	uint16_t bus_voltage;
	uint16_t power;
#ifdef CONFIG_INA230_TRIGGER
	const struct device *gpio;
	struct gpio_callback gpio_cb;
	struct k_work work;
	sensor_trigger_handler_t handler_alert;
#endif  /* CONFIG_INA230_TRIGGER */
};

struct ina230_config {
	struct i2c_dt_spec bus;
	uint16_t config;
	uint16_t current_lsb;
	uint16_t rshunt;
#ifdef CONFIG_INA230_TRIGGER
	bool trig_enabled;
	uint16_t mask;
	const struct gpio_dt_spec gpio_alert;
	uint16_t alert_limit;
#endif  /* CONFIG_INA230_TRIGGER */
};

int ina230_trigger_mode_init(const struct device *dev);
int ina230_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

#endif /* ZEPHYR_DRIVERS_SENSOR_INA23X_INA230_H_ */
