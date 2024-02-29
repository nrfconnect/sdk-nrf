/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LOCATION_DETAILS_H
#define MOSH_LOCATION_DETAILS_H

int location_details_json_payload_encode(
	const struct location_event_data *loc_evt_data,
	int64_t timestamp_ms,
	const char *mosh_cmd,
	char **json_str_out);

#endif /* MOSH_LOCATION_DETAILS_H */
