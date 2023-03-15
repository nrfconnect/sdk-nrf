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
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include <net/nrf_cloud_location.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_gnss.h"

LOG_MODULE_REGISTER(slm_gnss, CONFIG_SLM_LOG_LEVEL);

#define SERVICE_INFO_GNSS \
	"{\"state\":{\"reported\":{\"device\": {\"serviceInfo\":{\"ui\":[\"GNSS\"]}}}}}"

#define MODEM_AT_RSP \
	"{\"appId\":\"MODEM\", \"messageType\":\"RSP\", \"data\":\"%s\"}"

#define LOCATION_REPORT_MS 5000

/**@brief GNSS operations. */
enum slm_gnss_operation {
	GPS_STOP,
	GPS_START,
	nRF_CLOUD_DISCONNECT = GPS_STOP,
	nRF_CLOUD_CONNECT    = GPS_START,
	nRF_CLOUD_SEND,
	AGPS_STOP            = GPS_STOP,
	AGPS_START           = GPS_START,
	PGPS_STOP            = GPS_STOP,
	PGPS_START           = GPS_START,
	CELLPOS_STOP         = GPS_STOP,
	CELLPOS_START_SCELL  = GPS_START,
	CELLPOS_START_MCELL  = nRF_CLOUD_SEND,
	WIFIPOS_STOP         = GPS_STOP,
	WIFIPOS_START        = GPS_START
};

static struct k_work cloud_cmd;
static struct k_work agps_req;
static struct k_work pgps_req;
static struct k_work fix_rep;
static struct k_work cell_pos_req;
static struct k_work wifi_pos_req;
static enum nrf_cloud_location_type cell_pos_type;

static bool nrf_cloud_ready;
static bool location_signify;
static uint64_t ttft_start;
static enum {
	RUN_TYPE_NONE,
	RUN_TYPE_GPS,
	RUN_TYPE_AGPS,
	RUN_TYPE_PGPS,
	RUN_TYPE_CELL_POS,
	RUN_TYPE_WIFI_POS
} run_type;
static enum {
	RUN_STATUS_STOPPED,
	RUN_STATUS_STARTED,
	RUN_STATUS_PERIODIC_WAKEUP,
	RUN_STATUS_SLEEP_AFTER_TIMEOUT,
	RUN_STATUS_SLEEP_AFTER_FIX,
	RUN_STATUS_MAX
} run_status;

static K_SEM_DEFINE(sem_date_time, 0, 1);

/** Definitins for %NCELLMEAS notification
 * %NCELLMEAS: status [,<cell_id>, <plmn>, <tac>, <timing_advance>, <current_earfcn>,
 * <current_phys_cell_id>, <current_rsrp>, <current_rsrq>,<measurement_time>,]
 * [,<n_earfcn>1, <n_phys_cell_id>1, <n_rsrp>1, <n_rsrq>1,<time_diff>1]
 * [,<n_earfcn>2, <n_phys_cell_id>2, <n_rsrp>2, <n_rsrq>2,<time_diff>2] ...
 * [,<n_earfcn>17, <n_phys_cell_id>17, <n_rsrp>17, <n_rsrq>17,<time_diff>17
 *
 * Max 17 ncell, but align with CONFIG_SLM_AT_MAX_PARAM
 * 11 number of parameters for current cell (including "%NCELLMEAS")
 * 5  number of parameters for one neighboring cell
 */
#define MAX_PARAM_CELL   11
#define MAX_PARAM_NCELL  5
/* Must support at least all params for current cell plus one ncell */
BUILD_ASSERT(CONFIG_SLM_AT_MAX_PARAM > (MAX_PARAM_CELL + MAX_PARAM_NCELL),
	     "CONFIG_SLM_AT_MAX_PARAM too small");
#define NCELL_CNT ((CONFIG_SLM_AT_MAX_PARAM - MAX_PARAM_CELL) / MAX_PARAM_NCELL)

#define AP_CNT_MAX       5

static struct lte_lc_ncell neighbor_cells[NCELL_CNT];
static struct lte_lc_cells_info cell_data = {
	.neighbor_cells = neighbor_cells
};
static int ncell_meas_status;
static struct wifi_scan_result scan_results[AP_CNT_MAX];
static struct wifi_scan_info wifi_data = {
	.ap_info = scan_results
};
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN];

