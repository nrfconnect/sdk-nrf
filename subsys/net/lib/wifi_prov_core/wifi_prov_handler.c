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
#include <zephyr/net/wifi_certs.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>

#include <zephyr/net/wifi_credentials.h>

#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include <net/wifi_prov_core/wifi_prov_core.h>

/* Weak transport functions - can be overridden by transport layer */
__weak int wifi_prov_send_rsp(struct net_buf_simple *rsp)
{
	return -ENOTSUP;
}

__weak int wifi_prov_send_result(struct net_buf_simple *result)
{
	return -ENOTSUP;
}

LOG_MODULE_REGISTER(wifi_prov, CONFIG_WIFI_PROV_CORE_LOG_LEVEL);

#define WIFI_PROV_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT | \
				NET_EVENT_WIFI_CONNECT_RESULT)

static struct net_mgmt_event_callback wifi_prov_mgmt_cb;

/* This is the slot in ram to hold valid configuration */
static struct wifi_credentials_personal config_in_ram;
/* This is to hold temporary config  */
static struct wifi_credentials_personal config_buf;
static bool save_config_to_ram;

enum wifi_enterprise_cert_sec_tags {
	WIFI_CERT_CA_SEC_TAG = 0x1020001,
	WIFI_CERT_CLIENT_KEY_SEC_TAG,
	WIFI_CERT_SERVER_KEY_SEC_TAG,
	WIFI_CERT_CLIENT_SEC_TAG,
	WIFI_CERT_SERVER_SEC_TAG,
	/* Phase 2 */
	WIFI_CERT_CA_P2_SEC_TAG,
	WIFI_CERT_CLIENT_KEY_P2_SEC_TAG,
	WIFI_CERT_CLIENT_P2_SEC_TAG,
};

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

static void get_prov_info(void *cb_arg, const char *ssid, size_t ssid_len)
{
	int rc;

	rc = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len,
		(struct wifi_credentials_personal *)cb_arg);
	LOG_DBG("GET_STATUS: loading credentials returns %d", rc);
}

