/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net_buf.h>
#include <zephyr/logging/log.h>
#include <net/wifi_prov_core/wifi_prov_core.h>

/* Include nanopb headers */
#include "pb_decode.h"
#include "pb_encode.h"

/* Include generated protobuf headers */
#include "request.pb.h"
#include "response.pb.h"
#include "result.pb.h"
#include "common.pb.h"

LOG_MODULE_REGISTER(wifi_prov_transport, CONFIG_WIFI_PROV_INTERNAL_LOG_LEVEL);

/**
 * @brief Decode and log Wi-Fi provisioning response
 */
static void decode_response(const uint8_t *data, size_t len)
{
	Response response = Response_init_zero;
	pb_istream_t stream = pb_istream_from_buffer(data, len);

	bool _decode_result = pb_decode(&stream, Response_fields, &response);

	if (_decode_result) {
		LOG_INF("=== Wi-Fi Provisioning Response ===");

		if (response.has_request_op_code) {
			LOG_INF("Request OpCode: %d", response.request_op_code);
		}

		if (response.has_status) {
			LOG_INF("Status: %d", response.status);
		}

		/* Log simple responses even without device_status */
		LOG_INF("Response decoded successfully");

		if (response.has_device_status) {
			LOG_INF("Device Status:");
			if (response.device_status.has_state) {
				LOG_INF("  Connection State: %d", response.device_status.state);
			}

			if (response.device_status.has_provisioning_info) {
				LOG_INF("  Provisioning Info:");
				LOG_INF("    SSID: %.*s",
					   (int)response.device_status.provisioning_info.ssid.size,
					   response.device_status.provisioning_info.ssid.bytes);
				LOG_INF("    BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
					response.device_status.provisioning_info.bssid.bytes[0],
					response.device_status.provisioning_info.bssid.bytes[1],
					response.device_status.provisioning_info.bssid.bytes[2],
					response.device_status.provisioning_info.bssid.bytes[3],
					response.device_status.provisioning_info.bssid.bytes[4],
					response.device_status.provisioning_info.bssid.bytes[5]);
				LOG_INF("    Channel: %d",
					response.device_status.provisioning_info.channel);
				LOG_INF("    Auth: %d",
					response.device_status.provisioning_info.auth);
			}

			if (response.device_status.has_connection_info) {
				LOG_INF("  Connection Info:");
				LOG_INF("    IP4 Addr: %02x:%02x:%02x:%02x",
					response.device_status.connection_info.ip4_addr.bytes[0],
					response.device_status.connection_info.ip4_addr.bytes[1],
					response.device_status.connection_info.ip4_addr.bytes[2],
					response.device_status.connection_info.ip4_addr.bytes[3]);
			}

			if (response.device_status.has_scan_info) {
				LOG_INF("  Scan Info:");
				LOG_INF("    Band: %d", response.device_status.scan_info.band);
				LOG_INF("    Passive: %s",
					response.device_status.scan_info.passive ?
					"true" : "false");
				LOG_INF("    Period (ms): %d",
					response.device_status.scan_info.period_ms);
				LOG_INF("    Group Channels: %d",
					response.device_status.scan_info.group_channels);
		}

		LOG_INF("=================================");
		}
	} else {
		LOG_ERR("Failed to decode response: %s", PB_GET_ERROR(&stream));
		LOG_ERR("pb_decode returned: %s", _decode_result ? "true" : "false");
		LOG_ERR("Data length: %d bytes", len);
		LOG_HEXDUMP_ERR(data, len, "Raw response data:");
	}
}

/**
 * @brief Decode and log Wi-Fi provisioning result
 */
static void decode_result(const uint8_t *data, size_t len)
{
	Result result = Result_init_zero;
	pb_istream_t stream = pb_istream_from_buffer(data, len);

	bool _decode_result = pb_decode(&stream, Result_fields, &result);

	if (_decode_result) {
		LOG_INF("=== Wi-Fi Provisioning Result ===");

		if (result.has_state) {
			LOG_INF("Connection State: %d", result.state);
		}

		if (result.has_reason) {
			LOG_INF("Failure Reason: %d", result.reason);
		}

		if (result.has_scan_record) {
			LOG_INF("Scan Record:");
			if (result.scan_record.has_wifi) {
				LOG_INF("  SSID: %.*s",
					   (int)result.scan_record.wifi.ssid.size,
					   result.scan_record.wifi.ssid.bytes);
				LOG_INF("  BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
					   result.scan_record.wifi.bssid.bytes[0],
					   result.scan_record.wifi.bssid.bytes[1],
					   result.scan_record.wifi.bssid.bytes[2],
					   result.scan_record.wifi.bssid.bytes[3],
					   result.scan_record.wifi.bssid.bytes[4],
					   result.scan_record.wifi.bssid.bytes[5]);
				LOG_INF("  Channel: %d", result.scan_record.wifi.channel);
				LOG_INF("  Auth: %d", result.scan_record.wifi.auth);
			}

			if (result.scan_record.has_rssi) {
				LOG_INF("  RSSI: %d", result.scan_record.rssi);
			}
		}

		LOG_INF("===============================");
	} else {
		LOG_ERR("Failed to decode result: %s", PB_GET_ERROR(&stream));
		LOG_ERR("pb_decode returned: %s", _decode_result ? "true" : "false");
		LOG_ERR("Data length: %d bytes", len);
		LOG_HEXDUMP_ERR(data, len, "Raw result data:");
	}
}

int wifi_prov_send_rsp(struct net_buf_simple *rsp)
{
	LOG_INF("Wi-Fi provisioning response sent (stub): %d bytes", rsp->len);

	if (rsp->len > 0) {
		decode_response(rsp->data, rsp->len);
	}

	return 0;
}

int wifi_prov_send_result(struct net_buf_simple *result)
{
	LOG_INF("Wi-Fi provisioning result sent (stub): %d bytes", result->len);

	if (result->len > 0) {
		decode_result(result->data, result->len);
	}

	return 0;
}
