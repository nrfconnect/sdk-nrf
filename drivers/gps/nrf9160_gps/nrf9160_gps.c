/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/gps.h>
#include <init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <logging/log.h>
#include <nrf_modem_gnss.h>
#include <modem/at_cmd.h>
#ifdef CONFIG_NRF9160_GPS_HANDLE_MODEM_CONFIGURATION
#include <modem/lte_lc.h>
#endif

LOG_MODULE_REGISTER(nrf9160_gps, CONFIG_NRF9160_GPS_LOG_LEVEL);

/* Aligned strings describing satellite states based on flags. */
#define sv_used_str(x) ((x)?"    used":"not used")
#define sv_unhealthy_str(x) ((x)?"not healthy":"    healthy")

#define GPS_BLOCKED_TIMEOUT CONFIG_NRF9160_GPS_PRIORITY_WINDOW_TIMEOUT_SEC

struct gnss_config {
	uint16_t retry;
	uint16_t interval;
	uint16_t nmea_mask;
	uint32_t delete_mask;
	uint8_t power_mode;
	uint8_t use_case;
	bool priority;
};

struct gps_drv_data {
	const struct device *dev;
	gps_event_handler_t handler;
	struct gnss_config config;
	atomic_t is_init;
	atomic_t is_running;
	atomic_t fix_valid;
	atomic_t timeout_occurred;
	atomic_t operation_blocked;
	K_THREAD_STACK_MEMBER(thread_stack,
			      CONFIG_NRF9160_GPS_THREAD_STACK_SIZE);
	struct k_thread thread;
	k_tid_t thread_id;
	struct k_work start_work;
	struct k_work stop_work;
	struct k_work error_work;
	struct k_work_delayable timeout_work;
	struct k_work_delayable blocked_work;
};

/* Struct for an event item. The data is read in the GNSS event handler and passed as a part of the
 * event item because an NMEA string would get overwritten by the next NMEA string before the
 * consumer thread is able to read it. This is not a problem with PVT and A-GPS request data, but
 * all data is handled in the same way for simplicity.
 */
struct event_item {
	uint8_t id;
	void    *data;
};

K_MSGQ_DEFINE(event_msgq, sizeof(struct event_item), 10, 4);

static uint64_t start_timestamp;
static uint64_t fix_timestamp;

static uint16_t type_lookup_gps2gnss_api[] = {
	[GPS_AGPS_UTC_PARAMETERS]	= NRF_MODEM_GNSS_AGPS_UTC_PARAMETERS,
	[GPS_AGPS_EPHEMERIDES]		= NRF_MODEM_GNSS_AGPS_EPHEMERIDES,
	[GPS_AGPS_ALMANAC]		= NRF_MODEM_GNSS_AGPS_ALMANAC,
	[GPS_AGPS_KLOBUCHAR_CORRECTION]
		= NRF_MODEM_GNSS_AGPS_KLOBUCHAR_IONOSPHERIC_CORRECTION,
	[GPS_AGPS_NEQUICK_CORRECTION]
		= NRF_MODEM_GNSS_AGPS_NEQUICK_IONOSPHERIC_CORRECTION,
	[GPS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS]
		= NRF_MODEM_GNSS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS,
	[GPS_AGPS_LOCATION]		= NRF_MODEM_GNSS_AGPS_LOCATION,
	[GPS_AGPS_INTEGRITY]		= NRF_MODEM_GNSS_AGPS_INTEGRITY,
};

static int get_event_data(void **dest, uint8_t type, size_t len)
{
	void *data;

	data = k_malloc(len);
	if (data == NULL) {
		return -1;
	}

	if (nrf_modem_gnss_read(data, len, type) != 0) {
		k_free(data);
		return -1;
	}

	*dest = data;

	return 0;
}