/* global variable defined in different files */
extern struct k_work_q slm_work_q;
extern struct at_param_list at_param_list;
extern uint8_t at_buf[SLM_AT_MAX_CMD_LEN];

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

static int read_agps_req(struct nrf_modem_gnss_agps_data_frame *req)
{
	int err;

	err = nrf_modem_gnss_read((void *)req, sizeof(*req),
					NRF_MODEM_GNSS_DATA_AGPS_REQ);
	if (err) {
		LOG_ERR("Failed to read GNSS AGPS req, error %d", err);
		return -EAGAIN;
	}

	return 0;
}

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

AT_MONITOR(ncell_meas, "NCELLMEAS", ncell_meas_mon, PAUSED);

static void ncell_meas_mon(const char *notify)
{
	int err;
	uint32_t param_count;

	ncell_meas_status = -1;
	at_params_list_clear(&at_param_list);
	err = at_parser_params_from_str(notify, NULL, &at_param_list);
	if (err) {
		goto exit;
	}

	/* parse status, 0: success 1: fail */
	err = at_params_int_get(&at_param_list, 1, &ncell_meas_status);
	if (err) {
		goto exit;
	}
	if (ncell_meas_status != 0) {
		LOG_ERR("NCELLMEAS failed");
		err = -EAGAIN;
		goto exit;
	}
	param_count = at_params_valid_count_get(&at_param_list);
	if (param_count < MAX_PARAM_CELL) { /* at least current cell */
		LOG_ERR("Missing param in NCELLMEAS notification");
		err = -EAGAIN;
		goto exit;
	}

	/* parse Cell ID */
	char cid[9] = {0};
	size_t size;

	size = sizeof(cid);
	err = util_string_get(&at_param_list, 2, cid, &size);
	if (err) {
		goto exit;
	}
	err = util_str_to_int(cid, 16, (int *)&cell_data.current_cell.id);
	if (err) {
		goto exit;
	}

	/* parse PLMN */
	char plmn[6] = {0};
	char mcc[4]  = {0};

	size = sizeof(plmn);
	err = util_string_get(&at_param_list, 3, plmn, &size);
	if (err) {
		goto exit;
	}
	strncpy(mcc, plmn, 3); /* MCC always 3-digit */
	err = util_str_to_int(mcc, 10, &cell_data.current_cell.mcc);
	if (err) {
		goto exit;
	}
	err = util_str_to_int(&plmn[3], 10, &cell_data.current_cell.mnc);
	if (err) {
		goto exit;
	}

	/* parse TAC */
	char tac[9] = {0};

	size = sizeof(tac);
	err = util_string_get(&at_param_list, 4, tac, &size);
	if (err) {
		goto exit;
	}
	err = util_str_to_int(tac, 16, (int *)&cell_data.current_cell.tac);
	if (err) {
		goto exit;
	}

	/* omit timing_advance */
	cell_data.current_cell.timing_advance = NRF_CLOUD_LOCATION_CELL_OMIT_TIME_ADV;

	/* parse EARFCN */
	err = at_params_unsigned_int_get(&at_param_list, 6, &cell_data.current_cell.earfcn);
	if (err) {
		goto exit;
	}

	/* parse PCI */
	err = at_params_unsigned_short_get(&at_param_list, 7,
					   &cell_data.current_cell.phys_cell_id);
	if (err) {
		goto exit;
	}

	/* parse RSRP and RSRQ */
	err = at_params_short_get(&at_param_list, 8, &cell_data.current_cell.rsrp);
	if (err < 0) {
		goto exit;
	}
	err = at_params_short_get(&at_param_list, 9, &cell_data.current_cell.rsrq);
	if (err < 0) {
		goto exit;
	}

	/* omit measurement_time */

	cell_data.ncells_count = 0;
	for (int i = 0; i < NCELL_CNT; i++) {
		int offset = i * MAX_PARAM_NCELL + 11;

		if (param_count < (offset + MAX_PARAM_NCELL)) {
			break;
		}

		/* parse n_earfcn */
		err = at_params_unsigned_int_get(&at_param_list, offset,
						 &neighbor_cells[i].earfcn);
		if (err < 0) {
			goto exit;
		}

		/* parse n_phys_cell_id */
		err = at_params_unsigned_short_get(&at_param_list, offset + 1,
						   &neighbor_cells[i].phys_cell_id);
		if (err < 0) {
			goto exit;
		}

		/* parse n_rsrp */
		err = at_params_short_get(&at_param_list, offset + 2, &neighbor_cells[i].rsrp);
		if (err < 0) {
			goto exit;
		}

		/* parse n_rsrq */
		err = at_params_short_get(&at_param_list, offset + 3, &neighbor_cells[i].rsrq);
		if (err < 0) {
			goto exit;
		}

		/* omit time_diff */

		cell_data.ncells_count++;
	}

	err = 0;

exit:
	LOG_INF("NCELLMEAS notification parse (err: %d)", err);
}

