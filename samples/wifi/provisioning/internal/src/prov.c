/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net_buf.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/base64.h>
#include <stdlib.h>

#include <net/wifi_prov_core/wifi_prov_core.h>

/* Include nanopb headers */
#include "pb_decode.h"
#include "pb_encode.h"

/* Include generated protobuf headers */
#include "request.pb.h"
#include "response.pb.h"
#include "result.pb.h"
#include "common.pb.h"

LOG_MODULE_REGISTER(prov, CONFIG_WIFI_PROV_INTERNAL_LOG_LEVEL);

/* Generated Wi-Fi configuration data */
static const uint8_t wifi_config_data[] = {
#include "wifi_config.inc"
};

static const size_t wifi_config_data_len = sizeof(wifi_config_data);

/* Static buffer for requests */
static struct net_buf_simple req_buf_static;
static struct net_buf_simple *req_buf;

/**
 * @brief Convert base64 string to binary data using Zephyr's base64_decode
 */
static int base64_decode_string(const char *base64_str, uint8_t **binary, size_t *binary_len)
{
	size_t len = strlen(base64_str);
	size_t decoded_len = 0;
	uint8_t *decoded_data;
	int ret;

	/* Check input length */
	if (len > CONFIG_WIFI_PROV_MAX_BASE64_SIZE) {
		return -EINVAL;
	}

	/* Calculate decoded size (base64 is 4 chars per 3 bytes) */
	decoded_len = (len * 3) / 4;
	if (decoded_len > CONFIG_WIFI_PROV_MAX_DATA_SIZE) {
		return -EINVAL;
	}

	/* Allocate memory for decoded data */
	decoded_data = k_malloc(decoded_len);
	if (!decoded_data) {
		return -ENOMEM;
	}

	/* Use Zephyr's base64_decode function */
	ret = base64_decode(decoded_data, decoded_len, binary_len,
			   (const uint8_t *)base64_str, len);
	if (ret < 0) {
		k_free(decoded_data);
		return ret;
	}

	*binary = decoded_data;
	return 0;
}

/**
 * @brief Decode and log Wi-Fi provisioning request
 */
static void decode_request(const uint8_t *data, size_t len)
{
	Request request = Request_init_zero;
	pb_istream_t stream;

	stream = pb_istream_from_buffer(data, len);

	bool decode_result = pb_decode(&stream, Request_fields, &request);

	if (decode_result) {
		LOG_INF("=== Wi-Fi Provisioning Request ===");
		LOG_INF("Operation: %d", request.op_code);

		switch (request.op_code) {
		case OpCode_GET_STATUS:
			LOG_INF("Type: GET_STATUS");
			break;
		case OpCode_START_SCAN:
			LOG_INF("Type: START_SCAN");
			break;
		case OpCode_STOP_SCAN:
			LOG_INF("Type: STOP_SCAN");
			break;
		case OpCode_SET_CONFIG:
			LOG_INF("Type: SET_CONFIG");
			if (request.has_config) {
				LOG_INF("  SSID: %.*s", (int)request.config.wifi.ssid.size,
					   request.config.wifi.ssid.bytes);
				LOG_INF("  BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
					request.config.wifi.bssid.bytes[0],
					request.config.wifi.bssid.bytes[1],
					request.config.wifi.bssid.bytes[2],
					request.config.wifi.bssid.bytes[3],
					request.config.wifi.bssid.bytes[4],
					request.config.wifi.bssid.bytes[5]);
				LOG_INF("  Channel: %d", request.config.wifi.channel);
				LOG_INF("  Auth mode: %d", request.config.wifi.auth);
				if (request.config.has_passphrase) {
					LOG_INF("  Passphrase length: %d",
						(int)request.config.passphrase.size);
				}
			}
			break;
		case OpCode_FORGET_CONFIG:
			LOG_INF("Type: FORGET_CONFIG");
			break;
		default:
			LOG_INF("Type: UNKNOWN (%d)", request.op_code);
			break;
		}
		LOG_INF("===============================");
	} else {
		LOG_ERR("Failed to decode request: %s", PB_GET_ERROR(&stream));
		LOG_ERR("pb_decode returned: %s", decode_result ? "true" : "false");
		LOG_ERR("Data length: %d bytes", len);
		LOG_HEXDUMP_ERR(data, len, "Raw request data:");
	}
}

/**
 * @brief Decode and display scan results in human-readable format
 */
