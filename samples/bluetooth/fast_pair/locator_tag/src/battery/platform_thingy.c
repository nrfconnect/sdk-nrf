/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

#include <bluetooth/services/fast_pair/fmdn.h>

#include "app_battery.h"
#include "app_battery_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_DBG);

#define VBATT					DT_PATH(vbatt)
#define BATTERY_ADC_GAIN			ADC_GAIN_1

#define BATTERY_POLL_THREAD_PRIORITY		K_PRIO_PREEMPT(0)
#define BATTERY_POLL_THREAD_STACK_SIZE		512

static const struct gpio_dt_spec bat_mon_en = GPIO_DT_SPEC_GET(VBATT, power_gpios);
static const struct device *adc;
static struct adc_sequence adc_seq;
static struct adc_channel_cfg adc_cfg;
static int16_t adc_raw_data;

static uint8_t battery_charge;
static K_SEM_DEFINE(poll_sem, 0, 1);

static int battery_init(void)
{
	int err;

	adc = DEVICE_DT_GET(DT_IO_CHANNELS_CTLR(VBATT));
	if (!device_is_ready(adc)) {
		LOG_ERR("ADC device %s is not ready", adc->name);
		return -ENOENT;
	}

	if (!device_is_ready(bat_mon_en.port)) {
		LOG_ERR("BAT_MON_EN enable is not ready");
		return -EIO;
	}

	err = gpio_pin_configure_dt(&bat_mon_en, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Can't configure BAT_MON_EN pin (err %d)", err);
		return err;
	}

	adc_seq = (struct adc_sequence){
		.channels = BIT(0),
		.buffer = &adc_raw_data,
		.buffer_size = sizeof(adc_raw_data),
		.oversampling = 4,
		.calibrate = true,
		.resolution = 14
	};

	adc_cfg = (struct adc_channel_cfg){
		.gain = BATTERY_ADC_GAIN,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
		.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + DT_IO_CHANNELS_INPUT(VBATT)
	};

	return adc_channel_setup(adc, &adc_cfg);
}

static int battery_voltage_get(int32_t *voltage)
{
	int err;
	int32_t val_mv;

	err = adc_read(adc, &adc_seq);
	if (err) {
		LOG_ERR("Can't read ADC (err %d)", err);
		return err;
	}

	val_mv = adc_raw_data;
	adc_raw_to_millivolts(adc_ref_internal(adc),
			      adc_cfg.gain,
			      adc_seq.resolution,
			      &val_mv);

	val_mv *= DT_PROP(VBATT, full_ohms);
	val_mv /= DT_PROP(VBATT, output_ohms);

	*voltage = val_mv;

	return 0;
}

static uint8_t voltage_to_lipo_soc_convert(int32_t val)
{
	/*
	 * soc[%] = ((val - v_min)/(v_max - v_min)) * 100%
	 * soc[%] = ((val - 2500mV)/(4200mV - 2500mV)) * 100%
	 * soc[%] = (val - 2500mV)/(1700mV / 100) %
	 * soc[%] = (val - 2500mV)/17mV %
	 */
	__ASSERT(val >= 2500, "Invalid value of battery voltage, got %i mV", val);

	val -= 2500;
	val /= 17;

	return (uint8_t) val;
}

static int battery_measure(uint8_t *charge)
{
	int err;
	int val_mv;

	err = gpio_pin_set_dt(&bat_mon_en, 1);
	if (err) {
		LOG_ERR("Can't turn on BAT_MON_EN pin (err %d)", err);
		return err;
	}

	/* Wait for voltage to stabilize */
	k_msleep(1);

	err = battery_voltage_get(&val_mv);
	if (err) {
		LOG_ERR("Can't read battery voltage (err %d)", err);
		return err;
	}

	*charge = voltage_to_lipo_soc_convert(val_mv);

	return gpio_pin_set_dt(&bat_mon_en, 0);
}

static int battery_level_set(bool forced_set)
{
	int err;
	static uint8_t battery_level;

	if (!forced_set && (battery_level == battery_charge)) {
		LOG_DBG("FMDN: battery level did not change");
		return 0;
	}

	err = bt_fast_pair_fmdn_battery_level_set(battery_charge);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_fmdn_battery_level_set failed (err %d)",
			err);
		return err;
	}

	battery_level = battery_charge;

	app_battery_level_log(battery_level);

	return 0;
}

static void battery_level_set_work_handle(struct k_work *w)
{
	k_sem_give(&poll_sem);

	(void) battery_level_set(false);
}

static void battery_poll_thread_process(void)
{
	int err;

	static K_WORK_DEFINE(battery_level_set_work, battery_level_set_work_handle);

	while (1) {
		k_sem_take(&poll_sem, K_FOREVER);

		err = battery_measure(&battery_charge);
		if (err) {
			LOG_ERR("Battery measurement failed (err %d)", err);
		} else {
			LOG_INF("Measured battery charge: %d [%%]", battery_charge);
		}
		k_work_submit(&battery_level_set_work);

		k_sleep(K_SECONDS(CONFIG_APP_BATTERY_POLL_INTERVAL));
	}
}

K_THREAD_DEFINE(battery_poll_thread, BATTERY_POLL_THREAD_STACK_SIZE, battery_poll_thread_process,
		NULL, NULL, NULL, BATTERY_POLL_THREAD_PRIORITY, 0, 0);

static int battery_level_set_init(void)
{
	int err;

	err = battery_measure(&battery_charge);
	if (err) {
		return err;
	}

	err = battery_level_set(true);
	if (err) {
		return err;
	}

	return 0;
}

int app_battery_init(void)
{
	int err;

	err = battery_init();
	if (err) {
		LOG_ERR("Battery initialization failed (err %d)", err);
		return err;
	}

	err = battery_level_set_init();
	if (err) {
		LOG_ERR("Battery initialization level setting failed (err %d)", err);
		return err;
	}

	k_sem_give(&poll_sem);

	return 0;
}