static void cell_pos_req_wk(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	if (cell_pos_type == LOCATION_TYPE_SINGLE_CELL) {
		struct lte_lc_cells_info scell_inf = {.ncells_count = 0};

		/* Obtain the single cell info from the modem */
		err = nrf_cloud_location_scell_data_get(&scell_inf.current_cell);
		if (err) {
			LOG_ERR("Failed to obtain cellular network info, error: %d", err);
			return;
		}

		/* Request location using only the single cell info */
		err = nrf_cloud_location_request(&scell_inf, NULL, true, NULL);
		if (err) {
			LOG_ERR("Failed to request SCELL, error: %d", err);
			rsp_send("\r\n#XCELLPOS: %d\r\n", err);
			run_type = RUN_TYPE_NONE;
		} else {
			LOG_INF("nRF Cloud SCELL requested");
		}
	} else if (cell_pos_type == LOCATION_TYPE_MULTI_CELL) {
		if (ncell_meas_status == 0 && cell_data.current_cell.id != 0) {
			err = nrf_cloud_location_request(&cell_data, NULL, true, NULL);
			if (err) {
				LOG_ERR("Failed to request MCELL, error: %d", err);
				rsp_send("\r\n#XCELLPOS: %d\r\n", err);
				run_type = RUN_TYPE_NONE;
			} else {
				LOG_INF("nRF Cloud MCELL requested, with %d neighboring cells",
					cell_data.ncells_count);
			}
		} else {
			LOG_WRN("No request of MCELL");
			rsp_send("\r\n#XCELLPOS: \r\n");
			run_type = RUN_TYPE_NONE;
		}
	}
}

static void wifi_pos_req_wk(struct k_work *work)
{
	int err;

	/* Request location using Wi-Fi access point info */
	err = nrf_cloud_location_request(NULL, &wifi_data, true, NULL);
	if (err) {
		LOG_ERR("Failed to request Wi-Fi location, error: %d", err);
		rsp_send("\r\n#XWIFIPOS: %d\r\n", err);
		run_type = RUN_TYPE_NONE;
	} else {
		LOG_INF("nRF Cloud Wi-Fi location requested");
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
			/* All assistance elements as requested */
			err = nrf_cloud_pgps_inject(event->prediction, &req);
		} else {
			/* ephemerides assistance only */
			err = nrf_cloud_pgps_inject(event->prediction, NULL);
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

static int do_cloud_send_msg(const char *message, int len)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = message,
		.data.len = len,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
		.qos = MQTT_QOS_0_AT_MOST_ONCE
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send failed, error: %d", err);
	}

	return err;
}

