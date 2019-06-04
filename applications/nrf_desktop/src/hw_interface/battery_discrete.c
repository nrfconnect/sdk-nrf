/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <soc.h>
#include <device.h>
#include <adc.h>
#include <gpio.h>
#include <atomic.h>
#include <spinlock.h>

#include <hal/nrf_saadc.h>

#include "event_manager.h"
#include "power_event.h"
#include "battery_event.h"
#include "usb_event.h"
#include "battery_def.h"

#define MODULE battery
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BATTERY_LOG_LEVEL);


#define CSO_PERIOD_HZ		CONFIG_DESKTOP_BATTERY_CSO_FREQ
#define CSO_CHANGE_MAX		3

#define ERROR_CHECK_TIMEOUT_MS	(1 + CSO_CHANGE_MAX * (1000 / CSO_PERIOD_HZ))

#define ADC_DEVICE_NAME		DT_ADC_0_NAME
#define ADC_RESOLUTION		12
#define ADC_OVERSAMPLING	4 /* 2^ADC_OVERSAMPLING samples are averaged */
#define ADC_MAX 		4096
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_REF_INTERNAL_MV	600UL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_CHANNEL_ID		0
#define ADC_CHANNEL_INPUT	NRF_SAADC_INPUT_AIN3
/* ADC asynchronous read is scheduled on odd works. Value read happens during
 * even works. This is done to avoid creating a thread for battery monitor.
 */
#define BATTERY_WORK_INTERVAL	(CONFIG_DESKTOP_BATTERY_POLL_INTERVAL_MS / 2)
#define BATTERY_VOLTAGE(sample)	(sample * ADC_REF_INTERNAL_MV		\
		* (CONFIG_DESKTOP_BATTERY_VOLTAGE_DIVIDER_UPPER		\
		+ CONFIG_DESKTOP_BATTERY_VOLTAGE_DIVIDER_LOWER)		\
		/ CONFIG_DESKTOP_BATTERY_VOLTAGE_DIVIDER_LOWER / ADC_MAX)

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
static struct gpio_callback gpio_cb;

static struct k_delayed_work error_check;
static unsigned int cso_counter;
static struct k_spinlock lock;

static enum battery_state battery_state = -1;
static atomic_t active;
static bool sampling;


static void error_check_handler(struct k_work *work)
{
	unsigned int cnt;

	k_spinlock_key_t key = k_spin_lock(&lock);
	cnt = cso_counter;
	cso_counter = 0;
	k_spin_unlock(&lock, key);

	enum battery_state state;

	if (cnt <= CSO_CHANGE_MAX) {
		u32_t val = 0;
		gpio_pin_read(gpio_dev, CONFIG_DESKTOP_BATTERY_CSO_PIN, &val);
		state = (val == 0) ? (BATTERY_STATE_CHARGING) :
				     (BATTERY_STATE_IDLE);

	} else {
		gpio_pin_disable_callback(gpio_dev,
					  CONFIG_DESKTOP_BATTERY_CSO_PIN);
		state = BATTERY_STATE_ERROR;
	}

	if (state != battery_state) {
		battery_state = state;

		struct battery_state_event *event = new_battery_state_event();
		event->state = battery_state;

		EVENT_SUBMIT(event);
	}
}


static void cs_change(struct device *gpio_dev, struct gpio_callback *cb,
	       u32_t pins)
{
	if (!atomic_get(&active)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	if (cso_counter == 0) {
		k_delayed_work_submit(&error_check, ERROR_CHECK_TIMEOUT_MS);
	}
	cso_counter++;
	k_spin_unlock(&lock, key);
}

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
	static_assert(CONFIG_DESKTOP_BATTERY_MAX_LEVEL
		      - CONFIG_DESKTOP_BATTERY_MIN_LEVEL
		      == (ARRAY_SIZE(battery_voltage_to_soc) - 1)
		      * CONFIG_DESKTOP_VOLTAGE_TO_SOC_DELTA,
		      "Improper number of elements in lookup table");

	return 0;
}