static void prov_get_status_handler(Request *req, Response *rsp)
{
	int rc;
	struct wifi_credentials_personal config = { 0 };
	struct net_if *iface = net_if_get_default();
	struct wifi_iface_status status = { 0 };
	struct net_if_ipv4 *ipv4;

	LOG_INF("GET_STATUS received...");

	rsp->has_device_status = true;

	wifi_credentials_for_each_ssid(get_prov_info, &config);
	if (config.header.ssid_len == 0) { /* No entry in wifi cred lib */
		if (config_in_ram.header.ssid_len == 0) { /* No entry in ram */
			rsp->device_status.has_provisioning_info = false;
			/* If it is not provisioned, it should not connect any WiFi. */
			/* So we mark the operation successful and return directly. */
			rsp->has_status = true;
			rsp->status = Status_SUCCESS;
			return;
		}
		memcpy(&config, &config_in_ram, sizeof(config));
	}
	/* First, prepare provisioning data. */

	/* If the device is provisioned, retrieve data from flash and write into response. */
	rsp->device_status.has_provisioning_info = true;

	memcpy(rsp->device_status.provisioning_info.ssid.bytes, config.header.ssid,
							config.header.ssid_len);
	rsp->device_status.provisioning_info.ssid.size = config.header.ssid_len;
	memcpy(rsp->device_status.provisioning_info.bssid.bytes, config.header.bssid,
							WIFI_MAC_ADDR_LEN);
	rsp->device_status.provisioning_info.bssid.size = WIFI_MAC_ADDR_LEN;
	rsp->device_status.provisioning_info.channel = WIFI_CHANNEL_ANY;
	rsp->device_status.provisioning_info.has_auth = true;
	rsp->device_status.provisioning_info.auth = config.header.type;

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
			ipv4->unicast[0].ipv4.address.in_addr.s4_addr, 4);
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
	struct wifi_credentials_personal config = { 0 };
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
	memcpy(config.header.ssid, req->config.wifi.ssid.bytes, req->config.wifi.ssid.size);
	config.header.ssid_len = req->config.wifi.ssid.size;
	/* BSSID */
	if (req->config.wifi.bssid.size != WIFI_MAC_ADDR_LEN) {
		LOG_WRN("Expected BSSID len of 6, Actual len %d.", req->config.wifi.bssid.size);
	} else {
		memcpy(config.header.bssid, req->config.wifi.bssid.bytes,
			req->config.wifi.bssid.size);
		config.header.flags |= WIFI_CREDENTIALS_FLAG_BSSID;
	}
	/* Password and security */
	if (req->config.has_passphrase == true) {
		memcpy(config.password, req->config.passphrase.bytes, req->config.passphrase.size);
		config.password_len = req->config.passphrase.size;
		/* If passphrase is present, map authentication method if it exists. */
		if (req->config.wifi.has_auth == true) {
			if (req->config.wifi.auth == AuthMode_OPEN) {
				config.header.type = WIFI_SECURITY_TYPE_NONE;
			} else if (req->config.wifi.auth == AuthMode_WPA_WPA2_PSK) {
				config.header.type = WIFI_SECURITY_TYPE_PSK;
			} else if (req->config.wifi.auth == AuthMode_WPA2_PSK) {
				config.header.type = WIFI_SECURITY_TYPE_PSK_SHA256;
			} else if (req->config.wifi.auth == AuthMode_WPA3_PSK) {
				config.header.type = WIFI_SECURITY_TYPE_SAE;
			}
		}
	} else {
#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE)
		if (req->config.wifi.auth == AuthMode_WPA2_ENTERPRISE) {
			int ret;

#define WIFI_PROV_CHECK_TLS_ADD(_ret) \
	do { \
		if ((_ret) < 0) { \
			LOG_ERR("Failed to add credential: %d", (_ret)); \
			rsp->has_status = true; \
			rsp->status = Status_INTERNAL_ERROR; \
			return; \
		} \
	} while (0)

			config.header.type = WIFI_SECURITY_TYPE_EAP_TLS;
			if (req->config.has_certs == true) {
				/* Phase 1 credentials */
				if (req->config.certs.has_ca_cert) {
					ret = tls_credential_add(WIFI_CERT_CA_SEC_TAG,
						TLS_CREDENTIAL_CA_CERTIFICATE,
						req->config.certs.ca_cert.bytes,
						req->config.certs.ca_cert.size);
					WIFI_PROV_CHECK_TLS_ADD(ret);
				}
				if (req->config.certs.has_client_cert) {
					ret = tls_credential_add(WIFI_CERT_CLIENT_SEC_TAG,
						TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
						req->config.certs.client_cert.bytes,
						req->config.certs.client_cert.size);
					WIFI_PROV_CHECK_TLS_ADD(ret);
				}
				if (req->config.certs.has_private_key) {
					ret = tls_credential_add(WIFI_CERT_CLIENT_KEY_SEC_TAG,
						TLS_CREDENTIAL_PRIVATE_KEY,
						req->config.certs.private_key.bytes,
						req->config.certs.private_key.size);
					WIFI_PROV_CHECK_TLS_ADD(ret);
				}
				/* Phase 2 credentials (use *_P2_* sec tags, if present) */
				if (req->config.certs.has_ca_cert2) {
					ret = tls_credential_add(WIFI_CERT_CA_P2_SEC_TAG,
						TLS_CREDENTIAL_CA_CERTIFICATE,
						req->config.certs.ca_cert2.bytes,
						req->config.certs.ca_cert2.size);
					WIFI_PROV_CHECK_TLS_ADD(ret);
				}
				if (req->config.certs.has_client_cert2) {
					ret = tls_credential_add(WIFI_CERT_CLIENT_P2_SEC_TAG,
						TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
						req->config.certs.client_cert2.bytes,
						req->config.certs.client_cert2.size);
					WIFI_PROV_CHECK_TLS_ADD(ret);
				}
				if (req->config.certs.has_private_key2) {
					ret = tls_credential_add(WIFI_CERT_CLIENT_KEY_P2_SEC_TAG,
						TLS_CREDENTIAL_PRIVATE_KEY,
						req->config.certs.private_key2.bytes,
						req->config.certs.private_key2.size);
					WIFI_PROV_CHECK_TLS_ADD(ret);
				}
			}
#undef WIFI_PROV_CHECK_TLS_ADD
		} else
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */
		{
			/* If no passphrase provided, ignore the auth field and regard it as open */
			config.header.type = WIFI_SECURITY_TYPE_NONE;
		}
	}
	/* Band */
	if (req->config.wifi.has_band == true) {
		if (req->config.wifi.band == Band_BAND_2_4_GH) {
			config.header.flags |= WIFI_CREDENTIALS_FLAG_2_4GHz;
		} else if (req->config.wifi.band == Band_BAND_5_GH) {
			config.header.flags |= WIFI_CREDENTIALS_FLAG_5GHz;
		} else {
			/* Use 2.4G by default. */
			config.header.flags |= WIFI_CREDENTIALS_FLAG_2_4GHz;
		}
	} else {
		config.header.flags |= WIFI_CREDENTIALS_FLAG_2_4GHz;
	}
	/* If the value is not present, assume it is false, and the credential
	 * will be stored in both ram and flash.
	 */
	memcpy(&config_buf, &config, sizeof(struct wifi_credentials_personal));
	if (!req->config.has_volatileMemory || !req->config.volatileMemory) {
		save_config_to_ram = false;
	} else {
		save_config_to_ram = true;
	}

	/* Step 2: Convert local data type to wifi subsystem data type */

	net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	cnx_params.ssid = config.header.ssid;
	cnx_params.ssid_length = config.header.ssid_len;
	cnx_params.security = config.header.type;
	cnx_params.psk = NULL;
	cnx_params.psk_length = 0;
	cnx_params.sae_password = NULL;
	cnx_params.sae_password_length = 0;
