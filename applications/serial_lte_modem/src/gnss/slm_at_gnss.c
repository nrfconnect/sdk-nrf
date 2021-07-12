/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <logging/log.h>
#include <date_time.h>
#include <pm_config.h>
#include <net/cloud.h>
#include <nrf_modem_gnss.h>
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include <net/nrf_cloud_cell_pos.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_gnss.h"

LOG_MODULE_REGISTER(slm_gnss, CONFIG_SLM_LOG_LEVEL);

#define SERVICE_INFO_GPS \
	"{\"state\":{\"reported\":{\"device\": {\"serviceInfo\":{\"ui\":[\"GPS\"]}}}}}"

/**@brief GNSS operations. */
enum slm_gnss_operation {
	GPS_STOP,
	GPS_START,
	nRF_CLOUD_DISCONNECT = GPS_STOP,
	nRF_CLOUD_CONNECT    = GPS_START,
	AGPS_STOP            = GPS_STOP,
	AGPS_START           = GPS_START,
	PGPS_STOP            = GPS_STOP,
	PGPS_START           = GPS_START,
	CELLPOS_STOP         = GPS_STOP,
	CELLPOS_START_SCELL  = GPS_START,
	CELLPOS_START_MCELL
};

static struct k_work agps_req;
static struct k_work pgps_req;
static struct k_work fix_rep;
static struct k_work cell_pos_req;
static enum nrf_cloud_cell_pos_type cell_pos_type;

static struct cloud_backend *nrf_cloud;
static bool nrf_cloud_ready;
static bool location_signify;
static uint64_t ttft_start;
static enum {
	RUN_TYPE_NONE,
	RUN_TYPE_GPS,
	RUN_TYPE_AGPS,
	RUN_TYPE_PGPS,
	RUN_TYPE_CELL_POS
} run_type;

K_SEM_DEFINE(sem_date_time, 0, 1);

/* global functions defined in different files */
void rsp_send(const uint8_t *str, size_t len);

/* global variable defined in different files */
extern struct k_work_q slm_work_q;
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];

static int read_agps_req(struct gps_agps_request *req)
{
	int err;
	struct nrf_modem_gnss_agps_data_frame agps_data;

	err = nrf_modem_gnss_read((void *)&agps_data, sizeof(agps_data),
					NRF_MODEM_GNSS_DATA_AGPS_REQ);
	if (err) {
		LOG_ERR("Failed to read GNSS AGPS req, error %d", err);
		return -EAGAIN;
	}

	req->sv_mask_ephe = agps_data.sv_mask_ephe,
	req->sv_mask_alm  = agps_data.sv_mask_alm,
	req->utc          = (agps_data.data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) ? 1 : 0,
	req->klobuchar    = (agps_data.data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) ? 1 : 0,
	req->nequick      = (agps_data.data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) ? 1 : 0,
	req->system_time_tow =
	      (agps_data.data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) ? 1 : 0,
	req->position     = (agps_data.data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) ? 1 : 0,
	req->integrity    = (agps_data.data_flags & NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) ? 1 : 0;

	return 0;
}

static void agps_req_wk(struct k_work *work)
{
	int err;
	struct gps_agps_request req;

	ARG_UNUSED(work);

	err = read_agps_req(&req);
	if (err) {
		return;
	}

	err = nrf_cloud_agps_request(req);
	if (err) {
		LOG_ERR("Failed to request A-GPS data: %d", err);
	}
}

static void pgps_req_wk(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	/* Indirect request of P-GPS data and periodic injection */
	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		LOG_ERR("Failed to request notify of prediction: %d", err);
	}
}

static void cell_pos_req_wk(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	err = nrf_cloud_cell_pos_request(cell_pos_type, true);
	if (err) {
		LOG_ERR("Failed to request cell_pos %d, error: %d", cell_pos_type, err);
	}
}

static void pgps_event_handler(enum nrf_cloud_pgps_event event,
			       struct nrf_cloud_pgps_prediction *prediction)
{
	int err;

