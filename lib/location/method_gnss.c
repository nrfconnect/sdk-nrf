/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <nrf_modem_gnss.h>
#include <nrf_errno.h>
#include <logging/log.h>
#include <modem/location.h>
#include "location.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

extern location_event_handler_t event_handler;
extern struct loc_event_data current_event_data;

struct k_work gnss_fix_work;
struct k_work method_gnss_timeout_work;

int32_t gnss_fix_interval;

void method_gnss_event_handler(int event)
{
	switch (event) {
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_DBG("GNSS: Got fix");
		k_work_submit(&gnss_fix_work);
		break;

	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_DBG("GNSS: Timeout");
		k_work_submit(&method_gnss_timeout_work);
		break;
	}
}

void method_gnss_fix_work_fn(struct k_work *item)
{
	struct nrf_modem_gnss_pvt_data_frame pvt_data;
	struct loc_location location_result = { 0 };

	if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) != 0) {
		LOG_ERR("Failed to read PVT data from GNSS");
		return;
	}

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

	event_location_callback(&location_result);

	/* If configured for single fix mode, stop GNSS */
	if (!gnss_fix_interval) {
		nrf_modem_gnss_stop();
		LOG_DBG("GNSS stopped");
	}
}

void method_gnss_timeout_work_fn(struct k_work *item)
{
	event_location_callback_timeout();

	/* If configured for single fix mode, stop GNSS */
	if (!gnss_fix_interval) {
		nrf_modem_gnss_stop();
		LOG_DBG("GNSS stopped");
	}
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

int method_gnss_configure_and_start(const struct loc_method_config *config, uint16_t interval)
{
	int err = 0;

	const struct loc_gnss_config *gnss_config = &config->config.gnss;

	if (interval == 1 ) {
		LOG_ERR("Failed to configure GNSS, continuous navigation "
			"not supported at the moment.");
		return -EINVAL;
	}

	err = nrf_modem_gnss_fix_interval_set(interval);

	err |= nrf_modem_gnss_fix_retry_set(gnss_config->timeout);

	if (gnss_config->accuracy == LOC_ACCURACY_LOW)
	{
		uint8_t use_case;

		use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START |
			   NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;

		err |= nrf_modem_gnss_use_case_set(use_case);
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

	gnss_fix_interval = interval;

	return err;
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

	return -err;
}