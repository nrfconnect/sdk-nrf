/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <modem/lte_lc.h>
#include <cJSON.h>
#include <cJSON_os.h>

#include <logging/log.h>

#include <cJSON.h>

#include <net/srest_client.h>

#include "rest_skyhook_wlan.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#define HOSTNAME	CONFIG_LOCATION_METHOD_WLAN_SERVICE_SKYHOOK_HOSTNAME

BUILD_ASSERT(sizeof(HOSTNAME) > 1, "Hostname must be configured");
BUILD_ASSERT(
	sizeof(CONFIG_LOCATION_METHOD_WLAN_SERVICE_SKYHOOK_API_KEY) > 1, "API key must be configured");

#define API_LOCATE_PATH		"/wps2/json/location"
#define API_KEY_PARAM		"key="CONFIG_LOCATION_METHOD_WLAN_SERVICE_SKYHOOK_API_KEY
#define REQUEST_URL		API_LOCATE_PATH"?"API_KEY_PARAM"&user=foo" //TODO: fetch imei

#define HEADER_CONTENT_TYPE    "Content-Type: application/json\r\n"
#define HEADER_HOST            "Host: "HOSTNAME"\r\n"
#define HEADER_CONNECTION      "Connection: close\r\n"

#define HTTPS_PORT 443

#define SKYHOOK_WLAN_POS_JSON_KEY_WLAN "wifiAccessPoints"
/******************************************************************************/
/* Based on Skyhook JSON Location API v2.31:
 * https://resources.skyhook.com/wiki/type/documentation/precision-location/api-documentation---json-location-api-v2-31/218036682
 */

static int skyhook_wlan_rest_pos_req_json_format(
	const struct mac_address_info mac_address_arr[],
	uint8_t mac_addr_count,
	cJSON * const req_obj_out)
{
	cJSON *wlan_array = NULL;
	cJSON *wlan_info_obj = NULL;
	cJSON *mac_address_obj = NULL;
	cJSON *false_string_obj = NULL;

	if (!mac_address_arr || !mac_addr_count || !req_obj_out) {
		return -EINVAL;
	}
	/* Request payload format example:
	 * {"considerIp":"false","wifiAccessPoints":[{"macAddress":"06:06:06:06:06:06"},{},...]}
	 */
	false_string_obj = cJSON_CreateString("false");
	if (!false_string_obj) {
		goto cleanup;
	}
	cJSON_AddItemToObject(req_obj_out, "considerIP", false_string_obj);

	wlan_array = cJSON_AddArrayToObjectCS(req_obj_out, SKYHOOK_WLAN_POS_JSON_KEY_WLAN);
	if (!wlan_array) {
		goto cleanup;
	}

	for (size_t i = 0; i < mac_addr_count; ++i) {
		const struct mac_address_info current_mac = mac_address_arr[i];

		wlan_info_obj = cJSON_CreateObject();

		mac_address_obj = cJSON_CreateString(current_mac.mac_addr_str);
		if (!mac_address_obj) {
			goto cleanup;
		}
		cJSON_AddItemToObject(wlan_info_obj, "macAddress", mac_address_obj);

		//TODO: support for others attributes
		if (!cJSON_AddItemToArray(wlan_array, wlan_info_obj)) {
			cJSON_Delete(wlan_info_obj);
			goto cleanup;
		}
	}
	return 0;

cleanup:
	/* Only need to delete the wlan_array since all items (if any) were added to it */
	cJSON_DeleteItemFromObject(req_obj_out, SKYHOOK_WLAN_POS_JSON_KEY_WLAN);
	LOG_ERR("Failed to format Skyhook wlan location request, out of memory");
	return -ENOMEM;
}

static int skyhook_rest_format_wlan_pos_req_body(
	const struct mac_address_info mac_addresses[],
	uint8_t mac_addr_count,
	char **json_str_out)
{
	if (!mac_addresses || !mac_addr_count || !json_str_out) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *req_obj = cJSON_CreateObject();

	err = skyhook_wlan_rest_pos_req_json_format(mac_addresses, mac_addr_count, req_obj);
	if (err) {
		goto cleanup;
	}

	*json_str_out = cJSON_PrintUnformatted(req_obj);
	if (*json_str_out == NULL) {
		err = -ENOMEM;
	}

cleanup:
	cJSON_Delete(req_obj);

	return err;
}

