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

#include <net/rest_client.h>
#include <net/nrf_cloud.h>

#include "rest_nrf_cloud_wifi.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#define HOSTNAME CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD_HOSTNAME

BUILD_ASSERT(sizeof(HOSTNAME) > 1, "Hostname must be configured");

#define REQUEST_URL "/v1/location/wifi"

#define HEADER_CONTENT_TYPE "Content-Type: application/json\r\n"
#define HEADER_HOST "Host: " HOSTNAME "\r\n"
#define HEADER_CONNECTION "Connection: close\r\n"

#define HTTPS_PORT 443

#define NRF_CLOUD_WIFI_POS_JSON_KEY_AP "accessPoints"

static char jwt_buf[1024];

/******************************************************************************/

static int
nrf_cloud_wifi_rest_pos_req_json_format(const struct wifi_scanning_result_info scanning_results[],
					uint8_t wifi_scanning_result_count,
					cJSON *const req_obj_out)
{
	cJSON *wifi_info_array = NULL;
	cJSON *wifi_info_obj = NULL;

	if (!scanning_results || !wifi_scanning_result_count || !req_obj_out) {
		return -EINVAL;
	}
	/* Request payload format example:
	 * {"accessPoints":[{"macAddress": "24-5a-4c-6b-9e-11" },{"macAddress": "6c:55:e8:9b:84:6d"}]}
	 */

	wifi_info_array = cJSON_AddArrayToObjectCS(req_obj_out, NRF_CLOUD_WIFI_POS_JSON_KEY_AP);
	if (!wifi_info_array) {
		goto cleanup;
	}

	for (size_t i = 0; i < wifi_scanning_result_count; ++i) {
		const struct wifi_scanning_result_info scan_result = scanning_results[i];

		wifi_info_obj = cJSON_CreateObject();

		if (!cJSON_AddStringToObjectCS(wifi_info_obj, "macAddress",
					       scan_result.mac_addr_str)) {
			goto cleanup;
		}

		if (!cJSON_AddStringToObjectCS(wifi_info_obj, "ssid", scan_result.ssid_str)) {
			goto cleanup;
		}

		if (!cJSON_AddNumberToObjectCS(wifi_info_obj, "signalStrength", scan_result.rssi)) {
			goto cleanup;
		}

		if (!cJSON_AddNumberToObjectCS(wifi_info_obj, "channel", scan_result.channel)) {
			goto cleanup;
		}

		if (!cJSON_AddItemToArray(wifi_info_array, wifi_info_obj)) {
			goto cleanup;
		}
	}
	return 0;

cleanup:
	cJSON_Delete(wifi_info_obj);
	cJSON_DeleteItemFromObject(req_obj_out, NRF_CLOUD_WIFI_POS_JSON_KEY_AP);
	LOG_ERR("Failed to format nRF Cloud wifi location request, out of memory");
	return -ENOMEM;
}

static int
nrf_cloud_rest_format_wifi_pos_req_body(const struct wifi_scanning_result_info scanning_results[],
					uint8_t wifi_scanning_result_count, char **json_str_out)
{
	if (!scanning_results || !wifi_scanning_result_count || !json_str_out) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *req_obj = cJSON_CreateObject();

	err = nrf_cloud_wifi_rest_pos_req_json_format(scanning_results, wifi_scanning_result_count,
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

static int nrf_cloud_wifi_rest_pos_response_parse(const char *const buf,
						  struct rest_wifi_pos_result *result)
{
	int ret = 0;
	struct cJSON *root_obj, *lat_obj, *lon_obj, *uncertainty_obj;

	if (buf == NULL) {
		return -EINVAL;
	}
	/* Response payload format example:
	 * {"lat":45.52364882,"lon":-122.68331772,"uncertainty":196}
	 */

	root_obj = cJSON_Parse(buf);
	if (!root_obj) {
		LOG_ERR("No JSON found for skyhook wifi positioning response");
		ret = -ENOMSG;
		goto cleanup;
	}

	lat_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "lat");
	if (lat_obj == NULL) {
		LOG_ERR("No 'lat' object found");
		ret = -ENOMSG;
		goto cleanup;
	}

	lon_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "lon");
	if (lon_obj == NULL) {
		LOG_ERR("No 'lon' object found");
		ret = -ENOMSG;
		goto cleanup;
	}

	uncertainty_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "uncertainty");
	if (uncertainty_obj == NULL) {
		LOG_ERR("No 'uncertainty' object found");

		ret = -ENOMSG;
		goto cleanup;
	}

	result->latitude = lat_obj->valuedouble;
	result->longitude = lon_obj->valuedouble;
	result->accuracy = uncertainty_obj->valuedouble;

