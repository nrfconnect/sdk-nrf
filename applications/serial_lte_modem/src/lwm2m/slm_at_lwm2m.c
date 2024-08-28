/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <modem/pdn.h>
#include <modem/uicc_lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_binaryappdata.h>

#include "slm_at_host.h"
#include "slm_settings.h"
#include "slm_util.h"
#include "slm_at_lwm2m.h"

LOG_MODULE_REGISTER(slm_lwm2m, CONFIG_SLM_LOG_LEVEL);

#define IMEI_LEN 15
static uint8_t imei_buf[IMEI_LEN + 1];
#define ENDPOINT_NAME_LEN (IMEI_LEN + 14 + 1)
static uint8_t endpoint_name[ENDPOINT_NAME_LEN + 1];
static struct lwm2m_ctx client;

#define INST_UNSET 65535

/* Todo: Document event level */
enum lwm2m_event_t {
	LWM2M_EVENT_NONE,
	LWM2M_EVENT_FOTA,
	LWM2M_EVENT_CLIENT,
	LWM2M_EVENT_PDN,
	LWM2M_EVENT_LAST,
};

/* Todo: Document operations */
enum lwm2m_operation_t {
	LWM2M_OP_STOP,
	LWM2M_OP_START,
	LWM2M_OP_PAUSE,
	LWM2M_OP_UPDATE,
};

static bool connected;
static bool auto_connected;
static bool no_serv_suspended;
static int lwm2m_event_level;
static int operation;

static void slm_lwm2m_rd_client_start(void);
static void slm_lwm2m_rd_client_stop(void);
extern int slm_lwm2m_init_device(char *serial_num);

static void slm_lwm2m_event(int type, int event)
{
	if (type <= lwm2m_event_level) {
		rsp_send("\r\n#XLWM2MEVT: %d,%d\r\n", type, event);
	}
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_BINARYAPPDATA_OBJ_SUPPORT)
static void slm_lwm2m_rsp_data(uint16_t obj_inst_id, uint16_t res_inst_id,
			       const void *data, uint16_t data_len)
{
	char hex_data[2 * data_len + 1];

	bin2hex(data, data_len, hex_data, sizeof(hex_data));
	rsp_send("#XLWM2MDATA: %d,%d,\"%s\"\r\n", obj_inst_id, res_inst_id, hex_data);
}

static int slm_lwm2m_received_data(uint16_t obj_inst_id, uint16_t res_inst_id,
				   const void *data, uint16_t data_len)
{
	rsp_send("\r\n");
	slm_lwm2m_rsp_data(obj_inst_id, res_inst_id, data, data_len);

	return 0;
}

static uint16_t set_inst_start(uint16_t inst_start)
{
	return inst_start == INST_UNSET ? 0 : inst_start;
}

static int slm_lwm2m_binaryappdata_read(uint16_t obj_inst_start, uint16_t obj_inst_stop,
					uint16_t res_inst_start, uint16_t res_inst_stop)
{
	uint16_t obj_inst_id = set_inst_start(obj_inst_start);
	uint16_t res_inst_id = set_inst_start(res_inst_start);
	void *data_ptr;
	uint16_t data_len;
	bool first = true;
	bool found = false;
	int err_count = 0;

	while (obj_inst_id <= obj_inst_stop && err_count < 2) {
		if (lwm2m_binaryappdata_read(obj_inst_id, res_inst_id, &data_ptr, &data_len) == 0) {
			if (data_len) {
				if (first) {
					rsp_send("\r\n");
					first = false;
				}
				slm_lwm2m_rsp_data(obj_inst_id, res_inst_id, data_ptr, data_len);
			}
			found = true;
			if (res_inst_id < res_inst_stop) {
				res_inst_id++;
			} else {
				obj_inst_id++;
				res_inst_id = set_inst_start(res_inst_start);
			}
			err_count = 0;
		} else {
			/* First read error, try next object instance. */
			obj_inst_id++;
			res_inst_id = set_inst_start(res_inst_start);
			err_count++;
		}
	}

	return found ? 0 : -ENOENT;
}
#endif /* CONFIG_LWM2M_CLIENT_UTILS_BINARYAPPDATA_OBJ_SUPPORT */

