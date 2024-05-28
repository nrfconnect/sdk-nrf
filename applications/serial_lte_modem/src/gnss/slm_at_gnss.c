/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <assert.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <date_time.h>
#include <nrf_modem_gnss.h>

#if defined(CONFIG_SLM_NRF_CLOUD)
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include "slm_at_nrfcloud.h"

#if defined(CONFIG_NRF_CLOUD_AGNSS)
#include <net/nrf_cloud_agnss.h>
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
static void pgps_event_handler(struct nrf_cloud_pgps_event *event);
#endif

#endif /* CONFIG_SLM_NRF_CLOUD */

#if defined(CONFIG_SLM_CARRIER)
#include <time.h>
#include <lwm2m_carrier.h>
#endif

#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_gnss.h"

LOG_MODULE_REGISTER(slm_gnss, CONFIG_SLM_LOG_LEVEL);

#if defined(CONFIG_SLM_GNSS_OUTPUT_NMEA_ON_CMUX_CHANNEL)
#include "slm_cmux.h"
#include <zephyr/modem/cmux.h>
#include <zephyr/sys/ring_buffer.h>
static struct ring_buf gnss_nmea_rb;
static struct k_work gnss_nmea_send_work;
static struct modem_pipe *gnss_pipe;
#endif

/* Subscribe to NMEA messages also when debugging to troubleshoot fix acquisition. */
#define RECEIVE_NMEA (CONFIG_SLM_LOG_LEVEL_DBG || CONFIG_SLM_GNSS_OUTPUT_NMEA_ON_CMUX_CHANNEL)

#define LOCATION_REPORT_MS 5000

#define SEC_PER_HOUR	(MIN_PER_HOUR * SEC_PER_MIN)
#define SEC_PER_DAY	(HOUR_PER_DAY * SEC_PER_HOUR)
/* (6.1.1980 UTC - 1.1.1970 UTC) */
#define GPS_TO_UNIX_UTC_OFFSET_SECONDS	(315964800UL)
/* UTC/GPS time offset as of 1st of January 2017. */
#define GPS_TO_UTC_LEAP_SECONDS	(18UL)

/** #XGPS operations. */
enum slm_gnss_operation {
	GPS_STOP,
	GPS_START,
};

#if defined(CONFIG_SLM_NRF_CLOUD)

#if defined(CONFIG_NRF_CLOUD_AGNSS)
static struct k_work agnss_req_work;
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
static struct k_work pgps_req_work;
#endif

#endif /* CONFIG_SLM_NRF_CLOUD */

static struct k_work gnss_fix_send_work;

static int64_t gnss_ttff_start;

static bool gnss_running;
static enum gnss_status {
	GNSS_STATUS_STOPPED,
	GNSS_STATUS_STARTED,
	GNSS_STATUS_PERIODIC_WAKEUP,
	GNSS_STATUS_SLEEP_AFTER_TIMEOUT,
	GNSS_STATUS_SLEEP_AFTER_FIX,
} gnss_status;

static struct k_work gnss_status_notify_work;
/* FIFO to pass the GNSS statuses to the notifier worker. */
K_FIFO_DEFINE(gnss_status_fifo);

/* Whether to use the nRF Cloud assistive services that were compiled in (A-GNSS/P-GPS).
 * If enabled, the connection to nRF Cloud is required during GNSS startup
 * and assistance data will be downloaded.
 * If disabled, no request will be made to nRF Cloud but leftover assistance
 * data will still be injected if available locally.
 */
static uint16_t gnss_cloud_assistance;

/* If true, indicates that the NRF_MODEM_GNSS_EVT_AGNSS_REQ event
 * was received and ignored due to cloud assistance being disabled.
 * It has been observed that the event is not sent again after a restart of GNSS, so
 * this is to remember that A/P-GPS data was requested if cloud assistance gets enabled.
 */
static bool gnss_gps_req_to_send;

