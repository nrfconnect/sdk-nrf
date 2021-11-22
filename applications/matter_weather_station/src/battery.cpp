/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "battery.h"

#include <hal/nrf_saadc.h>

#include <drivers/adc.h>
#include <drivers/gpio.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(app);

#define VBATT DT_PATH(vbatt)
#define BATTERY_ADC_DEVICE_NAME DT_LABEL(DT_NODELABEL(adc))
#define BATTERY_ENABLE_MEASUREMENT_GPIO DT_GPIO_LABEL(VBATT, power_gpios)
#define BATTERY_ENABLE_MEASUREMENT_PIN DT_GPIO_PIN(VBATT, power_gpios)
#define BATTERY_ENABLE_MEASUREMENT_FLAGS DT_GPIO_FLAGS(VBATT, power_gpios)
#define BATTERY_CHARGE_GPIO DT_LABEL(DT_NODELABEL(gpio1))
#define BATTERY_CHARGE_PIN 0
#define BATTERY_FULL_OHMS DT_PROP(VBATT, full_ohms)
#define BATTERY_OUTPUT_OHMS DT_PROP(VBATT, output_ohms)

namespace
{
const device *sAdcController;
const device *sMeasurementGpioController;
const device *sChargeGpioController;

bool sBatteryConfigured = false;
int16_t sAdcBuffer = 0;

#ifdef CONFIG_ADC_NRFX_SAADC
const struct adc_channel_cfg sAdcConfig = {
	.gain = ADC_GAIN_1,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
	.input_positive = NRF_SAADC_INPUT_AIN2,
};
#else
#error Unsupported NRFX ADC
#endif

struct adc_sequence sAdcSeq = {
	.options = NULL,
	.channels = BIT(0),
	.buffer = &sAdcBuffer,
	.buffer_size = sizeof(sAdcBuffer),
	.resolution = 12,
	.oversampling = 4,
	.calibrate = true,
};
} /* namespace */

int BatteryMeasurementInit()
{
	int err;

	sMeasurementGpioController = device_get_binding(BATTERY_ENABLE_MEASUREMENT_GPIO);
	if (!sMeasurementGpioController) {
		LOG_ERR("Cannot get battery measurement GPIO device");
		return -ENODEV;
	}

	err = gpio_pin_configure(sMeasurementGpioController, BATTERY_ENABLE_MEASUREMENT_PIN,
				 GPIO_OUTPUT_INACTIVE | BATTERY_ENABLE_MEASUREMENT_FLAGS);
	if (err != 0) {
		LOG_ERR("Failed to configure battery measurement GPIO %d", err);
		return err;
	}

	sAdcController = device_get_binding(BATTERY_ADC_DEVICE_NAME);
	if (!sAdcController) {
		LOG_ERR("Cannot get ADC device");
		return -ENODEV;
	}

	err = adc_channel_setup(sAdcController, &sAdcConfig);
	if (err) {
		LOG_ERR("Setting up the ADC channel failed");
		return err;
	}

	sBatteryConfigured = true;

	return err;
}

int BatteryMeasurementEnable()
{
	int err = -ECANCELED;
	if (sBatteryConfigured) {
		err = gpio_pin_set(sMeasurementGpioController, BATTERY_ENABLE_MEASUREMENT_PIN, 1);
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
		result = adc_read(sAdcController, &sAdcSeq);
		if (result == 0) {
			int32_t val = sAdcBuffer;
			adc_raw_to_millivolts(adc_ref_internal(sAdcController), sAdcConfig.gain, sAdcSeq.resolution,
					      &val);
			result = static_cast<int32_t>(static_cast<int64_t>(val) * BATTERY_FULL_OHMS /
						      BATTERY_OUTPUT_OHMS);
		}
	}
	return result;
}

int BatteryChargeControlInit()
{
	sChargeGpioController = device_get_binding(BATTERY_CHARGE_GPIO);
	if (!sChargeGpioController) {
		LOG_ERR("Cannot get battery charge GPIO device");
		return -ENODEV;
	}

	int err = gpio_pin_configure(sChargeGpioController, BATTERY_CHARGE_PIN, GPIO_INPUT | GPIO_PULL_UP);
	if (err != 0) {
		LOG_ERR("Failed to configure battery charge GPIO %d", err);
	}

	return err;
}

bool BatteryCharged()
{
	if (sChargeGpioController) {
		/* Invert logic (low state means charging and high not charging) */
		return !gpio_pin_get(sChargeGpioController, BATTERY_CHARGE_PIN);
	}
	return true;
}