static int skyhook_wlan_rest_pos_response_parse(const char *const buf,
						struct rest_wlan_pos_result *result)
{
	int ret = 0;
	struct cJSON *root_obj, *location_obj, *lat_obj, *lng_obj, *accuracy_obj;

	if (buf == NULL) {
		return -EINVAL;
	}
	/* Response payload format example:
	 * {"location":{"lat":61.49372,"lng":23.775702},"accuracy":50.0}
	 */

	root_obj = cJSON_Parse(buf);
	if (!root_obj) {
		LOG_ERR("No JSON found for skyhook wlan positioning response");
		ret = -ENOMSG;
		goto cleanup;
	}

	location_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "location");
	if (location_obj == NULL) {
		LOG_ERR("No 'location' object found");
		ret = -ENOMSG;
		goto cleanup;
	}

	lat_obj = cJSON_GetObjectItemCaseSensitive(location_obj, "lat");
	if (lat_obj == NULL) {
		LOG_ERR("No 'lat' object found");

		ret = -ENOMSG;
		goto cleanup;
	}

	lng_obj = cJSON_GetObjectItemCaseSensitive(location_obj, "lng");
	if (lng_obj == NULL) {
		LOG_ERR("No 'lng' object found");

		ret = -ENOMSG;
		goto cleanup;
	}

	accuracy_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "accuracy");
	if (accuracy_obj == NULL) {
		LOG_ERR("No 'accuracy' object found");

		ret = -ENOMSG;
		goto cleanup;
	}

	result->latitude = lat_obj->valuedouble;
	result->longitude = lng_obj->valuedouble;
	result->accuracy = accuracy_obj->valuedouble;

cleanup:
	if (ret) {
		LOG_DBG("Unparsed response:\n%s", log_strdup(buf));
	}
	cJSON_Delete(root_obj);
	return ret;
}

/******************************************************************************/

int skyhook_rest_wlan_pos_get(
	const struct rest_wlan_pos_request *request,
	struct rest_wlan_pos_result *result)
{
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG(result != NULL);

	char response_buf[1024]; //TODO: kconfig and not from stack

	struct srest_req_resp_context rest_ctx = { 0 };
	char *const headers[] = {
		HEADER_HOST,
		HEADER_CONTENT_TYPE,
		HEADER_CONNECTION,
		/* Note: Content-length set according to payload */
		NULL
	};
	char *body = NULL;
	int ret = 0;

	/* Set the defaults: */
	srest_client_request_defaults_set(&rest_ctx);
	rest_ctx.http_method = HTTP_POST;
	rest_ctx.url = REQUEST_URL;
	rest_ctx.sec_tag = CONFIG_LOCATION_METHOD_WLAN_SERVICE_SKYHOOK_TLS_SEC_TAG;
	rest_ctx.port = HTTPS_PORT;
	rest_ctx.host = HOSTNAME;
	rest_ctx.header_fields = (const char **)headers;
	rest_ctx.resp_buff = response_buf;
	rest_ctx.resp_buff_len = 1024;

	/* Get the body/payload to request: */
	ret = skyhook_rest_format_wlan_pos_req_body(
		request->mac_addresses,
		request->mac_addr_count,
		&body);
	if (ret) {
		LOG_ERR("Failed to generate skyhook positioning request, err: %d", ret);
		goto clean_up;
	}
	rest_ctx.body = body;

	ret = srest_client_request(&rest_ctx);
	if (ret) {
		LOG_ERR("Error from srest client lib, err: %d", ret);
		goto clean_up;
	}

	if (rest_ctx.http_status_code != SREST_HTTP_STATUS_OK) {
		LOG_ERR("HTTP status: %d", rest_ctx.http_status_code);
		/* Let it fail in parsing */
	}

	ret = skyhook_wlan_rest_pos_response_parse(rest_ctx.response, result);
	if (ret) {
		LOG_ERR("Skyhook rest response parsing failed, err: %d", ret);
		ret = -EBADMSG;
	}
clean_up:
	if (body) {
		cJSON_free(body);
	}
	return ret;
}