static void send_location(struct nrf_modem_gnss_nmea_data_frame * const nmea_data)
{
	static int64_t last_ts_ms = NRF_CLOUD_NO_TIMESTAMP;
	int err;
	char *json_msg = NULL;
	cJSON *msg_obj = NULL;
	struct nrf_cloud_gnss_data gnss = {
		.ts_ms = NRF_CLOUD_NO_TIMESTAMP,
		.type = NRF_CLOUD_GNSS_TYPE_MODEM_NMEA,
		.mdm_nmea = nmea_data
	};

	/* On failure, NRF_CLOUD_NO_TIMESTAMP is used and the timestamp is omitted */
	(void)date_time_now(&gnss.ts_ms);

	if ((last_ts_ms == NRF_CLOUD_NO_TIMESTAMP) ||
	    (gnss.ts_ms == NRF_CLOUD_NO_TIMESTAMP) ||
	    (gnss.ts_ms > (last_ts_ms + LOCATION_REPORT_MS))) {
		last_ts_ms = gnss.ts_ms;
	} else {
		return;
	}

	msg_obj = cJSON_CreateObject();
	if (!msg_obj) {
		return;
	}

	err = nrf_cloud_gnss_msg_json_encode(&gnss, msg_obj);
	if (err) {
		goto clean_up;
	}

	json_msg = cJSON_PrintUnformatted(msg_obj);
	if (!json_msg) {
		err = -ENOMEM;
		goto clean_up;
	}

	err = do_cloud_send_msg(json_msg, strlen(json_msg));

clean_up:
	cJSON_Delete(msg_obj);
	if (json_msg) {
		cJSON_free((void *)json_msg);
	}

	if (err) {
		LOG_WRN("Failed to send location, error %d", err);
	}
}

