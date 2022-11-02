/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>

#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include "wifi_provisioning.h"
#include "wifi_prov_internal.h"

LOG_MODULE_REGISTER(wifi_prov, CONFIG_WIFI_PROVISIONING_LOG_LEVEL);

#define WIFI_PROV_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT | \
				NET_EVENT_WIFI_CONNECT_RESULT)

static struct net_mgmt_event_callback wifi_prov_mgmt_cb;

/* Configurator should read info before any other operations.
 * We should define some useful fields to facilicate bootstrapping.
 */

int wifi_prov_get_info(struct net_buf_simple *info)
{
	int rc;
	Info prov_info = Info_init_zero;
	pb_ostream_t ostream;

	prov_info.version = PROV_SVC_VER;
	ostream = pb_ostream_from_buffer(info->data, info->size);

	rc = pb_encode(&ostream, Info_fields, &prov_info);
	if (rc == false) {
		return -EIO;
	}

	info->len = ostream.bytes_written;
	return 0;
}

static void prov_get_status_handler(Request *req, Response *rsp)
{
	int rc;
	struct wifi_config config;
	struct net_if *iface = net_if_get_default();
	struct wifi_iface_status status = { 0 };
	struct net_if_ipv4 *ipv4;

	LOG_INF("GET_STATUS received...");

	rsp->has_device_status = true;

	/* First, prepare provisioning data. */
	if (!wifi_has_config()) {
		rsp->device_status.has_provisioning_info = false;
		/* If it is not provisioned, it should not connect any WiFi. */
		/* So we mark the operation successful and return directly. */
		rsp->has_status = true;
		rsp->status = Status_SUCCESS;
		return;

	}
	/* If the device is provisioned, retrieve data from flash and write into response. */
	rsp->device_status.has_provisioning_info = true;

	wifi_get_config(&config);

	memcpy(rsp->device_status.provisioning_info.ssid.bytes, config.ssid, config.ssid_len);
	rsp->device_status.provisioning_info.ssid.size = config.ssid_len;
	memcpy(rsp->device_status.provisioning_info.bssid.bytes, config.bssid, 6);
	rsp->device_status.provisioning_info.bssid.size = 6;
	rsp->device_status.provisioning_info.channel = config.channel;
	rsp->device_status.provisioning_info.has_auth = true;
	rsp->device_status.provisioning_info.auth = config.auth_type;

	/* Second, prepare connection state */
	rc = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
				sizeof(struct wifi_iface_status));
	if (rc != 0) {
		LOG_WRN("Cannot get wifi status.");
		rsp->has_status = true;
		rsp->status = Status_INTERNAL_ERROR;
		return;
	}
	rsp->device_status.has_state = true;
	if (status.state >= WIFI_STATE_ASSOCIATED) {
		rsp->device_status.state = ConnectionState_CONNECTED;
		/* If connected, retrieve IP address */
		if (net_if_config_ipv4_get(iface, &ipv4) < 0) {
			LOG_WRN("Cannot get IP address.");
			rsp->has_status = true;
			rsp->status = Status_INTERNAL_ERROR;
			return;
		}
		rsp->device_status.has_connection_info = true;
		rsp->device_status.connection_info.has_ip4_addr = true;
		memcpy(rsp->device_status.connection_info.ip4_addr.bytes,
			ipv4->unicast[0].address.in_addr.s4_addr, 4);
		rsp->device_status.connection_info.ip4_addr.size = 4;
	} else {
		rsp->device_status.state = ConnectionState_DISCONNECTED;
	}

	rsp->has_status = true;
	rsp->status = Status_SUCCESS;
}

/* Handler for START_SCAN command.
 * Input: None (no parameter required)
 * Output: None
 * Return: operation result
 */
static void prov_start_scan_handler(Request *req, Response *rsp)
{
	int rc;
	struct net_if *iface = net_if_get_default();

	LOG_INF("Start_Scan received...");

	/* Explicitly trigger disconnection request,
	 * and this is supposed to remove any config.
	 */
	net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	rc = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);

	/* Invalid argument. */
	if (rc == -EINVAL) {
		rsp->has_status = true;
		rsp->status = Status_INVALID_ARGUMENT;
		return;
	}
	/* Other error. */
	if (rc != 0) {
		rsp->has_status = true;
		rsp->status = Status_INTERNAL_ERROR;
		return;
	}
	/* No error. */
	rsp->has_status = true;
	rsp->status = Status_SUCCESS;
}

/* Handler for STOP_SCAN command
 * Input: None
 * Output: None
 * Return: operation result
 */