/* Automatically start/stop when the default PDN connection goes up/down. */
static void slm_pdp_ctx_event_cb(uint8_t cid, enum pdn_event event, int reason)
{
	ARG_UNUSED(cid);
	ARG_UNUSED(reason);

	switch (event) {
	case PDN_EVENT_ACTIVATED:
		LOG_INF("Connection up");
		if (IS_ENABLED(CONFIG_SLM_LWM2M_AUTO_STARTUP) && !auto_connected) {
			LOG_INF("LTE connected, auto-start LwM2M engine");
			slm_lwm2m_rd_client_start();
			auto_connected = true;
		} else if (connected && no_serv_suspended) {
			LOG_INF("LTE connected, resuming LwM2M engine");
			lwm2m_engine_resume();
			operation = LWM2M_OP_START;
		}
		no_serv_suspended = false;
		break;
	case PDN_EVENT_DEACTIVATED:
	case PDN_EVENT_NETWORK_DETACH:
		LOG_INF("Connection down");
		if (connected) {
			LOG_INF("LTE not connected, suspending LwM2M engine");
			lwm2m_engine_pause();
			operation = LWM2M_OP_PAUSE;
			no_serv_suspended = true;
		}
		break;
	default:
		LOG_INF("PDN connection event %d", event);
		break;
	}

	slm_lwm2m_event(LWM2M_EVENT_PDN, event);
}

void client_acknowledge(void)
{
	lwm2m_acknowledge(&client);
}

static int slm_lwm2m_firmware_event_cb(struct lwm2m_fota_event *event)
{
	switch (event->id) {
	case LWM2M_FOTA_DOWNLOAD_START:
		/* FOTA download process started */
		LOG_INF("FOTA download started for instance %d", event->download_start.obj_inst_id);
		break;

	case LWM2M_FOTA_DOWNLOAD_FINISHED:
		/* FOTA download process finished */
		LOG_INF("FOTA download ready for instance %d, dfu_type %d",
			event->download_ready.obj_inst_id, event->download_ready.dfu_type);
		break;

	case LWM2M_FOTA_UPDATE_IMAGE_REQ:
		/* FOTA update new image */
		LOG_INF("FOTA update request for instance %d, dfu_type %d",
			event->update_req.obj_inst_id, event->update_req.dfu_type);
		break;

	case LWM2M_FOTA_UPDATE_MODEM_RECONNECT_REQ:
		/* FOTA requests modem re-initialization and client re-connection */
		/* Return -1 to cause normal system reboot */
		return -1;

	case LWM2M_FOTA_UPDATE_ERROR:
		/* FOTA process failed or was cancelled */
		LOG_ERR("FOTA failure %d by status %d", event->failure.obj_inst_id,
			event->failure.update_failure);
		break;
	}

	slm_lwm2m_event(LWM2M_EVENT_FOTA, event->id);

	return 0;
}

static void slm_lwm2m_rd_client_event_cb(struct lwm2m_ctx *client_ctx,
					 enum lwm2m_rd_client_event client_event)
{
	switch (client_event) {
	case LWM2M_RD_CLIENT_EVENT_NONE:
		LOG_INF("Invalid event");
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE:
		LOG_WRN("Bootstrap registration failure");
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
		LOG_INF("Bootstrap registration complete");
		connected = false;
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
		LOG_INF("Bootstrap transfer complete");
		/* Workaround an issue with server being disabled after Register 4.03 Forbidden */
		/* lwm2m_server_reset_timestamps(); */
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
		LOG_WRN("Registration failure");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
		LOG_INF("Registration complete");
		connected = true;
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT:
		LOG_WRN("Registration timeout");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		LOG_INF("Registration update complete");
		connected = true;
		break;

	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		LOG_WRN("Deregister failure");
		connected = false;
		break;

	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		LOG_INF("Disconnected");
		connected = false;
		break;

	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
		LOG_INF("Queue mode RX window closed");
		break;

	case LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED:
		LOG_INF("Engine suspended");
		break;

	case LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR:
		LOG_WRN("Network error");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE:
		LOG_INF("Registration update");
		break;

	case LWM2M_RD_CLIENT_EVENT_DEREGISTER:
		LOG_INF("Deregister");
		break;

	case LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED:
		LOG_INF("Server disabled");
		break;
	}

