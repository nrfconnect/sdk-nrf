/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <date_time.h>
#include <nrf_modem_gnss.h>

#if defined(CONFIG_SLM_NRF_CLOUD)
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include "slm_at_nrfcloud.h"
#endif
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_gnss.h"

LOG_MODULE_REGISTER(slm_gnss, CONFIG_SLM_LOG_LEVEL);

#define LOCATION_REPORT_MS 5000

#define SEC_PER_MIN	(60UL)
#define MIN_PER_HOUR	(60UL)
#define SEC_PER_HOUR	(MIN_PER_HOUR * SEC_PER_MIN)
#define HOURS_PER_DAY	(24UL)
#define SEC_PER_DAY	(HOURS_PER_DAY * SEC_PER_HOUR)
/* (6.1.1980 UTC - 1.1.1970 UTC) */
#define GPS_TO_UNIX_UTC_OFFSET_SECONDS	(315964800UL)
/* UTC/GPS time offset as of 1st of January 2017. */
#define GPS_TO_UTC_LEAP_SECONDS	(18UL)

/**@brief GNSS operations. */
enum slm_gnss_operation {
	GPS_STOP,
	GPS_START,
	AGPS_STOP            = GPS_STOP,
	AGPS_START           = GPS_START,
	PGPS_STOP            = GPS_STOP,
	PGPS_START           = GPS_START
};

#if defined(CONFIG_NRF_CLOUD_AGPS)
static struct k_work agps_req;
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
static struct k_work pgps_req;
#endif
static struct k_work fix_rep;

static uint64_t ttft_start;
static enum {
	RUN_TYPE_NONE,
	RUN_TYPE_GPS,
	RUN_TYPE_AGPS,
	RUN_TYPE_PGPS,
} run_type;
static enum {
	RUN_STATUS_STOPPED,
	RUN_STATUS_STARTED,
	RUN_STATUS_PERIODIC_WAKEUP,
	RUN_STATUS_SLEEP_AFTER_TIMEOUT,
	RUN_STATUS_SLEEP_AFTER_FIX,
	RUN_STATUS_MAX
} run_status;

static bool is_gnss_activated(void)
{
	int gnss_support = 0;
	int cfun_mode = 0;

	/*parse %XSYSTEMMODE=<LTE_M_support>,<NB_IoT_support>,<GNSS_support>,<LTE_preference> */
	if (nrf_modem_at_scanf("AT%XSYSTEMMODE?",
			       "%XSYSTEMMODE: %*d,%*d,%d", &gnss_support) == 1) {
		if (gnss_support == 0) {
			return false;
		}
	}

	/*parse +CFUN: <fun> */
	if (nrf_modem_at_scanf("AT+CFUN?", "+CFUN: %d", &cfun_mode) == 1) {
		if (cfun_mode == 1 || cfun_mode == 31) {
			return true;
		}
	}

	return false;
}

static void gnss_status_notify(int status)
{
	if (run_type == RUN_TYPE_AGPS) {
		rsp_send("\r\n#XAGPS: 1,%d\r\n", status);
	} else if (run_type == RUN_TYPE_PGPS) {
		rsp_send("\r\n#XPGPS: 1,%d\r\n", status);
	} else {
		rsp_send("\r\n#XGPS: 1,%d\r\n", status);
	}
	run_status = status;
}

static int gnss_startup(int type)
{
	int ret;

	/* Set run_type first as modem send NRF_MODEM_GNSS_EVT_AGPS_REQ instantly */
	run_type = type;

	/* Subscribe to NMEA messages */
	if (IS_ENABLED(CONFIG_SLM_LOG_LEVEL_DBG)) {
		(void)nrf_modem_gnss_qzss_nmea_mode_set(
			NRF_MODEM_GNSS_QZSS_NMEA_MODE_CUSTOM);
		ret = nrf_modem_gnss_nmea_mask_set(NRF_MODEM_GNSS_NMEA_GGA_MASK |
						   NRF_MODEM_GNSS_NMEA_GLL_MASK |
						   NRF_MODEM_GNSS_NMEA_GSA_MASK |
						   NRF_MODEM_GNSS_NMEA_GSV_MASK |
						   NRF_MODEM_GNSS_NMEA_RMC_MASK);
	} else {
		ret = nrf_modem_gnss_nmea_mask_set(NRF_MODEM_GNSS_NMEA_GGA_MASK);
	}
	if (ret < 0) {
		LOG_ERR("Failed to set nmea mask, error: %d", ret);
		return ret;
	}

	ret = nrf_modem_gnss_start();
	if (ret) {
		LOG_ERR("Failed to start GPS, error: %d", ret);
		run_type = RUN_TYPE_NONE;
	} else {
		ttft_start = k_uptime_get();
		gnss_status_notify(RUN_STATUS_STARTED);
		LOG_INF("GNSS start %d", type);
	}

	return ret;
}

