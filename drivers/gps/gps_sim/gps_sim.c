/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <drivers/gpio.h>
#include <drivers/gps.h>
#include <init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <logging/log.h>
#include <math.h>

#define BASE_GPS_SAMPLE_HOUR	(CONFIG_GPS_SIM_BASE_TIMESTAMP / 10000)
#define BASE_GPS_SAMPLE_MINUTE	((CONFIG_GPS_SIM_BASE_TIMESTAMP / 100) % 100)
#define BASE_GPS_SAMPLE_SECOND	(CONFIG_GPS_SIM_BASE_TIMESTAMP % 100)
#define BASE_GSP_SAMPLE_LAT	(CONFIG_GPS_SIM_BASE_LATITUDE / 1000.0)
#define BASE_GSP_SAMPLE_LNG	(CONFIG_GPS_SIM_BASE_LONGITUDE / 1000.0)

/* Whole NMEA sentence, including CRC. */
#define GPS_NMEA_SENTENCE "$GPGGA,%02d%02d%02d.200,%8.3f,%c,%09.3f,%c,1,"      \
			  "12,1.0,0.0,M,0.0,M,,*%02X"

LOG_MODULE_REGISTER(gps_sim, CONFIG_GPS_SIM_LOG_LEVEL);

enum gps_sim_state {
	GPS_SIM_UNINIT,
	GPS_SIM_IDLE,
	GPS_SIM_ACTIVE_SEARCH,
	GPS_SIM_ACTIVE_TIMEOUT,
};

struct gps_sim_data {
	struct device *dev;
	enum gps_sim_state state;
	gps_event_handler_t handler;
	struct gps_config cfg;
	struct gps_nmea nmea_sample;
	struct k_delayed_work start_work;
	struct k_delayed_work stop_work;
	struct k_delayed_work timeout_work;
	struct k_delayed_work fix_work;

	K_THREAD_STACK_MEMBER(work_q_stack, CONFIG_GPS_SIM_STACK_SIZE);
	struct k_work_q work_q;
};

/**
 * @brief Calculates NMEA sentence checksum
 *
 * @param gps_data Pointer to NMEA sentence.
 */
static u8_t nmea_checksum_get(const u8_t *gps_data)
{
	const u8_t *i = gps_data + 1;
	u8_t checksum = 0;

	while (*i != '*') {
		if ((i - gps_data) > GPS_NMEA_SENTENCE_MAX_LENGTH) {
			return 0;
		}
		checksum ^= *i;
		i++;
	}

	return checksum;
}

/**
 * @brief Calculates sine from uptime
 * The input to the sin() function is limited to avoid overflow issues
 *
 * @param offset Offset for the sine.
 * @param amplitude Amplitude of sine.
 */
static double generate_sine(double offset, double amplitude)
{
	u32_t time = k_uptime_get_32() / K_MSEC(CONFIG_GPS_SIM_FIX_TIME);

	return offset + amplitude * sin(time % UINT16_MAX);
}

/**
 * @brief Calculates cosine from uptime
 * The input to the sin() function is limited to avoid overflow issues.
 *
 * @param offset Offset for the cosine.
 * @param amplitude Amplitude of cosine.
 */
static double generate_cosine(double offset, double amplitude)
{
	u32_t time = k_uptime_get_32() / K_MSEC(CONFIG_GPS_SIM_FIX_TIME);

	return offset + amplitude * cos(time % UINT16_MAX);
}

/**
 * @brief Generates a pseudo-random number between -1 and 1.
 */
static double generate_pseudo_random(void)
{
	return (double)rand() / ((double)RAND_MAX / 2.0) - 1.0;
}

/**
 * @brief Function generatig GPS data
 *
 * @param gps_nmea Pointer to gps_nmea struct where the NMEA
 * sentence will be stored.
 * @param max_variation The maximum value the latitude and longitude in the
 * generated sentence can vary for each iteration. In units of minutes.
 */