static bool is_gnss_activated(void)
{
	int activated = 0;
	int cfun_mode = 0;

	/*parse %XSYSTEMMODE=<LTE_M_support>,<NB_IoT_support>,<GNSS_support>,<LTE_preference> */
	if (slm_util_at_scanf("AT%XSYSTEMMODE?",
			       "%%XSYSTEMMODE: %*d,%*d,%d", &activated) == 1) {
		if (activated == 0) {
			return false;
		}
	}

	/*parse +CFUN: <fun> */
	if (slm_util_at_scanf("AT+CFUN?", "+CFUN: %d", &cfun_mode) == 1) {
		if (cfun_mode == 1 || cfun_mode == 31) {
			return true;
		}
	}

	return false;
}

static void gnss_status_notifier(struct k_work *)
{
	while (!k_fifo_is_empty(&gnss_status_fifo)) {
		gnss_status = (enum gnss_status)k_fifo_get(&gnss_status_fifo, K_NO_WAIT);
		rsp_send("\r\n#XGPS: 1,%d\r\n", gnss_status);
	}
}

static void gnss_status_set(enum gnss_status status)
{
	/* A notification is sent when the GNSS status is updated.
	 * However this gets called in interrupt context (from gnss_event_handler()),
	 * from which UART messages cannot be sent. This task is thus delegated
	 * to a worker, to which the new GNSS status is passed.
	 * A FIFO rather than a single variable is used to not miss GNSS status changes
	 * in case they happen twice (or more) before the worker had the time to process them.
	 */
	const int put_ret = k_fifo_alloc_put(&gnss_status_fifo, (void *)status);

	if (put_ret == 0) {
		k_work_submit_to_queue(&slm_work_q, &gnss_status_notify_work);
	} else {
		LOG_ERR("Failed to set GNSS status to %d (%d).", status, put_ret);
	}
}

#if defined(CONFIG_SLM_NRF_CLOUD) && defined(CONFIG_NRF_CLOUD_AGNSS)

static int64_t utc_to_gps_sec(const int64_t utc_sec)
{
	return (utc_sec - GPS_TO_UNIX_UTC_OFFSET_SECONDS) + GPS_TO_UTC_LEAP_SECONDS;
}

static void gps_sec_to_day_time(int64_t gps_sec, uint16_t *gps_day, uint32_t *gps_time_of_day)
{
	*gps_day = (uint16_t)(gps_sec / SEC_PER_DAY);
	*gps_time_of_day = (uint32_t)(gps_sec % SEC_PER_DAY);
}
#endif

static void on_gnss_evt_agnss_req(void)
{
	if (!gnss_cloud_assistance) {
		gnss_gps_req_to_send = true;
		return;
	}
	gnss_gps_req_to_send = false;

#if defined(CONFIG_SLM_NRF_CLOUD)

#if defined(CONFIG_NRF_CLOUD_AGNSS)
	k_work_submit_to_queue(&slm_work_q, &agnss_req_work);
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
	k_work_submit_to_queue(&slm_work_q, &pgps_req_work);
#endif

#endif /* CONFIG_SLM_NRF_CLOUD */
}