static int gnss_shutdown(void)
{
	int ret = nrf_modem_gnss_stop();

	LOG_INF("GNSS stop %d", ret);
	gnss_status_notify(RUN_STATUS_STOPPED);
	run_type = RUN_TYPE_NONE;

	return ret;
}

#if defined(CONFIG_SLM_NRF_CLOUD)

#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
static int read_agps_req(struct nrf_modem_gnss_agps_data_frame *req)
{
	int err;

	err = nrf_modem_gnss_read((void *)req, sizeof(*req),
					NRF_MODEM_GNSS_DATA_AGPS_REQ);
	if (err) {
		LOG_ERR("Failed to read GNSS AGPS req, error %d", err);
		return -EAGAIN;
	}

	LOG_DBG("AGPS_REQ.sv_mask_ephe = 0x%08x", req->sv_mask_ephe);
	LOG_DBG("AGPS_REQ.sv_mask_alm  = 0x%08x", req->sv_mask_alm);
	LOG_DBG("AGPS_REQ.data_flags   = 0x%08x", req->data_flags);

	return 0;
}
#endif /* CONFIG_NRF_CLOUD_AGPS || CONFIG_NRF_CLOUD_PGPS */

#if defined(CONFIG_NRF_CLOUD_AGPS)
static void agps_req_wk(struct k_work *work)
{
	int err;
	struct nrf_modem_gnss_agps_data_frame req;

	ARG_UNUSED(work);

	err = read_agps_req(&req);
	if (err) {
		return;
	}

	err = nrf_cloud_agps_request(&req);
	if (err) {
		LOG_ERR("Failed to request A-GPS data: %d", err);
	}
	LOG_INF("A-GPS requested");
}
#endif /* CONFIG_NRF_CLOUD_AGPS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void pgps_req_wk(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	/* Indirect request of P-GPS data and periodic injection */
	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		LOG_ERR("Failed to request notify of prediction: %d", err);
	} else {
		LOG_INF("P-GPS requested");
	}
}