cleanup:
	if (ret) {
		LOG_DBG("Unparsed response:\n%s", log_strdup(buf));
	}
	cJSON_Delete(root_obj);
	return ret;
}

/******************************************************************************/
#define AUTH_HDR_BEARER_TEMPLATE "Authorization: Bearer %s\r\n"

static int nrf_cloud_rest_wifi_generate_auth_header(const char *const tok, char **auth_hdr_out)
{
	if (!tok || !auth_hdr_out) {
		return -EINVAL;
	}

	int ret;
	size_t buff_size = sizeof(AUTH_HDR_BEARER_TEMPLATE) + strlen(tok);

	*auth_hdr_out = k_calloc(buff_size, 1);
	if (!*auth_hdr_out) {
		return -ENOMEM;
	}
	ret = snprintk(*auth_hdr_out, buff_size, AUTH_HDR_BEARER_TEMPLATE, tok);
	if (ret < 0 || ret >= buff_size) {
		k_free(*auth_hdr_out);
		*auth_hdr_out = NULL;
		return -ETXTBSY;
	}

	return 0;
}

/******************************************************************************/

int nrf_cloud_rest_wifi_pos_get(char *rcv_buf, size_t rcv_buf_len,
				const struct rest_wifi_pos_request *request,
				struct rest_wifi_pos_result *result)
{
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG(result != NULL);
	__ASSERT_NO_MSG(rcv_buf != NULL);
	__ASSERT_NO_MSG(rcv_buf_len > 0);

	struct rest_client_req_resp_context rest_ctx = { 0 };
	char *body = NULL;
	char *auth_hdr = NULL;
	char *jwt_str = NULL;
	int ret = 0;

	ret = nrf_cloud_jwt_generate(0, jwt_buf, sizeof(jwt_buf));
	if (ret) {
		LOG_ERR("Failed to generate JWT, error: %d", ret);
		return ret;
	}
	jwt_str = jwt_buf;

	/* Format auth header */
	ret = nrf_cloud_rest_wifi_generate_auth_header(jwt_str, &auth_hdr);
	if (ret) {
		LOG_ERR("Could not format HTTP auth header, err: %d", ret);
		goto clean_up;
	}
	char *const headers[] = { HEADER_HOST, HEADER_CONTENT_TYPE, HEADER_CONNECTION,
				  (char *const)auth_hdr,
				  /* Note: Content-length set according to payload */
				  NULL };

	/* Set the defaults: */
	rest_client_request_defaults_set(&rest_ctx);
	rest_ctx.http_method = HTTP_POST;
	rest_ctx.url = REQUEST_URL;
	rest_ctx.sec_tag = CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD_SEC_TAG;
	rest_ctx.port = HTTPS_PORT;
	rest_ctx.host = HOSTNAME;
	rest_ctx.header_fields = (const char **)headers;
	rest_ctx.resp_buff = rcv_buf;
	rest_ctx.resp_buff_len = rcv_buf_len;

	/* Get the body/payload to request: */
	ret = nrf_cloud_rest_format_wifi_pos_req_body(request->scanning_results,
						      request->wifi_scanning_result_count, &body);
	if (ret) {
		LOG_ERR("Failed to generate nrf cloud positioning request, err: %d", ret);
		goto clean_up;
	}
	rest_ctx.body = body;

	ret = rest_client_request(&rest_ctx);
	if (ret) {
		LOG_ERR("Error from rest_client lib, err: %d", ret);
		goto clean_up;
	}

	if (rest_ctx.http_status_code != REST_CLIENT_HTTP_STATUS_OK) {
		LOG_ERR("HTTP status: %d", rest_ctx.http_status_code);
		/* Let it fail in parsing */
	}

	ret = nrf_cloud_wifi_rest_pos_response_parse(rest_ctx.response, result);
	if (ret) {
		LOG_ERR("nRF Cloud rest response parsing failed, err: %d", ret);
		ret = -EBADMSG;
	}
clean_up:
	if (body) {
		cJSON_free(body);
	}
	return ret;
}
