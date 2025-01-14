/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#undef __XSI_VISIBLE
#define __XSI_VISIBLE 1
#include <time.h>
#include <zephyr/posix/time.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/modem_info.h>
#endif /* CONFIG_NRF_MODEM_LIB */
#include <date_time.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>

#include "cJSON.h"
#include "cJSON_os.h"
#include "ble_codec.h"
#include "ble_conn_mgr.h"
#include "ble.h"
#include "gateway.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_transport.h"

#define MAX_SERVICE_BUF_SIZE 300

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble_codec, CONFIG_NRFCLOUD_BLE_GATEWAY_LOG_LEVEL);

typedef int (*gateway_state_handler_t)(void *root_obj);

extern void nrf_cloud_register_gateway_state_handler(gateway_state_handler_t handler);
extern int nrf_cloud_modem_info_json_encode(const struct nrf_cloud_modem_info * const mod_inf,
					    cJSON * const mod_inf_obj);
extern int nrf_cloud_service_info_json_encode(const struct nrf_cloud_svc_info * const svc_inf,
					      cJSON * const svc_inf_obj);
extern int nrf_cloud_shadow_delta_response_encode(cJSON *input_obj,
						  bool accept,
						  struct nrf_cloud_data *const output);

extern struct ble_scanned_dev ble_scanned_devices[MAX_SCAN_RESULTS];

static char service_buffer[MAX_SERVICE_BUF_SIZE];

static bool first_service = true;
static bool first_chrc = true;
static bool desired_conns_strings;

#define JSON_KEY_SRVC_INFO	"serviceInfo"

/* define macros to enable memory allocation error checking and
 * cleanup, and also improve readability
 */
#define CJCREATE(_a_) do { \
	(_a_) = cJSON_CreateObject(); \
	if ((_a_) == NULL) { \
		goto cleanup; \
	} \
} while (0)

#define CJADDITEM cJSON_AddItemToObject
#define CJADDITEMCS cJSON_AddItemToObjectCS
#define CJADDREF cJSON_AddItemReferenceToObject
#define CJADDREFCS cJSON_AddItemReferenceToObjectCS

#define CJPRINT(_a_, _b_, _c_, _d_) do { \
	int _e_ = cJSON_PrintPreallocated((_a_), (_b_), (_c_), (_d_)); \
	if (!_e_) { \
		LOG_ERR("Insufficient buffer size %d", (_c_)); \
		goto cleanup; \
	} \
} while (0)

#define CJCHK(_a_) do { \
	if ((_a_) == NULL) { \
		LOG_ERR("cJSON out of memory in %s", __func__); \
		goto cleanup; \
	} \
} while (0)

#define CJADDCHK(_a_, _b_, _c_) do { \
	CJCHK(_c_); \
	cJSON_AddItemToObject((_a_), (_b_), (_c_)); \
} while (0)

#define CJADDBOOL(_a_, _b_, _c_) CJCHK( \
		cJSON_AddBoolToObject((_a_), (_b_), (_c_)))
#define CJADDBOOLCS(_a_, _b_, _c_) CJCHK( \
		cJSON_AddBoolToObjectCS((_a_), (_b_), (_c_)))

#define CJADDSTR(_a_, _b_, _c_) CJCHK( \
		cJSON_AddStringToObject((_a_), (_b_), (_c_)))
#define CJADDSTRCS(_a_, _b_, _c_) CJCHK( \
		cJSON_AddStringToObjectCS((_a_), (_b_), (_c_)))

#define CJADDNUM(_a_, _b_, _c_) CJCHK( \
		cJSON_AddNumberToObject((_a_), (_b_), (_c_)))
#define CJADDNUMCS(_a_, _b_, _c_) CJCHK( \
		cJSON_AddNumberToObjectCS((_a_), (_b_), (_c_)))

#define CJADDNULL(_a_, _b_) CJCHK( \
		cJSON_AddNullToObject((_a_), (_b_)))
#define CJADDNULLCS(_a_, _b_) CJCHK( \
		cJSON_AddNullToObjectCS((_a_), (_b_)))

#define CJADDARROBJ(_a_, _b_) do { \
	(_b_) = cJSON_CreateObject(); \
	CJCHK(_b_); \
	cJSON_AddItemToArray((_a_), (_b_)); \
} while (0)

#define CJADDARRNUM(_a_, _b_) do { \
	cJSON *tmp = cJSON_CreateNumber((_b_)); \
	CJCHK(tmp); \
	cJSON_AddItemToArray((_a_), tmp); \
} while (0)

#define CJADDARRSTR(_a_, _b_) do { \
	cJSON *tmp = cJSON_CreateString((_b_)); \
	CJCHK(tmp); \
	cJSON_AddItemToArray((_a_), tmp); \
} while (0)