/* This handler is called in ISR, so need to avoid heavy processing. */
static void gnss_event_handler(int event_id)
{
	int err = 0;
	struct event_item event = {
		.id = event_id,
		.data = NULL
	};

	switch (event_id) {
	case NRF_MODEM_GNSS_EVT_PVT:
		err = get_event_data(&event.data,
				     NRF_MODEM_GNSS_DATA_PVT,
				     sizeof(struct nrf_modem_gnss_pvt_data_frame));
		break;
	case NRF_MODEM_GNSS_EVT_NMEA:
		err = get_event_data(&event.data,
				     NRF_MODEM_GNSS_DATA_NMEA,
				     sizeof(struct nrf_modem_gnss_nmea_data_frame));
		break;
	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
		err = get_event_data(&event.data,
				     NRF_MODEM_GNSS_DATA_AGPS_REQ,
				     sizeof(struct nrf_modem_gnss_agps_data_frame));
		break;
	default:
		/* All other events are just ignored. */
		return;
	}

	if (err) {
		/* Failed to get event data. */
		return;
	}

	err = k_msgq_put(&event_msgq, &event, K_NO_WAIT);
	if (err) {
		/* Failed to put event into queue. */
		k_free(event.data);

		const struct device *dev = device_get_binding(CONFIG_NRF9160_GPS_DEV_NAME);

		if (dev != NULL) {
			struct gps_drv_data *drv_data = dev->data;

			k_work_submit(&drv_data->error_work);
		}
	}
}

static void copy_pvt(struct gps_pvt *dest, struct nrf_modem_gnss_pvt_data_frame *src)
{
	dest->latitude = src->latitude;
	dest->longitude = src->longitude;
	dest->altitude = src->altitude;
	dest->accuracy = src->accuracy;
	dest->speed = src->speed;
	dest->heading = src->heading;
	dest->datetime.year = src->datetime.year;
	dest->datetime.month = src->datetime.month;
	dest->datetime.day = src->datetime.day;
	dest->datetime.hour = src->datetime.hour;
	dest->datetime.minute = src->datetime.minute;
	dest->datetime.seconds = src->datetime.seconds;
	dest->datetime.ms = src->datetime.ms;
	dest->pdop = src->pdop;
	dest->hdop = src->hdop;
	dest->vdop = src->vdop;
	dest->tdop = src->tdop;

	for (size_t i = 0;
	     i < MIN(NRF_MODEM_GNSS_MAX_SATELLITES, GPS_PVT_MAX_SV_COUNT); i++) {
		dest->sv[i].sv = src->sv[i].sv;
		dest->sv[i].cn0 = src->sv[i].cn0;
		dest->sv[i].elevation = src->sv[i].elevation;
		dest->sv[i].azimuth = src->sv[i].azimuth;
		dest->sv[i].signal = src->sv[i].signal;
		dest->sv[i].in_fix =
			(src->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) ==
			NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;
		dest->sv[i].unhealthy =
			(src->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) ==
			NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY;
	}
}

static void print_satellite_stats(struct nrf_modem_gnss_pvt_data_frame *pvt)
{
	uint8_t  n_tracked = 0;
	uint8_t  n_used = 0;
	uint8_t  n_unhealthy = 0;

	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
		uint8_t sv = pvt->sv[i].sv;
		bool used = (pvt->sv[i].flags &
			     NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) ? true : false;
		bool unhealthy = (pvt->sv[i].flags &
				  NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) ? true : false;

		if (sv) { /* SV number 0 indicates no satellite */
			n_tracked++;
			if (used) {
				n_used++;
			}
			if (unhealthy) {
				n_unhealthy++;
			}

			LOG_DBG("Tracking SV %2u: %s, %s", sv,
				sv_used_str(used),
				sv_unhealthy_str(unhealthy));
		}
	}

	LOG_DBG("Tracking: %d Using: %d Unhealthy: %d", n_tracked,
							n_used,
							n_unhealthy);
	LOG_DBG("Seconds since last fix %d",
			(uint32_t)(k_uptime_get() - fix_timestamp) / 1000);
	if (fix_timestamp > start_timestamp) {
		char buf[35];

		snprintf(buf, sizeof(buf), "Seconds for this fix %d",
			 (uint32_t)(fix_timestamp - start_timestamp) / 1000);
		LOG_DBG("%s", log_strdup(buf));
		LOG_DBG("%02u:%02u:%02u.%03u %04u-%02u-%02u", pvt->datetime.hour,
			pvt->datetime.minute, pvt->datetime.seconds,
			pvt->datetime.ms, pvt->datetime.year,
			pvt->datetime.month, pvt->datetime.day);
	}
}

