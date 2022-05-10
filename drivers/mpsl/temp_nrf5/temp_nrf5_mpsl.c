/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_temp

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <mpsl_temp.h>

LOG_MODULE_REGISTER(temp_nrf5_mpsl, CONFIG_SENSOR_LOG_LEVEL);

/* The MPSL temperature API returns measurements in 0.25C
 * increments. Increments per degree.
 */
#define TEMP_NRF5_MPSL_INC_PER_DEGREE_C 4UL
/* Mask for fractional increments */
#define TEMP_NRF5_MPSL_FRACTIONAL_INC_MSK 3UL
/* Millidegrees Celsius per increment */
#define TEMP_NRF5_MPSL_MILLIDEGREE_C_PER_INC 250000UL

struct temp_nrf5_mpsl_data {
	int32_t sample;
};

static int temp_nrf5_mpsl_sample_fetch(const struct device *dev,
				       enum sensor_channel chan)
{
	struct temp_nrf5_mpsl_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	data->sample = mpsl_temperature_get();
	LOG_DBG("sample: %d", data->sample);

	return 0;
}

static int temp_nrf5_mpsl_channel_get(const struct device *dev,
				      enum sensor_channel chan,
				      struct sensor_value *val)
{
	struct temp_nrf5_mpsl_data *data = dev->data;
	int32_t uval;
	uint32_t uval_abs;
	uint32_t val1_abs;
	uint32_t val2_abs;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	uval = data->sample;
	uval_abs = (uval < 0) ? (-uval) : uval;

	val1_abs = uval_abs / TEMP_NRF5_MPSL_INC_PER_DEGREE_C;
	val2_abs = (uval_abs & TEMP_NRF5_MPSL_FRACTIONAL_INC_MSK) *
		   TEMP_NRF5_MPSL_MILLIDEGREE_C_PER_INC;

	if (uval < 0) {
		val->val1 = -(int32_t)val1_abs;
		val->val2 = -(int32_t)val2_abs;
	} else {
		val->val1 = (int32_t)val1_abs;
		val->val2 = (int32_t)val2_abs;
	}

	LOG_DBG("Temperature:%d,%d", val->val1, val->val2);

	return 0;
}

static const struct sensor_driver_api temp_nrf5_mpsl_driver_api = {
	.sample_fetch = temp_nrf5_mpsl_sample_fetch,
	.channel_get = temp_nrf5_mpsl_channel_get,
};

static int temp_nrf5_mpsl_init(const struct device *dev)
{
	(void)dev;

	LOG_DBG("");

	return 0;
}

static struct temp_nrf5_mpsl_data temp_nrf5_mpsl_driver;

DEVICE_DT_INST_DEFINE(0, temp_nrf5_mpsl_init, NULL,
		      &temp_nrf5_mpsl_driver, NULL, POST_KERNEL,
		      CONFIG_SENSOR_INIT_PRIORITY, &temp_nrf5_mpsl_driver_api);
