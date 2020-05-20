/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <drivers/gps.h>
#include <stdio.h>
#include <net/lwm2m.h>

#include "ui.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_loc, CONFIG_APP_LOG_LEVEL);

static struct gps_nmea nmea_data;
static struct device *gps_dev;

static void update_location_data(struct gps_nmea *nmea)
{
	ARG_UNUSED(nmea);
	LOG_DBG("nmea_data");
	/* TODO: change API to send actual values (not string)
	 * so we can update Location object resources
	 */
}

/**@brief Callback for GPS events */
static void gps_event_handler(struct device *dev, struct gps_event *evt)
{
	ARG_UNUSED(dev);
	static u32_t timestamp_prev;

	if (evt->type != GPS_EVT_NMEA_FIX) {
		return;
	}

	if (k_uptime_get_32() - timestamp_prev <
	    CONFIG_APP_HOLD_TIME_GPS * MSEC_PER_SEC) {
		return;
	}

	if (gps_dev == NULL) {
		LOG_ERR("Could not get %s device",
			log_strdup(CONFIG_GPS_DEV_NAME));
		return;
	}

	memcpy(&nmea_data, &evt->nmea, sizeof(struct gps_nmea));

	update_location_data(&nmea_data);
	timestamp_prev = k_uptime_get_32();
}

int lwm2m_init_location(void)
{
	int err;

	gps_dev = device_get_binding(CONFIG_GPS_DEV_NAME);
	if (gps_dev == NULL) {
		LOG_ERR("Could not get %s device",
			log_strdup(CONFIG_GPS_DEV_NAME));
		return -EINVAL;
	}

	LOG_DBG("GPS device found: %s", log_strdup(CONFIG_GPS_DEV_NAME));

	err = gps_init(gps_dev, gps_event_handler);
	if (err) {
		LOG_ERR("Could not set trigger, error code: %d", err);
		return err;
	}

	return 0;
}