static void notify_event(const struct device *dev, struct gps_event *evt)
{
	struct gps_drv_data *drv_data = dev->data;

	if (drv_data->handler) {
		drv_data->handler(dev, evt);
	}
}

static void cancel_works(struct gps_drv_data *drv_data)
{
	k_work_cancel_delayable(&drv_data->timeout_work);
	k_work_cancel_delayable(&drv_data->blocked_work);
}

static void gps_thread(int dev_ptr)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct gps_drv_data *drv_data = dev->data;
	struct event_item event;
	struct gps_event evt;
	struct nrf_modem_gnss_pvt_data_frame *pvt_data;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;
	struct nrf_modem_gnss_agps_data_frame *agps_data;
	size_t nmea_len;

	while (true) {
		k_msgq_get(&event_msgq, &event, K_FOREVER);

		switch (event.id) {
		case NRF_MODEM_GNSS_EVT_PVT:
			pvt_data = event.data;

			if (pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
				k_work_cancel_delayable(&drv_data->timeout_work);
			} else {
				k_work_reschedule(&drv_data->timeout_work, K_SECONDS(5));
			}

			if (drv_data->config.interval > 1) {
				/* Periodic mode. */
				if (atomic_get(&drv_data->fix_valid) ||
				    atomic_get(&drv_data->timeout_occurred)) {
					/* Next search is starting, send event. */
					atomic_set(&drv_data->timeout_occurred, 0);
					atomic_set(&drv_data->operation_blocked, 0);
					evt.type = GPS_EVT_SEARCH_STARTED;
					notify_event(dev, &evt);
					start_timestamp = k_uptime_get();
				}
			}

			atomic_set(&drv_data->fix_valid, 0);

			if (pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME ||
			    pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) {
				if (atomic_get(&drv_data->operation_blocked)) {
					/* Avoid spamming the logs and app. */
					goto free_event;
				}

				/* If LTE is used alongside GPS, PSM, eDRX or
				 * DRX needs to be enabled for the GPS to
				 * operate. If PSM is used, the GPS will
				 * normally operate when active time expires.
				 */
				LOG_DBG("Waiting for time window to operate");

				atomic_set(&drv_data->operation_blocked, 1);
				evt.type = GPS_EVT_OPERATION_BLOCKED;

				notify_event(dev, &evt);

				/* If GPS is blocked more than the specified
				 * duration and GPS priority is set by the
				 * application, GPS priority is requested.
				 */
				if (drv_data->config.priority) {
					k_work_schedule(
						&drv_data->blocked_work,
						K_SECONDS(GPS_BLOCKED_TIMEOUT));
				}

				goto free_event;
			} else if (atomic_get(&drv_data->operation_blocked)) {
				/* GPS has been unblocked. */
				LOG_DBG("GPS has time window to operate");

				atomic_set(&drv_data->operation_blocked, 0);
				evt.type = GPS_EVT_OPERATION_UNBLOCKED;

				notify_event(dev, &evt);

				k_work_cancel_delayable(&drv_data->blocked_work);
			}

			copy_pvt(&evt.pvt, pvt_data);

			if (pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
				LOG_DBG("PVT: Position fix");

				evt.type = GPS_EVT_PVT_FIX;
				fix_timestamp = k_uptime_get();
				atomic_set(&drv_data->fix_valid, 1);
			} else {
				evt.type = GPS_EVT_PVT;
				atomic_set(&drv_data->fix_valid, 0);
			}

			notify_event(dev, &evt);
			print_satellite_stats(pvt_data);
			break;
		case NRF_MODEM_GNSS_EVT_NMEA:
			if (atomic_get(&drv_data->operation_blocked)) {
				goto free_event;
			}

			nmea_data = event.data;
			nmea_len = strlen(nmea_data->nmea_str);
			memcpy(evt.nmea.buf, nmea_data->nmea_str, nmea_len + 1);

			/* Null termination is not included in length. */
			evt.nmea.len = nmea_len;

			if (atomic_get(&drv_data->fix_valid)) {
				LOG_DBG("NMEA: Position fix");

				evt.type = GPS_EVT_NMEA_FIX;
			} else {
				evt.type = GPS_EVT_NMEA;
			}

			notify_event(dev, &evt);
			break;
		case NRF_MODEM_GNSS_EVT_AGPS_REQ:
			LOG_DBG("A-GPS data update needed");

			agps_data = event.data;

			evt.type = GPS_EVT_AGPS_DATA_NEEDED;
			evt.agps_request.sv_mask_ephe =
				agps_data->sv_mask_ephe;
			evt.agps_request.sv_mask_alm =
				agps_data->sv_mask_alm;
			evt.agps_request.utc =
				agps_data->data_flags &
				NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST ? 1 : 0;
			evt.agps_request.klobuchar =
				agps_data->data_flags &
				NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST ? 1 : 0;
			evt.agps_request.nequick =
				agps_data->data_flags &
				NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST ? 1 : 0;
			evt.agps_request.system_time_tow =
				agps_data->data_flags &
				NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST ? 1 : 0;
			evt.agps_request.position =
				agps_data->data_flags &
				NRF_MODEM_GNSS_AGPS_POSITION_REQUEST ? 1 : 0;
			evt.agps_request.integrity =
				agps_data->data_flags &
				NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST ? 1 : 0;

			notify_event(dev, &evt);
			break;
		default:
			break;
		}

free_event:
		k_free(event.data);
	}
}