static void prov_stop_scan_handler(Request *req, Response *rsp)
{
	LOG_INF("Stop_Scan received...");

	/* No STOP SCAN API yet. Simply assign SUCCESS result. */
	rsp->has_status = true;
	rsp->status = Status_SUCCESS;
}

/* Handler for SET_CONFIG command.
 * Input: struct wifi_connect_req_params
 * Output: None
 * return: operation result
 */
static void prov_set_config_handler(Request *req, Response *rsp)
{
	int rc;
	struct wifi_config config;
	struct net_if *iface = net_if_get_default();
	struct wifi_connect_req_params cnx_params = { 0 };

	LOG_INF("Set_config received...");

	if (req->has_config == false) {
		rsp->has_status = true;
		rsp->status = Status_INVALID_ARGUMENT;
		return;
	}
	if (req->config.has_wifi == false) {
		rsp->has_status = true;
		rsp->status = Status_INVALID_ARGUMENT;
		return;
	}


	/* Step 1: convert protobuf data type to local data type */

	/* SSID */
	memcpy(config.ssid, req->config.wifi.ssid.bytes, req->config.wifi.ssid.size);
	config.ssid_len = req->config.wifi.ssid.size;
	/* BSSID */
	if (req->config.wifi.bssid.size != 6) {
		LOG_WRN("Expected BSSID len of 6, Actual len %d.", req->config.wifi.bssid.size);
	} else {
		memcpy(config.bssid, req->config.wifi.bssid.bytes, req->config.wifi.bssid.size);
	}
	/* Channel */
	config.channel = req->config.wifi.channel;
	/* Password and security */
	if (req->config.has_passphrase == true) {
		memcpy(config.password, req->config.passphrase.bytes, req->config.passphrase.size);
		config.password_len = req->config.passphrase.size;
		/* If passphrase is present, map authentication method if it exists. */
		if (req->config.wifi.has_auth == true) {
			if (req->config.wifi.auth == AuthMode_OPEN) {
				config.auth_type = WIFI_SECURITY_TYPE_NONE;
			} else if (req->config.wifi.auth == AuthMode_WPA_WPA2_PSK) {
				config.auth_type = WIFI_SECURITY_TYPE_PSK;
			} else if (req->config.wifi.auth == AuthMode_WPA2_PSK) {
				config.auth_type = WIFI_SECURITY_TYPE_PSK_SHA256;
			} else if (req->config.wifi.auth == AuthMode_WPA3_PSK) {
				config.auth_type = WIFI_SECURITY_TYPE_SAE;
			} else {
				config.auth_type = WIFI_SECURITY_TYPE_UNKNOWN;
			}
		}
	} else {
		/* If no passphrase provided, ignore the auth field and regard it as open */
		config.auth_type = WIFI_SECURITY_TYPE_NONE;
	}
	/* Band */
	if (req->config.wifi.has_band == true) {
		if (req->config.wifi.band == Band_BAND_2_4_GH) {
			config.band = WIFI_FREQ_BAND_2_4_GHZ;
		} else if (req->config.wifi.band == Band_BAND_5_GH) {
			config.band = WIFI_FREQ_BAND_5_GHZ;
		} else {
			config.band = WIFI_FREQ_BAND_2_4_GHZ; /* Use 2.4G by default. */
		}
	} else {
		config.band = WIFI_FREQ_BAND_2_4_GHZ;
	}
	/* If the value is not present, assume it is false, and the credential
	 * will be stored in both ram and flash.
	 */
	wifi_set_config(&config, req->config.has_volatileMemory == true ?
				req->config.volatileMemory : false);



	/* Step 2: Convert local data type to wifi subsystem data type */

	net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	cnx_params.ssid = config.ssid;
	cnx_params.ssid_length = config.ssid_len;
	cnx_params.security = config.auth_type;
	cnx_params.psk = NULL;
	cnx_params.psk_length = 0;
	cnx_params.sae_password = NULL;
	cnx_params.sae_password_length = 0;
	if (config.auth_type != WIFI_SECURITY_TYPE_NONE) {
		cnx_params.psk = config.password;
		cnx_params.psk_length = config.password_len;
	}

	cnx_params.channel = config.channel;
	cnx_params.band = config.band;
	cnx_params.mfp = WIFI_MFP_OPTIONAL;

	rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
		     &cnx_params, sizeof(struct wifi_connect_req_params));
	/* Invalid argument error. */
	if (rc == -EINVAL) {
		rsp->has_status = true;
		rsp->status = Status_INVALID_ARGUMENT;
		return;
	}
	/* Other error. */
	if (rc != 0) {
		rsp->has_status = true;
		rsp->status = Status_INTERNAL_ERROR;
		return;
	}

	/* Everything is handled properly. */
	rsp->has_status = true;
	rsp->status = Status_SUCCESS;
}