static void decode_scan_results(const uint8_t *data, size_t len)
{
	/* Try to decode as a single ScanRecord first */
	ScanRecord scan_record = ScanRecord_init_zero;
	pb_istream_t stream;

	stream = pb_istream_from_buffer(data, len);

	bool decode_result = pb_decode(&stream, ScanRecord_fields, &scan_record);

	if (decode_result) {
		LOG_INF("=== Wi-Fi Scan Record ===");

		if (scan_record.has_wifi) {
			const WifiInfo *wifi = &scan_record.wifi;

			/* SSID */
			if (wifi->ssid.size > 0) {
				LOG_INF("SSID: %.*s", (int)wifi->ssid.size,
					wifi->ssid.bytes);
			} else {
				LOG_INF("SSID: <hidden>");
			}

			/* BSSID */
			if (wifi->bssid.size == 6) {
				LOG_INF("BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
					wifi->bssid.bytes[0],
					wifi->bssid.bytes[1],
					wifi->bssid.bytes[2],
					wifi->bssid.bytes[3],
					wifi->bssid.bytes[4],
					wifi->bssid.bytes[5]);
			}

			/* Channel */
			LOG_INF("Channel: %d", wifi->channel);

			/* Band */
			if (wifi->has_band) {
				const char *band_str = "UNKNOWN";

				switch (wifi->band) {
				case Band_BAND_ANY:
					band_str = "ANY";
					break;
				case Band_BAND_2_4_GH:
					band_str = "2.4 GHz";
					break;
				case Band_BAND_5_GH:
					band_str = "5 GHz";
					break;
				}
				LOG_INF("Band: %s", band_str);
			}

			/* Auth Mode */
			if (wifi->has_auth) {
				const char *auth_str = "UNKNOWN";

				switch (wifi->auth) {
				case AuthMode_OPEN:
					auth_str = "OPEN";
					break;
				case AuthMode_WEP:
					auth_str = "WEP";
					break;
				case AuthMode_WPA_PSK:
					auth_str = "WPA_PSK";
					break;
				case AuthMode_WPA2_PSK:
					auth_str = "WPA2_PSK";
					break;
				case AuthMode_WPA_WPA2_PSK:
					auth_str = "WPA_WPA2_PSK";
					break;
				case AuthMode_WPA2_ENTERPRISE:
					auth_str = "WPA2_ENTERPRISE";
					break;
				case AuthMode_WPA3_PSK:
					auth_str = "WPA3_PSK";
					break;
				case AuthMode_NONE:
					auth_str = "NONE";
					break;
				case AuthMode_PSK:
					auth_str = "PSK";
					break;
				case AuthMode_PSK_SHA256:
					auth_str = "PSK_SHA256";
					break;
				case AuthMode_SAE:
					auth_str = "SAE";
					break;
				default:
					auth_str = "UNKNOWN";
					break;
				}
				LOG_INF("Security: %s", auth_str);
			}
		}

		/* RSSI */
		if (scan_record.has_rssi) {
			LOG_INF("RSSI: %d dBm", scan_record.rssi);
		}

		LOG_INF("===============================");
	} else {
		/* Try to decode as a Result message */
		stream = pb_istream_from_buffer(data, len);
		Result result = Result_init_zero;

		decode_result = pb_decode(&stream, Result_fields, &result);

		if (decode_result && result.has_scan_record) {
			LOG_INF("=== Wi-Fi Scan Result ===");

			const ScanRecord *record = &result.scan_record;

			if (record->has_wifi) {
				const WifiInfo *wifi = &record->wifi;

				/* SSID */
				if (wifi->ssid.size > 0) {
					LOG_INF("SSID: %.*s", (int)wifi->ssid.size,
						wifi->ssid.bytes);
				} else {
					LOG_INF("SSID: <hidden>");
				}

				/* BSSID */
				if (wifi->bssid.size == 6) {
					LOG_INF("BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
						wifi->bssid.bytes[0],
						wifi->bssid.bytes[1],
						wifi->bssid.bytes[2],
						wifi->bssid.bytes[3],
						wifi->bssid.bytes[4],
						wifi->bssid.bytes[5]);
				}

				/* Channel */
				LOG_INF("Channel: %d", wifi->channel);

				/* Band */
				if (wifi->has_band) {
					const char *band_str = "UNKNOWN";

					switch (wifi->band) {
					case Band_BAND_ANY:
						band_str = "ANY";
						break;
					case Band_BAND_2_4_GH:
						band_str = "2.4 GHz";
						break;
					case Band_BAND_5_GH:
						band_str = "5 GHz";
						break;
					}
					LOG_INF("Band: %s", band_str);
				}

				/* Auth Mode */
				if (wifi->has_auth) {
					const char *auth_str = "UNKNOWN";

					switch (wifi->auth) {
					case AuthMode_OPEN:
						auth_str = "OPEN";
						break;
					case AuthMode_WEP:
						auth_str = "WEP";
						break;
					case AuthMode_WPA_PSK:
						auth_str = "WPA_PSK";
						break;
					case AuthMode_WPA2_PSK:
						auth_str = "WPA2_PSK";
						break;
					case AuthMode_WPA_WPA2_PSK:
						auth_str = "WPA_WPA2_PSK";
						break;
					case AuthMode_WPA2_ENTERPRISE:
						auth_str = "WPA2_ENTERPRISE";
						break;
					case AuthMode_WPA3_PSK:
						auth_str = "WPA3_PSK";
						break;
					case AuthMode_NONE:
						auth_str = "NONE";
						break;
					case AuthMode_PSK:
						auth_str = "PSK";
						break;
					case AuthMode_PSK_SHA256:
						auth_str = "PSK_SHA256";
						break;
					case AuthMode_SAE:
						auth_str = "SAE";
						break;
					default:
						auth_str = "UNKNOWN";
						break;
					}
					LOG_INF("Security: %s", auth_str);
				}
			}

			/* RSSI */
			if (record->has_rssi) {
				LOG_INF("RSSI: %d dBm", record->rssi);
			}

			/* Connection State */
			if (result.has_state) {
				const char *state_str = "UNKNOWN";

				switch (result.state) {
				case ConnectionState_DISCONNECTED:
					state_str = "DISCONNECTED";
					break;
				case ConnectionState_AUTHENTICATION:
					state_str = "AUTHENTICATION";
					break;
				case ConnectionState_ASSOCIATION:
					state_str = "ASSOCIATION";
					break;
				case ConnectionState_OBTAINING_IP:
					state_str = "OBTAINING_IP";
					break;
				case ConnectionState_CONNECTED:
					state_str = "CONNECTED";
					break;
				case ConnectionState_CONNECTION_FAILED:
					state_str = "CONNECTION_FAILED";
					break;
				}
				LOG_INF("Connection State: %s", state_str);
			}

			/* Failure Reason */
			if (result.has_reason) {
				const char *reason_str = "UNKNOWN";

				switch (result.reason) {
				case ConnectionFailureReason_AUTH_ERROR:
					reason_str = "AUTH_ERROR";
					break;
				case ConnectionFailureReason_NETWORK_NOT_FOUND:
					reason_str = "NETWORK_NOT_FOUND";
					break;
				case ConnectionFailureReason_TIMEOUT:
					reason_str = "TIMEOUT";
					break;
				case ConnectionFailureReason_FAIL_IP:
					reason_str = "FAIL_IP";
					break;
				case ConnectionFailureReason_FAIL_CONN:
					reason_str = "FAIL_CONN";
					break;
				}
				LOG_INF("Failure Reason: %s", reason_str);
			}

			LOG_INF("===============================");
		} else {
			LOG_ERR("Failed to decode scan results: %s", PB_GET_ERROR(&stream));
			LOG_ERR("pb_decode returned: %s", decode_result ? "true" : "false");
			LOG_ERR("Data length: %d bytes", len);
			LOG_HEXDUMP_ERR(data, len, "Raw scan results data:");
		}
	}
}