static void fix_rep_wk(struct k_work *work)
{
	int err;
	struct nrf_modem_gnss_pvt_data_frame pvt;
	struct nrf_modem_gnss_nmea_data_frame nmea;

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

	if (IS_ENABLED(CONFIG_SLM_LOG_LEVEL_DBG)) {
		goto update_pgps;
	}

	/* Read $GPGGA NMEA message */
	err = nrf_modem_gnss_read((void *)&nmea, sizeof(nmea), NRF_MODEM_GNSS_DATA_NMEA);
	if (err) {
		LOG_WRN("Failed to read GNSS NMEA data, error %d", err);
	} else {
		/* Report to nRF Cloud by best-effort */
		if (nrf_cloud_ready && location_signify) {
			send_location(&nmea);
		}

		/* GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx \r\n */
		if (location_signify) {
			rsp_send("\r\n#XGPS: %s", nmea.nmea_str);
		}
	}

update_pgps:
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
	if (run_type == RUN_TYPE_AGPS) {
		k_work_submit_to_queue(&slm_work_q, &agps_req);
	} else if (run_type == RUN_TYPE_PGPS) {
		/* Check whether prediction data available or not */
		k_work_submit_to_queue(&slm_work_q, &pgps_req);
	}
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

static void on_cloud_evt_ready(void)
{
	if (location_signify) {
		int err;
		struct nrf_cloud_tx_data msg = {
			.data.ptr = SERVICE_INFO_GNSS,
			.data.len = strlen(SERVICE_INFO_GNSS),
			.topic_type = NRF_CLOUD_TOPIC_STATE,
			.qos = MQTT_QOS_0_AT_MOST_ONCE
		};

		/* Update nRF Cloud with GPS service info signifying GPS capabilities. */
		err = nrf_cloud_send(&msg);
		if (err) {
			LOG_WRN("Failed to send message to cloud, error: %d", err);
		}
	}

	nrf_cloud_ready = true;
	rsp_send("\r\n#XNRFCLOUD: %d,%d\r\n", nrf_cloud_ready, location_signify);
	at_monitor_resume(&ncell_meas);
}

static void on_cloud_evt_disconnected(void)
{
	nrf_cloud_ready = false;
	rsp_send("\r\n#XNRFCLOUD: %d,%d\r\n", nrf_cloud_ready, location_signify);
	at_monitor_pause(&ncell_meas);
}

static void on_cloud_evt_location_data_received(const struct nrf_cloud_data *const data)
{
	if (run_type != RUN_TYPE_CELL_POS && run_type != RUN_TYPE_WIFI_POS) {
		LOG_WRN("Not expecting location data during run_type: %d", run_type);
		return;
	}

	int err;
	struct nrf_cloud_location_result result;

	err = nrf_cloud_location_process(data->ptr, &result);
	if (err == 0) {
		if (ttft_start != 0) {
			LOG_INF("TTFF %ds", (int)k_uptime_delta(&ttft_start)/1000);
			ttft_start = 0;
		}
		if (run_type == RUN_TYPE_CELL_POS) {
			rsp_send("\r\n#XCELLPOS: %d,%lf,%lf,%d\r\n",
				result.type, result.lat, result.lon, result.unc);
		} else {
			rsp_send("\r\n#XWIFIPOS: %d,%lf,%lf,%d\r\n",
				result.type, result.lat, result.lon, result.unc);
		}
		run_type = RUN_TYPE_NONE;
	} else if (err == 1) {
		LOG_WRN("No position found");
	} else if (err == -EFAULT) {
		LOG_ERR("Unable to determine location from cell data, error: %d",
			result.err);
	} else {
		LOG_ERR("Failed to process cellular location response, error: %d",
			err);
	}
}

static void cloud_cmd_wk(struct k_work *work)
{
	int ret;
	char *cmd_rsp;

	ARG_UNUSED(work);

	/* Send AT command to modem */
	ret = nrf_modem_at_cmd(at_buf, sizeof(at_buf), "%s", at_buf);
	if (ret < 0) {
		LOG_ERR("AT command failed: %d", ret);
		return;
	} else if (ret > 0) {
		LOG_WRN("AT command error, type: %d", nrf_modem_at_err_type(ret));
	}
	LOG_INF("MODEM RSP %s", at_buf);
	/* replace \" with \' in JSON string-type value */
	for (int i = 0; i < strlen(at_buf); i++) {
		if (at_buf[i] == '\"') {
			at_buf[i] = '\'';
		}
	}
	/* format JSON reply */
	cmd_rsp = k_malloc(strlen(at_buf) + sizeof(MODEM_AT_RSP));
	if (cmd_rsp == NULL) {
		LOG_WRN("Unable to allocate buffer");
		return;
	}
	sprintf(cmd_rsp, MODEM_AT_RSP, at_buf);
	/* Send AT response to cloud */
	ret = do_cloud_send_msg(cmd_rsp, strlen(cmd_rsp));
	if (ret) {
		LOG_ERR("Send AT response to cloud error: %d", ret);
	}
	k_free(cmd_rsp);
}

static bool handle_cloud_cmd(const char *buf_in)
{
	const cJSON *app_id = NULL;
	const cJSON *msg_type = NULL;
	bool ret = false;

	cJSON *cloud_cmd_json = cJSON_Parse(buf_in);

	if (cloud_cmd_json == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();

		if (error_ptr != NULL) {
			LOG_ERR("JSON parsing error before: %s", error_ptr);
		}
		goto end;
	}

	app_id = cJSON_GetObjectItemCaseSensitive(cloud_cmd_json, NRF_CLOUD_JSON_APPID_KEY);
	if (cJSON_GetStringValue(app_id) == NULL) {
		goto end;
	}

	/* Format expected from nrf cloud:
	 * {"appId":"MODEM", "messageType":"CMD", "data":"<AT command>"}
	 */
	if (strcmp(app_id->valuestring, NRF_CLOUD_JSON_APPID_VAL_MODEM) == 0) {
		msg_type = cJSON_GetObjectItemCaseSensitive(cloud_cmd_json,
							    NRF_CLOUD_JSON_MSG_TYPE_KEY);
		if (cJSON_GetStringValue(msg_type) != NULL) {
			if (strcmp(msg_type->valuestring, NRF_CLOUD_JSON_MSG_TYPE_VAL_CMD) != 0) {
				goto end;
			}
		}

		const cJSON *at_cmd = NULL;

		/* The value of attribute "data" contains the actual command */
		at_cmd = cJSON_GetObjectItemCaseSensitive(cloud_cmd_json, NRF_CLOUD_JSON_DATA_KEY);
		if (cJSON_GetStringValue(at_cmd) != NULL) {
			LOG_INF("MODEM CMD %s", at_cmd->valuestring);
			strcpy(at_buf, at_cmd->valuestring);
			k_work_submit_to_queue(&slm_work_q, &cloud_cmd);
			ret = true;
		}
	/* Format expected from nrf cloud:
	 * {"appId":"DEVICE", "messageType":"DISCON"}
	 */
	} else if (strcmp(app_id->valuestring, NRF_CLOUD_JSON_APPID_VAL_DEVICE) == 0) {
		msg_type = cJSON_GetObjectItemCaseSensitive(cloud_cmd_json,
							    NRF_CLOUD_JSON_MSG_TYPE_KEY);
		if (cJSON_GetStringValue(msg_type) != NULL) {
			if (strcmp(msg_type->valuestring,
			    NRF_CLOUD_JSON_MSG_TYPE_VAL_DISCONNECT) == 0) {
				LOG_INF("DEVICE DISCON");
				/* No action required, handled in lib_nrf_cloud */
				ret = true;
			}
		}
	}

end:
	cJSON_Delete(cloud_cmd_json);
	return ret;
}

static void on_cloud_evt_data_received(const struct nrf_cloud_data *const data)
{
	if (nrf_cloud_ready) {
		if (((char *)data->ptr)[0] == '{') {
			/* Check if it's a cloud command sent from the cloud */
			if (handle_cloud_cmd(data->ptr)) {
				return;
			}
		}
		rsp_send("\r\n#XNRFCLOUD: %s\r\n", (char *)data->ptr);
	}
}

static void cloud_event_handler(const struct nrf_cloud_evt *evt)
{
	switch (evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_INF("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		LOG_ERR("NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR: status %d", evt->status);
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_INF("NRF_CLOUD_EVT_READY");
		on_cloud_evt_ready();
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_INF("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED: status %d", evt->status);
		on_cloud_evt_disconnected();
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_ERR("NRF_CLOUD_EVT_ERROR: status %d", evt->status);
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_RX_DATA_GENERAL:
		LOG_INF("NRF_CLOUD_EVT_RX_DATA_GENERAL");
		on_cloud_evt_data_received(&evt->data);
		break;
	case NRF_CLOUD_EVT_RX_DATA_LOCATION:
		LOG_INF("NRF_CLOUD_EVT_RX_DATA_LOCATION");
		on_cloud_evt_location_data_received(&evt->data);
		break;
	case NRF_CLOUD_EVT_RX_DATA_SHADOW:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_SHADOW");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATED");
		break;
	case NRF_CLOUD_EVT_FOTA_DONE:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_DONE");
		break;
	default:
		break;
	}
}

static void date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
	case DATE_TIME_OBTAINED_MODEM:
	case DATE_TIME_OBTAINED_NTP:
	case DATE_TIME_OBTAINED_EXT:
		LOG_DBG("DATE_TIME OBTAINED");
		k_sem_give(&sem_date_time);
		break;
	case DATE_TIME_NOT_OBTAINED:
		LOG_INF("DATE_TIME_NOT_OBTAINED");
		break;
	default:
		break;
	}
}