static void pgps_event_handler(struct nrf_cloud_pgps_event *event)
{
	int err;

	switch (event->type) {
	/* P-GPS initialization beginning. */
	case PGPS_EVT_INIT:
		LOG_INF("PGPS_EVT_INIT");
		break;
	/* There are currently no P-GPS predictions available. */
	case PGPS_EVT_UNAVAILABLE:
		LOG_INF("PGPS_EVT_UNAVAILABLE");
		break;
	/* P-GPS predictions are being loaded from the cloud. */
	case PGPS_EVT_LOADING:
		LOG_INF("PGPS_EVT_LOADING");
		break;
	/* A P-GPS prediction is available now for the current date and time. */
	case PGPS_EVT_AVAILABLE: {
		struct nrf_modem_gnss_agps_data_frame req;

		LOG_INF("PGPS_EVT_AVAILABLE");
		/* read out previous NRF_MODEM_GNSS_EVT_AGPS_REQ */
		err = read_agps_req(&req);
		if (err) {
			/* Ephemerides assistance only */
			LOG_INF("Failed to read A-GPS request: %d", err);
			LOG_INF("Use ephemerides assistance only");
			err = nrf_cloud_pgps_inject(event->prediction, NULL);
		} else {
			/* All assistance elements as requested */
			err = nrf_cloud_pgps_inject(event->prediction, &req);
		}
		if (err) {
			LOG_ERR("Unable to send prediction to modem: %d", err);
			break;
		}
		err = nrf_cloud_pgps_preemptive_updates();
		if (err) {
			LOG_ERR("Preemptive updates error: %d", err);
		}
	} break;
	/* All P-GPS predictions are available. */
	case PGPS_EVT_READY:
		LOG_INF("PGPS_EVT_READY");
		break;
	default:
		break;
	}
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

#endif /* CONFIG_SLM_NRF_CLOUD */

static void on_gnss_evt_nmea(void)
{
	struct nrf_modem_gnss_nmea_data_frame nmea;

	if (nrf_modem_gnss_read((void *)&nmea, sizeof(nmea), NRF_MODEM_GNSS_DATA_NMEA) == 0) {
		LOG_DBG("%s", nmea.nmea_str);
	}
}

static void on_gnss_evt_pvt(void)
{
	struct nrf_modem_gnss_pvt_data_frame pvt;
	int err;

	err = nrf_modem_gnss_read((void *)&pvt, sizeof(pvt), NRF_MODEM_GNSS_DATA_PVT);
	if (err) {
		LOG_ERR("Failed to read GNSS PVT data, error %d", err);
		return;
	}
	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
		if (pvt.sv[i].sv) { /* SV number 0 indicates no satellite */
			LOG_DBG("SV:%3d sig: %d c/n0:%4d el:%3d az:%3d in-fix: %d unhealthy: %d",
				pvt.sv[i].sv, pvt.sv[i].signal, pvt.sv[i].cn0,
				pvt.sv[i].elevation, pvt.sv[i].azimuth,
				(pvt.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) ? 1 : 0,
				(pvt.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) ? 1 : 0);
		}
	}
}

#if defined(CONFIG_SLM_NRF_CLOUD)
static int do_cloud_send_obj(struct nrf_cloud_obj *const obj)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.obj = obj,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
		.qos = MQTT_QOS_0_AT_MOST_ONCE
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send failed, error: %d", err);
	}

	(void)nrf_cloud_obj_free(obj);

	return err;
}

static void send_location(struct nrf_modem_gnss_pvt_data_frame * const pvt_data)
{
	static int64_t last_ts_ms = NRF_CLOUD_NO_TIMESTAMP;
	int err;
	struct nrf_cloud_gnss_data gnss = {
		.ts_ms = NRF_CLOUD_NO_TIMESTAMP,
		.type = NRF_CLOUD_GNSS_TYPE_MODEM_PVT,
		.mdm_pvt = pvt_data
	};
	NRF_CLOUD_OBJ_JSON_DEFINE(msg_obj);

	/* On failure, NRF_CLOUD_NO_TIMESTAMP is used and the timestamp is omitted */
	(void)date_time_now(&gnss.ts_ms);

	if ((last_ts_ms == NRF_CLOUD_NO_TIMESTAMP) ||
	    (gnss.ts_ms == NRF_CLOUD_NO_TIMESTAMP) ||
	    (gnss.ts_ms > (last_ts_ms + LOCATION_REPORT_MS))) {
		last_ts_ms = gnss.ts_ms;
	} else {
		return;
	}

	/* Encode the location data into a device message */
	err = nrf_cloud_obj_gnss_msg_create(&msg_obj, &gnss);
	if (!err) {
		err = do_cloud_send_obj(&msg_obj);
	}

	if (err) {
		LOG_WRN("Failed to send location, error %d", err);
	}
}
#endif /* CONFIG_SLM_NRF_CLOUD */

static void fix_rep_wk(struct k_work *work)
{
	int err;
	struct nrf_modem_gnss_pvt_data_frame pvt;

	ARG_UNUSED(work);

	err = nrf_modem_gnss_read((void *)&pvt, sizeof(pvt), NRF_MODEM_GNSS_DATA_PVT);
	if (err) {
		LOG_ERR("Failed to read GNSS PVT data, error %d", err);
		return;
	}

	/* GIS accuracy: http://wiki.gis.com/wiki/index.php/Decimal_degrees, use default .6lf */
	rsp_send("\r\n#XGPS: %lf,%lf,%f,%f,%f,%f,\"%04u-%02u-%02u %02u:%02u:%02u\"\r\n",
		pvt.latitude, pvt.longitude, pvt.altitude,
		pvt.accuracy, pvt.speed, pvt.heading,
		pvt.datetime.year, pvt.datetime.month, pvt.datetime.day,
		pvt.datetime.hour, pvt.datetime.minute, pvt.datetime.seconds);

	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
		if (pvt.sv[i].sv) { /* SV number 0 indicates no satellite */
			LOG_INF("SV:%3d sig: %d c/n0:%4d el:%3d az:%3d in-fix: %d unhealthy: %d",
				pvt.sv[i].sv, pvt.sv[i].signal, pvt.sv[i].cn0,
				pvt.sv[i].elevation, pvt.sv[i].azimuth,
				(pvt.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) ? 1 : 0,
				(pvt.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) ? 1 : 0);
		}
	}

