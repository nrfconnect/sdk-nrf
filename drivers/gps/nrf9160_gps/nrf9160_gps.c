/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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
#include <nrf_socket.h>
#include <net/socket.h>
#ifdef CONFIG_NRF9160_GPS_HANDLE_MODEM_CONFIGURATION
#include <at_cmd.h>
#include <at_cmd_parser/at_cmd_parser.h>
#endif

LOG_MODULE_REGISTER(nrf9160_gps, CONFIG_NRF9160_GPS_LOG_LEVEL);

#ifdef CONFIG_NRF9160_GPS_HANDLE_MODEM_CONFIGURATION
#define AT_CMD_LEN(x)			(sizeof(x) - 1)
#define AT_XSYSTEMMODE_REQUEST		"AT%XSYSTEMMODE?"
#define AT_XSYSTEMMODE_RESPONSE		"%XSYSTEMMODE:"
#define AT_XSYSTEMMODE_PROTO		"AT%%XSYSTEMMODE=%d,%d,%d,%d"
#define AT_XSYSTEMMODE_PARAMS_COUNT	5
#define AT_XSYSTEMMODE_GPS_PARAM_INDEX	3
#define AT_CFUN_REQUEST			"AT+CFUN?"
#define AT_CFUN_RESPONSE		"+CFUN:"
#define AT_CFUN_0			"AT+CFUN=0"
#define AT_CFUN_1			"AT+CFUN=1"
#define FUNCTIONAL_MODE_ENABLED		1
#endif

/* Aligned strings describing sattelite states based on flags */
#define sv_used_str(x) ((x)?"    used":"not used")
#define sv_unhealthy_str(x) ((x)?"not healthy":"    healthy")

struct gps_drv_data {
	gps_trigger_handler_t trigger_handler;
	struct gps_trigger trigger;
	struct k_mutex trigger_mutex;

	atomic_t gps_is_active;

	int socket;

	K_THREAD_STACK_MEMBER(thread_stack,
			      CONFIG_NRF9160_GPS_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem thread_run_sem;
};

static struct gps_data fresh_nmea;
static struct gps_data fresh_pvt;

static void copy_pvt(struct gps_pvt *dest, nrf_gnss_pvt_data_frame_t *src)
{

	dest->latitude = src->latitude;
	dest->longitude = src->longitude;
	dest->altitude = src->altitude;
	dest->accuracy = src->accuracy;
	dest->speed = src->speed;
	dest->heading = src->heading;
	dest->flags = src->flags;
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
	     i < MIN(NRF_GNSS_MAX_SATELLITES, GPS_MAX_SATELLITES); i++) {
		dest->sv[i].sv = src->sv[i].sv;
		dest->sv[i].cn0 = src->sv[i].cn0;
		dest->sv[i].elevation = src->sv[i].elevation;
		dest->sv[i].azimuth = src->sv[i].azimuth;
		dest->sv[i].flags = src->sv[i].flags;
		dest->sv[i].signal = src->sv[i].signal;
	}
}

static bool is_fix(struct gps_pvt *pvt)
{
	return ((pvt->flags & NRF_GNSS_PVT_FLAG_FIX_VALID_BIT)
		== NRF_GNSS_PVT_FLAG_FIX_VALID_BIT);
}

/**@brief Checks if GPS operation is blocked due to insufficient time windows */
static bool gps_is_blocked(nrf_gnss_pvt_data_frame_t *pvt)
{
/* TODO: Use bitmask from nrf_socket.h when it's available. */
#define PVT_FLAG_OP_BLOCKED_TIME_WINDOW BIT(4)

	return ((pvt->flags & PVT_FLAG_OP_BLOCKED_TIME_WINDOW)
		== PVT_FLAG_OP_BLOCKED_TIME_WINDOW);
}

