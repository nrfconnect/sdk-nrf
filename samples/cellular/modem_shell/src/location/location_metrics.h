/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LOCATION_METRICS_H
#define MOSH_LOCATION_METRICS_H

#define LOCATION_METRICS_CMD_STR_MAX_LEN 255
struct location_metrics_data {
	struct location_event_data event_data;
	int64_t timestamp_ms;
	char loc_cmd_str[LOCATION_METRICS_CMD_STR_MAX_LEN + 1];
};

int location_metrics_utils_json_payload_encode(
	const struct location_metrics_data *loc_metrics_data, char **json_str_out);

#endif /* MOSH_LOCATION_METRICS_H */