#if defined(CONFIG_SLM_NRF_CLOUD)
	if (slm_nrf_cloud_send_location && slm_nrf_cloud_ready) {
		/* Report to nRF Cloud by best-effort */
		send_location(&pvt);
	}

#if defined(CONFIG_NRF_CLOUD_PGPS)
	if (run_type == RUN_TYPE_PGPS) {
		struct tm gps_time = {
			.tm_year = pvt.datetime.year - 1900,
			.tm_mon  = pvt.datetime.month - 1,
			.tm_mday = pvt.datetime.day,
			.tm_hour = pvt.datetime.hour,
			.tm_min  = pvt.datetime.minute,
			.tm_sec  = pvt.datetime.seconds,
		};

		/* help date_time to save SNTP transactions */
		date_time_set(&gps_time);
		/* help nrf_cloud_pgps as most recent known location */
		nrf_cloud_pgps_set_location(pvt.latitude, pvt.longitude);
	}
#endif /* CONFIG_NRF_CLOUD_PGPS */
#endif /* CONFIG_SLM_NRF_CLOUD */
	return;
}

static void on_gnss_evt_fix(void)
{
	if (ttft_start != 0) {
		uint64_t delta = k_uptime_delta(&ttft_start);

		LOG_INF("TTFF %d.%ds", (int)(delta/1000), (int)(delta%1000));
		ttft_start = 0;
	}

	k_work_submit_to_queue(&slm_work_q, &fix_rep);
}

static void on_gnss_evt_agps_req(void)
{
#if defined(CONFIG_NRF_CLOUD_AGPS)
	if (run_type == RUN_TYPE_AGPS) {
		k_work_submit_to_queue(&slm_work_q, &agps_req);
		return;
	}
#elif defined(CONFIG_NRF_CLOUD_PGPS)
	if (run_type == RUN_TYPE_PGPS) {
		/* Check whether prediction data available or not */
		k_work_submit_to_queue(&slm_work_q, &pgps_req);
	}
#endif
}

/* NOTE this event handler runs in interrupt context */
static void gnss_event_handler(int event)
{
	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		if (IS_ENABLED(CONFIG_SLM_LOG_LEVEL_DBG)) {
			on_gnss_evt_pvt();
		}
		break;
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_INF("GNSS_EVT_FIX");
		on_gnss_evt_fix();
		break;
	case NRF_MODEM_GNSS_EVT_NMEA:
		if (IS_ENABLED(CONFIG_SLM_LOG_LEVEL_DBG)) {
			on_gnss_evt_nmea();
		}
		break;
	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
		LOG_INF("GNSS_EVT_AGPS_REQ");
		on_gnss_evt_agps_req();
		break;
	case NRF_MODEM_GNSS_EVT_BLOCKED:
		LOG_INF("GNSS_EVT_BLOCKED");
		break;
	case NRF_MODEM_GNSS_EVT_UNBLOCKED:
		LOG_INF("GNSS_EVT_UNBLOCKED");
		break;
	case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
		LOG_DBG("GNSS_EVT_PERIODIC_WAKEUP");
		run_status = RUN_STATUS_PERIODIC_WAKEUP;
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_INF("GNSS_EVT_SLEEP_AFTER_TIMEOUT");
		gnss_status_notify(RUN_STATUS_SLEEP_AFTER_TIMEOUT);
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
		LOG_DBG("GNSS_EVT_SLEEP_AFTER_FIX");
		run_status = RUN_STATUS_SLEEP_AFTER_FIX;
		break;
	case NRF_MODEM_GNSS_EVT_REF_ALT_EXPIRED:
		LOG_INF("GNSS_EVT_REF_ALT_EXPIRED");
		break;
	default:
		break;
	}
}