/**
 * @brief Wi-Fi provisioning shell command handler
 *
 * This command sends the pre-generated Wi-Fi configuration data
 * to the Wi-Fi provisioning service.
 */
static int cmd_wifi_prov(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	if (argc != 1) {
		shell_error(shell, "Usage: wifi_prov");
		return -EINVAL;
	}

	shell_info(shell, "Sending Wi-Fi configuration to provisioning service...");

	/* Allocate buffer for the request */
	req_buf = &req_buf_static;
	net_buf_simple_init_with_data(req_buf,
								 (void *)wifi_config_data,
								 wifi_config_data_len);

	/* Decode and log the request being sent */
	decode_request(req_buf->data, req_buf->len);

	/* Send the configuration to the Wi-Fi provisioning service */
	ret = wifi_prov_recv_req(req_buf);
	if (ret < 0) {
		shell_error(shell, "Failed to send Wi-Fi configuration: %d", ret);
		return ret;
	}

	shell_info(shell, "Wi-Fi configuration sent successfully");
	shell_info(shell, "Configuration size: %d bytes", wifi_config_data_len);

	req_buf = NULL;

	return 0;
}

/**
 * @brief Send raw binary data to Wi-Fi provisioning service
 */
static int cmd_wifi_prov_raw(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	uint8_t *raw_data = NULL;
	size_t raw_data_len = 0;
	static struct net_buf_simple raw_buf;

	if (argc != 2) {
		shell_error(shell, "Usage: wifi_prov_raw <protobuf_encoded_data>");
		shell_error(shell, "Example: wifi_prov_raw "
				"CARaLgoaCgpTYW1wbGVXaUZpEgYAESIzRFUYAiAAKAMSDnNhbXBsZXBhc3N3b3JkIAA=");
		return -EINVAL;
	}

	shell_info(shell, "Sending protobuf-encoded data to provisioning service...");

	/* Convert protobuf-encoded string to binary */
	ret = base64_decode_string(argv[1], &raw_data, &raw_data_len);
	if (ret < 0) {
		shell_error(shell, "Invalid protobuf-encoded string format: %d", ret);
		return ret;
	}

	shell_info(shell, "Binary data size: %d bytes", raw_data_len);

	/* Initialize buffer with the binary data */
	net_buf_simple_init_with_data(&raw_buf, raw_data, raw_data_len);

	/* Decode and log the request being sent */
	decode_request(raw_buf.data, raw_buf.len);

	/* Send the raw data to the Wi-Fi provisioning service */
	ret = wifi_prov_recv_req(&raw_buf);
	if (ret < 0) {
		shell_error(shell, "Failed to send protobuf data: %d", ret);
		k_free(raw_data);
		return ret;
	}

	shell_info(shell, "Protobuf data sent successfully");

	/* Clean up */
	k_free(raw_data);

	return 0;
}