static void init_thread(const struct device *dev)
{
	struct gps_drv_data *drv_data = dev->data;

	drv_data->thread_id =
		k_thread_create(&drv_data->thread, drv_data->thread_stack,
				K_THREAD_STACK_SIZEOF(drv_data->thread_stack),
				(k_thread_entry_t)gps_thread, (void *)dev, NULL, NULL,
				K_PRIO_PREEMPT(CONFIG_NRF9160_GPS_THREAD_PRIORITY),
				0, K_NO_WAIT);
	k_thread_name_set(drv_data->thread_id, "nrf9160_gps_driver");
}

#ifdef CONFIG_NRF9160_GPS_HANDLE_MODEM_CONFIGURATION
static int enable_gps(const struct device *dev)
{
	int err;
	enum lte_lc_system_mode system_mode;
	enum lte_lc_func_mode functional_mode;
	enum lte_lc_system_mode_preference preference;

	err = lte_lc_system_mode_get(&system_mode, &preference);
	if (err) {
		LOG_ERR("Could not get modem system mode, error: %d", err);
		return err;
	}

	if ((system_mode != LTE_LC_SYSTEM_MODE_GPS) &&
	    (system_mode != LTE_LC_SYSTEM_MODE_LTEM_GPS) &&
	    (system_mode != LTE_LC_SYSTEM_MODE_NBIOT_GPS) &&
	    (system_mode != LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS)) {
		enum lte_lc_system_mode new_mode = LTE_LC_SYSTEM_MODE_GPS;

		if (system_mode == LTE_LC_SYSTEM_MODE_LTEM) {
			new_mode = LTE_LC_SYSTEM_MODE_LTEM_GPS;
		} else if (system_mode == LTE_LC_SYSTEM_MODE_NBIOT) {
			new_mode = LTE_LC_SYSTEM_MODE_NBIOT_GPS;
		}

		LOG_DBG("GPS mode is not enabled, attempting to enable it");

		err = lte_lc_system_mode_set(new_mode, preference);
		if (err) {
			LOG_ERR("Could not enable GPS mode, error: %d", err);
			return err;
		}
	}

	LOG_DBG("GPS mode is enabled");

	err = lte_lc_func_mode_get(&functional_mode);
	if (err) {
		LOG_ERR("Could not get modem's functional mode, error: %d",
			err);
		return err;
	}

	if ((functional_mode != LTE_LC_FUNC_MODE_NORMAL) &&
	    (functional_mode != LTE_LC_FUNC_MODE_ACTIVATE_GNSS)) {
		LOG_ERR("GPS is not supported in current functional mode");
		return -EIO;
	}

	return err;
}
#endif

