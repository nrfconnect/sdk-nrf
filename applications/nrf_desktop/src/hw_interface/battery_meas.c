/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <soc.h>
#include <device.h>
#include <drivers/adc.h>
#include <drivers/gpio.h>
#include <sys/atomic.h>

#include <hal/nrf_saadc.h>

#include "event_manager.h"
#include "power_event.h"
#include "battery_event.h"
#include "battery_def.h"

#define MODULE battery_meas
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BATTERY_MEAS_LOG_LEVEL);

#define ADC_DEVICE_NAME		DT_LABEL(DT_ALIAS(adc_0))
#define ADC_RESOLUTION		12
#define ADC_OVERSAMPLING	4 /* 2^ADC_OVERSAMPLING samples are averaged */
#define ADC_MAX 		4096
#define ADC_GAIN		BATTERY_MEAS_ADC_GAIN
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_REF_INTERNAL_MV	600UL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_CHANNEL_ID		0
#define ADC_CHANNEL_INPUT	BATTERY_MEAS_ADC_INPUT
/* ADC asynchronous read is scheduled on odd works. Value read happens during
 * even works. This is done to avoid creating a thread for battery monitor.
 */
#define BATTERY_WORK_INTERVAL	(CONFIG_DESKTOP_BATTERY_MEAS_POLL_INTERVAL_MS / 2)

#if IS_ENABLED(CONFIG_DESKTOP_BATTERY_MEAS_HAS_VOLTAGE_DIVIDER)
#define BATTERY_VOLTAGE(sample)	(sample * BATTERY_MEAS_VOLTAGE_GAIN	\
		* ADC_REF_INTERNAL_MV					\
		* (CONFIG_DESKTOP_BATTERY_MEAS_VOLTAGE_DIVIDER_UPPER	\
		+ CONFIG_DESKTOP_BATTERY_MEAS_VOLTAGE_DIVIDER_LOWER)	\
		/ CONFIG_DESKTOP_BATTERY_MEAS_VOLTAGE_DIVIDER_LOWER / ADC_MAX)
#else
#define BATTERY_VOLTAGE(sample) (sample * BATTERY_MEAS_VOLTAGE_GAIN	\
				 * ADC_REF_INTERNAL_MV / ADC_MAX)
#endif

static struct device *adc_dev;
static s16_t adc_buffer;
static bool adc_async_read_pending;

static struct k_delayed_work battery_lvl_read;
static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event  async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);

static struct device *gpio_dev;

static atomic_t active;
static bool sampling;

static int init_adc(void)
{
	adc_dev = device_get_binding(ADC_DEVICE_NAME);
	if (!adc_dev) {
		LOG_ERR("Cannot get ADC device");
		return -ENXIO;
	}

	static const struct adc_channel_cfg channel_cfg = {
		.gain             = ADC_GAIN,
		.reference        = ADC_REFERENCE,
		.acquisition_time = ADC_ACQUISITION_TIME,
		.channel_id       = ADC_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
		.input_positive   = ADC_CHANNEL_INPUT,
#endif
	};
	int err = adc_channel_setup(adc_dev, &channel_cfg);
	if (err) {
		LOG_ERR("Setting up the ADC channel failed");
		return err;
	}

	/* Check if number of elements in LUT is proper */
	BUILD_ASSERT(CONFIG_DESKTOP_BATTERY_MEAS_MAX_LEVEL
			 - CONFIG_DESKTOP_BATTERY_MEAS_MIN_LEVEL
			 == (ARRAY_SIZE(battery_voltage_to_soc) - 1)
			 * CONFIG_DESKTOP_VOLTAGE_TO_SOC_DELTA,
			 "Improper number of elements in lookup table");

	return 0;
}

static int battery_monitor_start(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BATTERY_MEAS_HAS_ENABLE_PIN)) {
		int err = gpio_pin_set_raw(gpio_dev,
					   CONFIG_DESKTOP_BATTERY_MEAS_ENABLE_PIN,
					   1);
		if (err) {
			 LOG_ERR("Cannot enable battery monitor circuit");
			 return err;
		}
	}

	sampling = true;
	k_delayed_work_submit(&battery_lvl_read,
			      K_MSEC(BATTERY_WORK_INTERVAL));

	return 0;
}

