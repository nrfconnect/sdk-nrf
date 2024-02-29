/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LOCATION_CMD_UTILS_H
#define MOSH_LOCATION_CMD_UTILS_H

enum location_cmd_cloud_data_gnss_format { CLOUD_GNSS_FORMAT_PVT, CLOUD_GNSS_FORMAT_NMEA };

int location_cmd_utils_gnss_loc_to_cloud_payload_json_encode(
	enum nrf_cloud_gnss_type format, const struct location_data *location_data,
	int64_t timestamp_ms, char **json_str_out);

#endif /* MOSH_LOCATION_CMD_UTILS_H */
