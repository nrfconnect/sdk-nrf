/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#if defined(CONFIG_SLM_NRF_CLOUD)

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

static struct k_work cloud_cmd;
static K_SEM_DEFINE(sem_date_time, 0, 1);

#if defined(CONFIG_NRF_CLOUD_LOCATION)

/* Cellular positioning services .*/
enum slm_nrfcloud_cellpos {
	CELLPOS_NONE = 0,
	CELLPOS_SINGLE_CELL,
	CELLPOS_MULTI_CELL,

	CELLPOS_COUNT /* Keep last. */
};
/* Whether cellular positioning is requested and if so, which. */
static enum slm_nrfcloud_cellpos nrfcloud_cell_pos = CELLPOS_NONE;

/* Whether Wi-Fi positioning is requested. */
static bool nrfcloud_wifi_pos;

/* Whether a location request is currently being sent to nRF Cloud. */
static bool nrfcloud_sending_loc_req;

static struct k_work nrfcloud_loc_req;

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
#define NCELL_CNT ((CONFIG_SLM_AT_MAX_PARAM - MAX_PARAM_CELL) / MAX_PARAM_NCELL)
BUILD_ASSERT(NCELL_CNT > 0, "CONFIG_SLM_AT_MAX_PARAM too small");

/* Neighboring cell measurements. */
static struct lte_lc_ncell nrfcloud_ncells[NCELL_CNT];

/* nRF Cloud location request cellular data. */
static struct lte_lc_cells_info nrfcloud_cell_data = {
	.neighbor_cells = nrfcloud_ncells,
	.gci_cells_count = 0
};
static bool nrfcloud_ncellmeas_done;

#define WIFI_APS_BEGIN_IDX 3
BUILD_ASSERT(WIFI_APS_BEGIN_IDX + NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN
	< CONFIG_SLM_AT_MAX_PARAM, "CONFIG_SLM_AT_MAX_PARAM too small");

/* nRF Cloud location request Wi-Fi data. */
static struct wifi_scan_info nrfcloud_wifi_data;

#endif /* CONFIG_NRF_CLOUD_LOCATION */

static char nrfcloud_device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN];

bool slm_nrf_cloud_ready;
bool slm_nrf_cloud_send_location;

#if defined(CONFIG_NRF_CLOUD_LOCATION)
AT_MONITOR(ncell_meas, "NCELLMEAS", ncell_meas_mon, PAUSED);