static int gnss_startup(void)
{
	int ret;

#if defined(CONFIG_SLM_NRF_CLOUD)

#if defined(CONFIG_NRF_CLOUD_AGNSS)
	if (gnss_cloud_assistance) {
		int64_t utc_now_ms;

#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED)
		ret = nrf_modem_gnss_elevation_threshold_set(
					CONFIG_NRF_CLOUD_AGNSS_ELEVATION_MASK);
		if (ret) {
			LOG_ERR("Failed to set elevation threshold (%d).", ret);
			return ret;
		}
#endif
		if (!date_time_now(&utc_now_ms)) {
			/* Inject current time into GNSS. If there is valid cached A-GNSS data,
			 * the modem will not request new almanacs/ephemerides from the cloud.
			 */
			const int64_t gps_sec = utc_to_gps_sec(utc_now_ms / 1000);
			struct nrf_modem_gnss_agnss_gps_data_system_time_and_sv_tow gps_time = {0};

			gps_sec_to_day_time(gps_sec, &gps_time.date_day, &gps_time.time_full_s);
			ret = nrf_modem_gnss_agnss_write(&gps_time, sizeof(gps_time),
					NRF_MODEM_GNSS_AGNSS_GPS_SYSTEM_CLOCK_AND_TOWS);
			if (ret) {
				LOG_WRN("Failed to inject GNSS time (%d).", ret);
			} else {
				LOG_INF("Injected GNSS time (day %u, time %u).",
					gps_time.date_day, gps_time.time_full_s);
			}
		}
	}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
	if (gnss_cloud_assistance) {
		struct nrf_cloud_pgps_init_param param = {
			.event_handler = pgps_event_handler,
			/* Storage is defined by CONFIG_NRF_CLOUD_PGPS_STORAGE. */
			.storage_base = 0u,
			.storage_size = 0u
		};

		/* This will automatically download P-GPS data
		 * from the cloud in case any is missing.
		 */
		ret = nrf_cloud_pgps_init(&param);
		if (ret) {
			LOG_ERR("P-GPS initialization failed (%d).", ret);
			return ret;
		}
	}
#endif

#endif /* CONFIG_SLM_NRF_CLOUD */

#if RECEIVE_NMEA
	uint16_t nmea_mask = NRF_MODEM_GNSS_NMEA_GGA_MASK
			     | NRF_MODEM_GNSS_NMEA_GLL_MASK
			     | NRF_MODEM_GNSS_NMEA_RMC_MASK;

#if defined(CONFIG_SLM_GNSS_OUTPUT_NMEA_SATELLITES)
	ret = nrf_modem_gnss_qzss_nmea_mode_set(NRF_MODEM_GNSS_QZSS_NMEA_MODE_CUSTOM);
	if (ret) {
		LOG_ERR("Failed to set QZSS NMEA mode. (%d)", ret);
	}
	nmea_mask |= NRF_MODEM_GNSS_NMEA_GSV_MASK | NRF_MODEM_GNSS_NMEA_GSA_MASK;
#endif

	ret = nrf_modem_gnss_nmea_mask_set(nmea_mask);
	if (ret < 0) {
		LOG_ERR("Failed to set NMEA mask. (%d)", ret);
		return ret;
	}

#endif /* RECEIVE_NMEA */

	ret = nrf_modem_gnss_start();
	if (ret) {
		LOG_ERR("Failed to start GNSS. (%d)", ret);
		return ret;
	}
#if CONFIG_SLM_GNSS_OUTPUT_NMEA_ON_CMUX_CHANNEL
	gnss_pipe = slm_cmux_reserve(CMUX_GNSS_CHANNEL);
#endif
	gnss_running = true;
	gnss_ttff_start = k_uptime_get();
	gnss_status_set(GNSS_STATUS_STARTED);
	LOG_INF("GNSS started (with%s cloud assistance).", gnss_cloud_assistance ? "" : "out");

	if (gnss_gps_req_to_send) {
		on_gnss_evt_agnss_req();
	}
	return 0;
}

static int gnss_shutdown(void)
{
	const int ret = nrf_modem_gnss_stop();

	if (ret) {
		LOG_ERR("Failed to stop GNSS (%d).", ret);
		return ret;
	}
	gnss_status_set(GNSS_STATUS_STOPPED);
	gnss_running = false;

#if CONFIG_SLM_GNSS_OUTPUT_NMEA_ON_CMUX_CHANNEL
	slm_cmux_release(CMUX_GNSS_CHANNEL);
	gnss_pipe = NULL;
#endif
	return 0;
}

#if defined(CONFIG_SLM_NRF_CLOUD)

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
static int read_agnss_req(struct nrf_modem_gnss_agnss_data_frame *req)
{
	int err;

	err = nrf_modem_gnss_read((void *)req, sizeof(*req),
					NRF_MODEM_GNSS_DATA_AGNSS_REQ);
	if (err) {
		return err;
	}

	LOG_DBG("AGNSS_REQ.sv_mask_ephe = 0x%08x", (uint32_t)req->system[0].sv_mask_ephe);
	LOG_DBG("AGNSS_REQ.sv_mask_alm  = 0x%08x", (uint32_t)req->system[0].sv_mask_alm);
	LOG_DBG("AGNSS_REQ.data_flags   = 0x%08x", req->data_flags);

	return 0;
}
#endif /* CONFIG_NRF_CLOUD_AGNSS || CONFIG_NRF_CLOUD_PGPS */

