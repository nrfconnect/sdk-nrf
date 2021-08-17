/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOC_CORE_H
#define LOC_CORE_H

struct loc_method_api {
	enum loc_method method;
	char method_string[10];
	int  (*init)(void);
	int  (*validate_params)(const struct loc_method_config *config);
	int  (*location_request)(const struct loc_method_config *config);
	int  (*cancel_request)();
};

int loc_core_init(location_event_handler_t handler);
int loc_core_validate_params(const struct loc_config *config);
int loc_core_location_get(const struct loc_config *config);
int loc_core_cancel(void);

void loc_core_event_cb(const struct loc_location *location);
void loc_core_event_cb_error(void);
void loc_core_event_cb_timeout(void);

#endif /* LOC_CORE_H */
