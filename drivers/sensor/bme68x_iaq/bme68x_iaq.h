/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* NCS Integration for BME68X + BSEC */

#include <zephyr/kernel.h>
#include "bsec_interface.h"
#include "bme68x.h"
#include <drivers/bme68x_iaq.h>

#ifndef ZEPHYR_DRIVERS_SENSOR_BME68X_NCS
#define ZEPHYR_DRIVERS_SENSOR_BME68X_NCS

struct bme_sample_result {
	double temperature;
	double humidity;
	double pressure;
	uint16_t air_quality;
};

struct bme68x_iaq_config {
	const struct i2c_dt_spec i2c;
};

struct bme68x_iaq_data {
	/* Variable to store intermediate sample result */
	struct bme_sample_result latest;

	/* Trigger and corresponding handler */
	sensor_trigger_handler_t trg_handler;
	const struct sensor_trigger *trigger;

	/* Internal BSEC thread metadata value. */
	struct k_thread thread;

	/* Buffer used to maintain the BSEC library state. */
	uint8_t state_buffer[BSEC_MAX_STATE_BLOB_SIZE];

	/* Size of the saved state */
	int32_t state_len;

	bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
	uint8_t n_required_sensor_settings;

	/* some RAM space needed by bsec_get_state and bsec_set_state for (de-)serialization. */
	uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];

	bool initialized;

	struct bme68x_dev dev;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_BME68X_NCS */
