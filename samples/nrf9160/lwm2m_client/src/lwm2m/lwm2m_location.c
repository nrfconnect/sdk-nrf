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

static struct gps_data nmea_data;
static struct device *gps_dev;
static struct gps_trigger gps_trig = {
	.type = GPS_TRIG_DATA_READY,
};

static void update_location_data(struct gps_data *nmea)
{
	LOG_DBG("nmea_data");
	/* TODO: change API to send actual values (not string)
	 * so we can update Location object resources
	 */
}

/**@brief Callback for GPS trigger events */
static void gps_trigger_handler(struct device *dev, struct gps_trigger *trigger)
{
	ARG_UNUSED(trigger);
	int err;
	static u32_t timestamp_prev;

	if (k_uptime_get_32() - timestamp_prev <
	    K_SECONDS(CONFIG_APP_HOLD_TIME_GPS)) {
		return;
	}

	if (gps_dev == NULL) {
		LOG_ERR("Could not get %s device", CONFIG_GPS_DEV_NAME);
		return;
	}

	err = gps_sample_fetch(gps_dev);
	if (err < 0) {
		LOG_ERR("GPS sample fetch error: %d", err);
		return;
	}

	err = gps_channel_get(gps_dev, GPS_CHAN_NMEA, &nmea_data);
	if (err < 0) {
		LOG_ERR("GPS channel get error: %d", err);
		return;
	}

	update_location_data(&nmea_data);
	timestamp_prev = k_uptime_get_32();
}

int lwm2m_init_location(void)
{
	int err;

	gps_dev = device_get_binding(CONFIG_GPS_DEV_NAME);
	if (gps_dev == NULL) {
		LOG_ERR("Could not get %s device", CONFIG_GPS_DEV_NAME);
		return -EINVAL;
	}

	LOG_DBG("GPS device found: %s", CONFIG_GPS_DEV_NAME);

	if (IS_ENABLED(CONFIG_GPS_TRIGGER)) {
		err = gps_trigger_set(gps_dev, &gps_trig, gps_trigger_handler);
		if (err) {
			LOG_ERR("Could not set trigger, error code: %d", err);
			return err;
		}
	}

	err = gps_sample_fetch(gps_dev);
	__ASSERT(err == 0, "GPS sample could not be fetched.");

	err = gps_channel_get(gps_dev, GPS_CHAN_NMEA, &nmea_data);
	__ASSERT(err == 0, "GPS sample could not be retrieved.");

	update_location_data(&nmea_data);
	return 0;
}