static void generate_gps_data(struct gps_nmea *gps_data,
			      double max_variation)
{
	static u8_t hour = BASE_GPS_SAMPLE_HOUR;
	static u8_t minute = BASE_GPS_SAMPLE_MINUTE;
	static u8_t second = BASE_GPS_SAMPLE_SECOND;
	static u32_t last_uptime;
	double lat = BASE_GSP_SAMPLE_LAT;
	double lng = BASE_GSP_SAMPLE_LNG;
	char lat_heading = 'N';
	char lng_heading = 'E';
	u32_t uptime;
	u8_t checksum;

	if (IS_ENABLED(CONFIG_GPS_SIM_ELLIPSOID)) {
		lat = generate_sine(BASE_GSP_SAMPLE_LAT, max_variation / 2.0);
		lng = generate_cosine(BASE_GSP_SAMPLE_LNG, max_variation);
	}

	if (IS_ENABLED(CONFIG_GPS_SIM_PSEUDO_RANDOM)) {
		static double acc_lat;
		static double acc_lng;

		acc_lat += max_variation * generate_pseudo_random();
		acc_lng += max_variation * generate_pseudo_random();
		lat = BASE_GSP_SAMPLE_LAT + acc_lat;
		lng = BASE_GSP_SAMPLE_LNG + acc_lng;
	}

	uptime = k_uptime_get_32() / MSEC_PER_SEC;

	second += (uptime - last_uptime);
	last_uptime = uptime;

	if (second > 59) {
		second = second % 60;
		minute += 1;
		if (minute > 59) {
			minute = minute % 60;
			hour += 1;
			hour = hour % 24;
		}
	}

	if (lat < 0) {
		lat *= -1.0;
		lat_heading = 'S';
	}

	if (lng < 0) {
		lng *= -1.0;
		lng_heading = 'W';
	}

	/* Format the sentence, excluding the CRC. */
	snprintf(gps_data->buf,
		 GPS_NMEA_SENTENCE_MAX_LENGTH, GPS_NMEA_SENTENCE,
		 hour, minute, second, lat, lat_heading, lng, lng_heading, 0);

	/* Calculate the CRC (stop when '*' is found, thus excluding the CRC),
	 * then reformat the string, this time including the CRC.
	 *
	 * An alternative way to do this would have been to allocate a small
	 * buffer on the stack, format the CRC into it, and strcat it with
	 * the NMEA sentence.
	 */

	checksum = nmea_checksum_get(gps_data->buf);

	gps_data->len =
		snprintf(gps_data->buf, GPS_NMEA_SENTENCE_MAX_LENGTH,
			 GPS_NMEA_SENTENCE, hour, minute, second, lat,
			 lat_heading, lng, lng_heading, checksum);

	LOG_DBG("%s (%d bytes)", log_strdup(gps_data->buf), gps_data->len);
}

static void notify_event(struct device *dev, struct gps_event *evt)
{
	struct gps_sim_data *drv_data = dev->driver_data;

	if (drv_data->handler) {
		drv_data->handler(dev, evt);
	}
}

static void start_work_fn(struct k_work *work)
{
	struct gps_sim_data *drv_data =
		CONTAINER_OF(work, struct gps_sim_data, start_work);
	struct gps_event evt = {
		.type = GPS_EVT_SEARCH_STARTED,
	};

	notify_event(drv_data->dev, &evt);

	LOG_INF("GPS is started");

	switch (drv_data->cfg.nav_mode) {
	case GPS_NAV_MODE_PERIODIC:
		k_delayed_work_submit_to_queue(&drv_data->work_q,
					&drv_data->start_work,
					K_SECONDS(drv_data->cfg.interval));
		/* Fall through */
	case GPS_NAV_MODE_SINGLE_FIX:
		k_delayed_work_submit_to_queue(&drv_data->work_q,
					&drv_data->timeout_work,
					K_SECONDS(drv_data->cfg.timeout));
		break;
	default:
		break;
	}

	drv_data->state = GPS_SIM_ACTIVE_SEARCH;
	k_delayed_work_submit_to_queue(&drv_data->work_q,
				       &drv_data->fix_work,
				       CONFIG_GPS_SIM_FIX_TIME);
}

static void stop_work_fn(struct k_work *work)
{
	struct gps_sim_data *drv_data =
		CONTAINER_OF(work, struct gps_sim_data, stop_work);
	struct gps_event evt = {
		.type = GPS_EVT_SEARCH_STOPPED,
	};

	drv_data->state = GPS_SIM_IDLE;

	LOG_INF("GPS is stopped");

	notify_event(drv_data->dev, &evt);
}

static void timeout_work_fn(struct k_work *work)
{
	struct gps_sim_data *drv_data =
		CONTAINER_OF(work, struct gps_sim_data, timeout_work);
	struct gps_event evt = {
		.type = GPS_EVT_SEARCH_TIMEOUT,
	};

	if (drv_data->state != GPS_SIM_ACTIVE_SEARCH) {
		return;
	}

	drv_data->state = GPS_SIM_ACTIVE_TIMEOUT;

	LOG_INF("GPS search timed out without getting a position fix");

	notify_event(drv_data->dev, &evt);
}