static void battery_monitor_stop(void)
{
	k_delayed_work_cancel(&battery_lvl_read);
	sampling = false;
	int err = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_BATTERY_MEAS_HAS_ENABLE_PIN)) {
		err = gpio_pin_set_raw(gpio_dev,
				       CONFIG_DESKTOP_BATTERY_MEAS_ENABLE_PIN,
				       0);
		if (err) {
			LOG_ERR("Cannot disable battery monitor circuit");
			module_set_state(MODULE_STATE_ERROR);

			return;
		}
	}

	module_set_state(MODULE_STATE_STANDBY);
}

static void battery_lvl_process(void)
{
	u32_t voltage = BATTERY_VOLTAGE(adc_buffer);
	u8_t level;

	if (voltage > CONFIG_DESKTOP_BATTERY_MEAS_MAX_LEVEL) {
		level = 100;
	} else if (voltage < CONFIG_DESKTOP_BATTERY_MEAS_MIN_LEVEL) {
		level = 0;
		LOG_WRN("Low battery");
	} else {
		/* Using lookup table to convert voltage[mV] to SoC[%] */
		size_t lut_id = (voltage - CONFIG_DESKTOP_BATTERY_MEAS_MIN_LEVEL
				 + (CONFIG_DESKTOP_VOLTAGE_TO_SOC_DELTA >> 1))
				 / CONFIG_DESKTOP_VOLTAGE_TO_SOC_DELTA;
		level = battery_voltage_to_soc[lut_id];
	}

	struct battery_level_event *event = new_battery_level_event();
	event->level = level;

	EVENT_SUBMIT(event);

	LOG_INF("Battery level: %u%% (%u mV)", level, voltage);
}

static void battery_lvl_read_fn(struct k_work *work)
{
	int err;

	if (!adc_async_read_pending) {
		static const struct adc_sequence sequence = {
			.options	= NULL,
			.channels	= BIT(ADC_CHANNEL_ID),
			.buffer		= &adc_buffer,
			.buffer_size	= sizeof(adc_buffer),
			.resolution	= ADC_RESOLUTION,
			.oversampling	= ADC_OVERSAMPLING,
			.calibrate	= false,
		};
		static const struct adc_sequence sequence_calibrate = {
			.options	= NULL,
			.channels	= BIT(ADC_CHANNEL_ID),
			.buffer		= &adc_buffer,
			.buffer_size	= sizeof(adc_buffer),
			.resolution	= ADC_RESOLUTION,
			.oversampling	= ADC_OVERSAMPLING,
			.calibrate	= true,
		};

		static bool calibrated;

		if (likely(calibrated)) {
			err = adc_read_async(adc_dev, &sequence, &async_sig);
		} else {
			err = adc_read_async(adc_dev, &sequence_calibrate,
					     &async_sig);
			calibrated = true;
		}

		if (err) {
			LOG_WRN("Battery level async read failed");
		} else {
			adc_async_read_pending = true;
		}
	} else {
		err = k_poll(&async_evt, 1, K_NO_WAIT);
		if (err) {
			LOG_WRN("Battery level poll failed");
		} else {
			adc_async_read_pending = false;
			battery_lvl_process();
		}
	}

	if (atomic_get(&active) || adc_async_read_pending) {
		k_delayed_work_submit(&battery_lvl_read,
				      K_MSEC(BATTERY_WORK_INTERVAL));
	} else {
		battery_monitor_stop();
	}
}

static int init_fn(void)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_BATTERY_MEAS_HAS_ENABLE_PIN)) {
		gpio_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
		if (!gpio_dev) {
			LOG_ERR("Cannot get GPIO device");
			err = -ENXIO;
			goto error;
		}

		/* Enable battery monitoring */
		err = gpio_pin_configure(gpio_dev,
					 CONFIG_DESKTOP_BATTERY_MEAS_ENABLE_PIN,
					 GPIO_OUTPUT);
	}

	if (!err) {
		err = init_adc();
	}

	if (err) {
		goto error;
	}

	k_delayed_work_init(&battery_lvl_read, battery_lvl_read_fn);

	err = battery_monitor_start();
error:
	return err;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			int err = init_fn();

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
				atomic_set(&active, true);
			}

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!atomic_get(&active)) {
			atomic_set(&active, true);

			int err = battery_monitor_start();

			if (!err) {
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (atomic_get(&active)) {
			atomic_set(&active, false);

			if (adc_async_read_pending) {
				__ASSERT_NO_MSG(sampling);
				/* Poll ADC and postpone shutdown */
				k_delayed_work_submit(&battery_lvl_read,
						      K_MSEC(0));
			} else {
				/* No ADC sample left to read, go to standby */
				battery_monitor_stop();
			}
		}

		return sampling;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