static void set_nmea_mask(uint16_t *nmea_mask)
{
	*nmea_mask = 0;

#ifdef CONFIG_NRF9160_GPS_NMEA_GSV
	*nmea_mask |= NRF_MODEM_GNSS_NMEA_GSV_MASK;
#endif
#ifdef CONFIG_NRF9160_GPS_NMEA_GSA
	*nmea_mask |= NRF_MODEM_GNSS_NMEA_GSA_MASK;
#endif
#ifdef CONFIG_NRF9160_GPS_NMEA_GLL
	*nmea_mask |= NRF_MODEM_GNSS_NMEA_GLL_MASK;
#endif
#ifdef CONFIG_NRF9160_GPS_NMEA_GGA
	*nmea_mask |= NRF_MODEM_GNSS_NMEA_GGA_MASK;
#endif
#ifdef CONFIG_NRF9160_GPS_NMEA_RMC
	*nmea_mask |= NRF_MODEM_GNSS_NMEA_RMC_MASK;
#endif
}

static int parse_cfg(struct gps_config *cfg_src,
		     struct gnss_config *cfg_dst)
{
	memset(cfg_dst, 0, sizeof(struct gnss_config));

	switch (cfg_src->nav_mode) {
	case GPS_NAV_MODE_SINGLE_FIX:
		cfg_dst->interval = 0;
		cfg_dst->retry = cfg_src->timeout < 0 ? 0 : cfg_src->timeout;
		break;
	case GPS_NAV_MODE_CONTINUOUS:
		cfg_dst->retry = 0;
		cfg_dst->interval = 1;
		break;
	case GPS_NAV_MODE_PERIODIC:
		if (cfg_src->interval < 10) {
			LOG_ERR("Minimum periodic interval is 10 sec");
			return -EINVAL;
		}

		cfg_dst->retry = cfg_src->timeout;
		cfg_dst->interval = cfg_src->interval;
		break;
	default:
		LOG_ERR("Invalid operation mode (%d), GPS will not start",
			cfg_src->nav_mode);
		return -EINVAL;
	}

	if (cfg_src->delete_agps_data) {
		cfg_dst->delete_mask = 0x7F;
	}

	set_nmea_mask(&cfg_dst->nmea_mask);

	if (cfg_src->power_mode == GPS_POWER_MODE_PERFORMANCE) {
		cfg_dst->power_mode = NRF_MODEM_GNSS_PSM_DUTY_CYCLING_PERFORMANCE;
	} else if (cfg_src->power_mode == GPS_POWER_MODE_SAVE) {
		cfg_dst->power_mode = NRF_MODEM_GNSS_PSM_DUTY_CYCLING_POWER;
	}

	cfg_dst->priority = cfg_src->priority;

	/* This is the only start mode supported by GNSS. */
	cfg_dst->use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

	if (cfg_src->accuracy == GPS_ACCURACY_LOW) {
		cfg_dst->use_case |= NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;
	}

	return 0;
}

static int configure_antenna(void)
{
	int err = 0;

#if CONFIG_NRF9160_GPS_SET_MAGPIO
	err = at_cmd_write(CONFIG_NRF9160_GPS_MAGPIO_STRING,
				NULL, 0, NULL);
	if (err) {
		LOG_ERR("Could not configure MAGPIO, error: %d", err);
		return err;
	}

	LOG_DBG("MAGPIO set: %s",
		log_strdup(CONFIG_NRF9160_GPS_MAGPIO_STRING));
#endif /* CONFIG_NRF9160_GPS_SET_MAGPIO */

#if CONFIG_NRF9160_GPS_SET_COEX0
	err = at_cmd_write(CONFIG_NRF9160_GPS_COEX0_STRING,
				NULL, 0, NULL);
	if (err) {
		LOG_ERR("Could not configure COEX0, error: %d", err);
		return err;
	}

	LOG_DBG("COEX0 set: %s",
		log_strdup(CONFIG_NRF9160_GPS_COEX0_STRING));
#endif /* CONFIG_NRF9160_GPS_SET_COEX0 */

	return err;
}

static void start_work_fn(struct k_work *work)
{
	struct gps_drv_data *drv_data =
		CONTAINER_OF(work, struct gps_drv_data, start_work);
	const struct device *dev = drv_data->dev;
	struct gps_event evt = {
		.type = GPS_EVT_SEARCH_STARTED
	};

	notify_event(dev, &evt);
}