	slm_lwm2m_event(LWM2M_EVENT_CLIENT, client_event);
}

int slm_at_lwm2m_init(void)
{
	int ret;

	pdn_default_ctx_cb_reg(slm_pdp_ctx_event_cb);

	ret = modem_info_init();
	if (ret < 0) {
		LOG_ERR("Unable to init modem_info (%d)", ret);
		return ret;
	}

	/* Query IMEI. */
	ret = modem_info_string_get(MODEM_INFO_IMEI, imei_buf, sizeof(imei_buf));
	if (ret < 0) {
		LOG_ERR("Unable to get IMEI");
		return ret;
	}

	/* Use IMEI as unique endpoint name. */
	snprintk(endpoint_name, sizeof(endpoint_name), "%s%s", "urn:imei:", imei_buf);

	lwm2m_init_security(&client, endpoint_name, NULL);
	slm_lwm2m_init_device(imei_buf);

#if defined(CONFIG_LWM2M_CLIENT_UTILS_BINARYAPPDATA_OBJ_SUPPORT)
	lwm2m_binaryappdata_set_recv_cb(slm_lwm2m_received_data);
#endif /* CONFIG_LWM2M_CLIENT_UTILS_BINARYAPPDATA_OBJ_SUPPORT */

	if (sizeof(CONFIG_SLM_LWM2M_PSK) > 1) {
		/* Write hard-coded PSK key to the engine. First security instance is the right
		 * one, because in bootstrap mode, it is the bootstrap PSK. In normal mode, it is
		 * the server key.
		 */
		lwm2m_security_set_psk(0, CONFIG_SLM_LWM2M_PSK,
				       sizeof(CONFIG_SLM_LWM2M_PSK), true,
				       endpoint_name);
	}

	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT)) {
		lwm2m_init_firmware_cb(slm_lwm2m_firmware_event_cb);

		ret = lwm2m_init_image();
		if (ret < 0) {
			LOG_ERR("Failed to setup image properties (%d)", ret);
			return 0;
		}
	}

	/* Disable unnecessary time updates. */
	lwm2m_update_device_service_period(0);

	return 0;
}

int slm_at_lwm2m_uninit(void)
{
	connected = false;
	auto_connected = false;
	no_serv_suspended = false;

	slm_lwm2m_rd_client_stop();

	return 0;
}

static void slm_lwm2m_rd_client_start_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	uint32_t flags = 0;

	if (lwm2m_security_needs_bootstrap()) {
		flags |= LWM2M_RD_CLIENT_FLAG_BOOTSTRAP;
	}

	LOG_INF("Starting LwM2M client");
	lwm2m_rd_client_start(&client, endpoint_name, flags, slm_lwm2m_rd_client_event_cb, NULL);
	operation = LWM2M_OP_START;
}

static K_WORK_DEFINE(slm_lwm2m_rd_client_start_work, slm_lwm2m_rd_client_start_work_fn);

static void slm_lwm2m_rd_client_start(void)
{
	k_work_submit(&slm_lwm2m_rd_client_start_work);
}

static void slm_lwm2m_rd_client_stop_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("Stopping LwM2M client");
	lwm2m_rd_client_stop(&client, slm_lwm2m_rd_client_event_cb, false);
	operation = LWM2M_OP_STOP;
}

static K_WORK_DEFINE(slm_lwm2m_rd_client_stop_work, slm_lwm2m_rd_client_stop_work_fn);

static void slm_lwm2m_rd_client_stop(void)
{
	k_work_submit(&slm_lwm2m_rd_client_stop_work);
}