static void ncell_meas_mon(const char *notify)
{
	int err;
	uint32_t param_count;
	int ncellmeas_status;
	char cid[9] = {0};
	size_t size;
	char plmn[6] = {0};
	char mcc[4]  = {0};
	char tac[9] = {0};
	unsigned int ncells_count = 0;

	nrfcloud_ncellmeas_done = false;
	at_params_list_clear(&slm_at_param_list);
	err = at_parser_params_from_str(notify, NULL, &slm_at_param_list);
	if (err == -E2BIG) {
		LOG_WRN("%%NCELLMEAS result notification truncated"
			" because its parameter count exceeds CONFIG_SLM_AT_MAX_PARAM.");
	} else if (err) {
		goto exit;
	}

	/* parse status, 0: success 1: fail */
	err = at_params_int_get(&slm_at_param_list, 1, &ncellmeas_status);
	if (err) {
		goto exit;
	}
	if (ncellmeas_status != 0) {
		LOG_ERR("NCELLMEAS failed");
		err = -EAGAIN;
		goto exit;
	}
	param_count = at_params_valid_count_get(&slm_at_param_list);
	if (param_count < MAX_PARAM_CELL) { /* at least current cell */
		LOG_ERR("Missing param in NCELLMEAS notification");
		err = -EAGAIN;
		goto exit;
	}

	/* parse Cell ID */
	size = sizeof(cid);
	err = util_string_get(&slm_at_param_list, 2, cid, &size);
	if (err) {
		goto exit;
	}
	err = util_str_to_int(cid, 16, (int *)&nrfcloud_cell_data.current_cell.id);
	if (err) {
		goto exit;
	}

	/* parse PLMN */
	size = sizeof(plmn);
	err = util_string_get(&slm_at_param_list, 3, plmn, &size);
	if (err) {
		goto exit;
	}
	strncpy(mcc, plmn, 3); /* MCC always 3-digit */
	err = util_str_to_int(mcc, 10, &nrfcloud_cell_data.current_cell.mcc);
	if (err) {
		goto exit;
	}
	err = util_str_to_int(&plmn[3], 10, &nrfcloud_cell_data.current_cell.mnc);
	if (err) {
		goto exit;
	}

	/* parse TAC */
	size = sizeof(tac);
	err = util_string_get(&slm_at_param_list, 4, tac, &size);
	if (err) {
		goto exit;
	}
	err = util_str_to_int(tac, 16, (int *)&nrfcloud_cell_data.current_cell.tac);
	if (err) {
		goto exit;
	}

	/* omit timing_advance */
	nrfcloud_cell_data.current_cell.timing_advance = NRF_CLOUD_LOCATION_CELL_OMIT_TIME_ADV;

	/* parse EARFCN */
	err = at_params_unsigned_int_get(&slm_at_param_list, 6,
		&nrfcloud_cell_data.current_cell.earfcn);
	if (err) {
		goto exit;
	}

	/* parse PCI */
	err = at_params_unsigned_short_get(&slm_at_param_list, 7,
		&nrfcloud_cell_data.current_cell.phys_cell_id);
	if (err) {
		goto exit;
	}

	/* parse RSRP and RSRQ */
	err = at_params_short_get(&slm_at_param_list, 8, &nrfcloud_cell_data.current_cell.rsrp);
	if (err < 0) {
		goto exit;
	}
	err = at_params_short_get(&slm_at_param_list, 9, &nrfcloud_cell_data.current_cell.rsrq);
	if (err < 0) {
		goto exit;
	}

	/* omit measurement_time and parse neighboring cells */

	ncells_count = (param_count - MAX_PARAM_CELL) / MAX_PARAM_NCELL;
	for (unsigned int i = 0; i != ncells_count; ++i) {
		const unsigned int offset = MAX_PARAM_CELL + i * MAX_PARAM_NCELL;

		/* parse n_earfcn */
		err = at_params_unsigned_int_get(&slm_at_param_list, offset,
						 &nrfcloud_ncells[i].earfcn);
		if (err < 0) {
			goto exit;
		}

		/* parse n_phys_cell_id */
		err = at_params_unsigned_short_get(&slm_at_param_list, offset + 1,
						   &nrfcloud_ncells[i].phys_cell_id);
		if (err < 0) {
			goto exit;
		}

		/* parse n_rsrp */
		err = at_params_short_get(&slm_at_param_list, offset + 2, &nrfcloud_ncells[i].rsrp);
		if (err < 0) {
			goto exit;
		}

		/* parse n_rsrq */
		err = at_params_short_get(&slm_at_param_list, offset + 3, &nrfcloud_ncells[i].rsrq);
		if (err < 0) {
			goto exit;
		}
		/* omit time_diff */
	}

	err = 0;
	nrfcloud_cell_data.ncells_count = ncells_count;
	nrfcloud_ncellmeas_done = true;

exit:
	LOG_INF("NCELLMEAS notification parse (err: %d)", err);
}