/* Handler for FORGET_CONFIG command.
 * Input: None
 * Output: None
 * Return: operation result
 */
static void prov_forget_config_handler(Request *req, Response *rsp)
{
	int rc;
	struct net_if *iface = net_if_get_default();

	LOG_INF("Forget config received...");

	rc = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
	if (rc != 0) {
		rsp->has_status = true;
		rsp->status = Status_INTERNAL_ERROR;
		return;
	}

	rc = wifi_remove_config();
	if (rc != 0) {
		rsp->has_status = true;
		rsp->status = Status_INTERNAL_ERROR;
		return;
	}

	rsp->has_status = true;
	rsp->status = Status_SUCCESS;
}

void prov_request_handler(Request *req, Response *rsp)
{
	if ((!req->has_op_code) ||
		!(req->op_code == OpCode_GET_STATUS
		|| req->op_code == OpCode_START_SCAN
		|| req->op_code == OpCode_STOP_SCAN
		|| req->op_code == OpCode_SET_CONFIG
		|| req->op_code == OpCode_FORGET_CONFIG)) {
		LOG_INF("Invalid Opcode.");
		rsp->has_status = true;
		rsp->status = Status_INVALID_ARGUMENT;

		return;
	}

	/* Valid opcode. Pass to corrsponding handler. */
	/* The error can only occur in wifi subsys,
	 * and it is reflected in the response message.
	 * So the handler don't need to return any error code.
	 */
	rsp->has_request_op_code = true;
	rsp->request_op_code = req->op_code;
	LOG_INF("Start parsing...");
	if (req->op_code == OpCode_GET_STATUS) {
		prov_get_status_handler(req, rsp);
		return;
	}
	if (req->op_code == OpCode_START_SCAN) {
		prov_start_scan_handler(req, rsp);
		return;
	}
	if (req->op_code == OpCode_STOP_SCAN) {
		prov_stop_scan_handler(req, rsp);
		return;
	}
	if (req->op_code == OpCode_SET_CONFIG) {
		prov_set_config_handler(req, rsp);
		return;
	}
	if (req->op_code == OpCode_FORGET_CONFIG) {
		prov_forget_config_handler(req, rsp);
		return;
	}

	/* Unsupported opcode */
	LOG_INF("Unsupported opcode.");
	rsp->has_request_op_code = true;
	rsp->request_op_code = req->op_code;
	rsp->has_status = true;
	rsp->status = Status_INVALID_ARGUMENT;
}

/* Currently this is the only entry pointer of command of provisioning serice.
 * The command is dispatched and corresponding handler is called.
 */
int wifi_prov_recv_req(struct net_buf_simple *req_stream)
{
	int rc;
	Request req = Request_init_zero;
	Response rsp = Response_init_zero;
	pb_istream_t istream;
	pb_ostream_t ostream;

	NET_BUF_SIMPLE_DEFINE(rsp_stream, RESPONSE_MSG_MAX_LENGTH);

	LOG_HEXDUMP_DBG(req_stream->data, req_stream->len, "Control point rx: ");

	/* Parse the message and dispatch to the handler */
	istream = pb_istream_from_buffer(req_stream->data, req_stream->len);

	rc = pb_decode(&istream, Request_fields, &req);

	if (rc == false) {
		LOG_ERR("Decoding of request message failed. Msg: %s.", istream.errmsg);
		rsp.has_status = true;
		rsp.status = Status_INVALID_PROTO;
	} else {
		prov_request_handler(&req, &rsp);
	}

	ostream = pb_ostream_from_buffer(rsp_stream.data, rsp_stream.size);

	rc = pb_encode(&ostream, Response_fields, &rsp);
	if (rc == false) {
		/* Impossible to reply with protocol buffers. Throw error to BLE. */
		return -EIO;
	}
	rsp_stream.len = ostream.bytes_written;
	wifi_prov_send_rsp(&rsp_stream);

	return 0;
}

/* Following part defines and registers net_mgmt event callback.
 * These callbacks are forked in net_mgmt context, so we need
 * to assign larger stack size to net_mgmt thread (e.g. 2048).
 */
