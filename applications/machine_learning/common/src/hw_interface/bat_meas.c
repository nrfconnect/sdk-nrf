/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <app_event_manager.h>

#include "battery_event.h"
#include "battery_def.h"

#define MODULE bat_meas
#include <caf/events/module_state_event.h>

LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_BATTERY_MEAS_LOG_LEVEL);

#define BATTERY_WORK_INTERVAL (CONFIG_ML_APP_BATTERY_MEAS_POLL_INTERVAL_MS / 2)

static const struct adc_dt_spec adc_batt = ADC_DT_SPEC_GET_BY_NAME(DT_PATH(zephyr_user), batt_meas);
static const struct gpio_dt_spec gpio_batt =
	GPIO_DT_SPEC_GET_BY_IDX_OR(DT_NODELABEL(batt_meas_pin), gpios, 0, {0});

static bool batt_pin_supported;
static struct k_work_delayable battery_read_work;
static struct adc_sequence batt_read;
static uint16_t battery_voltage;

static struct k_poll_signal adc_signal = K_POLL_SIGNAL_INITIALIZER(adc_signal);
static struct k_poll_event adc_event = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
								K_POLL_MODE_NOTIFY_ONLY,
								&adc_signal);

static void battery_level_notify(void)
{
	struct battery_level_event *event = new_battery_level_event();
	int32_t voltage = battery_voltage;
	uint8_t level;

	int err = adc_raw_to_millivolts_dt(&adc_batt, &voltage);
	if (err) {
		LOG_ERR("Failed to covert battery voltage, err: %d", err);
		return;
	}

	if (voltage > CONFIG_ML_APP_BATTERY_MEAS_MAX_LEVEL) {
		level = 100;
	} else if (voltage < CONFIG_ML_APP_BATTERY_MEAS_MIN_LEVEL) {
		level = 0;
		LOG_WRN("Low battery");
	} else {
		/* Using lookup table to convert voltage[mV] to SoC[%] */
		size_t lut_id = (voltage - CONFIG_ML_APP_BATTERY_MEAS_MIN_LEVEL
				 + (CONFIG_ML_APP_BATTERY_MEAS_VOLTAGE_TO_SOC_DELTA >> 1))
				 / CONFIG_ML_APP_BATTERY_MEAS_VOLTAGE_TO_SOC_DELTA;
		level = battery_voltage_to_soc[lut_id];
	}

	event->level = level;

	APP_EVENT_SUBMIT(event);
}

static void battery_read_handler(struct k_work *work)
{
	int err;
	static bool calibrated;
	static bool read_pending;

	if (likely(calibrated)) {
		batt_read.calibrate = false;
	} else {
		batt_read.calibrate = true;
	}

	if (!read_pending) {
		err = adc_read_async(adc_batt.dev, &batt_read, &adc_signal);
		if (err) {
			LOG_ERR("Failed to read battery voltage, err: %d", err);
		}

		read_pending = true;
	} else {
		err = k_poll(&adc_event, 1, K_NO_WAIT);
		if (err) {
			LOG_WRN("Failed to read battery value");
		} else {
			battery_level_notify();
			read_pending = false;
		}
	}

	k_work_schedule(&battery_read_work, K_MSEC(BATTERY_WORK_INTERVAL));
}

static int battery_measurement_start(void)
{
	if (batt_pin_supported) {
		int err = gpio_pin_set_dt(&gpio_batt, 1);
		if (err) {
			LOG_ERR("Failed to activate battery pin");
			return err;
		}
	}

	k_work_schedule(&battery_read_work, K_NO_WAIT);

	return 0;
}

static int adc_init(void)
{
	int err;

	if (!adc_is_ready_dt(&adc_batt)) {
		LOG_ERR("ADC for battery measurement is not ready");
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&adc_batt);
	if (err) {
		LOG_ERR("Failed to configure ADC channel, err: %d", err);
		return err;
	}

	err = adc_sequence_init_dt(&adc_batt, &batt_read);
	if (err) {
		LOG_ERR("Failed to initialize ADC read sequence, err: %d", err);
		return err;
	}

	batt_read.buffer = &battery_voltage;
	batt_read.buffer_size = sizeof(battery_voltage);

	return 0;
}

static int init(void)
{
	int err;

	if (gpio_is_ready_dt(&gpio_batt)) {
		err = gpio_pin_configure_dt(&gpio_batt, GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("Failed to configure Battery measurement GPIO pin, err: %d", err);
			return err;
		}

		batt_pin_supported = true;
	}

	err = adc_init();
	if (err) {
		return err;
	}

	k_work_init_delayable(&battery_read_work, battery_read_handler);

	return battery_measurement_start();
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		static bool initialized;

		__ASSERT_NO_MSG(!initialized);
		initialized = true;

		int err = init();

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
		} else {
			module_set_state(MODULE_STATE_READY);
		}
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	/* Should not happen. */
	__ASSERT_NO_MSG(false);
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