#if defined(CONFIG_NRF_CLOUD_AGNSS)
static void agnss_requestor(struct k_work *)
{
	int err;
	struct nrf_modem_gnss_agnss_data_frame req;

	err = read_agnss_req(&req);
	if (err) {
		LOG_ERR("Failed to read A-GNSS request (%d).", err);
		return;
	}

	err = nrf_cloud_agnss_request(&req);
	if (err) {
		LOG_ERR("Failed to request A-GNSS data (%d).", err);
	} else {
		LOG_INF("A-GNSS data requested.");
		/* When A-GNSS data is received it gets injected in the background by
		 * nrf_cloud_agnss_process() without notification. In the case where
		 * CONFIG_NRF_CLOUD_PGPS=y, nrf_cloud_pgps_inject() (called on PGPS_EVT_AVAILABLE)
		 * plays a role as A-GNSS expects P-GPS to process some of the data.
		 */
	}
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void pgps_requestor(struct k_work *)
{
	int err;

	/* Indirect request of P-GPS data and periodic injection */
	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		LOG_ERR("Failed to request P-GPS prediction notification (%d).", err);
	} else {
		LOG_INF("P-GPS prediction notification requested.");
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
		struct nrf_modem_gnss_agnss_data_frame req;

		LOG_INF("PGPS_EVT_AVAILABLE");
		/* read out previous NRF_MODEM_GNSS_EVT_AGNSS_REQ */
		err = read_agnss_req(&req);
		if (err) {
			LOG_INF("Failed to read back A-GNSS request (%d)."
				" Using ephemerides assistance only.", err);
		}
		err = nrf_cloud_pgps_inject(event->prediction, err ? NULL : &req);
		if (err) {
			LOG_ERR("Failed to inject P-GPS data to modem (%d).", err);
			break;
		}
		if (gnss_cloud_assistance) {
			err = nrf_cloud_pgps_preemptive_updates();
			if (err) {
				/* This is expected to fail if new predictions are to be downloaded
				 * but the connection to nRF Cloud is not available.
				 * CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD controls
				 * how often an attempt to download new predictions will be made.
				 */
				LOG_WRN("Failed to request new P-GPS predictions (%d).", err);
			}
		}
	} break;
	/* All P-GPS predictions are available. */
	case PGPS_EVT_READY:
		LOG_INF("PGPS_EVT_READY");
		break;
	case PGPS_EVT_REQUEST:
		LOG_INF("PGPS_EVT_REQUEST");
		break;
	}
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

#endif /* CONFIG_SLM_NRF_CLOUD */

#if CONFIG_SLM_GNSS_OUTPUT_NMEA_ON_CMUX_CHANNEL
static void gnss_nmea_sender(struct k_work *)
{
	int ret;
	size_t size;
	uint8_t *data;

	while ((size = ring_buf_get_claim(&gnss_nmea_rb, &data, UINT32_MAX))) {
		ret = modem_pipe_transmit(gnss_pipe, data, size);
		ring_buf_get_finish(&gnss_nmea_rb, MAX(0, ret));
		if (ret < 0) {
			LOG_ERR("Failed to send NMEA data. (%d)", ret);
			break;
		}
	}
}
#endif

