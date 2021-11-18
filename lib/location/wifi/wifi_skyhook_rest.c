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

#include <modem/modem_jwt.h>
#include <net/rest_client.h>

#include "wifi_skyhook_rest.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#define HOSTNAME	CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK_HOSTNAME

BUILD_ASSERT(sizeof(HOSTNAME) > 1, "Hostname must be configured");
BUILD_ASSERT(sizeof(CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK_API_KEY) > 1,
	     "API key must be configured");

#define API_LOCATE_PATH		"/wps2/json/location"
#define API_KEY_PARAM		"key="CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK_API_KEY

#define REQUEST_URL		API_LOCATE_PATH"?"API_KEY_PARAM"&user=%s"
#define REQUEST_URL_NO_USER	API_LOCATE_PATH"?"API_KEY_PARAM

#define HEADER_CONTENT_TYPE    "Content-Type: application/json\r\n"

#define HTTPS_PORT 443

#define SKYHOOK_WIFI_POS_JSON_KEY_WIFI_AP "wifiAccessPoints"

/******************************************************************************/
/* Based on Skyhook JSON Location API v2.31:
 * https://resources.skyhook.com/wiki/type/documentation/precision-location/api-documentation---json-location-api-v2-31/218036682
 */

static int wifi_skyhook_rest_pos_req_json_format(
	const struct wifi_scanning_result_info scanning_results[],
	uint8_t wifi_scanning_result_count,
	cJSON * const req_obj_out)
{
	cJSON *wifi_info_array = NULL;
	cJSON *wifi_info_obj = NULL;

	if (!scanning_results || !wifi_scanning_result_count || !req_obj_out) {
		return -EINVAL;
	}
	/* Request payload format example:
	 * {"considerIp":"false","wifiAccessPoints":[{"macAddress":"06:06:06:06:06:06"},{},...]}
	 */
	if (!cJSON_AddStringToObjectCS(req_obj_out, "considerIP", "false")) {
		goto cleanup;
	}

	wifi_info_array = cJSON_AddArrayToObjectCS(req_obj_out, SKYHOOK_WIFI_POS_JSON_KEY_WIFI_AP);
	if (!wifi_info_array) {
		goto cleanup;
	}

	for (size_t i = 0; i < wifi_scanning_result_count; ++i) {
		const struct wifi_scanning_result_info scan_result = scanning_results[i];

		wifi_info_obj = cJSON_CreateObject();

		if (!cJSON_AddItemToArray(wifi_info_array, wifi_info_obj)) {
			cJSON_Delete(wifi_info_obj);
			goto cleanup;
		}

		if (!cJSON_AddStringToObjectCS(wifi_info_obj, "macAddress",
					       scan_result.mac_addr_str)) {
			goto cleanup;
		}

		if (!cJSON_AddStringToObjectCS(wifi_info_obj, "ssid", scan_result.ssid_str)) {
			goto cleanup;
		}

		if (!cJSON_AddNumberToObjectCS(wifi_info_obj, "channel", scan_result.channel)) {
			goto cleanup;
		}

		if (!cJSON_AddNumberToObjectCS(wifi_info_obj, "rssi", scan_result.rssi)) {
			goto cleanup;
		}
	}
	return 0;

cleanup:
	/* Only need to delete the wifi_info_array since all items (if any) were added to it */
	cJSON_DeleteItemFromObject(req_obj_out, SKYHOOK_WIFI_POS_JSON_KEY_WIFI_AP);
	LOG_ERR("Failed to format Skyhook Wi-Fi location request, out of memory");
	return -ENOMEM;
}

static int wifi_skyhook_rest_format_pos_req_body(
	const struct wifi_scanning_result_info scanning_results[],
	uint8_t wifi_scanning_result_count,
	char **json_str_out)
{
	if (!scanning_results || !wifi_scanning_result_count || !json_str_out) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *req_obj = cJSON_CreateObject();

	err = wifi_skyhook_rest_pos_req_json_format(scanning_results, wifi_scanning_result_count,
						    req_obj);
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

static int wifi_skyhook_rest_pos_response_parse(const char *const buf,
						struct location_data *result)
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
		LOG_ERR("No JSON found for skyhook Wi-Fi positioning response");
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

static int wifi_skyhook_rest_generate_url(char * const request_url, const size_t request_url_len)
{
	int err;
	int len;
	struct nrf_modem_fw_uuid mfw_uuid;

	err = modem_jwt_get_uuids(NULL, &mfw_uuid);
	if (err) {
		LOG_WRN("modem_jwt_get_uuids failed (error: %d), not setting UUID as user id", err);
		len = snprintk(request_url, request_url_len, REQUEST_URL_NO_USER);
	} else {
		len = snprintk(request_url, request_url_len, REQUEST_URL, mfw_uuid.str);
	}

	if ((len < 0) || (len >= request_url_len)) {
		LOG_ERR("Too small buffer for HTTP request URL");
		return -ENOMEM;
	}

	return 0;
}

/******************************************************************************/

int wifi_skyhook_rest_pos_get(
	char *rcv_buf,
	size_t rcv_buf_len,
	const struct rest_wifi_pos_request *request,
	struct location_data *result)
{
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG(result != NULL);
	__ASSERT_NO_MSG(rcv_buf != NULL);
	__ASSERT_NO_MSG(rcv_buf_len > 0);

	struct rest_client_req_context req_ctx = { 0 };
	struct rest_client_resp_context resp_ctx = { 0 };
	/* Reserving NRF_MODEM_FW_UUID_STR_LEN bytes for UUID */
	char request_url[sizeof(REQUEST_URL) + NRF_MODEM_FW_UUID_STR_LEN + 1];
	char *const headers[] = {
		HEADER_CONTENT_TYPE,
		/* Note: Host and Content-length set by http_client */
		NULL
	};
	char *body = NULL;
	int ret = 0;

	/* Generate the URL for the request */
	ret = wifi_skyhook_rest_generate_url(request_url, sizeof(request_url));
	if (ret) {
		LOG_ERR("wifi_skyhook_rest_generate_url failed, error: %d", ret);
		return ret;
	}
	req_ctx.url = request_url;

	/* Generate the body/payload to request */
	ret = wifi_skyhook_rest_format_pos_req_body(
		request->scanning_results,
		request->wifi_scanning_result_count,
		&body);
	if (ret) {
		LOG_ERR("Failed to generate skyhook positioning request, err: %d", ret);
		goto clean_up;
	}
	req_ctx.body = body;

	/* Set the defaults */
	rest_client_request_defaults_set(&req_ctx);
	req_ctx.http_method = HTTP_POST;
	req_ctx.sec_tag = CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK_TLS_SEC_TAG;
	req_ctx.port = HTTPS_PORT;
	req_ctx.host = HOSTNAME;
	req_ctx.header_fields = (const char **)headers;
	req_ctx.resp_buff = rcv_buf;
	req_ctx.resp_buff_len = rcv_buf_len;
	req_ctx.timeout_ms = request->timeout_ms;

	ret = rest_client_request(&req_ctx, &resp_ctx);
	if (ret) {
		LOG_ERR("Error from rest_client lib, err: %d", ret);
		goto clean_up;
	}

	if (resp_ctx.http_status_code != REST_CLIENT_HTTP_STATUS_OK) {
		LOG_ERR("HTTP status: %d", resp_ctx.http_status_code);
		/* Let it fail in parsing */
	}

	ret = wifi_skyhook_rest_pos_response_parse(resp_ctx.response, result);
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
