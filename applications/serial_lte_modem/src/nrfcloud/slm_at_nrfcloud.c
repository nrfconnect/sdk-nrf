/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <date_time.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include <net/nrf_cloud_location.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_nrfcloud.h"

LOG_MODULE_REGISTER(slm_nrfcloud, CONFIG_SLM_LOG_LEVEL);

#define SERVICE_INFO_GNSS \
	"{\"state\":{\"reported\":{\"device\": {\"serviceInfo\":{\"ui\":[\"GNSS\"]}}}}}"

#define MODEM_AT_RSP \
	"{\"appId\":\"MODEM\", \"messageType\":\"RSP\", \"data\":\"%s\"}"

/**@brief nRF Cloud operations. */
enum slm_nrfcloud_operation {
	SLM_NRF_CLOUD_DISCONNECT,
	SLM_NRF_CLOUD_CONNECT,
	SLM_NRF_CLOUD_SEND,
	SLM_NRF_CLOUD_CELLPOS_STOP         = SLM_NRF_CLOUD_DISCONNECT,
	SLM_NRF_CLOUD_CELLPOS_START_SCELL  = SLM_NRF_CLOUD_CONNECT,
	SLM_NRF_CLOUD_CELLPOS_START_MCELL  = SLM_NRF_CLOUD_SEND,
	SLM_NRF_CLOUD_WIFIPOS_STOP         = SLM_NRF_CLOUD_DISCONNECT,
	SLM_NRF_CLOUD_WIFIPOS_START        = SLM_NRF_CLOUD_CONNECT
};

static struct k_work cloud_cmd;
static K_SEM_DEFINE(sem_date_time, 0, 1);

#if defined(CONFIG_NRF_CLOUD_LOCATION)
static struct k_work cell_pos_req;
static struct k_work wifi_pos_req;
static enum nrf_cloud_location_type cell_pos_type;
static uint64_t ttft_start;
static enum {
	RUN_TYPE_NONE,
	RUN_TYPE_CELL_POS,
	RUN_TYPE_WIFI_POS
} run_type;

/** Definitions for %NCELLMEAS notification
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
#endif /* CONFIG_NRF_CLOUD_LOCATION */

static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN];

/* global variable to be used in different files */
bool nrf_cloud_ready;
bool nrf_cloud_location_signify;

/* global variable defined in different files */
extern struct k_work_q slm_work_q;
extern struct at_param_list at_param_list;
extern uint8_t at_buf[SLM_AT_MAX_CMD_LEN];

#if defined(CONFIG_NRF_CLOUD_LOCATION)
AT_MONITOR(ncell_meas, "NCELLMEAS", ncell_meas_mon, PAUSED);