#if RECEIVE_NMEA
static void on_gnss_evt_nmea(void)
{
	int err;
	size_t len;
	char *nmea_str;
	struct nrf_modem_gnss_nmea_data_frame nmea;

	err = nrf_modem_gnss_read(&nmea, sizeof(nmea), NRF_MODEM_GNSS_DATA_NMEA);
	if (err) {
		LOG_ERR("Failed to read GNSS NMEA data. (%d)", err);
	}
	nmea_str = nmea.nmea_str;
	len = strlen(nmea_str);

#if CONFIG_SLM_GNSS_OUTPUT_NMEA_ON_CMUX_CHANNEL
	const size_t space = ring_buf_space_get(&gnss_nmea_rb);

	if (space < len) {
		LOG_ERR("NMEA buffer full. Dropping %zu bytes.", ring_buf_size_get(&gnss_nmea_rb));
		ring_buf_reset(&gnss_nmea_rb);
	}
	ring_buf_put(&gnss_nmea_rb, nmea_str, len);
	k_work_submit_to_queue(&slm_work_q, &gnss_nmea_send_work);
#endif

#if CONFIG_SLM_LOG_LEVEL_DBG
	assert(len >= 2);
	len -= 2;
	assert(!strcmp(nmea_str + len, "\r\n"));
	LOG_DBG("%.*s", len, nmea_str);
#endif
}
#endif /* RECEIVE_NMEA */

#if defined(CONFIG_SLM_LOG_LEVEL_DBG)
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
#endif /* CONFIG_SLM_LOG_LEVEL_DBG */

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

static void gnss_fix_sender(struct k_work *)
{
	int err;
	struct nrf_modem_gnss_pvt_data_frame pvt;

	err = nrf_modem_gnss_read((void *)&pvt, sizeof(pvt), NRF_MODEM_GNSS_DATA_PVT);
	if (err) {
		LOG_ERR("Failed to read GNSS PVT data, error %d", err);
		return;
	}

	/* GIS accuracy: http://wiki.gis.com/wiki/index.php/Decimal_degrees, use default .6lf */
	rsp_send("\r\n#XGPS: %lf,%lf,%f,%f,%f,%f,\"%04u-%02u-%02u %02u:%02u:%02u\"\r\n",
		pvt.latitude, pvt.longitude, (double)pvt.altitude,
		(double)pvt.accuracy, (double)pvt.speed, (double)pvt.heading,
		pvt.datetime.year, pvt.datetime.month, pvt.datetime.day,
		pvt.datetime.hour, pvt.datetime.minute, pvt.datetime.seconds);

	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
		if (pvt.sv[i].sv) { /* SV number 0 indicates no satellite */
			LOG_INF("SV:%3d sig:%d c/n0:%4d el:%3d az:%3d in-fix:%d unhealthy:%d",
				pvt.sv[i].sv, pvt.sv[i].signal, pvt.sv[i].cn0,
				pvt.sv[i].elevation, pvt.sv[i].azimuth,
				(pvt.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) ? 1 : 0,
				(pvt.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) ? 1 : 0);
		}
	}

#if defined(CONFIG_SLM_PGPS_INJECT_FIX_DATA) || defined(CONFIG_SLM_CARRIER)
	struct tm gps_time = {
		.tm_year = pvt.datetime.year - 1900,
		.tm_mon  = pvt.datetime.month - 1,
		.tm_mday = pvt.datetime.day,
		.tm_hour = pvt.datetime.hour,
		.tm_min  = pvt.datetime.minute,
		.tm_sec  = pvt.datetime.seconds,
	};
#endif

#if defined(CONFIG_SLM_NRF_CLOUD)

	if (slm_nrf_cloud_send_location && slm_nrf_cloud_ready) {
		/* Report to nRF Cloud by best-effort */
		send_location(&pvt);
	}

#if defined(CONFIG_SLM_PGPS_INJECT_FIX_DATA)
	/* help date_time to save SNTP transactions */
	date_time_set(&gps_time);
	/* help nrf_cloud_pgps as most recent known location */
	nrf_cloud_pgps_set_location(pvt.latitude, pvt.longitude);
#endif

#endif /* CONFIG_SLM_NRF_CLOUD */

#if defined(CONFIG_SLM_CARRIER)
	lwm2m_carrier_location_set(pvt.latitude,
				   pvt.longitude,
				   pvt.altitude,
				   (uint32_t)mktime(&gps_time),
				   pvt.accuracy);
	lwm2m_carrier_velocity_set(pvt.heading,
				   pvt.speed,
				   pvt.vertical_speed,
				   pvt.speed_accuracy,
				   pvt.vertical_speed_accuracy);