static void stop_work_fn(struct k_work *work)
{
	struct gps_drv_data *drv_data =
		CONTAINER_OF(work, struct gps_drv_data, stop_work);
	const struct device *dev = drv_data->dev;
	struct gps_event evt = {
		.type = GPS_EVT_SEARCH_STOPPED
	};

	notify_event(dev, &evt);
}


static void error_work_fn(struct k_work *work)
{
	struct gps_drv_data *drv_data =
		CONTAINER_OF(work, struct gps_drv_data, error_work);
	const struct device *dev = drv_data->dev;
	struct gps_event evt = {
		.type = GPS_EVT_ERROR,
		/* Only one error code used, so it can be hardcoded. */
		.error = GPS_ERROR_EVT_QUEUE_FULL
	};

	notify_event(dev, &evt);
}

static void timeout_work_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gps_drv_data *drv_data =
		CONTAINER_OF(dwork, struct gps_drv_data, timeout_work);
	const struct device *dev = drv_data->dev;
	struct gps_event evt = {
		.type = GPS_EVT_SEARCH_TIMEOUT
	};

	atomic_set(&drv_data->timeout_occurred, 1);
	notify_event(dev, &evt);
}

static void blocked_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;

	err = nrf_modem_gnss_prio_mode_enable();
	if (err) {
		LOG_ERR("Failed to enable GPS priority, error: %d", err);
		return;
	}

	LOG_DBG("GPS priority enabled");
}

static int setup(const struct device *dev)
{
	struct gps_drv_data *drv_data = dev->data;

	drv_data->dev = dev;

	return 0;
}

static int init(const struct device *dev, gps_event_handler_t handler)
{
	struct gps_drv_data *drv_data = dev->data;
	int err;

	if (atomic_get(&drv_data->is_init)) {
		LOG_WRN("GPS is already initialized");
		return -EALREADY;
	}

	if (handler == NULL) {
		LOG_ERR("No event handler provided");
		return -EINVAL;
	}

	drv_data->handler = handler;

	err = configure_antenna();
	if (err) {
		LOG_ERR("Failed to configure antenna, error: %d", err);
		return err;
	}

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error: %d", err);
		return err;
	}

	init_thread(dev);

	k_work_init(&drv_data->start_work, start_work_fn);
	k_work_init(&drv_data->stop_work, stop_work_fn);
	k_work_init(&drv_data->error_work, error_work_fn);
	k_work_init_delayable(&drv_data->timeout_work, timeout_work_fn);
	k_work_init_delayable(&drv_data->blocked_work, blocked_work_fn);

	atomic_set(&drv_data->is_init, 1);
	atomic_set(&drv_data->is_running, 0);

	return 0;
}

