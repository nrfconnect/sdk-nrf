/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_CLIENT_UTILS_LOCATION_ASSISTANCE_INTERNAL_H__
#define LWM2M_CLIENT_UTILS_LOCATION_ASSISTANCE_INTERNAL_H__

#include <zephyr/kernel.h>

#define LOCATION_ASSISTANT_RESULT_TIMEOUT 180

#define LOCATION_ASSISTANT_UPDATE_PERIOD 60
#define LOCATION_ASSISTANT_INITIAL_RETRY_INTERVAL 675
#define LOCATION_ASSISTANT_MAXIMUM_RETRY_INTERVAL 86400

struct assistance_retry_data {
	struct k_work_delayable worker;
	int timeout_period;
	int retry_left;
	int retry_seconds;
	bool temp_error;
};

void location_retry_timer_cancel(struct assistance_retry_data *retry_info);
void location_retry_timer_start(struct assistance_retry_data *retry_info);
void location_retry_timer_temp_error(struct assistance_retry_data *retry_info);
void location_retry_timer_ok(struct assistance_retry_data *retry_info);
bool location_retry_timer_timeout(struct assistance_retry_data *retry_info);
bool location_retry_timer_report_no_response(struct assistance_retry_data *retry_info);

#endif /* LWM2M_CLIENT_UTILS_LOCATION_ASSISTANCE_INTERNAL_H__ */
