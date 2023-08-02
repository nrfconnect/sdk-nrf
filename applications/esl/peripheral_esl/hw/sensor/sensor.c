/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrfx_temp.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "esl.h"

LOG_MODULE_DECLARE(peripheral_esl);

#if (CONFIG_BT_ESL_SENSOR_MAX > 0)
/**
 * @brief Function for handling TEMP driver events.
 *
 * @param[in] temperature Temperature data passed to the event handler.
 */
static void temp_handler(int32_t temperature)
{
	struct bt_esls *esl_obj = esl_get_esl_obj();

	int32_t celsius_temperature = nrfx_temp_calculate(temperature);
	int32_t whole_celsius = celsius_temperature / 100;
	uint8_t fraction_celsius = NRFX_ABS(celsius_temperature % 100);

	atomic_set_bit(&esl_obj->basic_state, ESL_SERVICE_NEEDED_BIT);
	esl_obj->sensor_data[0].data_available = true;
	LOG_INF("Measured temperature: %d.%02u [C], raw %d", whole_celsius, fraction_celsius,
		celsius_temperature);
	sys_put_le16((uint16_t)celsius_temperature, &esl_obj->sensor_data[0].data[0]);
}

int sensor_init(void)
{
	nrfx_err_t status;
	nrfx_temp_config_t config = NRFX_TEMP_DEFAULT_CONFIG;

	status = nrfx_temp_init(&config, temp_handler);
	IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TEMP), IRQ_PRIO_LOWEST, nrfx_temp_irq_handler,
			   0);
	return 0;
}

int sensor_control(uint8_t sensor_idx, uint8_t *len, uint8_t *data)
{
	struct bt_esls *esl_obj = esl_get_esl_obj();

	if (sensor_idx >= CONFIG_BT_ESL_SENSOR_MAX) {
		return -EIO;
	}

	*len = esl_obj->sensor_data[sensor_idx].size;
	/* Temperature sensor */
	if (sensor_idx == 0) {
		nrfx_err_t status;
		(void)status;
#if defined(CONFIG_BT_ESL_PTS)
		status = nrfx_temp_measure();
		memcpy(data, esl_obj->sensor_data[0].data, *len);
#else
		if (esl_obj->sensor_data[sensor_idx].data_available) {
			LOG_INF("nrfx_temp_measure done");
			memcpy(data, esl_obj->sensor_data[0].data, *len);
			esl_obj->sensor_data[0].data_available = false;
		} else {
			status = nrfx_temp_measure();
			LOG_INF("nrfx_temp_measure status %d", (status - NRFX_ERROR_BASE_NUM));
			return -EBUSY;
		}
#endif /* CONFIG_BT_ESL_PTS */
	} else {
		/* mock up value */
		for (uint8_t idx = 0; idx < *len; idx++) {
			esl_obj->sensor_data[sensor_idx].data[idx] = 0x12 + idx;
		}
	}

	memcpy(data, esl_obj->sensor_data[sensor_idx].data, *len);
	return 0;
}

#else
int sensor_init(void)
{
	return 0;
}
int sensor_control(uint8_t sensor_idx, uint8_t *len, uint8_t *data)
{
	return 0;
}
#endif /* CONFIG_BT_ESL_SENSOR_MAX */