#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE)
	if (config.header.type == WIFI_SECURITY_TYPE_EAP_TLS) {
		/* If EAP TLS is used, we need to set the private key password. */
		const EnterpriseCertConfig *config_ptr = &req->config.certs;

		if (config_ptr->has_private_key_passwd) {
			cnx_params.key_passwd = config_ptr->private_key_passwd;
			cnx_params.key_passwd_length = sizeof(config_ptr->private_key_passwd);
		} else {
			cnx_params.key_passwd = NULL;
			cnx_params.key_passwd_length = 0;
		}
		if (config_ptr->has_private_key_passwd2) {
			cnx_params.key2_passwd = config_ptr->private_key_passwd2;
			cnx_params.key2_passwd_length = sizeof(config_ptr->private_key_passwd2);
		} else {
			cnx_params.key2_passwd = NULL;
			cnx_params.key2_passwd_length = 0;
		}
		if (wifi_set_enterprise_credentials(iface, 0) != 0) {
			LOG_ERR("Failed to set enterprise credentials");
		}
	} else if (config.header.type != WIFI_SECURITY_TYPE_NONE) {
#else /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */
	if (config.header.type != WIFI_SECURITY_TYPE_NONE) {
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */
		cnx_params.psk = config.password;
		cnx_params.psk_length = config.password_len;
	}

	cnx_params.channel = WIFI_CHANNEL_ANY;
	cnx_params.band = config.header.flags & WIFI_CREDENTIALS_FLAG_5GHz ?
			WIFI_FREQ_BAND_5_GHZ : WIFI_FREQ_BAND_2_4_GHZ;
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
static void clear_all_credential(void *cb_arg, const char *ssid, size_t ssid_len)
{
	wifi_credentials_delete_by_ssid(ssid, ssid_len);
}

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

	wifi_credentials_for_each_ssid(clear_all_credential, NULL);
	memset(&config_in_ram, 0, sizeof(config_in_ram));

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
		if (config_buf.header.ssid_len) {
			if (!save_config_to_ram) {
				wifi_credentials_set_personal_struct(&config_buf);
			} else {
				memcpy(&config_in_ram, &config_buf, sizeof(config_buf));
			}
		}
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint64_t mgmt_event, struct net_if *iface)
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

bool wifi_prov_state_get(void)
{
	struct wifi_credentials_personal config = { 0 };

	wifi_credentials_for_each_ssid(get_prov_info, &config);
	if (config.header.ssid_len == 0 && config_in_ram.header.ssid_len == 0) {
		return false;
	} else {
		return true;
	}
}

int wifi_prov_init(void)
{
	net_mgmt_init_event_callback(&wifi_prov_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_PROV_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_prov_mgmt_cb);

	return 0;
}