/**@brief Checks if PVT frame is invalid due to missed processing deadline */
static bool pvt_deadline_missed(nrf_gnss_pvt_data_frame_t *pvt)
{
/* TODO: Use bitmask from nrf_socket.h when it's available. */
#define PVT_FLAG_DEADLINE_MISSED BIT(3)

	return ((pvt->flags & PVT_FLAG_DEADLINE_MISSED)
		== PVT_FLAG_DEADLINE_MISSED);
}

static u64_t fix_timestamp;

static void print_satellite_stats(nrf_gnss_data_frame_t *pvt_data)
{
	u8_t  n_tracked = 0;
	u8_t  n_used = 0;
	u8_t  n_unhealthy = 0;

	for (int i = 0; i < NRF_GNSS_MAX_SATELLITES; ++i) {
		u8_t sv = pvt_data->pvt.sv[i].sv;
		bool used = (pvt_data->pvt.sv[i].flags &
			     NRF_GNSS_SV_FLAG_USED_IN_FIX) ? true : false;
		bool unhealthy = (pvt_data->pvt.sv[i].flags &
				  NRF_GNSS_SV_FLAG_UNHEALTHY) ? true : false;

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

	LOG_DBG("Seconds since last fix %lld",
			(k_uptime_get() - fix_timestamp) / 1000);
}

static void gps_thread(int dev_ptr)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct gps_drv_data *drv_data = dev->driver_data;
	int len;
	nrf_gnss_data_frame_t raw_gps_data;
	bool trigger_send = false;
	bool operation_blocked = false;
wait:
	k_sem_take(&drv_data->thread_run_sem, K_FOREVER);

	while (true) {
		len = recv(drv_data->socket, &raw_gps_data,
			       sizeof(nrf_gnss_data_frame_t), 0);
		if (len <= 0) {
			/* Is the GPS stopped, causing this error? */
			if (!atomic_get(&drv_data->gps_is_active)) {
				goto wait;
			}
			LOG_ERR("recv() returned error: %d", len);
			continue;
		}

		switch (raw_gps_data.data_id) {
		case NRF_GNSS_PVT_DATA_ID:
			if (gps_is_blocked(&raw_gps_data.pvt)) {
				if (operation_blocked) {
					/* Avoid spamming the logs. */
					continue;
				}

				/* If LTE is used alongside GPS, PSM, eDRX or
				 * DRX needs to be enabled for the GPS to
				 * operate. If PSM is used, the GPS will
				 * normally operate when active time expires.
				 */
				LOG_DBG("Waiting for time window to operate");

				operation_blocked = true;
				continue;
			}

			operation_blocked = false;

			if (pvt_deadline_missed(&raw_gps_data.pvt)) {
				LOG_DBG("Invalid PVT frame, discarding");
				continue;
			}

			print_satellite_stats(&raw_gps_data);
			copy_pvt(&fresh_pvt.pvt, &raw_gps_data.pvt);

			if ((drv_data->trigger.chan == GPS_CHAN_PVT)
			   && (drv_data->trigger.type == GPS_TRIG_DATA_READY)) {
				trigger_send = true;
				LOG_DBG("PVT data ready");
			}

			if ((drv_data->trigger.type == GPS_TRIG_FIX) &&
			    is_fix(&fresh_pvt.pvt)) {
				if (drv_data->trigger.chan == GPS_CHAN_PVT) {
					trigger_send = true;
				}
				LOG_DBG("PVT: Position fix");
				fix_timestamp = k_uptime_get();
			}

			break;

		case NRF_GNSS_NMEA_DATA_ID:
			if (operation_blocked) {
				continue;
			}

			memcpy(fresh_nmea.nmea.buf, raw_gps_data.nmea, len);
			fresh_nmea.nmea.len = strlen(raw_gps_data.nmea);

			if ((drv_data->trigger.chan == GPS_CHAN_NMEA) &&
			    (drv_data->trigger.type == GPS_TRIG_DATA_READY)) {
				trigger_send = true;
				LOG_DBG("NMEA data ready");
			}

			if ((drv_data->trigger.type == GPS_TRIG_FIX)
			    && is_fix(&fresh_pvt.pvt)) {
				if (drv_data->trigger.chan == GPS_CHAN_NMEA) {
					trigger_send = true;
				}
				LOG_DBG("NMEA: Position fix");
			}
			break;

		default:
			continue;
		}

		if (!trigger_send) {
			continue;
		}

		trigger_send = false;

		k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);

		if (drv_data->trigger_handler != NULL) {
			drv_data->trigger_handler(dev, &drv_data->trigger);
		}

		k_mutex_unlock(&drv_data->trigger_mutex);
	}
}