static void ncell_meas_mon(const char *notify)
{
	int err;
	uint32_t param_count;
	char cid[9] = {0};
	size_t size;
	char plmn[6] = {0};
	char mcc[4]  = {0};
	char tac[9] = {0};

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

	/* omit measurement_time and parse neighboring cells */
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
#endif /* CONFIG_NRF_CLOUD_LOCATION */

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

static void on_cloud_evt_ready(void)
{
	int err;

	if (nrf_cloud_location_signify) {
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
	rsp_send("\r\n#XNRFCLOUD: %d,%d\r\n", nrf_cloud_ready, nrf_cloud_location_signify);
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	at_monitor_resume(&ncell_meas);
#endif
}

static void on_cloud_evt_disconnected(void)
{
	nrf_cloud_ready = false;
	rsp_send("\r\n#XNRFCLOUD: %d,%d\r\n", nrf_cloud_ready, nrf_cloud_location_signify);
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	at_monitor_pause(&ncell_meas);
#endif
}

static void on_cloud_evt_location_data_received(const struct nrf_cloud_data *const data)
{
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	int err;
	struct nrf_cloud_location_result result;

	if (run_type != RUN_TYPE_CELL_POS && run_type != RUN_TYPE_WIFI_POS) {
		LOG_WRN("Not expecting location data during run_type: %d", run_type);
		return;
	}

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
#else
	ARG_UNUSED(data);
#endif
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
	const cJSON *at_cmd = NULL;
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
		if (evt->status != NRF_CLOUD_CONNECT_RES_SUCCESS) {
			LOG_ERR("Failed to connect to nRF Cloud, status: %d",
				(enum nrf_cloud_connect_result)evt->status);
		}
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_INF("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_INF("NRF_CLOUD_EVT_READY");
		on_cloud_evt_ready();
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_INF("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED: status %d",
			(enum nrf_cloud_disconnect_status)evt->status);
		on_cloud_evt_disconnected();
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_ERR("NRF_CLOUD_EVT_ERROR: %d", evt->status);
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

static int nrf_cloud_datamode_callback(uint8_t op, const uint8_t *data, int len, uint8_t flags)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if ((flags & SLM_DATAMODE_FLAGS_MORE_DATA) != 0) {
			LOG_ERR("Datamode buffer overflow");
			(void)exit_datamode_handler(-EOVERFLOW);
			return -EOVERFLOW;
		}
		ret = do_cloud_send_msg(data, len);
		LOG_INF("datamode send: %d", ret);
		if (ret < 0) {
			(void)exit_datamode_handler(ret);
		}
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
	}

	return ret;
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
		if (op == SLM_NRF_CLOUD_CONNECT && !nrf_cloud_ready) {
			nrf_cloud_location_signify = 0;
			if (at_params_valid_count_get(&at_param_list) > 2) {
				err = at_params_unsigned_short_get(&at_param_list, 2, &signify);
				if (err < 0) {
					return err;
				}
				nrf_cloud_location_signify = (signify > 0);
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
		} else if (op == SLM_NRF_CLOUD_SEND && nrf_cloud_ready) {
			/* enter data mode */
			err = enter_datamode(nrf_cloud_datamode_callback);
		} else if (op == SLM_NRF_CLOUD_DISCONNECT && nrf_cloud_ready) {
			err = nrf_cloud_disconnect();
			if (err) {
				LOG_ERR("Cloud disconnection failed, error: %d", err);
			}
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND: {
		rsp_send("\r\n#XNRFCLOUD: %d,%d,%d,\"%s\"\r\n", nrf_cloud_ready,
			nrf_cloud_location_signify, CONFIG_NRF_CLOUD_SEC_TAG, device_id);
		err = 0;
	} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XNRFCLOUD: (%d,%d,%d),<signify>\r\n",
			SLM_NRF_CLOUD_DISCONNECT, SLM_NRF_CLOUD_CONNECT, SLM_NRF_CLOUD_SEND);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

#if defined(CONFIG_NRF_CLOUD_LOCATION)
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
		if ((op == SLM_NRF_CLOUD_CELLPOS_START_SCELL ||
		     op == SLM_NRF_CLOUD_CELLPOS_START_MCELL) &&
		    nrf_cloud_ready && run_type == RUN_TYPE_NONE) {
			if (op == SLM_NRF_CLOUD_CELLPOS_START_SCELL) {
				cell_pos_type = LOCATION_TYPE_SINGLE_CELL;
			} else {
				cell_pos_type = LOCATION_TYPE_MULTI_CELL;
			}
			ttft_start = k_uptime_get();
			k_work_submit_to_queue(&slm_work_q, &cell_pos_req);
			run_type = RUN_TYPE_CELL_POS;
		} else if (op == SLM_NRF_CLOUD_CELLPOS_STOP) {
			run_type = RUN_TYPE_NONE;
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XCELLPOS: %d\r\n", (run_type == RUN_TYPE_CELL_POS) ? 1 : 0);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XCELLPOS: (%d,%d,%d)\r\n", SLM_NRF_CLOUD_CELLPOS_STOP,
			 SLM_NRF_CLOUD_CELLPOS_START_SCELL, SLM_NRF_CLOUD_CELLPOS_START_MCELL);
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

		if (op == SLM_NRF_CLOUD_WIFIPOS_START && nrf_cloud_ready &&
		    run_type == RUN_TYPE_NONE) {
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
					break;  /* Max AP reached */
				} else if (index == count - 1) {
					break;  /* no more input params */
				}
			}

			ttft_start = k_uptime_get();
			k_work_submit_to_queue(&slm_work_q, &wifi_pos_req);
			run_type = RUN_TYPE_WIFI_POS;
			err = 0;
		} else if (op == SLM_NRF_CLOUD_WIFIPOS_STOP) {
			run_type = RUN_TYPE_NONE;
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		rsp_send("\r\n#XWIFIPOS: %d\r\n", (run_type == RUN_TYPE_WIFI_POS) ? 1 : 0);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XWIFIPOS: (%d,%d)\r\n", SLM_NRF_CLOUD_WIFIPOS_STOP,
			 SLM_NRF_CLOUD_WIFIPOS_START);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}
#endif

/**@brief API to initialize nRF Cloud AT commands handler
 */
int slm_at_nrfcloud_init(void)
{
	int err = 0;
	struct nrf_cloud_init_param init_param = {
		.event_handler = cloud_event_handler
	};

	err = nrf_cloud_init(&init_param);
	if (err && err != -EACCES) {
		LOG_ERR("Cloud could not be initialized, error: %d", err);
		return err;
	}

	k_work_init(&cloud_cmd, cloud_cmd_wk);
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	k_work_init(&cell_pos_req, cell_pos_req_wk);
	k_work_init(&wifi_pos_req, wifi_pos_req_wk);
#endif
	nrf_cloud_client_id_get(device_id, sizeof(device_id));

	return err;
}

/**@brief API to uninitialize nRF Cloud AT commands handler
 */
int slm_at_nrfcloud_uninit(void)
{
	if (nrf_cloud_ready) {
		(void)nrf_cloud_disconnect();
	}
	(void)nrf_cloud_uninit();

	return 0;
}
