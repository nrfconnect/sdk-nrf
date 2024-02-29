/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LOCATION_DETAILS_H
#define MOSH_LOCATION_DETAILS_H

#define LOCATION_DETAILS_CMD_STR_MAX_LEN 255
struct location_details_data {
	struct location_event_data event_data;
	int64_t timestamp_ms;
	char loc_cmd_str[LOCATION_DETAILS_CMD_STR_MAX_LEN + 1];
};

int location_details_utils_json_payload_encode(
	const struct location_details_data *loc_details_data, char **json_str_out);

#endif /* MOSH_LOCATION_DETAILS_H */
