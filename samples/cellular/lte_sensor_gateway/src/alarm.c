/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>
#include <stdbool.h>
#include <net/nrf_cloud.h>
#include <zephyr/logging/log.h>

#include "alarm.h"

#include "aggregator.h"

LOG_MODULE_DECLARE(lte_ble_gw);

static bool alarm_pending;

extern void sensor_data_send(struct nrf_cloud_sensor_data *data);

extern struct k_work_delayable aggregated_work;

char *orientation_strings[] = {"LEFT", "NORMAL", "RIGHT", "UPSIDE_DOWN"};

void alarm(void)
{
	alarm_pending = true;
}

void send_aggregated_data(struct k_work *work)
{
	ARG_UNUSED(work);

	static uint8_t gps_data_buffer[NRF_MODEM_GNSS_NMEA_MAX_LEN];

	static struct nrf_cloud_sensor_data gnss_cloud_data = {
		.type = NRF_CLOUD_SENSOR_GNSS,
		.data.ptr = gps_data_buffer,
		.ts_ms = NRF_CLOUD_NO_TIMESTAMP
	};

	static struct nrf_cloud_sensor_data flip_cloud_data = {
		.type = NRF_CLOUD_SENSOR_FLIP,
		.ts_ms = NRF_CLOUD_NO_TIMESTAMP
	};

	struct sensor_data aggregator_data;

	k_work_schedule(&aggregated_work, K_MSEC(100));

	if (!alarm_pending) {
		return;
	}

	alarm_pending = false;

	LOG_INF("Alarm triggered!");
	while (1) {
		if (aggregator_get(&aggregator_data) == -ENODATA) {
			break;
		}
		switch (aggregator_data.type) {
		case THINGY_ORIENTATION:
			LOG_INF("[%d] Sending FLIP data", aggregator_element_count_get());
			if (aggregator_data.length != 1 ||
				aggregator_data.data[0] >=
				ARRAY_SIZE(orientation_strings)) {
				LOG_ERR("Unexpected FLIP data format, dropping");
				continue;
			}
			flip_cloud_data.data.ptr =
				orientation_strings[aggregator_data.data[0]];
			flip_cloud_data.data.len = strlen(
				orientation_strings[aggregator_data.data[0]]) - 1;
			sensor_data_send(&flip_cloud_data);
			break;

		case GNSS_POSITION:
			LOG_INF("[%d] Sending GNSS data", aggregator_element_count_get());
			gnss_cloud_data.data.ptr = &aggregator_data.data[4];
			gnss_cloud_data.data.len = aggregator_data.length;
			gnss_cloud_data.tag =
			    *((uint32_t *)&aggregator_data.data[0]);
			sensor_data_send(&gnss_cloud_data);
			break;

		default:
			LOG_ERR("Unsupported data type from aggregator: %d", aggregator_data.type);
			continue;
		}
	}
}
