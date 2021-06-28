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

LOG_MODULE_REGISTER(location, CONFIG_LOCATION_LOG_LEVEL);

static location_event_handler_t event_handler;
static struct loc_event_data event_data;
static struct k_work gnss_fix_work;
static struct k_work gnss_timeout_work;

static void gnss_event_handler(int event)
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

static void event_data_init(enum loc_event_id event_id, enum loc_method method)
{
	memset(&event_data, 0, sizeof(event_data));

	event_data.id = event_id;
	event_data.method = method;
}

static void gnss_fix_work_fn(struct k_work *item)
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

	nrf_modem_gnss_stop();
}

static void gnss_timeout_work_fn(struct k_work *item)
{
	event_data_init(LOC_EVT_TIMEOUT, LOC_METHOD_GNSS);

	event_handler(&event_data);

	nrf_modem_gnss_stop();
}

int location_init(location_event_handler_t handler)
{
	int err;
	static bool initialized;

	if (initialized) {
		/* Already initialized */
		return -EPERM;
	}

	if (handler == NULL) {
		LOG_ERR("No event handler given");
		return -EINVAL;
	}

	event_handler = handler;

	err = nrf_modem_gnss_init();
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

	initialized = true;

	LOG_DBG("Library initialized");

	return 0;
}

int location_request(const struct loc_config *config)
{
	int err;
	struct loc_method_config selected_method = { 0 };

	if (config->interval > 0) {
		LOG_ERR("Periodic location updates not yet supported.");
		return -EINVAL;
	}

	for (int i = 0; i < LOC_MAX_METHODS; i++) {
		switch (config->methods[i].method) {
		case LOC_METHOD_CELL_ID:
			LOG_DBG("Cell ID not yet supported");
			break;

		case LOC_METHOD_GNSS:
			LOG_DBG("GNSS selected");
			selected_method = config->methods[i];
			break;

		default:
			break;
		}

		if (selected_method.method) {
			continue;
		}
	}

	if (!selected_method.method) {
		LOG_ERR("No location method found");
		return -EINVAL;
	}

	/* TODO: Add protection so that only one request is handled at a time */

	/* Single fix mode */
	err = nrf_modem_gnss_fix_interval_set(0);
	err |= nrf_modem_gnss_fix_retry_set(selected_method.config.gnss.timeout);
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

	return 0;
}

int location_request_cancel(void)
{
	return -1;
}