SLM_AT_CMD_CUSTOM(xlwm2m_op, "AT#XLWM2MOP", do_lwm2m_op);
static int do_lwm2m_op(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
		       uint32_t param_count)
{
	int op = 0;
	int err = 0;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (param_count != 2) {
			return -EINVAL;
		}

		err = at_parser_num_get(parser, 1, &op);
		if (err) {
			return err;
		}

		switch (op) {
		case LWM2M_OP_STOP:
			if (operation == LWM2M_OP_START || operation == LWM2M_OP_PAUSE) {
				LOG_INF("Stopping LwM2M client");
				err = lwm2m_rd_client_stop(&client, slm_lwm2m_rd_client_event_cb,
							   true);
				operation = LWM2M_OP_STOP;
			}
			break;

		case LWM2M_OP_START:
			if (operation == LWM2M_OP_STOP) {
				slm_lwm2m_rd_client_start();
			} else if (operation == LWM2M_OP_PAUSE) {
				err = lwm2m_engine_resume();
				operation = LWM2M_OP_START;
			}
			break;

		case LWM2M_OP_PAUSE:
			if (operation == LWM2M_OP_START) {
				err = lwm2m_engine_pause();
				operation = LWM2M_OP_PAUSE;
			} else if (operation == LWM2M_OP_STOP) {
				err = -EINVAL;
			}
			break;

		case LWM2M_OP_UPDATE:
			if (operation == LWM2M_OP_START) {
				lwm2m_rd_client_update();
			} else {
				err = -EINVAL;
			}
			break;

		default:
			err = -EINVAL;
			break;
		}
		break;

	case AT_PARSER_CMD_TYPE_READ:
		rsp_send("\r\n#XLWM2MOP: %d\r\n", operation);
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XLWM2MOP: (0,1,2,3)\r\n");
		break;

	default:
		break;
	}

	return err;
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_BINARYAPPDATA_OBJ_SUPPORT)
SLM_AT_CMD_CUSTOM(xlwm2mdata, "AT#XLWM2MDATA", handle_at_lwm2m_data);
static int handle_at_lwm2m_data(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
				uint32_t param_count)
{
	uint16_t obj_inst_id = INST_UNSET;
	uint16_t res_inst_id = INST_UNSET;
	int err = 0;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (param_count > 1) {
			err = at_parser_num_get(parser, 1, &obj_inst_id);
			if (err) {
				return err;
			}
		}

		if (param_count > 2) {
			err = at_parser_num_get(parser, 2, &res_inst_id);
			if (err) {
				return err;
			}
		}

		if (param_count < 4) {
			/* Read resource instance data */
			err = slm_lwm2m_binaryappdata_read(obj_inst_id, obj_inst_id,
							   res_inst_id, res_inst_id);
		} else if (param_count == 4) {
			/* Write resource instance data */
			const char *hex_data;
			uint32_t hex_data_len;

			err = at_parser_string_ptr_get(parser, 3, &hex_data, &hex_data_len);
			if (err) {
				return err;
			}

			uint8_t data[hex_data_len / 2];
			size_t data_len;

			data_len = hex2bin(hex_data, hex_data_len, data, sizeof(data));
			if (hex_data_len > 0 && data_len == 0) {
				return -EINVAL;
			}
			err = lwm2m_binaryappdata_write(obj_inst_id, res_inst_id, data, data_len);
		} else {
			err = -EINVAL;
		}
		break;

	case AT_PARSER_CMD_TYPE_READ:
		err = slm_lwm2m_binaryappdata_read(0, INST_UNSET, 0, INST_UNSET);
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XLWM2MDATA: (0,1),(0,1),\"<hex_data>\"\r\n");
		break;

	default:
		break;
	}

	return err;
}
#endif /* CONFIG_LWM2M_CLIENT_UTILS_BINARYAPPDATA_OBJ_SUPPORT */

SLM_AT_CMD_CUSTOM(xlwm2mevt, "AT#XLWM2MEVT", handle_at_lwm2m_event);
static int handle_at_lwm2m_event(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
				 uint32_t param_count)
{
	int event_level;
	int err = 0;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		/* Set LwM2M event reporting level. */
		err = at_parser_num_get(parser, 1, &event_level);
		if (err || (event_level < LWM2M_EVENT_NONE) || (event_level >= LWM2M_EVENT_LAST)) {
			err = -EINVAL;
		} else {
			lwm2m_event_level = event_level;
		}
		break;

	case AT_PARSER_CMD_TYPE_READ:
		rsp_send("\r\n#XLWM2MEVT: %d\r\n", lwm2m_event_level);
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XLWM2MEVT: (0,1,2,3)\r\n");
		break;

	default:
		break;
	}

	return err;
}
