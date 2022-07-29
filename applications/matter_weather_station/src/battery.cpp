/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "battery.h"

#include <hal/nrf_saadc.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app);

#define VBATT DT_PATH(vbatt)
#define ZEPHYR_USER DT_PATH(zephyr_user)

namespace
{
const struct gpio_dt_spec sPowerGpio = GPIO_DT_SPEC_GET(VBATT, power_gpios);
const struct gpio_dt_spec sChargeGpio = GPIO_DT_SPEC_GET(ZEPHYR_USER, battery_charge_gpios);
const uint32_t sFullOhms = DT_PROP(VBATT, full_ohms);
const uint32_t sOutputOhms = DT_PROP(VBATT, output_ohms);
const struct adc_dt_spec sAdc = ADC_DT_SPEC_GET(VBATT);

bool sBatteryConfigured = false;
bool sBatteryChargeConfigured = false;
int16_t sAdcBuffer = 0;

struct adc_sequence sAdcSeq = {
	.buffer = &sAdcBuffer,
	.buffer_size = sizeof(sAdcBuffer),
	.calibrate = true,
};
} /* namespace */

int BatteryMeasurementInit()
{
	int err;

	if (!device_is_ready(sPowerGpio.port)) {
		LOG_ERR("Battery measurement GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&sPowerGpio, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("Failed to configure battery measurement GPIO %d", err);
		return err;
	}

	if (!device_is_ready(sAdc.dev)) {
		LOG_ERR("ADC controller not ready");
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&sAdc);
	if (err) {
		LOG_ERR("Setting up the ADC channel failed");
		return err;
	}

	(void)adc_sequence_init_dt(&sAdc, &sAdcSeq);

	sBatteryConfigured = true;

	return err;
}

int BatteryMeasurementEnable()
{
	int err = -ECANCELED;
	if (sBatteryConfigured) {
		err = gpio_pin_set_dt(&sPowerGpio, 1);
		if (err != 0) {
			LOG_ERR("Failed to enable measurement pin %d", err);
		}
	}
	return err;
}

int32_t BatteryMeasurementReadVoltageMv()
{
	int32_t result = -ECANCELED;
	if (sBatteryConfigured) {
		result = adc_read(sAdc.dev, &sAdcSeq);
		if (result == 0) {
			int32_t val = sAdcBuffer;
			adc_raw_to_millivolts_dt(&sAdc, &val);
			result = static_cast<int32_t>(static_cast<int64_t>(val) * sFullOhms / sOutputOhms);
		}
	}
	return result;
}

int BatteryChargeControlInit()
{
	if (!device_is_ready(sChargeGpio.port)) {
		LOG_ERR("Charge GPIO controller not ready");
		return -ENODEV;
	}

	int err = gpio_pin_configure_dt(&sChargeGpio, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Failed to configure battery charge GPIO %d", err);
		return err;
	}

	sBatteryChargeConfigured = true;

	return 0;
}

bool BatteryCharged()
{
	if (!sBatteryChargeConfigured) {
		return true;
	}

	return static_cast<bool>(gpio_pin_get_dt(&sChargeGpio));
}