/**
 * @brief Get Wi-Fi provisioning status
 */
static int cmd_wifi_prov_get_status(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	if (argc != 1) {
		shell_error(shell, "Usage: wifi_prov_get_status");
		return -EINVAL;
	}

	shell_info(shell, "Getting Wi-Fi provisioning status...");

	/* Create a simple GET_STATUS request */
	static uint8_t status_req_data[] = {0x08, 0x01}; /* op_code = GET_STATUS */
	static struct net_buf_simple status_req_buf;

	net_buf_simple_init_with_data(&status_req_buf, status_req_data, sizeof(status_req_data));

	/* Decode and log the request being sent */
	decode_request(status_req_buf.data, status_req_buf.len);

	ret = wifi_prov_recv_req(&status_req_buf);
	if (ret < 0) {
		shell_error(shell, "Failed to get Wi-Fi status: %d", ret);
		return ret;
	}

	shell_info(shell, "Wi-Fi status request sent successfully");
	return 0;
}

/**
 * @brief Start Wi-Fi scan
 */
static int cmd_wifi_prov_start_scan(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	if (argc != 1) {
		shell_error(shell, "Usage: wifi_prov_start_scan");
		return -EINVAL;
	}

	shell_info(shell, "Starting Wi-Fi scan...");

	/* Create a simple START_SCAN request */
	static uint8_t scan_req_data[] = {0x08, 0x02}; /* op_code = START_SCAN */
	static struct net_buf_simple scan_req_buf;

	net_buf_simple_init_with_data(&scan_req_buf, scan_req_data, sizeof(scan_req_data));

	/* Decode and log the request being sent */
	decode_request(scan_req_buf.data, scan_req_buf.len);

	ret = wifi_prov_recv_req(&scan_req_buf);
	if (ret < 0) {
		shell_error(shell, "Failed to start Wi-Fi scan: %d", ret);
		return ret;
	}

	shell_info(shell, "Wi-Fi scan started successfully");
	return 0;
}

/**
 * @brief Stop Wi-Fi scan
 */
static int cmd_wifi_prov_stop_scan(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	if (argc != 1) {
		shell_error(shell, "Usage: wifi_prov_stop_scan");
		return -EINVAL;
	}

	shell_info(shell, "Stopping Wi-Fi scan...");

	/* Create a simple STOP_SCAN request */
	static uint8_t stop_scan_req_data[] = {0x08, 0x03}; /* op_code = STOP_SCAN */
	static struct net_buf_simple stop_scan_req_buf;

	net_buf_simple_init_with_data(&stop_scan_req_buf, stop_scan_req_data,
					sizeof(stop_scan_req_data));

	/* Decode and log the request being sent */
	decode_request(stop_scan_req_buf.data, stop_scan_req_buf.len);

	ret = wifi_prov_recv_req(&stop_scan_req_buf);
	if (ret < 0) {
		shell_error(shell, "Failed to stop Wi-Fi scan: %d", ret);
		return ret;
	}

	shell_info(shell, "Wi-Fi scan stopped successfully");
	return 0;
}

/**
 * @brief Set Wi-Fi configuration
 */