#endif /* CONFIG_SLM_CARRIER */
}

static void on_gnss_evt_fix(void)
{
	if (gnss_ttff_start != 0) {
		const int64_t delta = k_uptime_delta(&gnss_ttff_start);

		LOG_INF("TTFF %d.%ds", (int)(delta/1000), (int)(delta%1000));
		gnss_ttff_start = 0;
	}

	k_work_submit_to_queue(&slm_work_q, &gnss_fix_send_work);
}

/* NOTE this event handler runs in interrupt context */
static void gnss_event_handler(int event)
{
	switch (event) {
#if defined(CONFIG_SLM_LOG_LEVEL_DBG)
	case NRF_MODEM_GNSS_EVT_PVT:
		on_gnss_evt_pvt();
		break;
#endif
#if RECEIVE_NMEA
	case NRF_MODEM_GNSS_EVT_NMEA:
		on_gnss_evt_nmea();
		break;
#endif
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_INF("GNSS_EVT_FIX");
		on_gnss_evt_fix();
		break;
	case NRF_MODEM_GNSS_EVT_AGNSS_REQ:
		LOG_INF("GNSS_EVT_AGNSS_REQ");
		on_gnss_evt_agnss_req();
		break;
	case NRF_MODEM_GNSS_EVT_BLOCKED:
		LOG_INF("GNSS_EVT_BLOCKED");
		break;
	case NRF_MODEM_GNSS_EVT_UNBLOCKED:
		LOG_INF("GNSS_EVT_UNBLOCKED");
		break;
	case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
		LOG_DBG("GNSS_EVT_PERIODIC_WAKEUP");
		gnss_status_set(GNSS_STATUS_PERIODIC_WAKEUP);
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_INF("GNSS_EVT_SLEEP_AFTER_TIMEOUT");
		gnss_status_set(GNSS_STATUS_SLEEP_AFTER_TIMEOUT);
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
		LOG_DBG("GNSS_EVT_SLEEP_AFTER_FIX");
		gnss_status_set(GNSS_STATUS_SLEEP_AFTER_FIX);
		break;
	case NRF_MODEM_GNSS_EVT_REF_ALT_EXPIRED:
		LOG_INF("GNSS_EVT_REF_ALT_EXPIRED");
		break;
	default:
		break;
	}
}

