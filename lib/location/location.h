/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_H
#define LOCATION_H

struct loc_method_api {
	enum loc_method method;
	char method_string[10];
	int  (*init)(void);
	int  (*validate_params)(const struct loc_method_config *config);
	int  (*location_request)(const struct loc_method_config *config);
	int  (*cancel_request)();
};

void event_location_callback(const struct loc_location *location);
void event_location_callback_error();
void event_location_callback_timeout();

#endif /* LOCATION_H */
