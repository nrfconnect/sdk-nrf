/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <net/lwm2m_client_utils_location.h>
#include <zephyr/net/lwm2m.h>

#include "location_assistance.h"

void location_retry_timer_cancel(struct assistance_retry_data *retry_info)
{
	int status = k_work_delayable_busy_get(&retry_info->worker);

	retry_info->timeout_period = 0;
	if (status && !(status & K_WORK_RUNNING)) {
		k_work_cancel_delayable(&retry_info->worker);
	}
}

void location_retry_timer_start(struct assistance_retry_data *retry_info)
{
	/* Init or cancel worker */
	location_retry_timer_cancel(retry_info);
	retry_info->timeout_period = LOCATION_ASSISTANT_RESULT_TIMEOUT;
	k_work_schedule(&retry_info->worker, K_SECONDS(LOCATION_ASSISTANT_UPDATE_PERIOD));
}

void location_retry_timer_temp_error(struct assistance_retry_data *retry_info)
{
	retry_info->temp_error = true;
	k_work_schedule(&retry_info->worker, K_SECONDS(retry_info->retry_seconds));
	if (retry_info->retry_seconds < LOCATION_ASSISTANT_MAXIMUM_RETRY_INTERVAL) {
		retry_info->retry_seconds *= 2;
	}
}

void location_retry_timer_ok(struct assistance_retry_data *retry_info)
{
	retry_info->retry_seconds = 0;
	retry_info->temp_error = false;
}

bool location_retry_timer_timeout(struct assistance_retry_data *retry_info)
{

	if (!retry_info->timeout_period) {
		return true;
	}

	if (retry_info->timeout_period > LOCATION_ASSISTANT_UPDATE_PERIOD) {
		retry_info->timeout_period -= LOCATION_ASSISTANT_UPDATE_PERIOD;
		k_work_schedule(&retry_info->worker, K_SECONDS(LOCATION_ASSISTANT_UPDATE_PERIOD));
		/* Trigger update for possible pending data at server */
		lwm2m_rd_client_update();
		return false;
	}

	retry_info->timeout_period = 0;
	return true;
}

bool location_retry_timer_report_no_response(struct assistance_retry_data *retry_info)
{
	if (retry_info->retry_left) {
		retry_info->retry_left--;
		return false;
	}
	return true;
}