static int nrf_cloud_datamode_callback(uint8_t op, const uint8_t *data, int len)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		ret = do_cloud_send_msg(data, len);
		LOG_INF("datamode send: %d", ret);
		if (ret < 0) {
			(void)exit_datamode(ret);
		}
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
	}

	return ret;
}

/**@brief handle AT#XGPS commands
 *  AT#XGPS=<op>[,<interval>[,<timeout>]]
 *  AT#XGPS?
 *  AT#XGPS=?
 */
int handle_at_gps(enum at_cmd_type cmd_type)
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
		if (op == GPS_START && run_type == RUN_TYPE_NONE) {
			err = at_params_unsigned_short_get(&at_param_list, 2, &interval);
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
			if (at_params_unsigned_short_get(&at_param_list, 3, &timeout) == 0) {
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

/**@brief handle AT#XNRFCLOUD commands
 *  AT#XNRFCLOUD=<op>[,<signify>]
 *  AT#XNRFCLOUD?
 *  AT#XNRFCLOUD=?
 */
int handle_at_nrf_cloud(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t signify = 0;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if (op == nRF_CLOUD_CONNECT && !nrf_cloud_ready) {
			location_signify = 0;
			if (at_params_valid_count_get(&at_param_list) > 2) {
				err = at_params_unsigned_short_get(&at_param_list, 2, &signify);
				if (err < 0) {
					return err;
				}
				location_signify = (signify > 0);
			}
			err = nrf_cloud_connect();
			if (err) {
				LOG_ERR("Cloud connection failed, error: %d", err);
			} else {
				/* A-GPS & P-GPS needs date_time, trigger to update current time */
				date_time_update_async(date_time_event_handler);
				if (k_sem_take(&sem_date_time, K_SECONDS(10)) != 0) {
					LOG_WRN("Failed to get current time");
				}
			}
		} else if (op == nRF_CLOUD_SEND && nrf_cloud_ready) {
			/* enter data mode */
			err = enter_datamode(nrf_cloud_datamode_callback);
		} else if (op == nRF_CLOUD_DISCONNECT && nrf_cloud_ready) {
			err = nrf_cloud_disconnect();
			if (err) {
				LOG_ERR("Cloud disconnection failed, error: %d", err);
			}
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND: {
		rsp_send("\r\n#XNRFCLOUD: %d,%d,%d,\"%s\"\r\n", nrf_cloud_ready,
			location_signify, CONFIG_NRF_CLOUD_SEC_TAG, device_id);
		err = 0;
	} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XNRFCLOUD: (%d,%d,%d),<signify>\r\n",
			nRF_CLOUD_DISCONNECT, nRF_CLOUD_CONNECT, nRF_CLOUD_SEND);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XAGPS commands
 *  AT#XAGPS=<op>[,<interval>[,<timeout>]]
 *  AT#XAGPS?
 *  AT#XAGPS=?
 */
int handle_at_agps(enum at_cmd_type cmd_type)
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
		if (op == AGPS_START && nrf_cloud_ready && run_type == RUN_TYPE_NONE) {
			err = at_params_unsigned_short_get(&at_param_list, 2, &interval);
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
			if (at_params_unsigned_short_get(&at_param_list, 3, &timeout) == 0) {
				err = nrf_modem_gnss_fix_retry_set(timeout);
				if (err) {
					LOG_ERR("Failed to set fix retry, error: %d", err);
					return err;
				}
			} /* else leave it to default or previously configured timeout */

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

/**@brief handle AT#XPGPS commands
 *  AT#XPGPS=<op>[,<interval>[,<timeout>]]
 *  AT#XPGPS?
 *  AT#XPGPS=?
 */
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
		if (op == PGPS_START && nrf_cloud_ready && run_type == RUN_TYPE_NONE) {
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

/**@brief handle AT#XGPSDEL commands
 *  AT#XGPSDEL=<mask>
 *  AT#XGPSDEL? READ command not supported
 *  AT#XGPSDEL=?
 */
int handle_at_gps_delete(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint32_t mask;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_int_get(&at_param_list, 1, &mask);
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

/**@brief handle AT#XCELLPOS commands
 *  AT#XCELLPOS=<op>
 *  AT#XCELLPOS?
 *  AT#XCELLPOS=?
 */
int handle_at_cellpos(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if ((op == CELLPOS_START_SCELL || op == CELLPOS_START_MCELL) &&
		    nrf_cloud_ready && run_type == RUN_TYPE_NONE) {
			if (op == CELLPOS_START_SCELL) {
				cell_pos_type = LOCATION_TYPE_SINGLE_CELL;
			} else {
				cell_pos_type = LOCATION_TYPE_MULTI_CELL;
			}
			ttft_start = k_uptime_get();
			k_work_submit_to_queue(&slm_work_q, &cell_pos_req);
			run_type = RUN_TYPE_CELL_POS;
		} else if (op == CELLPOS_STOP) {
			run_type = RUN_TYPE_NONE;
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XCELLPOS: %d\r\n", (run_type == RUN_TYPE_CELL_POS) ? 1 : 0);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XCELLPOS: (%d,%d,%d)\r\n",
			CELLPOS_STOP, CELLPOS_START_SCELL, CELLPOS_START_MCELL);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XWIFIPOS commands
 *  AT#XWIFIPOS=<op>[,<ssid>,<mac>[,<ssid>,<mac>[...]]]
 *  AT#XWIFIPOS?
 *  AT#XWIFIPOS=?
 */
int handle_at_wifipos(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint32_t count = at_params_valid_count_get(&at_param_list);
	char mac_addr_str[WIFI_MAC_ADDR_STR_LEN];
	uint32_t mac_addr[WIFI_MAC_ADDR_LEN];
	struct wifi_scan_result *ap;
	uint16_t op;
	int index = 1;
	int size;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, index, &op);
		if (err < 0) {
			return err;
		}

		if (op == WIFIPOS_START && nrf_cloud_ready && run_type == RUN_TYPE_NONE) {
			if (count < (2 + NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN * 2)) {
				LOG_ERR("Too few APs %d, minimal %d", (count - 2) / 2,
					NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN);
				return -EINVAL;
			}

			wifi_data.cnt = 0;
			while (true) {
				ap = &wifi_data.ap_info[wifi_data.cnt];

				size = WIFI_SSID_MAX_LEN;
				err = at_params_string_get(&at_param_list, ++index, ap->ssid,
							   &size);
				if (err < 0) {
					return err;
				}
				ap->ssid_length = size;
				size = WIFI_MAC_ADDR_STR_LEN;
				err = at_params_string_get(&at_param_list, ++index, mac_addr_str,
							   &size);
				if (err < 0 || size != WIFI_MAC_ADDR_STR_LEN) {
					return err;
				}
				err = sscanf(mac_addr_str, WIFI_MAC_ADDR_TEMPLATE,
					     &mac_addr[0], &mac_addr[1], &mac_addr[2],
					     &mac_addr[3], &mac_addr[4], &mac_addr[5]);
				if (err != WIFI_MAC_ADDR_LEN) {
					LOG_ERR("Parse MAC address %d error", wifi_data.cnt);
					return -EINVAL;
				}
				ap->mac[0] = mac_addr[0];
				ap->mac[1] = mac_addr[1];
				ap->mac[2] = mac_addr[2];
				ap->mac[3] = mac_addr[3];
				ap->mac[4] = mac_addr[4];
				ap->mac[5] = mac_addr[5];
				ap->mac_length = WIFI_MAC_ADDR_LEN;
				ap->channel = NRF_CLOUD_LOCATION_WIFI_OMIT_CHAN;
				ap->rssi    = NRF_CLOUD_LOCATION_WIFI_OMIT_RSSI;

				wifi_data.cnt++;
				if (wifi_data.cnt == AP_CNT_MAX) {
					break;  /* max AP reached */
				} else if (index == count - 1) {
					break;  /* no more params */
				}
			}

			ttft_start = k_uptime_get();
			k_work_submit_to_queue(&slm_work_q, &wifi_pos_req);
			run_type = RUN_TYPE_WIFI_POS;
			err = 0;
		} else if (op == WIFIPOS_STOP) {
			run_type = RUN_TYPE_NONE;
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XWIFIPOS: %d\r\n", (run_type == RUN_TYPE_WIFI_POS) ? 1 : 0);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XWIFIPOS: (%d,%d)\r\n", WIFIPOS_STOP, WIFIPOS_START);
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
	struct nrf_cloud_init_param init_param = {
		.event_handler = cloud_event_handler
	};

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Could set GNSS event handler, error: %d", err);
		return err;
	}

	err = nrf_cloud_init(&init_param);
	if (err && err != -EACCES) {
		LOG_ERR("Cloud could not be initialized, error: %d", err);
		return err;
	}

	k_work_init(&cloud_cmd, cloud_cmd_wk);
	k_work_init(&agps_req, agps_req_wk);
	k_work_init(&pgps_req, pgps_req_wk);
	k_work_init(&cell_pos_req, cell_pos_req_wk);
	k_work_init(&wifi_pos_req, wifi_pos_req_wk);
	k_work_init(&fix_rep, fix_rep_wk);
	nrf_cloud_client_id_get(device_id, sizeof(device_id));

	return err;
}

/**@brief API to uninitialize GNSS AT commands handler
 */
int slm_at_gnss_uninit(void)
{
	if (nrf_cloud_ready) {
		(void)nrf_cloud_disconnect();
	}
	(void)nrf_cloud_uninit();

	return 0;
}