char *get_time_str(char *dst, size_t len)
{
	int64_t unix_time_ms;
	int err;

#ifdef CONFIG_DATE_TIME
	err = date_time_now(&unix_time_ms);
	if (!err) {
		time_t unix_time;
		struct tm *time;

		unix_time = unix_time_ms / MSEC_PER_SEC;
		LOG_DBG("Unix time %lld", unix_time);
		time = gmtime(&unix_time);
		if (time) {
			/* 2020-02-19T18:38:50.363Z */
			strftime(dst, len, "%Y-%m-%dT%H:%M:%S.000Z", time);
			LOG_DBG("Date/time %s", dst);
		} else {
			LOG_WRN("Time not valid");
			dst = NULL;
		}
	} else {
		dst = NULL;
		LOG_ERR("Date/time not available: %d", err);
	}
#else
	struct tm tm;
	struct timespec ts;
	time_t t;

	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		LOG_DBG("Unix time %lld", ts.tv_sec);
		t = (time_t)ts.tv_sec;
		tm = *(gmtime(&t));
		/* 2020-02-19T18:38:50.363Z */
		strftime(dst, len, "%Y-%m-%dT%H:%M:%S.000Z", &tm);
		LOG_DBG("Date/time %s", dst);
	}
#endif
	return dst;
}

int device_error_encode(const char *ble_address, const char *error_msg,
			struct gw_msg *msg)
{
	/* TODO: Front end doesn't handle error messages yet.
	 * This format may change.
	 */
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *event = cJSON_CreateObject();
	cJSON *device = cJSON_CreateObject();
	cJSON *error = cJSON_CreateObject();
	char str[64];