static int init_thread(struct device *dev)
{
	struct gps_drv_data *drv_data = dev->driver_data;

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			K_THREAD_STACK_SIZEOF(drv_data->thread_stack),
			(k_thread_entry_t)gps_thread, dev, NULL, NULL,
			K_PRIO_PREEMPT(CONFIG_NRF9160_GPS_THREAD_PRIORITY),
			0, 0);

	return 0;
}

#ifdef CONFIG_NRF9160_GPS_HANDLE_MODEM_CONFIGURATION
static int enable_gps(struct device *dev)
{
	int err;
	char buf[50] = {0};
	struct at_param_list at_resp_list = {0};
	u16_t gps_param_value, functional_mode;

	err = at_params_list_init(&at_resp_list, AT_XSYSTEMMODE_PARAMS_COUNT);
	if (err) {
		LOG_ERR("Could init AT params list, error: %d", err);
		return err; /* No need to clean up; the list was never init'd */
	}

	err = at_cmd_write(AT_XSYSTEMMODE_REQUEST, buf, sizeof(buf), NULL);
	if (err) {
		LOG_ERR("Could not get modem's system mode");
		err = -EIO;
		goto enable_gps_clean_exit;
	}

	err = at_parser_max_params_from_str(buf,
					    NULL,
					    &at_resp_list,
					    AT_XSYSTEMMODE_PARAMS_COUNT);
	if (err) {
		LOG_ERR("Could not parse AT response, error: %d", err);
		goto enable_gps_clean_exit;
	}

	err = at_params_short_get(&at_resp_list,
				  AT_XSYSTEMMODE_GPS_PARAM_INDEX,
				  &gps_param_value);
	if (err) {
		LOG_ERR("Could not get GPS mode state, error: %d", err);
		goto enable_gps_clean_exit;
	}

	if (gps_param_value != 1) {
		char cmd[sizeof(AT_XSYSTEMMODE_PROTO)];
		size_t len;
		u16_t values[AT_XSYSTEMMODE_PARAMS_COUNT] = {0};

		LOG_DBG("GPS mode is not enabled, attempting to enable it");

		for (size_t i = 0; i < AT_XSYSTEMMODE_PARAMS_COUNT; i++) {
			at_params_short_get(&at_resp_list, i, &values[i]);
		}

		values[AT_XSYSTEMMODE_GPS_PARAM_INDEX] = 1;

		len = snprintf(cmd, sizeof(cmd), AT_XSYSTEMMODE_PROTO,
			       values[0], values[1], values[2], values[3]);

		LOG_DBG("Sending AT command: %s", log_strdup(cmd));

		err = at_cmd_write(cmd, NULL, 0, NULL);
		if (err) {
			LOG_ERR("Could not enable GPS mode, error: %d", err);
			goto enable_gps_clean_exit;
		}
	}

	LOG_DBG("GPS mode is enabled");

	err = at_cmd_write(AT_CFUN_REQUEST, buf, sizeof(buf), NULL);
	if (err) {
		LOG_ERR("Could not get functional mode, error: %d", err);
		goto enable_gps_clean_exit;
	}

	err = at_parser_max_params_from_str(buf,
					    NULL,
					    &at_resp_list, 2);
	if (err) {
		LOG_ERR("Could not parse functional mode response, error: %d",
			err);
		goto enable_gps_clean_exit;
	}

	err = at_params_short_get(&at_resp_list, 1, &functional_mode);
	if (err) {
		LOG_ERR("Could not get value of functional mode, error: %d",
			err);
		goto enable_gps_clean_exit;
	}

	LOG_DBG("Functional mode: %d", functional_mode);

	if (functional_mode != FUNCTIONAL_MODE_ENABLED) {
		LOG_DBG("Functional mode was %d, attemping to set to %d",
			functional_mode, FUNCTIONAL_MODE_ENABLED);

		err = at_cmd_write(AT_CFUN_1, NULL, 0, NULL);
		if (err) {
			LOG_ERR("Could not set functional mode to %d",
				FUNCTIONAL_MODE_ENABLED);
			goto enable_gps_clean_exit;
		}
		LOG_DBG("Functional mode set to %d", FUNCTIONAL_MODE_ENABLED);
	}

enable_gps_clean_exit:
	at_params_list_free(&at_resp_list);
	return err;
}
#endif