SLM_AT_CMD_CUSTOM(xgps_set, "AT#XGPS=", handle_at_gps);
SLM_AT_CMD_CUSTOM(xgps_read, "AT#XGPS?", handle_at_gps);
static int handle_at_gps(enum at_cmd_type cmd_type, const struct at_param_list *param_list,
			 uint32_t param_count)
{
	int err = 0;
	uint16_t op;
	uint16_t interval;
	uint16_t timeout;

	enum {
		OP_IDX = 1,
		CLOUD_ASSISTANCE_IDX,
		INTERVAL_IDX,
		TIMEOUT_IDX,
		MAX_PARAM_COUNT
	};
	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(param_list, OP_IDX, &op);
		if (err) {
			return err;
		}
		if (op == GPS_START) {
			if (gnss_running) {
				LOG_ERR("GNSS is already running. Stop it first.");
				return -EBUSY;
			}

			err = at_params_unsigned_short_get(
				param_list, CLOUD_ASSISTANCE_IDX, &gnss_cloud_assistance);
			if (err || gnss_cloud_assistance > 1) {
				return -EINVAL;
			}
			if (!IS_ENABLED(CONFIG_NRF_CLOUD_AGNSS) &&
			    !IS_ENABLED(CONFIG_NRF_CLOUD_PGPS)
			&& gnss_cloud_assistance) {
				LOG_ERR("A-GNSS and/or P-GPS must be enabled during compilation.");
				return -ENOTSUP;
			}

			if (gnss_cloud_assistance
#if defined(CONFIG_SLM_NRF_CLOUD)
			&& !slm_nrf_cloud_ready
#endif
			) {
				LOG_ERR(
					"Connection to nRF Cloud is needed for starting A-GNSS/P-GPS.");
				return -ENOTCONN;
			}

			err = at_params_unsigned_short_get(param_list, INTERVAL_IDX, &interval);
			if (err || (interval > 1 && interval < 10)) {
				return -EINVAL;
			}
			err = nrf_modem_gnss_fix_interval_set(interval);
			if (err) {
				LOG_ERR("Failed to set GNSS fix interval (%d).", err);
				return err;
			}

			/* Disable scheduled downloads for periodical tracking */
			if (interval >= 10) {
				err = nrf_modem_gnss_use_case_set(
						NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START |
						NRF_MODEM_GNSS_USE_CASE_SCHED_DOWNLOAD_DISABLE);
			} else {
				err = nrf_modem_gnss_use_case_set(
						NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START);
			}
			if (err) {
				LOG_ERR("Failed to set use case, error: %d", err);
				return err;
			}

			if (interval == 1) {
				/* Continuous navigation mode; the timeout must not be given. */
				if (param_count != TIMEOUT_IDX) {
					return -EINVAL;
				}
			} else {
				if (param_count == TIMEOUT_IDX) {
					timeout = 60;  /* default value */
				} else {
					err = at_params_unsigned_short_get(
						param_list, TIMEOUT_IDX, &timeout);
					if (err || param_count != MAX_PARAM_COUNT) {
						return -EINVAL;
					}
				}
				/* Make sure to configure the timeout unconditionally in single-fix
				 * and periodic navigation modes.
				 * An old value might otherwise remain from a previous GPS run.
				 */
				err = nrf_modem_gnss_fix_retry_set(timeout);
				if (err) {
					LOG_ERR("Failed to set GNSS fix retry (%d).", err);
					return err;
				}
			}

			err = gnss_startup();
		} else if (op == GPS_STOP && param_count == 2) {
			if (gnss_running) {
				err = gnss_shutdown();
			} else {
				err = 0;
			}
		} else {
			err = -EINVAL;
		}
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XGPS: %d,%d\r\n", (int)is_gnss_activated(), gnss_status);
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XGPS: (%d,%d),(0,1),<interval>,<timeout>\r\n",
			GPS_STOP, GPS_START);
		break;

	default:
		err = -EINVAL;
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xgpsdel, "AT#XGPSDEL", handle_at_gps_delete);
static int handle_at_gps_delete(enum at_cmd_type cmd_type, const struct at_param_list *param_list,
				uint32_t)
{
	int err = -EINVAL;
	uint32_t mask;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_int_get(param_list, 1, &mask);
		if (err || !mask || (mask & NRF_MODEM_GNSS_DELETE_TCXO_OFFSET)) {
			return -EINVAL;
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
	k_work_init(&gnss_status_notify_work, gnss_status_notifier);

#if defined(CONFIG_SLM_NRF_CLOUD)
#if defined(CONFIG_NRF_CLOUD_AGNSS)
	k_work_init(&agnss_req_work, agnss_requestor);
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
	k_work_init(&pgps_req_work, pgps_requestor);
#endif
#endif /* CONFIG_SLM_NRF_CLOUD */
	k_work_init(&gnss_fix_send_work, gnss_fix_sender);

#if defined(CONFIG_SLM_GNSS_OUTPUT_NMEA_ON_CMUX_CHANNEL)
	{
		/* NMEA GSV messages contain up to 4 satellites.
		 * Reserve space for them when we receive satellite data.
		 */
		static char gnss_nmea_buf[256 + IS_ENABLED(CONFIG_SLM_GNSS_OUTPUT_NMEA_SATELLITES) ?
			(NRF_MODEM_GNSS_NUM_GPS_SATELLITES / 4) * NRF_MODEM_GNSS_NMEA_MAX_LEN : 0];

		ring_buf_init(&gnss_nmea_rb, sizeof(gnss_nmea_buf), gnss_nmea_buf);
	}
	k_work_init(&gnss_nmea_send_work, gnss_nmea_sender);
#endif

	return err;
}

/**@brief API to uninitialize GNSS AT commands handler
 */
int slm_at_gnss_uninit(void)
{
	return 0;
}