	if ((root_obj == NULL) || (event == NULL) || (device == NULL) ||
	    (error == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(root_obj, "type", "event");
	CJADDSTRCS(root_obj, "gatewayId", gateway_id);
	CJADDSTRCS(root_obj, "timestamp", get_time_str(str, sizeof(str)));

	CJADDSTRCS(event, "type", "error");
	CJADDSTRCS(error, "description", error_msg);
	CJADDSTRCS(device, "deviceAddress", ble_address);

	/* use references so deleting is cleaner below; must be last
	 * operation on any given cJSON object
	 */
	CJADDREFCS(event, "device", device);
	CJADDREFCS(event, "error", error);
	CJADDREFCS(root_obj, "event", event);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	ret = 0;

cleanup:
	cJSON_Delete(error);
	cJSON_Delete(device);
	cJSON_Delete(event);
	cJSON_Delete(root_obj);
	return ret;
}

int device_found_encode(uint8_t num_devices_found, struct gw_msg *msg)
{
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *event = cJSON_CreateObject();
	cJSON *devices = cJSON_CreateArray();
	cJSON *device = NULL;
	cJSON *address = NULL;
	char str[64];

	if ((root_obj == NULL) || (event == NULL) || (devices == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(root_obj, "type", "event");
	CJADDSTRCS(root_obj, "gatewayId", gateway_id);
	CJADDNULLCS(root_obj, "requestId");

	CJADDSTRCS(event, "type", "scan_result");
	CJADDSTRCS(event, "timestamp", get_time_str(str, sizeof(str)));
	CJADDSTRCS(event, "subType", "instant");
	CJADDBOOLCS(event, "timeout", true);

	for (int i = 0; i < num_devices_found; i++) {
		/* TODO: Update for beacons */
		CJADDARROBJ(devices, device);
		CJADDSTRCS(device, "deviceType", "BLE");
		CJADDNUMCS(device, "rssi", ble_scanned_devices[i].rssi);
		if (strlen(ble_scanned_devices[i].name) > 0) {
			CJADDSTRCS(device, "name", ble_scanned_devices[i].name);
		}

		CJCREATE(address);
		CJADDSTRCS(address, "address", ble_scanned_devices[i].ble_mac_addr_str);
		CJADDITEMCS(device, "address", address);
		address = NULL;
	}

	/* Figure out a messageId:
	 * CJADDNUMCS(root_obj, "messageId", 1);
	 */
	CJADDREFCS(event, "devices", devices);
	CJADDREFCS(root_obj, "event", event);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	ret = 0;

cleanup:
	if (address) {
		cJSON_Delete(address);
	}
	cJSON_Delete(devices);
	cJSON_Delete(event);
	cJSON_Delete(root_obj);
	return ret;
}

int device_connect_result_encode(char *ble_address, bool conn_status,
				 struct gw_msg *msg)
{
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *event = cJSON_CreateObject();
	cJSON *device = cJSON_CreateObject();
	cJSON *address = cJSON_CreateObject();
	cJSON *status = cJSON_CreateObject();
	char str[64];

	if ((root_obj == NULL) || (event == NULL) || (device == NULL) ||
	    (address == NULL) || (status == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(root_obj, "type", "event");
	CJADDSTRCS(root_obj, "gatewayId", gateway_id);
	CJADDNULLCS(root_obj, "requestId");

	CJADDSTRCS(event, "type", "device_connect_result");
	CJADDSTRCS(event, "timestamp", get_time_str(str, sizeof(str)));

	CJADDSTRCS(device, "id", ble_address);
	CJADDSTRCS(address, "address", ble_address);
	CJADDBOOLCS(status, "connected", conn_status);

	CJADDREFCS(device, "address", address);
	CJADDREFCS(device, "status", status);
	CJADDREFCS(event, "device", device);
	CJADDREFCS(root_obj, "event", event);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	ret = 0;

cleanup:
	cJSON_Delete(status);
	cJSON_Delete(address);
	cJSON_Delete(device);
	cJSON_Delete(event);
	cJSON_Delete(root_obj);
	return ret;
}

int device_disconnect_result_encode(char *ble_address, bool conn_status,
				    struct gw_msg *msg)
{
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *event = cJSON_CreateObject();
	cJSON *device = cJSON_CreateObject();
	cJSON *address = cJSON_CreateObject();
	cJSON *status = cJSON_CreateObject();
	char str[64];

	if ((root_obj == NULL) || (event == NULL) || (device == NULL) ||
	    (address == NULL) || (status == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(root_obj, "type", "event");
	CJADDSTRCS(root_obj, "gatewayId", gateway_id);
	CJADDNULLCS(root_obj, "requestId");

	CJADDSTRCS(event, "type", "device_disconnect");
	CJADDSTRCS(event, "timestamp", get_time_str(str, sizeof(str)));

	CJADDSTRCS(device, "id", ble_address);
	CJADDSTRCS(address, "address", ble_address);
	CJADDBOOLCS(status, "connected", conn_status);

	CJADDREFCS(device, "status", status);
	CJADDREFCS(device, "address", address);
	CJADDREFCS(event, "device", device);
	CJADDREFCS(root_obj, "event", event);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	ret = 0;

cleanup:
	cJSON_Delete(status);
	cJSON_Delete(address);
	cJSON_Delete(device);
	cJSON_Delete(event);
	cJSON_Delete(root_obj);
	return ret;
}

int device_value_changed_encode(char *ble_address, char *uuid, char *path,
				char *value, uint16_t value_length,
				struct gw_msg *msg)
{
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *event = cJSON_CreateObject();
	cJSON *device = cJSON_CreateObject();
	cJSON *address = cJSON_CreateObject();
	cJSON *chrc = cJSON_CreateObject();
	cJSON *value_arr = cJSON_CreateArray();
	char str[64];

	if ((root_obj == NULL) || (event == NULL) || (device == NULL) ||
	    (address == NULL) || (chrc == NULL) || (value_arr == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(root_obj, "type", "event");
	CJADDSTRCS(root_obj, "gatewayId", gateway_id);
	CJADDNULLCS(root_obj, "requestId");

	CJADDSTRCS(event, "type", "device_characteristic_value_changed");
	CJADDSTRCS(event, "timestamp", get_time_str(str, sizeof(str)));

	CJADDSTRCS(device, "id", ble_address);
	CJADDSTRCS(address, "address", ble_address);
	/* TODO: Get Type; */
	CJADDSTRCS(address, "type", "random");

	CJADDSTRCS(chrc, "uuid", uuid);
	CJADDSTRCS(chrc, "path", path);

	for (int i = 0; i < value_length; i++) {
		CJADDARRNUM(value_arr, value[i]);
	}

	CJADDREFCS(device, "address", address);
	CJADDREFCS(chrc, "value", value_arr);
	CJADDREFCS(event, "device", device);
	CJADDREFCS(event, "characteristic", chrc);
	CJADDREFCS(root_obj, "event", event);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	ret = 0;

cleanup:
	cJSON_Delete(value_arr);
	cJSON_Delete(chrc);
	cJSON_Delete(address);
	cJSON_Delete(device);
	cJSON_Delete(event);
	cJSON_Delete(root_obj);
	return ret;
}

int device_value_write_result_encode(const char *ble_address, const char *uuid, const char *path,
				     const char *value, uint16_t value_length,
				     struct gw_msg *msg)
{
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *event = cJSON_CreateObject();
	cJSON *device = cJSON_CreateObject();
	cJSON *address = cJSON_CreateObject();
	cJSON *desc = cJSON_CreateObject();
	cJSON *value_arr = cJSON_CreateArray();
	char str[64];

	if ((root_obj == NULL) || (event == NULL) || (device == NULL) ||
	    (address == NULL) || (desc == NULL) || (value_arr == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(root_obj, "type", "event");
	CJADDSTRCS(root_obj, "gatewayId", gateway_id);
	CJADDNULLCS(root_obj, "requestId");

	CJADDSTRCS(event, "type", "device_descriptor_value_write_result");
	CJADDSTRCS(event, "timestamp", get_time_str(str, sizeof(str)));

	CJADDSTRCS(device, "id", ble_address);
	CJADDSTRCS(address, "address", ble_address);
	/* TODO: Get Type; */
	CJADDSTRCS(address, "type", "random");

	CJADDSTRCS(desc, "uuid", uuid);
	CJADDSTRCS(desc, "path", path);

	for (int i = 0; i < value_length; i++) {
		CJADDARRNUM(value_arr, value[i]);
	}

	CJADDREFCS(device, "address", address);
	CJADDREFCS(desc, "value", value_arr);
	CJADDREFCS(event, "device", device);
	CJADDREFCS(event, "descriptor", desc);
	CJADDREFCS(root_obj, "event", event);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	ret = 0;

cleanup:
	cJSON_Delete(value_arr);
	cJSON_Delete(desc);
	cJSON_Delete(address);
	cJSON_Delete(device);
	cJSON_Delete(event);
	cJSON_Delete(root_obj);
	return ret;
}

int device_descriptor_value_encode(const char *ble_address, char *uuid,
				   const char *path, char *value,
				   uint16_t value_length,
				   struct gw_msg *msg, bool changed)
{
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *event = cJSON_CreateObject();
	cJSON *device = cJSON_CreateObject();
	cJSON *address = cJSON_CreateObject();
	cJSON *desc = cJSON_CreateObject();
	cJSON *value_arr = cJSON_CreateArray();
	char str[64];

	if ((root_obj == NULL) || (event == NULL) || (device == NULL) ||
	    (address == NULL) || (desc == NULL) || (value_arr == NULL)) {
		LOG_ERR("Invalid parameters");
		goto cleanup;
	}

	CJADDSTRCS(root_obj, "type", "event");
	CJADDSTRCS(root_obj, "gatewayId", gateway_id);
	CJADDNULLCS(root_obj, "requestId");

	CJADDSTRCS(event, "type", changed ?
		   "device_descriptor_value_changed" :
		   "device_descriptor_value_read_result");
	CJADDSTRCS(event, "timestamp", get_time_str(str, sizeof(str)));

	CJADDSTRCS(device, "id", ble_address);
	CJADDSTRCS(address, "address", ble_address);
	/* TODO: Get Type; */
	CJADDSTRCS(address, "type", "random");

	CJADDSTRCS(desc, "uuid", uuid);
	CJADDSTRCS(desc, "path", path);

	for (int i = 0; i < value_length; i++) {
		CJADDARRNUM(value_arr, value[i]);
	}

	CJADDREFCS(device, "address", address);
	CJADDREFCS(desc, "value", value_arr);
	CJADDREFCS(event, "device", device);
	CJADDREFCS(event, "descriptor", desc);
	CJADDREFCS(root_obj, "event", event);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	ret = 0;

cleanup:
	cJSON_Delete(value_arr);
	cJSON_Delete(desc);
	cJSON_Delete(address);
	cJSON_Delete(device);
	cJSON_Delete(event);
	cJSON_Delete(root_obj);
	return ret;
}

int device_chrc_read_encode(char *ble_address, char *uuid, char *path,
			    char *value, uint16_t value_length,
			    struct gw_msg *msg)
{
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *event = cJSON_CreateObject();
	cJSON *device = cJSON_CreateObject();
	cJSON *address = cJSON_CreateObject();
	cJSON *chrc = cJSON_CreateObject();
	cJSON *value_arr = cJSON_CreateArray();
	char str[128];
	bool is_string = true;

	if ((root_obj == NULL) || (event == NULL) || (device == NULL) ||
	    (address == NULL) || (chrc == NULL) || (value_arr == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(root_obj, "type", "event");
	CJADDSTRCS(root_obj, "gatewayId", gateway_id);
	CJADDNULLCS(root_obj, "requestId");

	CJADDSTRCS(event, "type", "device_characteristic_value_changed");
	/** This used to use "device_characteristic_value_read_result"
	 * but that no longer works in the cloud backend.
	 */
	CJADDSTRCS(event, "timestamp", get_time_str(str, sizeof(str)));

	CJADDSTRCS(device, "id", ble_address);
	CJADDSTRCS(address, "address", ble_address);
	CJADDSTRCS(address, "type", "random"); /* Not accurate: we do not know the type. */
	CJADDSTRCS(chrc, "uuid", uuid);
	CJADDSTRCS(chrc, "path", path);
	CJADDREFCS(device, "address", address);

	for (int i = 0; i < value_length; i++) {
		if ((value[i] == '\0') && (i == (value_length - 1))) {
			break;
		}
		if (!isprint(value[i])) {
			is_string = false;
			LOG_DBG("char at %d not printable: 0x%02X", i, value[i]);
			break;
		}
	}

	if (is_string && (value_length > 1)) {
		strncpy(str, value, sizeof(str) - 1);
		str[MIN(sizeof(str) - 1, value_length)] = '\0';
		CJADDSTRCS(chrc, "value", str);
		LOG_DBG("Value: %s", value);
	} else {
		LOG_HEXDUMP_DBG(value, value_length, "Value");
	}

	for (int i = 0; i < value_length; i++) {
		CJADDARRNUM(value_arr, value[i]);
	}
	CJADDREFCS(chrc, "value", value_arr);

	CJADDREFCS(event, "device", device);
	CJADDREFCS(event, "characteristic", chrc);
	CJADDREFCS(root_obj, "event", event);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	ret = 0;

cleanup:
	cJSON_Delete(value_arr);
	cJSON_Delete(chrc);
	cJSON_Delete(address);
	cJSON_Delete(device);
	cJSON_Delete(event);
	cJSON_Delete(root_obj);
	return ret;
}

static int create_device_wrapper(char *ble_address, bool conn_status,
				 struct gw_msg *msg)
{
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *event = cJSON_CreateObject();
	cJSON *device = cJSON_CreateObject();
	cJSON *address = cJSON_CreateObject();
	cJSON *status = cJSON_CreateObject();
	cJSON *services = cJSON_CreateObject();
	char str[64];

	if ((root_obj == NULL) || (event == NULL) || (device == NULL) ||
	    (address == NULL) || (status == NULL) || (services == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(root_obj, "type", "event");
	CJADDSTRCS(root_obj, "gatewayId", gateway_id);
	CJADDNULLCS(root_obj, "requestId");

	CJADDSTRCS(event, "type", "device_discover_result");
	CJADDSTRCS(event, "timestamp", get_time_str(str, sizeof(str)));

	CJADDSTRCS(device, "id", ble_address);
	CJADDSTRCS(address, "address", ble_address);
	CJADDBOOLCS(status, "connected", conn_status);

	CJADDREFCS(device, "status", status);
	CJADDREFCS(device, "address", address);
	CJADDREFCS(event, "device", device);
	CJADDREFCS(event, "services", services);
	CJADDREFCS(root_obj, "event", event);

	char *ptr = (char *)msg->data.ptr;

	CJPRINT(root_obj, ptr, msg->data_max_len, 0);
	ptr[strlen(ptr) - 3] = 0;
	msg->data.len = strlen(ptr);
	ret = 0;

cleanup:
	cJSON_Delete(services);
	cJSON_Delete(status);
	cJSON_Delete(address);
	cJSON_Delete(device);
	cJSON_Delete(event);
	cJSON_Delete(root_obj);
	return ret;
}

int device_shadow_data_encode(const char *ble_address, bool connecting,
			      bool connected, struct gw_msg *msg)
{
	__ASSERT_NO_MSG(msg != NULL);
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *reported_obj = cJSON_CreateObject();
	cJSON *status_connections = cJSON_CreateObject();
	cJSON *device = cJSON_CreateObject();
	cJSON *status = cJSON_CreateObject();

	if ((root_obj == NULL) || (state_obj == NULL) ||
	    (reported_obj == NULL) || (status_connections == NULL) ||
	    (device == NULL) || (status == NULL)) {
		LOG_ERR("Error creating shadow data");
		goto cleanup;
	}

	CJADDSTRCS(device, "id", ble_address);
	CJADDBOOLCS(status, "connected", connected);
	CJADDBOOLCS(status, "connecting", connecting);

	CJADDREFCS(device, "status", status);
	CJADDREF(status_connections, ble_address, device);
	CJADDREFCS(reported_obj, "statusConnections", status_connections);
	CJADDREFCS(state_obj, "reported", reported_obj);
	CJADDREFCS(root_obj, "state", state_obj);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	ret = 0;

cleanup:
	cJSON_Delete(status);
	cJSON_Delete(device);
	cJSON_Delete(status_connections);
	cJSON_Delete(reported_obj);
	cJSON_Delete(state_obj);
	cJSON_Delete(root_obj);
	return ret;
}

int gateway_reported_encode(struct gw_msg *msg)
{
	__ASSERT_NO_MSG(msg != NULL);
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *reported_obj = cJSON_CreateObject();
	cJSON *connections_obj = cJSON_CreateNull();
	cJSON *status_connections = cJSON_CreateNull();

	if ((root_obj == NULL) || (state_obj == NULL) ||
	    (reported_obj == NULL) || (connections_obj == NULL) || (status_connections == NULL)) {
		goto cleanup;
	}
	CJADDCHK(reported_obj, "desiredConnections", connections_obj);
	CJADDCHK(reported_obj, "statusConnections", status_connections);
	CJADDCHK(state_obj, "reported", reported_obj);
	CJADDCHK(root_obj, "state", state_obj);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);
	cJSON_Delete(root_obj);
	return 0;

cleanup:
	cJSON_Delete(status_connections);
	cJSON_Delete(connections_obj);
	cJSON_Delete(reported_obj);
	cJSON_Delete(state_obj);
	cJSON_Delete(root_obj);

	return ret;
}

int gateway_desired_list_encode(const struct desired_conn *desired, int num_desired,
				struct gw_msg *msg)
{
	__ASSERT_NO_MSG(msg != NULL);
	int ret = -ENOMEM;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *desired_obj = cJSON_CreateObject();
	cJSON *connections_obj = cJSON_CreateArray();

	if ((root_obj == NULL) || (state_obj == NULL) ||
	    (desired_obj == NULL) || (connections_obj == NULL)) {
		goto cleanup;
	}

	/* create array of desired BLE addresses */
	for (int i = 0; i < num_desired; i++) {
		if (!desired[i].active) {
			continue;
		}
		if (desired_conns_strings) {
			/* nrfcloud stage wants the new, simpler form of
			 * desiredConnections where its just an array of strings
			 */
			CJADDARRSTR(connections_obj, desired[i].ble_mac_addr_str);
		} else {
			/* nrfcloud stage still uses older array of objects */
			cJSON *item;

			CJADDARROBJ(connections_obj, item);
			CJADDSTRCS(item, "id", desired[i].ble_mac_addr_str);
		}
	}

	if (!num_desired) {
		connections_obj = cJSON_CreateNull();
	}

	CJADDCHK(desired_obj, "desiredConnections", connections_obj);
	CJADDCHK(state_obj, "desired", desired_obj);
	CJADDCHK(root_obj, "state", state_obj);

	CJPRINT(root_obj, (char *)msg->data.ptr, msg->data_max_len, 0);
	msg->data.len = strlen((char *)msg->data.ptr);

	ret = 0;

cleanup:
	cJSON_Delete(connections_obj);
	cJSON_Delete(desired_obj);
	cJSON_Delete(state_obj);
	cJSON_Delete(root_obj);

	return ret;
}

/* Start of functions to assemble large JSON string. No room to create these
 * all at once in cJSON
 */
static int device_discover_add_service(char *discovered_json,
				       struct gw_msg *msg)
{
	char *ptr = (char *)msg->data.ptr;

	if (first_service == false) {
		strcat(ptr, "}},");
	}

	strcat(ptr, discovered_json + 1);

	if (strlen(ptr) >= 3) {
		ptr[strlen(ptr) - 3] = 0;
	}
	msg->data.len = strlen(ptr);

	memset(discovered_json, 0, MAX_SERVICE_BUF_SIZE);
	first_service = false;
	first_chrc = true;
	return 0;
}

static int device_discover_add_chrc(char *discovered_json,
				    struct gw_msg *msg)
{
	char *ptr = (char *)msg->data.ptr;

	if (first_chrc == false) {
		strcat(ptr, ",");
	}

	strcat(ptr, discovered_json + 1);

	if (strlen(ptr) >= 1) {
		ptr[strlen(ptr) - 1] = 0;
	}
	msg->data.len = strlen(ptr);

	memset(discovered_json, 0, MAX_SERVICE_BUF_SIZE);

	first_chrc = false;
	return 0;
}

static int device_discover_add_ccc(char *discovered_json,
				   struct gw_msg *msg)
{
	char *ptr = (char *)msg->data.ptr;

	if (strlen(ptr) >= 2) {
		ptr[strlen(ptr) - 2] = 0;
	}

	strcat(ptr, discovered_json + 1);
	strcat(ptr, "}");
	msg->data.len = strlen(ptr);

	memset(discovered_json, 0, MAX_SERVICE_BUF_SIZE);

	first_chrc = false;
	return 0;
}

static int svc_attr_encode(char *uuid, char *path,
			    struct gw_msg *msg)
{
	int ret = -ENOMEM;
	cJSON *services = cJSON_CreateObject();
	cJSON *service = cJSON_CreateObject();
	cJSON *chrcs = cJSON_CreateObject();

	if ((services == NULL) || (service == NULL) || (chrcs == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(service, "uuid", uuid);

	CJADDREFCS(service, "characteristics", chrcs);
	CJADDREF(services, uuid, service);

	/* Print services and add to wrapper */
	CJPRINT(services, service_buffer, MAX_SERVICE_BUF_SIZE, 0);
	ret = device_discover_add_service(service_buffer, msg);

cleanup:
	cJSON_Delete(chrcs);
	cJSON_Delete(service);
	cJSON_Delete(services);
	return ret;
}

static int chrc_attr_encode(char *uuid, char *path, struct uuid_handle_pair *h,
			    struct gw_msg *msg)
{
	int ret = -ENOMEM;
	cJSON *chrc = cJSON_CreateObject();
	cJSON *descriptors = cJSON_CreateObject();
	cJSON *props = cJSON_CreateObject();
	cJSON *value_arr = cJSON_CreateArray();
	cJSON *parent_chrc = cJSON_CreateObject();
	uint8_t properties = h->properties;

	if ((chrc == NULL) || (descriptors == NULL) || (props == NULL) ||
	    (value_arr == NULL) || (parent_chrc == NULL)) {
		goto cleanup;
	}

	CJADDSTRCS(chrc, "uuid", uuid);
	CJADDSTRCS(chrc, "path", path);

	if (!h->value_len) {
		CJADDARRNUM(value_arr, 0);
	} else {
		for (int i = 0; i < h->value_len; i++) {
			CJADDARRNUM(value_arr, h->value[i]);
		}
	}

	/* Check and add properties */
	if (properties & BT_GATT_CHRC_READ) {
		CJADDBOOLCS(props, "read", true);
	}
	if (properties & BT_GATT_CHRC_WRITE) {
		CJADDBOOLCS(props, "write", true);
	}
	if (properties & BT_GATT_CHRC_INDICATE) {
		CJADDBOOLCS(props, "indicate", true);
	}
	if (properties & BT_GATT_CHRC_NOTIFY) {
		CJADDBOOLCS(props, "notify", true);
	}
	if (properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
		CJADDBOOLCS(props, "writeWithoutResponse", true);
	}
	if (properties & BT_GATT_CHRC_AUTH) {
		CJADDBOOLCS(props, "authorizedSignedWrite", true);
	}
	if ((properties == 0) || ((properties &
		~(BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
		  BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_NOTIFY |
		  BT_GATT_CHRC_WRITE_WITHOUT_RESP |
		  BT_GATT_CHRC_AUTH)) != 0)) {
		LOG_ERR("Unknown CHRC property: %d\n", properties);
		cJSON_Delete(parent_chrc);
		return -EINVAL;
	}

	CJADDREFCS(chrc, "properties", props);
	CJADDREFCS(chrc, "value", value_arr);
	CJADDREFCS(chrc, "descriptors", descriptors);
	CJADDREF(parent_chrc, uuid, chrc);

	/* Print parent_chrhc and add to service */
	CJPRINT(parent_chrc, service_buffer, MAX_SERVICE_BUF_SIZE, 0);
	ret = device_discover_add_chrc(service_buffer, msg);

cleanup:
	cJSON_Delete(parent_chrc);
	cJSON_Delete(value_arr);
	cJSON_Delete(props);
	cJSON_Delete(descriptors);
	cJSON_Delete(chrc);
	return ret;
}

static int ccc_attr_encode(char *uuid, char *path,
			   struct gw_msg *msg, bool sub_enabled)
{
	int ret = -ENOMEM;
	cJSON *descriptor = cJSON_CreateObject();
	cJSON *value_arr = cJSON_CreateArray();
	cJSON *parent_ccc =  cJSON_CreateObject();

	if ((descriptor == NULL) || (value_arr == NULL) ||
	     (parent_ccc == NULL)) {
		goto cleanup;
	}
	uint8_t val = sub_enabled ? BT_GATT_CCC_NOTIFY : 0;

	CJADDSTRCS(descriptor, "uuid", uuid);
	CJADDARRNUM(value_arr, val);
	CJADDARRNUM(value_arr, 0);
	CJADDSTRCS(descriptor, "path", path);

	CJADDREFCS(descriptor, "value", value_arr);
	CJADDREF(parent_ccc, uuid, descriptor);

	/* Print parent_ccc and add to service */
	CJPRINT(parent_ccc, service_buffer, MAX_SERVICE_BUF_SIZE, 0);
	ret = device_discover_add_ccc(service_buffer, msg);

cleanup:
	cJSON_Delete(parent_ccc);
	cJSON_Delete(value_arr);
	cJSON_Delete(descriptor);
	return ret;
}

static int attr_encode(struct uuid_handle_pair *uuid_handle,
		       struct uuid_handle_pair *other_handle,
		       char *uuid_str, char *path,
		       struct gw_msg *msg)
{
	int ret = 0;

	bt_to_upper(uuid_str, strlen(uuid_str));
	bt_to_upper(path, strlen(path));

	if (uuid_handle->attr_type ==  BT_ATTR_SERVICE) {
		LOG_INF("Encoding Service : UUID: %s", uuid_str);
		ret = svc_attr_encode(uuid_str, path, msg);

	} else if (uuid_handle->attr_type == BT_ATTR_CHRC) {
		ret = chrc_attr_encode(uuid_str, path, uuid_handle, msg);

	} else if (uuid_handle->attr_type == BT_ATTR_CCC) {
		ret = ccc_attr_encode(uuid_str, path, msg,
				      uuid_handle->sub_enabled ||
				      (other_handle ? other_handle->sub_enabled : 0));

	} else {
		LOG_ERR("Unknown Attr Type");
		ret = -EINVAL;
	}
	memset(service_buffer, 0, MAX_SERVICE_BUF_SIZE);
	return ret;
}

void get_uuid_str(struct uuid_handle_pair *uuid_handle, char *str, size_t len)
{
	struct bt_uuid *uuid;

	if (uuid_handle->uuid_type == BT_UUID_TYPE_16) {
		uuid = &uuid_handle->uuid_16.uuid;
	} else if (uuid_handle->uuid_type == BT_UUID_TYPE_128) {
		uuid = &uuid_handle->uuid_128.uuid;
	} else {
		str[0] = '\0';
		return;
	}
	bt_uuid_get_str(uuid, str, len);
}

int device_discovery_encode(struct ble_device_conn *conn_ptr,
			    struct gw_msg *msg)
{
	char uuid_str[BT_UUID_STR_LEN];
	char service_attr_str[BT_UUID_STR_LEN];
	char path_dep_two_str[BT_UUID_STR_LEN];
	char path_str[BT_MAX_PATH_LEN];
	int ret = 0;
	uint8_t num_encoded = 0;

	first_service = true;

	LOG_INF("Num Pairs: %d", conn_ptr->num_pairs);

	ret = create_device_wrapper(conn_ptr->ble_mac_addr_str, true, msg);
	if (ret) {
		return ret;
	}

	for (int i = 0; i < conn_ptr->num_pairs; i++) {
		struct uuid_handle_pair *uuid_handle;
		struct uuid_handle_pair *uh = NULL;

		uuid_handle = conn_ptr->uuid_handle_pairs[i];
		if (uuid_handle == NULL) {
			continue;
		}

		if ((uuid_handle->uuid_type != BT_UUID_TYPE_16) &&
		    (uuid_handle->uuid_type != BT_UUID_TYPE_128)) {
			LOG_ERR("ERROR: unknown UUID type");
			ret = -EINVAL;
			continue;
		}
		get_uuid_str(uuid_handle, uuid_str, BT_UUID_STR_LEN);

		switch (uuid_handle->path_depth) {
		case 1:
			for (int j = i; j >= 0; j--) {
				uh = conn_ptr->uuid_handle_pairs[j];
				if (uh == NULL) {
					continue;
				}
				if (uh->is_service == true) {
					get_uuid_str(uh, service_attr_str,
						     BT_UUID_STR_LEN);
					break;
				}
			}
			snprintk(path_str, BT_MAX_PATH_LEN, "%s/%s",
				 service_attr_str, uuid_str);
			break;
		case 2:
			uh = conn_ptr->uuid_handle_pairs[i - 1];
			if (uh != NULL) {
				get_uuid_str(uh, path_dep_two_str,
					     BT_UUID_STR_LEN);
				snprintk(path_str, BT_MAX_PATH_LEN, "%s/%s/%s",
					 service_attr_str, path_dep_two_str,
					 uuid_str);
			}
			break;
		}

		ret = attr_encode(uuid_handle, uh, uuid_str, path_str, msg);
		if (!ret) {
			num_encoded++;
		}
	}

	/* make sure we output at least one attribute, or
	 * device_discovery_send() will send malformed JSON
	 */
	if (num_encoded) {
		/* Add the remaing brackets to the JSON string */
		strcat((char *)msg->data.ptr, "}}}}}");
		msg->data.len = strlen((char *)msg->data.ptr);

		/* ignore invalid UUID type since others were ok */
		ret = 0;

	} else if (!ret) {
		ret = -EINVAL; /* no data to send */
	}
	return ret;
}

static char *get_addr_from_des_conn_array(cJSON *array, int index)
{
	cJSON *item;
	cJSON *address_obj;
	cJSON *ble_address;

	item = cJSON_GetArrayItem(array, index);
	if (item == NULL) {
		return NULL;
	}

	if (item->type != cJSON_String) {
		address_obj = cJSON_GetObjectItem(item, "address");
		if (address_obj == NULL) {
			ble_address = cJSON_GetObjectItem(item, "id");
		} else {
			ble_address = cJSON_GetObjectItem(address_obj, "address");
		}
		/* the nrfcloud stage we are on uses an array of objects
		 * named 'address' or 'id', so remember this later if
		 * the shell needs to manipulate the desiredConnections
		 * array
		 */
		desired_conns_strings = false;
	} else {
		/* the nrfcloud stage uses a simple array of strings */
		desired_conns_strings = true;
		return item->valuestring;
	}

	if (ble_address != NULL) {
		return ble_address->valuestring;
	}
	return NULL;
}

int desired_conns_handler(cJSON *desired_connections_obj)
{
	bool changed = false;
	int num_cons;
	char *ble_mac_addr_str;

	if (!desired_connections_obj) {
		return 0;
	}

	char *ptr = cJSON_Print(desired_connections_obj);

	LOG_INF("Desired conns: %s", ptr);
	nrf_cloud_free(ptr);

	if (!cJSON_GetArraySize(desired_connections_obj)) {
		/* there are none, so we can't tell what format the nrfcloud
		 * stage wants to use... pick something here, but it may be
		 * wrong, which means later, if the shell user creates the first
		 * connection, it could be in the wrong format; but, this
		 * is a temporary development issue and will go away soon
		 * (Q1 2021)
		 */
		desired_conns_strings = false;
	}

	struct desired_conn *cons = get_desired_array(&num_cons);

	/* check whether anything has actually changed */
	/* first, search for additions to the array */
	for (int i = 0; i < cJSON_GetArraySize(desired_connections_obj); i++) {
		ble_mac_addr_str = get_addr_from_des_conn_array(desired_connections_obj, i);

		if (ble_mac_addr_str != NULL) {
			bool found = false;

			for (int j = 0; j < num_cons; j++) {
				if ((strcmp(ble_mac_addr_str, cons[j].ble_mac_addr_str) == 0) &&
				    cons[j].active) {
					found = true;
					break;
				}
			}
			/* new device found in cloud's array */
			if (!found) {
				LOG_INF("New device added by cloud: %s",
					ble_mac_addr_str);
				changed = true;
				break;
			}
		}
	}

	/* second, search for removals from the array */
	for (int i = 0; i < num_cons; i++) {
		bool found = false;

		if (!cons[i].active) {
			continue;
		}

		for (int j = 0; j < cJSON_GetArraySize(desired_connections_obj);
		     j++) {
			ble_mac_addr_str = get_addr_from_des_conn_array(
						    desired_connections_obj, j);
			if (ble_mac_addr_str != NULL) {
				if ((strcmp(ble_mac_addr_str, cons[i].ble_mac_addr_str) == 0) &&
				    cons[i].active) {
					found = true;
					break;
				}
			}
		}

		/* device removed from cloud's array, and it wasn't manually added */
		if (!found && !cons[i].manual) {
			LOG_INF("Device removed by cloud: %s",
				cons[i].ble_mac_addr_str);
			changed = true;
			break;
		}
	}

	if (!changed) {
#if defined(ACK_DELTA)
		int ret;
		struct nrf_cloud_tx_data tx_data = {
			.topic_type = NRF_CLOUD_TOPIC_STATE,
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		};

		ret = nrf_cloud_shadow_delta_response_encode(state_obj, true, &tx_data.data);

		if (ret == 0) {
			ret = nrf_cloud_send(&tx_data);
			nrf_cloud_free((void *)tx_data.data.ptr);
			if (ret) {
				LOG_ERR("nct_cc_send failed %d", ret);
			}
		} else {
			LOG_ERR("Error acking shadow delta: %d", ret);
		}
#else
		LOG_INF("Desired connections did not change. Ignoring delta.");
#endif
	}

	LOG_DBG("Gateway state change detected");

	ble_conn_mgr_clear_desired(false);

	for (int i = 0; i < cJSON_GetArraySize(desired_connections_obj); i++) {

		ble_mac_addr_str = get_addr_from_des_conn_array(desired_connections_obj, i);

		if (ble_mac_addr_str != NULL) {
			LOG_DBG("Desired BLE address: %s", ble_mac_addr_str);

			if (!ble_conn_mgr_enabled(ble_mac_addr_str)) {
				LOG_INF("Skipping disabled device: %s",
					ble_mac_addr_str);
				continue;
			}
			if (ble_conn_mgr_add_conn(ble_mac_addr_str)) {
				LOG_DBG("Conn already added");
			}
			ble_conn_mgr_update_desired(ble_mac_addr_str, i);
		} else {
			LOG_ERR("Invalid desired connection");
			return -EINVAL;
		}
	}
	ble_conn_mgr_update_connections();
	return 0;
}

void ble_codec_init(void)
{
	/* Nothing to do. */
}