static void loc_req_wk(struct k_work *work)
{
	int err = 0;

	if (nrfcloud_cell_pos == CELLPOS_SINGLE_CELL) {
		/* Obtain the single cell info from the modem */
		err = nrf_cloud_location_scell_data_get(&nrfcloud_cell_data.current_cell);
		if (err) {
			LOG_ERR("Failed to obtain single-cell cellular network information (%d).",
				err);
		} else {
			nrfcloud_cell_data.ncells_count = 0;
			/* Invalidate the last neighboring cell measurements
			 * because they have been partly overwritten.
			 */
			nrfcloud_ncellmeas_done = false;
		}
	}

	if (!err) {
		err = nrf_cloud_location_request(
			nrfcloud_cell_pos ? &nrfcloud_cell_data : NULL,
			nrfcloud_wifi_pos ? &nrfcloud_wifi_data : NULL, true, NULL);
		if (err) {
			LOG_ERR("Failed to request nRF Cloud location (%d).", err);
		} else {
			LOG_INF("nRF Cloud location requested.");
		}
	}

	if (err) {
		rsp_send("\r\n#XNRFCLOUDPOS: %d\r\n", err);
	}
	if (nrfcloud_wifi_pos) {
		k_free(nrfcloud_wifi_data.ap_info);
	}
	nrfcloud_sending_loc_req = false;
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

	if (slm_nrf_cloud_send_location) {
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

	slm_nrf_cloud_ready = true;
	rsp_send("\r\n#XNRFCLOUD: %d,%d\r\n", slm_nrf_cloud_ready, slm_nrf_cloud_send_location);
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	at_monitor_resume(&ncell_meas);
#endif
}

static void on_cloud_evt_disconnected(void)
{
	slm_nrf_cloud_ready = false;
	rsp_send("\r\n#XNRFCLOUD: %d,%d\r\n", slm_nrf_cloud_ready, slm_nrf_cloud_send_location);
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	at_monitor_pause(&ncell_meas);
#endif
}

static void on_cloud_evt_location_data_received(const struct nrf_cloud_data *const data)
{
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	int err;
	struct nrf_cloud_location_result result;

	err = nrf_cloud_location_process(data->ptr, &result);
	if (err == 0) {
		rsp_send("\r\n#XNRFCLOUDPOS: %d,%lf,%lf,%d\r\n",
			result.type, result.lat, result.lon, result.unc);
	} else {
		if (err == 1) {
			err = -ENOMSG;
		} else if (err == -EFAULT) {
			err = result.err;
		}
		LOG_ERR("Failed to process the location request response (%d).", err);
		rsp_send("\r\n#XNRFCLOUDPOS: %d\r\n", err);
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
	ret = nrf_modem_at_cmd(slm_at_buf, sizeof(slm_at_buf), "%s", slm_at_buf);
	if (ret < 0) {
		LOG_ERR("AT command failed: %d", ret);
		return;
	} else if (ret > 0) {
		LOG_WRN("AT command error, type: %d", nrf_modem_at_err_type(ret));
	}
	LOG_INF("MODEM RSP %s", slm_at_buf);
	/* replace \" with \' in JSON string-type value */
	for (int i = 0; i < strlen(slm_at_buf); i++) {
		if (slm_at_buf[i] == '\"') {
			slm_at_buf[i] = '\'';
		}
	}
	/* format JSON reply */
	cmd_rsp = k_malloc(strlen(slm_at_buf) + sizeof(MODEM_AT_RSP));
	if (cmd_rsp == NULL) {
		LOG_WRN("Unable to allocate buffer");
		return;
	}
	sprintf(cmd_rsp, MODEM_AT_RSP, slm_at_buf);
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
			strcpy(slm_at_buf, at_cmd->valuestring);
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
	if (slm_nrf_cloud_ready) {
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
		LOG_INF("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED: %d",
			(enum nrf_cloud_disconnect_status)evt->status);
		on_cloud_evt_disconnected();
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_ERR("NRF_CLOUD_EVT_ERROR: %d",
			(enum nrf_cloud_error_status)evt->status);
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
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		LOG_INF("NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR: %d",
			(enum nrf_cloud_connect_result)evt->status);
		break;
	default:
		LOG_DBG("Unknown NRF_CLOUD_EVT %d: %u", evt->type, evt->status);
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

/* Handles the AT#XNRFCLOUD commands. */
int handle_at_nrf_cloud(enum at_cmd_type cmd_type)
{
	enum slm_nrfcloud_operation {
		SLM_NRF_CLOUD_DISCONNECT,
		SLM_NRF_CLOUD_CONNECT,
		SLM_NRF_CLOUD_SEND,
	};
	int err = -EINVAL;
	uint16_t op;
	uint16_t send_location = 0;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&slm_at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if (op == SLM_NRF_CLOUD_CONNECT && !slm_nrf_cloud_ready) {
			if (at_params_valid_count_get(&slm_at_param_list) > 2) {
				err = at_params_unsigned_short_get(&slm_at_param_list, 2,
					&send_location);
				if (send_location != 0 && send_location != 1) {
					err = -EINVAL;
				}
				if (err < 0) {
					return err;
				}
			}
			err = nrf_cloud_connect();
			if (err) {
				LOG_ERR("Cloud connection failed, error: %d", err);
			} else {
				slm_nrf_cloud_send_location = send_location;
				/* A-GPS & P-GPS needs date_time, trigger to update current time */
				date_time_update_async(date_time_event_handler);
				if (k_sem_take(&sem_date_time, K_SECONDS(10)) != 0) {
					LOG_WRN("Failed to get current time");
				}
			}
		} else if (op == SLM_NRF_CLOUD_SEND && slm_nrf_cloud_ready) {
			/* enter data mode */
			err = enter_datamode(nrf_cloud_datamode_callback);
		} else if (op == SLM_NRF_CLOUD_DISCONNECT && slm_nrf_cloud_ready) {
			err = nrf_cloud_disconnect();
			if (err) {
				LOG_ERR("Cloud disconnection failed, error: %d", err);
			}
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND: {
		rsp_send("\r\n#XNRFCLOUD: %d,%d,%d,\"%s\"\r\n", slm_nrf_cloud_ready,
			slm_nrf_cloud_send_location, CONFIG_NRF_CLOUD_SEC_TAG, nrfcloud_device_id);
		err = 0;
	} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XNRFCLOUD: (%d,%d,%d),<send_location>\r\n",
			SLM_NRF_CLOUD_DISCONNECT, SLM_NRF_CLOUD_CONNECT, SLM_NRF_CLOUD_SEND);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

#if defined(CONFIG_NRF_CLOUD_LOCATION)

/* Handles the AT#XNRFCLOUDPOS command. */
int handle_at_nrf_cloud_pos(enum at_cmd_type cmd_type)
{
	int err;
	const uint32_t param_count = at_params_valid_count_get(&slm_at_param_list);
	uint16_t cell_pos, wifi_pos;

	if (cmd_type != AT_CMD_TYPE_SET_COMMAND) {
		return -ENOTSUP;
	}

	if (!slm_nrf_cloud_ready) {
		LOG_ERR("Not connected to nRF Cloud.");
		return -ENOTCONN;
	}

	if (nrfcloud_sending_loc_req) {
		/* Avoid potential concurrency issues writing to global variables. */
		LOG_ERR("nRF Cloud location request sending already ongoing.");
		return -EBUSY;
	}

	if (param_count < WIFI_APS_BEGIN_IDX) {
		return -EINVAL;
	}

	err = at_params_unsigned_short_get(&slm_at_param_list, 1, &cell_pos);
	if (err) {
		return err;
	}

	err = at_params_unsigned_short_get(&slm_at_param_list, 2, &wifi_pos);
	if (err) {
		return err;
	}

	if (cell_pos >= CELLPOS_COUNT || wifi_pos > 1) {
		return -EINVAL;
	}

	if (!cell_pos && !wifi_pos) {
		LOG_ERR("At least one of cellular/Wi-Fi information must be included.");
		return -EINVAL;
	}

	if (cell_pos == CELLPOS_MULTI_CELL && !nrfcloud_ncellmeas_done) {
		LOG_ERR("%s", "No neighboring cell measurement. Did you run `AT%NCELLMEAS`?");
		return -EAGAIN;
	}

	if (!wifi_pos && param_count > WIFI_APS_BEGIN_IDX) {
		/* No Wi-Fi AP allowed if no Wi-Fi positioning. */
		return -E2BIG;
	}

	if (wifi_pos) {
		nrfcloud_wifi_data.ap_info = k_malloc(
			sizeof(*nrfcloud_wifi_data.ap_info) * (param_count - WIFI_APS_BEGIN_IDX));
		if (!nrfcloud_wifi_data.ap_info) {
			return -ENOMEM;
		}
		/* Parse the AP parameters. */
		nrfcloud_wifi_data.cnt = 0;
		for (unsigned int param_idx = WIFI_APS_BEGIN_IDX; param_idx < param_count;
		++param_idx) {
			struct wifi_scan_result *const ap =
				&nrfcloud_wifi_data.ap_info[nrfcloud_wifi_data.cnt];
			char mac_addr_str[WIFI_MAC_ADDR_STR_LEN];
			unsigned int mac_addr[WIFI_MAC_ADDR_LEN];
			int32_t rssi = NRF_CLOUD_LOCATION_WIFI_OMIT_RSSI;
			size_t len;

			++nrfcloud_wifi_data.cnt;

			/* Parse the MAC address. */
			len = sizeof(mac_addr_str);
			err = at_params_string_get(
				&slm_at_param_list, param_idx, mac_addr_str, &len);
			if (!err && (len != sizeof(mac_addr_str)
				|| sscanf(mac_addr_str, WIFI_MAC_ADDR_TEMPLATE,
						&mac_addr[0], &mac_addr[1], &mac_addr[2],
						&mac_addr[3], &mac_addr[4], &mac_addr[5])
					!= WIFI_MAC_ADDR_LEN)) {
				err = -EBADMSG; /* A different error code to differentiate. */
			}
			if (err) {
				LOG_ERR("MAC address %u malformed (%d).",
					nrfcloud_wifi_data.cnt, err);
				break;
			}
			for (unsigned int i = 0; i != WIFI_MAC_ADDR_LEN; ++i) {
				ap->mac[i] = mac_addr[i];
			}
			ap->mac_length = WIFI_MAC_ADDR_LEN;

			/* Parse the RSSI, if present. */
			if (!at_params_int_get(&slm_at_param_list, param_idx + 1, &rssi)) {
				++param_idx;
				const int rssi_min = -128;
				const int rssi_max = 0;

				if (rssi < rssi_min || rssi > rssi_max) {
					err = -EINVAL;
					LOG_ERR("RSSI %u out of bounds ([%d,%d]).",
						nrfcloud_wifi_data.cnt, rssi_min, rssi_max);
					break;
				}
			}
			ap->rssi = rssi;

			ap->band     = 0;
			ap->security = WIFI_SECURITY_TYPE_UNKNOWN;
			ap->mfp      = WIFI_MFP_UNKNOWN;
			/* CONFIG_NRF_CLOUD_WIFI_LOCATION_ENCODE_OPT excludes the other members. */
		}

		if (nrfcloud_wifi_data.cnt < NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN) {
			err = -EINVAL;
			LOG_ERR("Insufficient access point count (got %u, min %u).",
				nrfcloud_wifi_data.cnt, NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN);
		}
		if (err) {
			k_free(nrfcloud_wifi_data.ap_info);
			return err;
		}
	}

	nrfcloud_cell_pos = cell_pos;
	nrfcloud_wifi_pos = wifi_pos;

	nrfcloud_sending_loc_req = true;
	k_work_submit_to_queue(&slm_work_q, &nrfcloud_loc_req);
	return 0;
}

#endif /* CONFIG_NRF_CLOUD_LOCATION */

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
	k_work_init(&nrfcloud_loc_req, loc_req_wk);
#endif
	nrf_cloud_client_id_get(nrfcloud_device_id, sizeof(nrfcloud_device_id));

	return err;
}

int slm_at_nrfcloud_uninit(void)
{
	if (slm_nrf_cloud_ready) {
		(void)nrf_cloud_disconnect();
	}
	(void)nrf_cloud_uninit();

	return 0;
}
#endif /* CONFIG_SLM_NRF_CLOUD */