static int battery_monitor_start(void)
{
	int err = gpio_pin_write(gpio_dev,
				 CONFIG_DESKTOP_BATTERY_MONITOR_ENABLE_PIN,
				 1);
	if (err) {
		 LOG_ERR("Cannot enable battery monitor circuit");
		 return err;
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

	int err = gpio_pin_write(gpio_dev,
				 CONFIG_DESKTOP_BATTERY_MONITOR_ENABLE_PIN,
				 0);
	if (err) {
		LOG_ERR("Cannot disable battery monitor circuit");
		module_set_state(MODULE_STATE_ERROR);
	} else {
		module_set_state(MODULE_STATE_STANDBY);
	}
}

static void battery_lvl_process(void)
{
	u32_t voltage = BATTERY_VOLTAGE(adc_buffer);
	u8_t level;

	if (voltage > CONFIG_DESKTOP_BATTERY_MAX_LEVEL) {
		level = 100;
	} else if (voltage < CONFIG_DESKTOP_BATTERY_MIN_LEVEL) {
		level = 0;
		LOG_WRN("Low battery");
	} else {
		/* Using lookup table to convert voltage[mV] to SoC[%] */
		size_t lut_id = (voltage - CONFIG_DESKTOP_BATTERY_MIN_LEVEL
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
			.options     = NULL,
			.channels    = BIT(ADC_CHANNEL_ID),
			.buffer      = &adc_buffer,
			.buffer_size = sizeof(adc_buffer),
			.resolution  = ADC_RESOLUTION,
			.oversampling = ADC_OVERSAMPLING,
		};

		err = adc_read_async(adc_dev, &sequence, &async_sig);
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

static int cso_pin_control(bool enable)
{
	int flags = GPIO_DIR_IN;

	if (enable) {
		flags |= GPIO_INT | GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE;

		if (IS_ENABLED(CONFIG_DESKTOP_BATTERY_CSO_PULL_UP)) {
			flags |= GPIO_PUD_PULL_UP;
		} else if (IS_ENABLED(CONFIG_DESKTOP_BATTERY_CSO_PULL_DOWN)) {
			flags |= GPIO_PUD_PULL_DOWN;
		}
	}

	int err = gpio_pin_configure(gpio_dev, CONFIG_DESKTOP_BATTERY_CSO_PIN,
				     flags);
	if (err) {
		LOG_ERR("Cannot configure CSO pin");
	}

	return err;
}

static int charging_enable(void)
{
	return gpio_pin_write(gpio_dev,
			      CONFIG_DESKTOP_BATTERY_CHARGE_ENABLE_PIN,
			      1);
}

static int charging_disable(void)
{
	return gpio_pin_write(gpio_dev,
			      CONFIG_DESKTOP_BATTERY_CHARGE_ENABLE_PIN,
			      0);
}

static int start_battery_state_check(void)
{
	int err = gpio_pin_disable_callback(gpio_dev,
					    CONFIG_DESKTOP_BATTERY_CSO_PIN);

	if (!err) {
		/* Lock is not needed as interrupt is disabled */
		cso_counter = 1;
		k_delayed_work_submit(&error_check, ERROR_CHECK_TIMEOUT_MS);

		err = gpio_pin_enable_callback(gpio_dev,
					       CONFIG_DESKTOP_BATTERY_CSO_PIN);
	}

	if (err) {
		LOG_ERR("Cannot start battery state check");
	}

	return err;
}

static int init_fn(void)
{
	int err;

	gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		LOG_ERR("Cannot get GPIO device");
		err = -ENXIO;
		goto error;
	}

	err = gpio_pin_configure(gpio_dev,
				 CONFIG_DESKTOP_BATTERY_CHARGE_ENABLE_PIN,
				 GPIO_DIR_OUT);
	if (err) {
		goto error;
	}

	err = charging_disable();
	if (err) {
		goto error;
	}

	err = cso_pin_control(true);

	gpio_init_callback(&gpio_cb, cs_change,
			   BIT(CONFIG_DESKTOP_BATTERY_CSO_PIN));
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		goto error;
	}

	k_delayed_work_init(&error_check, error_check_handler);

	err = start_battery_state_check();
	if (err) {
		goto error;
	}

	/* Enable battery monitoring */
	err = gpio_pin_configure(gpio_dev,
				 CONFIG_DESKTOP_BATTERY_MONITOR_ENABLE_PIN,
				 GPIO_DIR_OUT);
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

			int err = cso_pin_control(true);
			if (!err) {
				err = start_battery_state_check();
			}

			if (!err) {
				err = battery_monitor_start();
			}

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

			int err = gpio_pin_disable_callback(gpio_dev,
					CONFIG_DESKTOP_BATTERY_CSO_PIN);

			if (!err) {
				err = cso_pin_control(false);
			}

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			}

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

	if (is_usb_state_event(eh)) {
		struct usb_state_event *event = cast_usb_state_event(eh);
		int err;

		switch (event->state) {
		case USB_STATE_SUSPENDED:
		case USB_STATE_DISCONNECTED:
			err = charging_disable();
			break;

		case USB_STATE_POWERED:
		case USB_STATE_ACTIVE:
			err = charging_enable();
			break;

		default:
			/* Ignore */
			err = 0;
			break;
		}
		if (!err) {
			err = start_battery_state_check();
		}
		if (err) {
			module_set_state(MODULE_STATE_ERROR);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE(MODULE, usb_state_event);
