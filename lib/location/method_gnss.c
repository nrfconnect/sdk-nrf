/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <nrf_modem_gnss.h>
#include <logging/log.h>
#include <modem/location.h>
#include "location.h"

LOG_MODULE_REGISTER(method_gnss, CONFIG_LOCATION_LOG_LEVEL);

extern location_event_handler_t event_handler;
extern struct loc_event_data event_data;

struct k_work gnss_fix_work;
struct k_work gnss_timeout_work;

int32_t gnss_fix_interval;

void gnss_event_handler(int event)
{
	switch (event) {
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_DBG("GNSS: Got fix");
		k_work_submit(&gnss_fix_work);
		break;

	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_DBG("GNSS: Timeout");
		k_work_submit(&gnss_timeout_work);
		break;
	}
}

void gnss_fix_work_fn(struct k_work *item)
{
	struct nrf_modem_gnss_pvt_data_frame pvt_data;

	if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) != 0) {
		LOG_ERR("Failed to read PVT data from GNSS");
		return;
	}

	event_data_init(LOC_EVT_LOCATION, LOC_METHOD_GNSS);
	event_data.location.latitude = pvt_data.latitude;
	event_data.location.longitude = pvt_data.longitude;
	event_data.location.accuracy = pvt_data.accuracy;
	event_data.location.datetime.valid = true;
	event_data.location.datetime.year = pvt_data.datetime.year;
	event_data.location.datetime.month = pvt_data.datetime.month;
	event_data.location.datetime.day = pvt_data.datetime.day;
	event_data.location.datetime.hour = pvt_data.datetime.hour;
	event_data.location.datetime.minute = pvt_data.datetime.minute;
	event_data.location.datetime.second = pvt_data.datetime.seconds;
	event_data.location.datetime.ms = pvt_data.datetime.ms;

	event_handler(&event_data);

	/* If configured for single fix mode, stop GNSS */
	if (!gnss_fix_interval) {
		nrf_modem_gnss_stop();
		LOG_DBG("GNSS stopped");
	}
}

void gnss_timeout_work_fn(struct k_work *item)
{
	event_data_init(LOC_EVT_TIMEOUT, LOC_METHOD_GNSS);

	event_handler(&event_data);

	/* If configured for single fix mode, stop GNSS */
	if (!gnss_fix_interval) {
		nrf_modem_gnss_stop();
		LOG_DBG("GNSS stopped");
	}
}

int gnss_init()
{
	int err = nrf_modem_gnss_init();
	if (err) {
		LOG_ERR("Failed to initialize GNSS interface, error %d", err);
		return -err;
	}

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return -err;
	}

	k_work_init(&gnss_fix_work, gnss_fix_work_fn);
	k_work_init(&gnss_timeout_work, gnss_timeout_work_fn);

	return 0;
}

int gnss_configure_and_start(struct loc_gnss_config *gnss_config, uint16_t interval)
{
	int err = 0;

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