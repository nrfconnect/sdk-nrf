/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_NPMX_NPMX_DRIVER_H__
#define ZEPHYR_DRIVERS_NPMX_NPMX_DRIVER_H__

#include <npmx.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

struct npmx_data {
	const struct device *dev;
	npmx_instance_t npmx_instance;
	struct k_work work;
	struct gpio_callback gpio_cb;
};

struct npmx_config {
	const struct i2c_dt_spec i2c;
	const struct gpio_dt_spec int_gpio;
};

/**
 * @brief Register callback handler to specified callback type event/error
 * If no callback is registered, generic_cb is called.
 * This function should be called in other drivers init function e.g. charger, vbus,
 * with initialization priority higher than @ref CONFIG_NPMX_INIT_PRIORITY
 *
 * @param[in] cb   The function callback pointer to be registered
 * @param[in] type The type of registered function callback pointer
 * @return true  Successfully registered callback
 * @return false Callback is NULL or npmx core not initialized yet
 */
bool npmx_driver_register_cb(npmx_callback_t cb, npmx_callback_type_t type);

/**
 * @brief Initialize gpio interrupt to handle events from nPM device
 * 
 * @param[in] dev The pointer to nPM Zephyr device
 * @return int Return error code, 0 when succeed
 */
int npmx_gpio_interrupt_init(const struct device *dev);

/**
 * @brief Enable event interrupt
 * This function should be called in other drivers init function e.g. charger, vbus,
 * with initialization priority higher than @ref CONFIG_NPMX_INIT_PRIORITY
 *
 * @param[in] event      Specified event group type
 * @param[in] flags_mask Specified bits in event group, see npmx_event_group_xxx_t for selected event
 * @return true  Interrupt enabled successfully
 * @return false Interrupt not enabled
 */
bool npmx_driver_event_interrupt_enable(npmx_event_group_t event, uint8_t flags_mask);

#endif /* ZEPHYR_DRIVERS_NPMX_NPMX_DRIVER_H__ */