static void fix_work_fn(struct k_work *work)
{
	struct gps_sim_data *drv_data =
		CONTAINER_OF(work, struct gps_sim_data, fix_work);
	struct gps_event evt = {
		.type = GPS_EVT_NMEA_FIX,
	};

	if (drv_data->state != GPS_SIM_ACTIVE_SEARCH) {
		return;
	}

	k_delayed_work_cancel(&drv_data->timeout_work);

	generate_gps_data(&drv_data->nmea_sample,
			  CONFIG_GPS_SIM_MAX_STEP / 1000.0);

	evt.nmea.len = drv_data->nmea_sample.len;

	memcpy(evt.nmea.buf, drv_data->nmea_sample.buf,
	       drv_data->nmea_sample.len);
	notify_event(drv_data->dev, &evt);

	if (drv_data->cfg.nav_mode == GPS_NAV_MODE_CONTINUOUS) {
		k_delayed_work_submit_to_queue(&drv_data->work_q,
					       &drv_data->fix_work,
					       CONFIG_GPS_SIM_FIX_TIME);
	} else if (drv_data->cfg.nav_mode == GPS_NAV_MODE_SINGLE_FIX) {
		drv_data->state = GPS_SIM_IDLE;
		k_delayed_work_submit_to_queue(&drv_data->work_q,
					       &drv_data->stop_work,
					       K_NO_WAIT);
	}
}

static int start(struct device *dev, struct gps_config *cfg)
{
	struct gps_sim_data *drv_data = dev->driver_data;

	if ((dev == NULL) || (cfg == NULL)) {
		return -EINVAL;
	}

	if (drv_data->state != GPS_SIM_IDLE) {
		LOG_ERR("The GPS must be initialized and stopped first");
		return -EALREADY;
	}

	if (cfg->timeout >= cfg->interval) {
		LOG_ERR("The timeout must be less than the interval");
		return -EINVAL;
	}

	memcpy(&drv_data->cfg, cfg, sizeof(struct gps_config));
	k_delayed_work_submit_to_queue(&drv_data->work_q,
				       &drv_data->start_work,
				       K_NO_WAIT);

	return 0;
}

static int stop(struct device *dev)
{
	struct gps_sim_data *drv_data = dev->driver_data;

	if (dev == NULL) {
		return -EINVAL;
	}

	if (drv_data->state == GPS_SIM_IDLE) {
		LOG_WRN("The GPS is already stopped");
		return -EALREADY;
	}

	drv_data->state = GPS_SIM_IDLE;

	k_delayed_work_cancel(&drv_data->timeout_work);
	k_delayed_work_cancel(&drv_data->fix_work);
	k_delayed_work_cancel(&drv_data->start_work);

	k_delayed_work_submit_to_queue(&drv_data->work_q,
				       &drv_data->stop_work,
				       K_NO_WAIT);

	return 0;
}

static int init(struct device *dev, gps_event_handler_t handler)
{
	struct gps_sim_data *drv_data = dev->driver_data;

	if (handler == NULL) {
		LOG_ERR("Event handler must be provided");
		return -EINVAL;
	}

	if (drv_data->state != GPS_SIM_UNINIT) {
		LOG_ERR("The GPS simulator is already initialized");
		return -EALREADY;
	}

	drv_data->handler = handler;
	drv_data->state = GPS_SIM_IDLE;

	return 0;
}

static int gps_sim_setup(struct device *dev)
{
	struct gps_sim_data *drv_data = dev->driver_data;

	drv_data->dev = dev;
	drv_data->state = GPS_SIM_UNINIT;

	k_delayed_work_init(&drv_data->start_work, start_work_fn);
	k_delayed_work_init(&drv_data->stop_work, stop_work_fn);
	k_delayed_work_init(&drv_data->timeout_work, timeout_work_fn);
	k_delayed_work_init(&drv_data->fix_work, fix_work_fn);
	k_work_q_start(&drv_data->work_q, drv_data->work_q_stack,
		       K_THREAD_STACK_SIZEOF(drv_data->work_q_stack),
		       K_PRIO_PREEMPT(CONFIG_GPS_SIM_WORKQUEUE_PRIORITY));

	return 0;
}

static struct gps_sim_data gps_sim_data;
static const struct gps_driver_api gps_sim_api_funcs = {
	.init = init,
	.start = start,
	.stop = stop,
};

/* TODO: Remove this when the GPS API has Kconfig that sets the priority */
#ifdef CONFIG_GPS_INIT_PRIORITY
#define GPS_INIT_PRIORITY CONFIG_GPS_INIT_PRIORITY
#else
#define GPS_INIT_PRIORITY 90
#endif

DEVICE_AND_API_INIT(gps_sim, CONFIG_GPS_SIM_DEV_NAME, gps_sim_setup,
		    &gps_sim_data, NULL, POST_KERNEL, GPS_INIT_PRIORITY,
		    &gps_sim_api_funcs);