static int cmd_wifi_prov_set_config(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	if (argc != 1) {
		shell_error(shell, "Usage: wifi_prov_set_config");
		return -EINVAL;
	}

	shell_info(shell, "Setting Wi-Fi configuration...");

	/* Use the pre-generated Wi-Fi configuration */
	static struct net_buf_simple config_req_buf;

	net_buf_simple_init_with_data(&config_req_buf,
								 (void *)wifi_config_data,
								 wifi_config_data_len);

	/* Decode and log the request being sent */
	decode_request(config_req_buf.data, config_req_buf.len);

	ret = wifi_prov_recv_req(&config_req_buf);
	if (ret < 0) {
		shell_error(shell, "Failed to set Wi-Fi configuration: %d", ret);
		return ret;
	}

	shell_info(shell, "Wi-Fi configuration set successfully");
	return 0;
}

/**
 * @brief Forget Wi-Fi configuration
 */
static int cmd_wifi_prov_forget_config(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	if (argc != 1) {
		shell_error(shell, "Usage: wifi_prov_forget_config");
		return -EINVAL;
	}

	shell_info(shell, "Forgetting Wi-Fi configuration...");

	/* Create a simple FORGET_CONFIG request */
	static uint8_t forget_req_data[] = {0x08, 0x05}; /* op_code = FORGET_CONFIG */
	static struct net_buf_simple forget_req_buf;

	net_buf_simple_init_with_data(&forget_req_buf, forget_req_data, sizeof(forget_req_data));

	/* Decode and log the request being sent */
	decode_request(forget_req_buf.data, forget_req_buf.len);

	ret = wifi_prov_recv_req(&forget_req_buf);
	if (ret < 0) {
		shell_error(shell, "Failed to forget Wi-Fi configuration: %d", ret);
		return ret;
	}

	shell_info(shell, "Wi-Fi configuration forgotten successfully");
	return 0;
}

/**
 * @brief Dump scan results in human-readable format
 */
static int cmd_wifi_prov_dump_scan(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;
	uint8_t *scan_data = NULL;
	size_t scan_data_len = 0;

	if (argc != 2) {
		shell_error(shell, "Usage: wifi_prov dump_scan <base64_scan_results>");
		shell_error(shell, "Example: wifi_prov dump_scan "
				"<base64_encoded_scan_results_data>");
		return -EINVAL;
	}

	shell_info(shell, "Decoding scan results...");

	/* Convert base64 string to binary */
	ret = base64_decode_string(argv[1], &scan_data, &scan_data_len);
	if (ret < 0) {
		shell_error(shell, "Invalid base64 format: %d", ret);
		return ret;
	}

	shell_info(shell, "Scan results data size: %d bytes", scan_data_len);

	/* Decode and display scan results */
	decode_scan_results(scan_data, scan_data_len);

	/* Clean up */
	k_free(scan_data);

	return 0;
}

/**
 * @brief Wi-Fi provisioning status shell command handler
 *
 * This command shows information about the generated Wi-Fi configuration.
 */
static int cmd_wifi_prov_info(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_info(shell, "Wi-Fi Configuration Information:");
	shell_info(shell, "  Size: %d bytes", wifi_config_data_len);
	shell_info(shell, "  Data: %p", wifi_config_data);

	/* Decode and display using decode_request for consistency */
	shell_info(shell, "=== Decoded Wi-Fi Configuration ===");
	decode_request(wifi_config_data, wifi_config_data_len);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_prov_subcmds,
	SHELL_CMD(info, NULL, "Show Wi-Fi configuration information", cmd_wifi_prov_info),
	SHELL_CMD(get_status, NULL, "Get Wi-Fi provisioning status", cmd_wifi_prov_get_status),
	SHELL_CMD(start_scan, NULL, "Start Wi-Fi scan", cmd_wifi_prov_start_scan),
	SHELL_CMD(stop_scan, NULL, "Stop Wi-Fi scan",
	cmd_wifi_prov_stop_scan),
	SHELL_CMD(set_config, NULL, "Set Wi-Fi configuration", cmd_wifi_prov_set_config),
	SHELL_CMD(forget_config, NULL, "Forget Wi-Fi configuration", cmd_wifi_prov_forget_config),
	SHELL_CMD(raw, NULL, "Send raw binary data (hex format)", cmd_wifi_prov_raw),
	SHELL_CMD(dump_scan, NULL,
	"Decode and display scan results (Base64)", cmd_wifi_prov_dump_scan),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wifi_prov, &wifi_prov_subcmds, "Wi-Fi provisioning commands", cmd_wifi_prov);
