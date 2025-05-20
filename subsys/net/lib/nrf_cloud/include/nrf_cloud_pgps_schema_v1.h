/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include "nrf_cloud_agnss_schema_v1.h"

#ifndef NRF_CLOUD_PGPS_SCHEMA_V1_H_
#define NRF_CLOUD_PGPS_SCHEMA_V1_H_

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_CLOUD_PGPS_BIN_SCHEMA_VERSION		(1)
#define NRF_CLOUD_PGPS_PREDICTION_HEADER		(NRF_CLOUD_AGNSS__RSVD_PREDICTION_DATA)

#define NRF_CLOUD_PGPS_BIN_SCHEMA_VERSION_INDEX		(0)
#define NRF_CLOUD_PGPS_BIN_SCHEMA_VERSION_SIZE		(1)
#define NRF_CLOUD_PGPS_BIN_TYPE_OFFSET			(0)
#define NRF_CLOUD_PGPS_BIN_TYPE_SIZE			(1)
#define NRF_CLOUD_PGPS_BIN_COUNT_OFFSET			(1)
#define NRF_CLOUD_PGPS_BIN_COUNT_SIZE			(2)

#define PGPS_PREDICTION_STORAGE_SIZE 2048
#define PGPS_PREDICTION_PAD (PGPS_PREDICTION_STORAGE_SIZE - \
			     sizeof(struct nrf_cloud_pgps_prediction))
#define PGPS_SCHEMA_SIZE sizeof(((struct nrf_cloud_pgps_prediction *)0)->schema_version)
#define PGPS_SENTINEL_SIZE sizeof(((struct nrf_cloud_pgps_prediction *)0)->sentinel)
#define PGPS_PREDICTION_DL_SIZE (sizeof(struct nrf_cloud_pgps_prediction) - \
				 PGPS_SCHEMA_SIZE - PGPS_SENTINEL_SIZE)

struct nrf_cloud_pgps_header {
	uint8_t schema_version;
	uint8_t array_type;
	uint16_t num_items;
	uint16_t prediction_count;
	uint16_t prediction_size;
	uint16_t prediction_period_min;
	int16_t gps_day;
	int32_t gps_time_of_day;
	/** file header is followed immediately by
	 *  struct nrf_cloud_pgps_prediction predictions[prediction_count];
	 */
} __packed;

struct agnss_header {
	uint8_t type;
	uint16_t count;
	uint8_t data[];
} __packed;

struct pgps_sys_time {
	uint8_t schema_version;
	uint8_t type;
	uint16_t count;
	struct nrf_cloud_agnss_system_time time;
} __packed;

struct pgps_location {
	uint8_t schema_version;
	uint8_t type;
	uint16_t count;
	struct nrf_cloud_agnss_location location;
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_PGPS_SCHEMA_V1_H_ */