static int start(struct device *dev)
{
	int retval;
	struct gps_drv_data *drv_data = dev->driver_data;
	nrf_gnss_fix_retry_t    fix_retry    = 0;
	nrf_gnss_fix_interval_t fix_interval = 1;
	nrf_gnss_nmea_mask_t    nmea_mask    = 0;
	nrf_gnss_delete_mask_t  delete_mask  = 0;

#ifdef CONFIG_NRF9160_GPS_NMEA_GSV
	nmea_mask |= NRF_GNSS_NMEA_GSV_MASK;
#endif
#ifdef CONFIG_NRF9160_GPS_NMEA_GSA
	nmea_mask |= NRF_GNSS_NMEA_GSA_MASK;
#endif
#ifdef CONFIG_NRF9160_GPS_NMEA_GLL
	nmea_mask |= NRF_GNSS_NMEA_GLL_MASK;
#endif
#ifdef CONFIG_NRF9160_GPS_NMEA_GGA
	nmea_mask |= NRF_GNSS_NMEA_GGA_MASK;
#endif
#ifdef CONFIG_NRF9160_GPS_NMEA_RMC
	nmea_mask |= NRF_GNSS_NMEA_RMC_MASK;
#endif

#ifdef CONFIG_NRF9160_GPS_HANDLE_MODEM_CONFIGURATION
	if (enable_gps(dev) != 0) {
		LOG_ERR("Failed to enable GPS");
		return -EIO;
	}
#endif

	if (drv_data->socket < 0) {
		drv_data->socket = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_DGRAM,
					      NRF_PROTO_GNSS);

		if (drv_data->socket >= 0) {
			LOG_DBG("GPS socket created");
		} else {
			LOG_ERR("Could not init socket (err: %d)",
				drv_data->socket);
			return -EIO;
		}
	}

	retval = nrf_setsockopt(drv_data->socket,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_FIX_RETRY,
				&fix_retry,
				sizeof(fix_retry));
	if (retval != 0) {
		LOG_ERR("Failed to set fix retry value");
		return -EIO;
	}

	retval = nrf_setsockopt(drv_data->socket,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_FIX_INTERVAL,
				&fix_interval,
				sizeof(fix_interval));

	if (retval != 0) {
		LOG_ERR("Failed to set fix interval value");
		return -EIO;
	}

	retval = nrf_setsockopt(drv_data->socket,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_NMEA_MASK,
				&nmea_mask,
				sizeof(nmea_mask));

	if (retval != 0) {
		LOG_ERR("Failed to set nmea mask");
		return -EIO;
	}

	retval = nrf_setsockopt(drv_data->socket,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_START,
				&delete_mask,
				sizeof(delete_mask));

	if (retval != 0) {
		LOG_ERR("Failed to start GPS");
		return -EIO;
	}

	atomic_set(&drv_data->gps_is_active, 1);
	k_sem_give(&drv_data->thread_run_sem);

	LOG_DBG("GPS operational");

	return retval;
}