	switch (event) {
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
		struct gps_agps_request req;

		LOG_INF("PGPS_EVT_AVAILABLE");
		/* read out previous NRF_MODEM_GNSS_EVT_AGPS_REQ */
		err = read_agps_req(&req);
		if (err) {
			/* All assistance elements as requested */
			err = nrf_cloud_pgps_inject(prediction, &req, NULL);
		} else {
			/* ephemerides assistance only */
			err = nrf_cloud_pgps_inject(prediction, NULL, NULL);
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

	sprintf(rsp_buf,
		"\r\n#XGPS: %lf,%lf,%f,%f,%f,%f,\"%04u-%02u-%02u %02u:%02u:%02u\"\r\n",
		pvt.latitude, pvt.longitude, pvt.altitude,
		pvt.accuracy, pvt.speed, pvt.heading,
		pvt.datetime.year, pvt.datetime.month, pvt.datetime.day,
		pvt.datetime.hour, pvt.datetime.minute, pvt.datetime.seconds);
	rsp_send(rsp_buf, strlen(rsp_buf));

	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
		if (pvt.sv[i].sv) { /* SV number 0 indicates no satellite */
			LOG_INF("SV:%3d sig: %d c/n0:%4d el:%3d az:%3d in-fix: %d unhealthy: %d",
				pvt.sv[i].sv, pvt.sv[i].signal, pvt.sv[i].cn0,
				pvt.sv[i].elevation, pvt.sv[i].azimuth,
				(pvt.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) ? 1 : 0,
				(pvt.sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) ? 1 : 0);
		}
	}

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
		LOG_INF("TTFF %ds", (int)k_uptime_delta(&ttft_start)/1000);
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
		//LOG_INF("GNSS_EVT_PVT");
		LOG_DBG("GNSS_EVT_PVT");
		on_gnss_evt_pvt();
		break;
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_INF("GNSS_EVT_FIX");
		on_gnss_evt_fix();
		break;
	case NRF_MODEM_GNSS_EVT_NMEA:
		LOG_DBG("GNSS_EVT_NMEA");
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
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_DBG("GNSS_EVT_SLEEP_AFTER_TIMEOUT");
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
		LOG_DBG("GNSS_EVT_SLEEP_AFTER_FIX");
		break;
	case NRF_MODEM_GNSS_EVT_REF_ALT_EXPIRED:
		LOG_DBG("GNSS_EVT_REF_ALT_EXPIRED");
		break;
	default:
		break;
	}
}

static void on_cloud_evt_ready(void)
{
	if (location_signify) {
		int err;
		struct cloud_msg msg = {
			.qos = CLOUD_QOS_AT_MOST_ONCE,
			.endpoint.type = CLOUD_EP_STATE,
			.buf = SERVICE_INFO_GPS,
			.len = strlen(SERVICE_INFO_GPS)
		};

		/* Update nRF Cloud with GPS service info signifying GPS capabilities. */
		err = cloud_send(nrf_cloud, &msg);
		if (err) {
			LOG_WRN("Failed to send message to cloud, error: %d", err);
		}
	}

	nrf_cloud_ready = true;
	sprintf(rsp_buf, "\r\n#XNRFCLOUD: %d,%d\r\n", nrf_cloud_ready, location_signify);
	rsp_send(rsp_buf, strlen(rsp_buf));
}

static void on_cloud_evt_disconnected(void)
{
	nrf_cloud_ready = false;
	sprintf(rsp_buf, "\r\n#XNRFCLOUD: %d,%d\r\n", nrf_cloud_ready, location_signify);
	rsp_send(rsp_buf, strlen(rsp_buf));
}

static void on_cloud_evt_data_received(const struct cloud_event *const evt)
{
	int err = 0;

	if (run_type == RUN_TYPE_AGPS) {
		err = nrf_cloud_agps_process(evt->data.msg.buf, evt->data.msg.len, NULL);
		if (err) {
			LOG_INF("Unable to process A-GPS data, error: %d", err);
		}
	} else if (run_type == RUN_TYPE_PGPS) {
		err = nrf_cloud_pgps_process(evt->data.msg.buf, evt->data.msg.len);
		if (err) {
			LOG_ERR("Unable to process P-GPS data, error: %d", err);
		}
	} else if (run_type == RUN_TYPE_CELL_POS) {
		struct nrf_cloud_cell_pos_result result;

		err = nrf_cloud_cell_pos_process(evt->data.msg.buf, &result);
		if (err == 0) {
			if (ttft_start != 0) {
				LOG_INF("TTFF %ds", (int)k_uptime_delta(&ttft_start)/1000);
				ttft_start = 0;
			}
			sprintf(rsp_buf, "\r\n#XCELLPOS: %d,%lf,%lf,%d\r\n",
				result.type, result.lat, result.lon, result.unc);
			rsp_send(rsp_buf, strlen(rsp_buf));
			run_type = RUN_TYPE_NONE;
		} else if (err == 1) {
			LOG_WRN("No position found");
		} else {
			LOG_ERR("Unable to process cell pos data, error: %d", err);
		}
	}
}