/* Handles AT#XGPS commands. */
int handle_at_gps(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t interval;
	uint16_t timeout;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&slm_at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if (op == GPS_START && run_type == RUN_TYPE_NONE) {
			err = at_params_unsigned_short_get(&slm_at_param_list, 2, &interval);
			if (err < 0) {
				return err;
			}
			/* GNSS API spec check */
			if (interval != 0 && interval != 1 &&
			   (interval < 10 || interval > 65535)) {
				return -EINVAL;
			}
			err = nrf_modem_gnss_fix_interval_set(interval);
			if (err) {
				LOG_ERR("Failed to set fix interval, error: %d", err);
				return err;
			}
			if (at_params_unsigned_short_get(&slm_at_param_list, 3, &timeout) == 0) {
				err = nrf_modem_gnss_fix_retry_set(timeout);
				if (err) {
					LOG_ERR("Failed to set fix retry, error: %d", err);
					return err;
				}
			} /* else leave it to default or previously configured timeout */

			err = gnss_startup(RUN_TYPE_GPS);
		} else if (op == GPS_STOP && run_type == RUN_TYPE_GPS) {
			err = gnss_shutdown();
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XGPS: %d,%d\r\n", (int)is_gnss_activated(), run_status);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XGPS: (%d,%d),<interval>,<timeout>\r\n", GPS_STOP, GPS_START);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

#if defined(CONFIG_SLM_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_AGPS)

static int64_t utc_to_gps_sec(const int64_t utc_sec)
{
	return (utc_sec - GPS_TO_UNIX_UTC_OFFSET_SECONDS) + GPS_TO_UTC_LEAP_SECONDS;
}

static void gps_sec_to_day_time(int64_t gps_sec, uint16_t *gps_day, uint32_t *gps_time_of_day)
{
	*gps_day = (uint16_t)(gps_sec / SEC_PER_DAY);
	*gps_time_of_day = (uint32_t)(gps_sec % SEC_PER_DAY);
}

/* Handles AT#XAGPS commands. */
int handle_at_agps(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t interval;
	uint16_t timeout;
	int64_t utc_now_ms;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&slm_at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if (op == AGPS_START && slm_nrf_cloud_ready && run_type == RUN_TYPE_NONE) {
			err = at_params_unsigned_short_get(&slm_at_param_list, 2, &interval);
			if (err < 0) {
				return err;
			}
			/* GNSS API spec check */
			if (interval != 0 && interval != 1 &&
			   (interval < 10 || interval > 65535)) {
				return -EINVAL;
			}
#if defined(CONFIG_NRF_CLOUD_AGPS_FILTERED)
			err = nrf_modem_gnss_elevation_threshold_set(
						CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK);
			if (err) {
				LOG_ERR("Failed to set elevation threshold, error: %d", err);
				return err;
			}
#endif
			/* no scheduled downloads for peoridical tracking */
			if (interval >= 10) {
				err = nrf_modem_gnss_use_case_set(
						NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START |
						NRF_MODEM_GNSS_USE_CASE_SCHED_DOWNLOAD_DISABLE);
				if (err) {
					LOG_ERR("Failed to set use case, error: %d", err);
					return err;
				}
			}
			err = nrf_modem_gnss_fix_interval_set(interval);
			if (err) {
				LOG_ERR("Failed to set fix interval, error: %d", err);
				return err;
			}
			if (at_params_unsigned_short_get(&slm_at_param_list, 3, &timeout) == 0) {
				err = nrf_modem_gnss_fix_retry_set(timeout);
				if (err) {
					LOG_ERR("Failed to set fix retry, error: %d", err);
					return err;
				}
			} /* else leave it to default or previously configured timeout */

			/* Inject current GPS time. If there is cached A-GPS data before expiry,
			 * modem will not request new almanac and/or ephemerides from cloud.
			 */
			err = date_time_now(&utc_now_ms);
			if (err == 0) {
				int64_t gps_sec = utc_to_gps_sec(utc_now_ms/1000);
				struct nrf_modem_gnss_agps_data_system_time_and_sv_tow gps_time
					= { 0 };

				gps_sec_to_day_time(gps_sec, &gps_time.date_day,
						    &gps_time.time_full_s);
				err = nrf_modem_gnss_agps_write(&gps_time, sizeof(gps_time),
						NRF_MODEM_GNSS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS);
				if (err) {
					LOG_WRN("Fail to inject time, error %d", err);
				} else {
					LOG_DBG("Inject time (GPS day %u, GPS time of day %u)",
						gps_time.date_day, gps_time.time_full_s);
				}
			}

			err = gnss_startup(RUN_TYPE_AGPS);
		} else if (op == AGPS_STOP && run_type == RUN_TYPE_AGPS) {
			err = gnss_shutdown();
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XAGPS: %d,%d\r\n", (int)is_gnss_activated(), run_status);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XAGPS: (%d,%d),<interval>,<timeout>\r\n", AGPS_STOP, AGPS_START);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}
#endif /* CONFIG_NRF_CLOUD_AGPS */

#if defined(CONFIG_SLM_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_PGPS)
/* Handles AT#XPGPS commands. */
int handle_at_pgps(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t interval;
	uint16_t timeout;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if (op == PGPS_START && slm_nrf_cloud_ready && run_type == RUN_TYPE_NONE) {
			err = at_params_unsigned_short_get(&at_param_list, 2, &interval);
			if (err < 0) {
				return err;
			}
			/* GNSS API spec check, P-GPS is used in periodic mode only */
			if (interval < 10 || interval > 65535) {
				return -EINVAL;
			}
			/* no scheduled downloads for peoridical tracking */
			err = nrf_modem_gnss_use_case_set(
					NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START |
					NRF_MODEM_GNSS_USE_CASE_SCHED_DOWNLOAD_DISABLE);
			if (err) {
				LOG_ERR("Failed to set use case, error: %d", err);
				return err;
			}
			err = nrf_modem_gnss_fix_interval_set(interval);
			if (err) {
				LOG_ERR("Failed to set fix interval, error: %d", err);
				return err;
			}
			if (at_params_unsigned_short_get(&at_param_list, 3, &timeout) == 0) {
				err = nrf_modem_gnss_fix_retry_set(timeout);
				if (err) {
					LOG_ERR("Failed to set fix retry, error: %d", err);
					return err;
				}
			} /* else leave it to default or previously configured timeout */

			struct nrf_cloud_pgps_init_param param = {
				.event_handler = pgps_event_handler,
				/* storage is defined by CONFIG_NRF_CLOUD_PGPS_STORAGE */
				.storage_base = 0u,
				.storage_size = 0u
			};

			err = nrf_cloud_pgps_init(&param);
			if (err) {
				LOG_ERR("Error from P-GPS init: %d", err);
				return err;
			}

			err = gnss_startup(RUN_TYPE_PGPS);
		} else if (op == PGPS_STOP && run_type == RUN_TYPE_PGPS) {
			err = gnss_shutdown();
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XPGPS: %d,%d\r\n", (int)is_gnss_activated(), run_status);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XPGPS: (%d,%d),<interval>,<timeout>\r\n", PGPS_STOP, PGPS_START);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

/* Handles AT#XGPSDEL commands. */
int handle_at_gps_delete(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint32_t mask;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_int_get(&slm_at_param_list, 1, &mask);
		if (err < 0) {
			return err;
		}
		err = nrf_modem_gnss_nv_data_delete(mask);
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XGPSDEL: <mask>\r\n");
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to initialize GNSS AT commands handler
 */
int slm_at_gnss_init(void)
{
	int err = 0;

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Could set GNSS event handler, error: %d", err);
		return err;
	}
#if defined(CONFIG_SLM_NRF_CLOUD)
#if defined(CONFIG_NRF_CLOUD_AGPS)
	k_work_init(&agps_req, agps_req_wk);
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
	k_work_init(&pgps_req, pgps_req_wk);
#endif
#endif /* CONFIG_SLM_NRF_CLOUD */
	k_work_init(&fix_rep, fix_rep_wk);

	return err;
}

/**@brief API to uninitialize GNSS AT commands handler
 */
int slm_at_gnss_uninit(void)
{
	return 0;
}