static int start(const struct device *dev, struct gps_config *cfg)
{
	int err;
	struct gps_drv_data *drv_data = dev->data;

	if (atomic_get(&drv_data->is_init) != 1) {
		LOG_WRN("GPS must be initialized first");
		return -ENODEV;
	}

	if (atomic_get(&drv_data->is_running)) {
		LOG_DBG("Restarting GPS");

		cancel_works(drv_data);

		atomic_set(&drv_data->is_running, 0);

		err = nrf_modem_gnss_stop();
		if (err) {
			LOG_WRN("Stopping GPS failed, error: %d", err);
			/* Try to restart GPS regardless of the error. */
		}
	}

	if (parse_cfg(cfg, &drv_data->config) != 0) {
		LOG_ERR("Invalid GPS configuration");
		return -EINVAL;
	}

#ifdef CONFIG_NRF9160_GPS_HANDLE_MODEM_CONFIGURATION
	if (enable_gps(dev) != 0) {
		LOG_ERR("Failed to enable GPS");
		return -EIO;
	}
#endif

	err = nrf_modem_gnss_fix_retry_set(drv_data->config.retry);
	if (err) {
		LOG_ERR("Failed to set fix retry value, error: %d", err);
		return -EIO;
	}

	err = nrf_modem_gnss_fix_interval_set(drv_data->config.interval);
	if (err) {
		LOG_ERR("Failed to set fix interval value, error: %d", err);
		return -EIO;
	}

	err = nrf_modem_gnss_nmea_mask_set(drv_data->config.nmea_mask);
	if (err) {
		LOG_ERR("Failed to set NMEA mask, error: %d", err);
		return -EIO;
	}

	err = nrf_modem_gnss_power_mode_set(drv_data->config.power_mode);
	if (err) {
		LOG_ERR("Failed to set GPS power mode, error: %d", err);
		return -EIO;
	}

#ifdef CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK
	err = nrf_modem_gnss_elevation_threshold_set(CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK);
	if (!err) {
		LOG_DBG("Set elevation threshold to %d", CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK);
	} else {
		LOG_ERR("Failed to set elevation threshold: %d", err);
	}
#endif

	uint8_t use_case = drv_data->config.use_case;

	if (IS_ENABLED(CONFIG_NRF_CLOUD_AGPS_FILTERED) &&
	    (drv_data->config.interval > 1)) {
		/** when in periodic tracking mode, prevent modem from wasting time
		 *   downloading missing ephemerides
		 */
		use_case |= NRF_MODEM_GNSS_USE_CASE_SCHED_DOWNLOAD_DISABLE;
		err = nrf_modem_gnss_use_case_set(use_case);

		if (!err) {
			LOG_DBG("Disabled scheduled GNSS downloads");
		} else {
			LOG_DBG("Could not disable scheduled GNSS downloads: %d", err);
			use_case &= ~NRF_MODEM_GNSS_USE_CASE_SCHED_DOWNLOAD_DISABLE;
			err = nrf_modem_gnss_use_case_set(use_case);
		}
	} else {
		err = nrf_modem_gnss_use_case_set(use_case);
	}
	if (err) {
		LOG_ERR("Failed to configure use case: %d", err);
		return -EIO;
	}

	if (drv_data->config.delete_mask != 0) {
		err = nrf_modem_gnss_nv_data_delete(drv_data->config.delete_mask);
		if (err) {
			LOG_ERR("Failed to delete NV data, error: %d", err);
			return -EIO;
		}
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Starting GPS failed, error: %d", err);
		return -EIO;
	}

	atomic_set(&drv_data->is_running, 1);
	atomic_set(&drv_data->fix_valid, 0);
	atomic_set(&drv_data->timeout_occurred, 0);
	atomic_set(&drv_data->operation_blocked, 0);

	start_timestamp = k_uptime_get();

	/* Offloading event dispatching to workqueue, as also other events
	 * in this driver are sent from context different than the calling
	 * context.
	 */
	k_work_submit(&drv_data->start_work);

	LOG_DBG("GPS operational");

	return 0;
}

static int stop(const struct device *dev)
{
	int err;
	struct gps_drv_data *drv_data = dev->data;

	if (atomic_get(&drv_data->is_running) == 0) {
		/* The GPS is already stopped, attempting to stop it again would
		 * result in an error. Instead, notify that it's stopped.
		 */
		goto notify;
	}

	LOG_DBG("Stopping GPS");

	cancel_works(drv_data);

	atomic_set(&drv_data->is_running, 0);

	err = nrf_modem_gnss_stop();
	if (err) {
		LOG_ERR("Stopping GPS failed, error: %d", err);
		return -EIO;
	}

notify:
	/* Offloading event dispatching to workqueue, as also other events
	 * in this driver are sent from context different than the calling
	 * context.
	 */
	k_work_submit(&drv_data->stop_work);

	return 0;
}

static int agps_write(const struct device *dev, enum gps_agps_type type,
		      void *data, size_t data_len)
{
	ARG_UNUSED(dev);

	int err;
	uint16_t data_type = type_lookup_gps2gnss_api[type];

	err = nrf_modem_gnss_agps_write(data, data_len, data_type);
	if (err) {
		LOG_ERR("Failed to send A-GPS data to modem, error: %d", err);
		return err;
	}

	LOG_DBG("Sent A-GPS data to modem, type: %d", type);

	return 0;
}

static struct gps_drv_data gps_drv_data = {
};

static const struct gps_driver_api gps_api_funcs = {
	.init = init,
	.start = start,
	.stop = stop,
	.agps_write = agps_write,
};

DEVICE_DEFINE(nrf9160_gps, CONFIG_NRF9160_GPS_DEV_NAME,
	      setup, NULL,
	      &gps_drv_data, NULL, POST_KERNEL,
	      CONFIG_NRF9160_GPS_INIT_PRIO, &gps_api_funcs);
