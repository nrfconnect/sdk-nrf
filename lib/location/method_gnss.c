/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <logging/log.h>

#include <modem/location.h>
#include <nrf_modem_gnss.h>
#include <nrf_errno.h>

#include "loc_core.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

extern location_event_handler_t event_handler;
extern struct loc_event_data current_event_data;

struct k_work gnss_fix_work;
struct k_work method_gnss_timeout_work;

static int fix_attemps_remaining;
static bool first_fix_obtained;

void method_gnss_event_handler(int event)
{
	switch (event) {
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_DBG("GNSS: Got fix");
		k_work_submit_to_queue(loc_core_work_queue_get(), &gnss_fix_work);
		break;

	case NRF_MODEM_GNSS_EVT_PVT:
		LOG_DBG("GNSS: Got PVT event");
		k_work_submit_to_queue(loc_core_work_queue_get(), &gnss_fix_work);
		break;

	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_DBG("GNSS: Timeout");
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_timeout_work);
		break;
	}
}

int method_gnss_cancel(void)
{
	int err = nrf_modem_gnss_stop();

	if (!err) {
		LOG_DBG("GNSS stopped");
	} else if (err == NRF_EPERM) {
		LOG_ERR("GNSS is not running");
	} else {
		LOG_ERR("Failed to stop GNSS");
	}

	fix_attemps_remaining = 0;
	first_fix_obtained = false;

	return -err;
}

void method_gnss_fix_work_fn(struct k_work *item)
{
	struct nrf_modem_gnss_pvt_data_frame pvt_data;
	static struct loc_location location_result = { 0 };

	if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) != 0) {
		LOG_ERR("Failed to read PVT data from GNSS");
		return;
	}

	/* Store fix data only if we get a valid fix. Thus, the last valid data is always kept
	 * in memory and it is not overwritten in case we get an invalid fix. */
	if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		first_fix_obtained = true;

		location_result.latitude = pvt_data.latitude;
		location_result.longitude = pvt_data.longitude;
		location_result.accuracy = pvt_data.accuracy;
		location_result.datetime.valid = true;
		location_result.datetime.year = pvt_data.datetime.year;
		location_result.datetime.month = pvt_data.datetime.month;
		location_result.datetime.day = pvt_data.datetime.day;
		location_result.datetime.hour = pvt_data.datetime.hour;
		location_result.datetime.minute = pvt_data.datetime.minute;
		location_result.datetime.second = pvt_data.datetime.seconds;
		location_result.datetime.ms = pvt_data.datetime.ms;
	}

	/* Start countdown of remaining fix attemps only once we get the first valid fix. */
	if (first_fix_obtained) {
		if (!fix_attemps_remaining) {
			/* We are done, stop GNSS and publish the fix */
			method_gnss_cancel();
			loc_core_event_cb(&location_result);
		} else {
			fix_attemps_remaining--;
		}
	}
}

void method_gnss_timeout_work_fn(struct k_work *item)
{
	loc_core_event_cb_timeout();

	method_gnss_cancel();
}

int method_gnss_init(void)
{
	int err = nrf_modem_gnss_init();

	if (err) {
		LOG_ERR("Failed to initialize GNSS interface, error %d", err);
		return -err;
	}

	err = nrf_modem_gnss_event_handler_set(method_gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return -err;
	}

	k_work_init(&gnss_fix_work, method_gnss_fix_work_fn);
	k_work_init(&method_gnss_timeout_work, method_gnss_timeout_work_fn);

	return 0;
}

int method_gnss_location_get(const struct loc_method_config *config)
{
	int err = 0;

	const struct loc_gnss_config *gnss_config = &config->config.gnss;

	/* Configure GNSS to continuous tracking mode */
	err = nrf_modem_gnss_fix_interval_set(1);

	uint8_t use_case;

	switch (gnss_config->accuracy) {
	case LOC_ACCURACY_LOW:
		use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START |
			   NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;
		err |= nrf_modem_gnss_use_case_set(use_case);
		break;

	case LOC_ACCURACY_NORMAL:
		use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;
		err |= nrf_modem_gnss_use_case_set(use_case);
		break;

	case LOC_ACCURACY_HIGH:
		use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;
		err |= nrf_modem_gnss_use_case_set(use_case);
		if (!err) {
			fix_attemps_remaining = gnss_config->num_consecutive_fixes;
		}
		break;
	}

	if (err) {
		LOG_ERR("Failed to configure GNSS");
		return -EINVAL;
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Failed to start GNSS");
		return err;
	}

	LOG_DBG("GNSS started");

	return err;
}