static int init(struct device *dev)
{
	struct gps_drv_data *drv_data = dev->driver_data;

	drv_data->socket = -1;
	atomic_set(&drv_data->gps_is_active, 0);

	k_sem_init(&drv_data->thread_run_sem, 0, 1);
	k_mutex_init(&drv_data->trigger_mutex);

	init_thread(dev);

	#if CONFIG_NRF9160_GPS_SET_MAGPIO || CONFIG_NRF9160_GPS_SET_COEX0
		int err;
	#endif

	#if CONFIG_NRF9160_GPS_SET_MAGPIO
		err = at_cmd_write(CONFIG_NRF9160_GPS_MAGPIO_STRING,
				   NULL, 0, NULL);
		if (err) {
			LOG_ERR("Could not confiugure MAGPIO, error: %d", err);
			return err;
		}

		LOG_DBG("MAGPIO set: %s",
			log_strdup(CONFIG_NRF9160_GPS_MAGPIO_STRING));
	#endif /* CONFIG_NRF9160_GPS_SET_MAGPIO */

	#if CONFIG_NRF9160_GPS_SET_COEX0
		err = at_cmd_write(CONFIG_NRF9160_GPS_COEX0_STRING,
				   NULL, 0, NULL);
		if (err) {
			LOG_ERR("Could not confiugure COEX0, error: %d", err);
			return err;
		}

		LOG_DBG("COEX0 set: %s",
			log_strdup(CONFIG_NRF9160_GPS_COEX0_STRING));
	#endif /* CONFIG_NRF9160_GPS_SET_COEX0 */

	return 0;
}

static int sample_fetch(struct device *dev)
{
	return 0;
}

static int channel_get(struct device *dev, enum gps_channel chan,
		       struct gps_data *sample)
{
	switch (chan) {
	case GPS_CHAN_NMEA:
		memcpy(sample->nmea.buf, fresh_nmea.nmea.buf,
			fresh_nmea.nmea.len);
		sample->nmea.len = fresh_nmea.nmea.len;
		break;
	case GPS_CHAN_PVT:
		memcpy(sample, &fresh_pvt, sizeof(struct gps_data));
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int stop(struct device *dev)
{
	struct gps_drv_data *drv_data = dev->driver_data;
	int retval;

	LOG_DBG("Stopping GPS");

	atomic_set(&drv_data->gps_is_active, 0);

	retval = nrf_setsockopt(drv_data->socket,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_STOP,
				NULL,
				0);

	if (retval != 0) {
		LOG_ERR("Failed to stop GPS");
		return -EIO;
	}

	return 0;
}

static int trigger_set(struct device *dev,
			       const struct gps_trigger *trig,
			       gps_trigger_handler_t handler)
{
	int ret = 0;
	struct gps_drv_data *drv_data = dev->driver_data;
	(void)drv_data;

	switch (trig->type) {
	case GPS_TRIG_DATA_READY:
	case GPS_TRIG_FIX:
		k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
		drv_data->trigger_handler = handler;
		drv_data->trigger = *trig;
		k_mutex_unlock(&drv_data->trigger_mutex);
		break;
	default:
		LOG_ERR("Unsupported GPS trigger");
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static struct gps_drv_data gps_drv_data;

static const struct gps_driver_api gps_api_funcs = {
	.sample_fetch = sample_fetch,
	.channel_get = channel_get,
	.trigger_set = trigger_set,
	.start = start,
	.stop = stop
};

DEVICE_AND_API_INIT(nrf9160_gps, CONFIG_NRF9160_GPS_DEV_NAME, init,
		    &gps_drv_data, NULL, APPLICATION,
		    CONFIG_NRF9160_GPS_INIT_PRIO, &gps_api_funcs);
