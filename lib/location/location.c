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
static struct location_data location;
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

static void gnss_fix_work_fn(struct k_work *item)
{
	struct nrf_modem_gnss_pvt_data_frame pvt_data;

	if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) != 0) {
		LOG_ERR("Failed to read PVT data from GNSS");
		return;
	}

	location.used_method = LOC_METHOD_GNSS;
	location.latitude = pvt_data.latitude;
	location.longitude = pvt_data.longitude;
	location.accuracy = pvt_data.accuracy;

	event_handler(LOC_EVT_LOCATION, &location);

	nrf_modem_gnss_stop();
}

static void gnss_timeout_work_fn(struct k_work *item)
{
	event_handler(LOC_EVT_TIMEOUT, NULL);

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

int location_get(const struct location_config *config)
{
	int err;

	if (config->method_count < 1 || config->method_count > LOC_MAX_METHODS) {
		LOG_ERR("Too few or too many location methods in the configuration");
		return -1;
	}

	/* TODO: Add protection so that only one request is handled at a time */

	/* Single fix mode */
	err = nrf_modem_gnss_fix_interval_set(0);
	/* TODO: Make GNSS timeout configurable */
	err |= nrf_modem_gnss_fix_retry_set(120);
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

int location_periodic_start(const struct location_config *config,
			    uint16_t interval)
{
	return -1;
}

int location_periodic_stop(void)
{
	return -1;
}
