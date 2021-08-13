/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_H
#define LOCATION_H

struct location_method_api {
	char method_string[10];
	int  (*init)(void);
	int  (*location_request)(const struct loc_method_config *config, uint16_t interval);
	int  (*cancel_request)();
};

struct location_method_supported {
	enum loc_method method;
	const struct location_method_api *api;
};

void event_location_callback(const struct loc_location *location);
void event_location_callback_error();
void event_location_callback_timeout();

#endif /* LOCATION_H */