static void cloud_event_handler(const struct cloud_backend *const backend,
				const struct cloud_event *const evt, void *user_data)
{
	ARG_UNUSED(backend);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTING:
		LOG_DBG("CLOUD_EVT_CONNECTING");
		break;
	case CLOUD_EVT_CONNECTED:
		LOG_INF("CLOUD_EVT_CONNECTED");
		break;
	case CLOUD_EVT_READY:
		LOG_INF("CLOUD_EVT_READY");
		on_cloud_evt_ready();
		break;
	case CLOUD_EVT_DISCONNECTED:
		LOG_INF("CLOUD_EVT_DISCONNECTED");
		on_cloud_evt_disconnected();
		break;
	case CLOUD_EVT_ERROR:
		LOG_ERR("CLOUD_EVT_ERROR");
		break;
	case CLOUD_EVT_DATA_SENT:
		LOG_DBG("CLOUD_EVT_DATA_SENT");
		break;
	case CLOUD_EVT_DATA_RECEIVED:
		LOG_INF("CLOUD_EVT_DATA_RECEIVED");
		on_cloud_evt_data_received(evt);
		break;
	case CLOUD_EVT_PAIR_REQUEST:
		LOG_DBG("CLOUD_EVT_PAIR_REQUEST");
		break;
	case CLOUD_EVT_PAIR_DONE:
		LOG_DBG("CLOUD_EVT_PAIR_DONE");
		break;
	case CLOUD_EVT_FOTA_DONE:
		LOG_DBG("CLOUD_EVT_FOTA_DONE");
		break;
	case CLOUD_EVT_FOTA_ERROR:
		LOG_ERR("CLOUD_EVT_FOTA_ERROR");
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

/**@brief handle AT#XGPS commands
 *  AT#XGPS=<op>[,<interval>[,<timeout>]]
 *  AT#XGPS? READ command not supported
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
			if (interval == 0) {
				err = nrf_modem_gnss_use_case_set(
						NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY);
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
			}
			err = nrf_modem_gnss_start();
			if (err) {
				LOG_ERR("Failed to start GPS, error: %d", err);
			} else {
				run_type = RUN_TYPE_GPS;
				ttft_start = k_uptime_get();
			}
		} else if (op == GPS_STOP && run_type == RUN_TYPE_GPS) {
			err = nrf_modem_gnss_stop();
			run_type = RUN_TYPE_NONE;
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XGPS: (%d,%d),<interval>,<timeout>\r\n",
			GPS_STOP, GPS_START);
		rsp_send(rsp_buf, strlen(rsp_buf));
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
			err = cloud_connect(nrf_cloud);
			if (err) {
				LOG_ERR("Cloud connection failed, error: %d", err);
			}
		} else if (op == nRF_CLOUD_DISCONNECT && nrf_cloud_ready) {
			err = cloud_disconnect(nrf_cloud);
			if (err) {
				LOG_ERR("Cloud disconnection failed, error: %d", err);
			}
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		sprintf(rsp_buf, "\r\n#XNRFCLOUD: %d,%d\r\n", nrf_cloud_ready, location_signify);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XNRFCLOUD: (%d,%d),<signify>\r\n",
			nRF_CLOUD_CONNECT, nRF_CLOUD_DISCONNECT);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XAGPS commands
 *  AT#XAGPS=<op>[,<interval>[,<timeout>]]
 *  AT#XAGPS? READ command not supported
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
			if (interval == 0) {
				err = nrf_modem_gnss_use_case_set(
						NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY);
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
			}

			/* A-GPS needs date_time, trigger to update current time */
			date_time_update_async(date_time_event_handler);
			if (k_sem_take(&sem_date_time, K_SECONDS(10)) != 0) {
				LOG_ERR("Failed to get current time");
				return -EAGAIN;
			}

			err = nrf_modem_gnss_start();
			if (err) {
				LOG_ERR("Failed to start GNSS, error: %d", err);
			} else {
				run_type = RUN_TYPE_AGPS;
				ttft_start = k_uptime_get();
			}
		} else if (op == AGPS_STOP && run_type == RUN_TYPE_AGPS) {
			err = nrf_modem_gnss_stop();
			run_type = RUN_TYPE_NONE;
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XAGPS: (%d,%d),<interval>,<timeout>\r\n",
			AGPS_STOP, AGPS_START);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XPGPS commands
 *  AT#XPGPS=<op>[,<interval>[,<timeout>]]
 *  AT#XPGPS? READ command not supported
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
			}

			/* P-GPS needs date_time, trigger to update current time */
			date_time_update_async(date_time_event_handler);
			if (k_sem_take(&sem_date_time, K_SECONDS(10)) != 0) {
				LOG_ERR("Failed to get current time");
				return -EAGAIN;
			}

			struct nrf_cloud_pgps_init_param param = {
				.event_handler = pgps_event_handler,
				.storage_base = PM_MCUBOOT_SECONDARY_ADDRESS,
				.storage_size = PM_MCUBOOT_SECONDARY_SIZE};

			err = nrf_cloud_pgps_init(&param);
			if (err) {
				LOG_ERR("Error from P-GPS init: %d", err);
				return err;
			}

			err = nrf_modem_gnss_start();
			if (err) {
				LOG_ERR("Failed to start GNSS, error: %d", err);
			} else {
				run_type = RUN_TYPE_PGPS;
				ttft_start = k_uptime_get();
			}
		} else if (op == PGPS_STOP && run_type == RUN_TYPE_PGPS) {
			err = nrf_modem_gnss_stop();
			run_type = RUN_TYPE_NONE;
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XPGPS: (%d,%d),<interval>,<timeout>\r\n",
			PGPS_STOP, PGPS_START);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XCELLPOS commands
 *  AT#XCELLPOS=<op>
 *  AT#XCELLPOS? READ command not supported
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
				cell_pos_type = CELL_POS_TYPE_SINGLE;
			} else {
				sprintf(rsp_buf, "\r\n#XCELLPOS: \"not supported\"\r\n");
				rsp_send(rsp_buf, strlen(rsp_buf));
				return -ENOTSUP;
			}
			ttft_start = k_uptime_get();
			k_work_submit_to_queue(&slm_work_q, &cell_pos_req);
			run_type = RUN_TYPE_CELL_POS;
		} else if (op == CELLPOS_STOP) {
			run_type = RUN_TYPE_NONE;
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XCELLPOS: (%d,%d,%d)\r\n",
			CELLPOS_STOP, CELLPOS_START_SCELL, CELLPOS_START_MCELL);
		rsp_send(rsp_buf, strlen(rsp_buf));
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

	err = nrf_modem_gnss_init();
	if (err) {
		LOG_ERR("Could not initialize GNSS, error: %d", err);
		return err;
	}
	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Could set GNSS event handler, error: %d", err);
		return err;
	}

	if (nrf_cloud == NULL) {
		nrf_cloud = cloud_get_binding("NRF_CLOUD");
		if (nrf_cloud == NULL) {
			LOG_ERR("Could not get binding to backend");
			return -EINVAL;
		}
		err = cloud_init(nrf_cloud, cloud_event_handler);
		if (err) {
			LOG_ERR("Cloud backend could not be initialized, error: %d", err);
			return err;
		}
	}

	k_work_init(&agps_req, agps_req_wk);
	k_work_init(&pgps_req, pgps_req_wk);
	k_work_init(&cell_pos_req, cell_pos_req_wk);
	k_work_init(&fix_rep, fix_rep_wk);

	return err;
}

/**@brief API to uninitialize GNSS AT commands handler
 */
int slm_at_gnss_uninit(void)
{
	if (nrf_cloud_ready) {
		(void)cloud_disconnect(nrf_cloud);
	}
	(void)cloud_uninit(nrf_cloud);
	nrf_cloud = NULL;
	(void)nrf_modem_gnss_deinit();

	return 0;
}