/* Callback for scan result. */
static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	int rc;
	const struct wifi_scan_result *entry =
		(const struct wifi_scan_result *)cb->info;
	Result scan_result = Result_init_zero;
	pb_ostream_t ostream;

	NET_BUF_SIMPLE_DEFINE(scan_result_stream, RESULT_MSG_MAX_LENGTH);

	scan_result.has_scan_record = true;
	scan_result.scan_record.has_wifi = true;
	memcpy(scan_result.scan_record.wifi.ssid.bytes, entry->ssid, entry->ssid_length);
	scan_result.scan_record.wifi.ssid.size = entry->ssid_length;
	memcpy(scan_result.scan_record.wifi.bssid.bytes, entry->mac,
		entry->mac_length > 6 ? 6 : entry->mac_length);
	scan_result.scan_record.wifi.bssid.size =
		entry->mac_length > 6 ? 6 : entry->mac_length;
	scan_result.scan_record.wifi.has_auth = true;
	if (entry->security == WIFI_SECURITY_TYPE_NONE) {
		scan_result.scan_record.wifi.auth = AuthMode_OPEN;
	} else if (entry->security == WIFI_SECURITY_TYPE_PSK) {
		scan_result.scan_record.wifi.auth = AuthMode_WPA_WPA2_PSK;
	} else if (entry->security == WIFI_SECURITY_TYPE_PSK_SHA256) {
		scan_result.scan_record.wifi.auth = AuthMode_WPA2_PSK;
	} else if (entry->security == WIFI_SECURITY_TYPE_SAE) {
		scan_result.scan_record.wifi.auth = AuthMode_WPA3_PSK;
	} else {
		/* use AuthMode_WPA_WPA2_PSK by default */
		scan_result.scan_record.wifi.auth = AuthMode_WPA_WPA2_PSK;
	}
	scan_result.scan_record.wifi.channel = entry->channel;
	scan_result.scan_record.wifi.has_band = true;
	if (entry->band == WIFI_FREQ_BAND_2_4_GHZ) {
		scan_result.scan_record.wifi.band = Band_BAND_2_4_GH;
	} else if (entry->band == WIFI_FREQ_BAND_5_GHZ) {
		scan_result.scan_record.wifi.band = Band_BAND_5_GH;
	} else {
		scan_result.scan_record.wifi.band = Band_BAND_ANY;
	}

	scan_result.scan_record.has_rssi = true;
	scan_result.scan_record.rssi = entry->rssi;

	scan_result.has_state = false;

	ostream = pb_ostream_from_buffer(scan_result_stream.data,
							scan_result_stream.size);

	rc = pb_encode(&ostream, Result_fields, &scan_result);
	scan_result_stream.len = ostream.bytes_written;
	if (!rc) {
		LOG_ERR("Encoding failed: %s.", PB_GET_ERROR(&ostream));
		return;
	}

	wifi_prov_send_result(&scan_result_stream);
}

/* Callback for connect result. */
static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	int rc;
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;
	Result state_update = Result_init_zero;
	pb_ostream_t ostream;

	NET_BUF_SIMPLE_DEFINE(state_stream, RESULT_MSG_MAX_LENGTH);
	ConnectionState success_state_sequence[] = {ConnectionState_AUTHENTICATION,
						ConnectionState_ASSOCIATION,
						ConnectionState_OBTAINING_IP,
						ConnectionState_CONNECTED};

	if (status->status != 0) {
		state_update.has_scan_record = false;
		state_update.has_state = true;
		state_update.state = ConnectionState_CONNECTION_FAILED;
		ostream = pb_ostream_from_buffer(state_stream.data, state_stream.size);

		rc = pb_encode(&ostream, Result_fields, &state_update);

		if (!rc) {
			LOG_ERR("Encoding failed: %s.", PB_GET_ERROR(&ostream));
			return;
		}
		state_stream.len = ostream.bytes_written;
		wifi_prov_send_result(&state_stream);
	} else {
		for (int i = 0; i < ARRAY_SIZE(success_state_sequence); i++) {
			/* Reset it before reuse of net_buf_simple. */
			net_buf_simple_reset(&state_stream);
			state_update.has_scan_record = false;
			state_update.has_state = true;
			state_update.state = success_state_sequence[i];
			ostream = pb_ostream_from_buffer(state_stream.data, state_stream.size);

			rc = pb_encode(&ostream, Result_fields, &state_update);
			if (!rc) {
				LOG_ERR("Encoding failed: %s.", PB_GET_ERROR(&ostream));
				return;
			}
			state_stream.len = ostream.bytes_written;
			wifi_prov_send_result(&state_stream);
		}
		wifi_commit_config();
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		handle_wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	default:
		break;
	}
}

int wifi_prov_init(void)
{
	int rc;

	rc = wifi_prov_transport_layer_init();
	if (rc != 0) {
		LOG_WRN("Initializing transport module failed, err = %d.", rc);
	}

	net_mgmt_init_event_callback(&wifi_prov_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_PROV_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_prov_mgmt_cb);

	return 0;
}
