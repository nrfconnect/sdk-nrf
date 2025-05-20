/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ui_sense_led.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_sensors, CONFIG_APP_LOG_LEVEL);

#if !DT_NODE_HAS_STATUS(DT_NODELABEL(bh1749), okay)
#define LIGHT_SENSOR_SIMULATED 1
#define LIGHT_SIM_BASE CONFIG_LIGHT_SENSOR_LIGHT_SIM_VAL
#define COLOUR_SIM_BASE CONFIG_LIGHT_SENSOR_COLOUR_SIM_VAL
#define LIGHT_SIM_MAX_DIFF CONFIG_LIGHT_SENSOR_LIGHT_SIM_MAX_DIFF
#define COLOUR_SIM_MAX_DIFF CONFIG_LIGHT_SENSOR_COLOUR_SIM_MAX_DIFF
#endif

/* Macros to scale light and colour measurements to uint8_t */
#define SCALE_LIGHT_MEAS(raw_value)                                                                \
	MIN((raw_value * 255U / CONFIG_LIGHT_SENSOR_LIGHT_MEASUREMENT_MAX_VALUE), UINT8_MAX)
#define SCALE_COLOUR_MEAS(raw_value)                                                               \
	MIN((raw_value * 255U / CONFIG_LIGHT_SENSOR_COLOUR_MEASUREMENT_MAX_VALUE), UINT8_MAX)

#define NUM_CHANNELS 4U
#define RGBIR_STR_LENGTH 11U /* Format: '0xRRGGBBIR\0'. */
#define SENSE_LED_ON_TIME_MS 300U /* Time the sense LED stays on during colour read. */

#if !defined(LIGHT_SENSOR_SIMULATED)
static const struct device *light_sensor_dev = DEVICE_DT_GET(DT_NODELABEL(bh1749));
static int64_t fetch_timestamp;


static int _sensor_read(struct sensor_value measurements[])
{
	int ret;

	ret = sensor_channel_get(light_sensor_dev, SENSOR_CHAN_IR, &measurements[0]);
	if (ret) {
		LOG_ERR("Get IR channel failed (%d)", ret);
		return ret;
	}
	ret = sensor_channel_get(light_sensor_dev, SENSOR_CHAN_BLUE, &measurements[1]);
	if (ret) {
		LOG_ERR("Get blue channel failed (%d)", ret);
		return ret;
	}
	ret = sensor_channel_get(light_sensor_dev, SENSOR_CHAN_GREEN, &measurements[2]);
	if (ret) {
		LOG_ERR("Get green channel failed (%d)", ret);
		return ret;
	}
	ret = sensor_channel_get(light_sensor_dev, SENSOR_CHAN_RED, &measurements[3]);
	if (ret) {
		LOG_ERR("Get red channel failed (%d)", ret);
		return ret;
	}

	return 0;
}
#endif /* if defined(CONFIG_LIGHT_SENSOR_USE_EXTERNAL) */

int light_sensor_read(uint32_t *light_value)
{
	struct sensor_value light_values[NUM_CHANNELS];
	uint32_t scaled_measurement;

#if !defined(LIGHT_SENSOR_SIMULATED)
	int ret;

	ret = _sensor_read(light_values);
	if (ret == -EBUSY) {
		return ret;
	} else if (ret) {
		LOG_ERR("Sensor read failed (%d)", ret);
		return ret;
	}

#else
	/* TODO: Simulate with rng */
	for (int i = 0; i < NUM_CHANNELS; i++) {
		light_values[i].val1 = MAX(0, LIGHT_SIM_BASE + (rand() % LIGHT_SIM_MAX_DIFF) *
								       (1 - 2 * (rand() % 2)));
	}
#endif

	*light_value = 0;

	/* Scale measurements and combine in 4-byte light value, one byte per colour channel */
	for (int i = 0; i < NUM_CHANNELS; ++i) {
		scaled_measurement = SCALE_LIGHT_MEAS(light_values[i].val1);
		*light_value |= scaled_measurement << 8 * i;
	}

	return 0;
}

int colour_sensor_read(uint32_t *colour_value)
{
	struct sensor_value colour_values[NUM_CHANNELS];
	uint32_t scaled_measurement;

#if !defined(LIGHT_SENSOR_SIMULATED)
	int ret;

	ui_sense_led_on_off(true);
	k_sleep(K_MSEC(SENSE_LED_ON_TIME_MS));

	ret = _sensor_read(colour_values);

	ui_sense_led_on_off(false);

	if (ret == -EBUSY) {
		return ret;
	} else if (ret) {
		LOG_ERR("Sensor read failed (%d)", ret);
		return ret;
	}
#else
	/* TODO: Simulate with rng */
	for (int i = 0; i < NUM_CHANNELS; i++) {
		colour_values[i].val1 = MAX(0, COLOUR_SIM_BASE + (rand() % COLOUR_SIM_MAX_DIFF) *
									 (1 - 2 * (rand() % 2)));
	}
#endif

	*colour_value = 0;

	/* Scale measurements and combine in 4-byte colour value, one byte per colour channel */
	for (int i = 0; i < 4; ++i) {
		scaled_measurement = SCALE_COLOUR_MEAS(colour_values[i].val1);
		*colour_value |= scaled_measurement << 8 * i;
	}

	return 0;
}

int light_sensor_init(void)
{
#if !defined(LIGHT_SENSOR_SIMULATED)
	fetch_timestamp = 0;

	ui_sense_led_init();

	if (!device_is_ready(light_sensor_dev)) {
		LOG_ERR("Light Sensor device not ready");
		return -ENODEV;
	}
#endif

	return 0;
}
