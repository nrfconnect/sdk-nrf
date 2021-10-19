/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <modem/modem_jwt.h>
#include <net/rest_client.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <net/multicell_location.h>

#include "location_service.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(multicell_location_skyhook, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

#define API_KEY		CONFIG_MULTICELL_LOCATION_SKYHOOK_API_KEY
#define HOSTNAME	CONFIG_MULTICELL_LOCATION_SKYHOOK_HOSTNAME

/* The timing advance returned by the nRF9160 modem must be divided by 16
 * to have the range expected by Skyhook.
 */
#define TA_DIVIDER		16
/* Estimated size for current cell information */
#define CURRENT_CELL_SIZE	250
/* Buffer size for neighbor element objects */
#define NEIGHBOR_ELEMENT_SIZE	120
/* Neighbor buffer size that will hold all neighbor cell objects */
#define NEIGHBOR_BUFFER_SIZE	(CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS * NEIGHBOR_ELEMENT_SIZE)
/* Estimated size for HTTP request body */
#define HTTP_BODY_SIZE		(CURRENT_CELL_SIZE + NEIGHBOR_BUFFER_SIZE)

/* URL and query string format:
 * https://global.skyhookwireless.com/wps2/json/location?key=<API KEY>&user=<DEVICE ID>
 */

#define API_LOCATE_PATH		"/wps2/json/location"
#define API_KEY_PARAM		"key="API_KEY
#define REQUEST_URL		API_LOCATE_PATH"?"API_KEY_PARAM"&user=%s"
#define REQUEST_URL_NO_USER	API_LOCATE_PATH"?"API_KEY_PARAM

#define HEADER_CONTENT_TYPE    "Content-Type: application/json\r\n"
#define HEADER_CONNECTION      "Connection: close\r\n"


#define HTTP_REQUEST_BODY                                               \
	"{"								\
		"\"considerIp\": \"false\","				\
		"\"cellTowers\": ["					\
			"{"						\
				"\"radioType\": \"%s\","		\
				"\"mobileCountryCode\": %d,"		\
				"\"mobileNetworkCode\": %d,"		\
				"\"locationAreaCode\": %d,"		\
				"\"cellId\": %d,"			\
				"\"neighborId\": %d,"			\
				"%s" /* Timing advance */		\
				"\"signalStrength\": %d,"		\
				"\"channel\": %d,"			\
				"\"serving\": true"			\
			"},"						\
			"%s"						\
		"]"							\
	"}"

#define HTTP_REQUEST_BODY_NO_NEIGHBORS					\
	"{"								\
		"\"considerIp\": \"false\","				\
		"\"cellTowers\": ["					\
			"{"						\
				"\"radioType\": \"%s\","		\
				"\"mobileCountryCode\": %d,"		\
				"\"mobileNetworkCode\": %d,"		\
				"\"locationAreaCode\": %d,"		\
				"\"cellId\": %d,"			\
				"\"neighborId\": %d,"			\
				"%s" /* Timing advance */		\
				"\"signalStrength\": %d,"		\
				"\"channel\": %d,"			\
				"\"serving\": true"			\
			"}"						\
		"]"							\
	"}"

#define HTTP_REQUEST_BODY_NEIGHBOR_ELEMENT				\
	"{"								\
		"\"radioType\": \"%s\","				\
		"\"neighborId\": %d,"					\
		"\"signalStrength\": %d,"				\
		"\"channel\": %d,"					\
		"\"serving\": false"					\
	"}"

BUILD_ASSERT(sizeof(API_KEY) > 1, "API key must be configured");
BUILD_ASSERT(sizeof(HOSTNAME) > 1, "Hostname must be configured");

static char body[HTTP_BODY_SIZE];
static char neighbors[NEIGHBOR_BUFFER_SIZE];

/* TLS certificate:
 *	DigiCert Global Root CA
 *	CN=DigiCert Global Root CA
 *	O=DigiCert Inc
 *	OU=www.digicert.com
 *	Serial number=08:3B:E0:56:90:42:46:B1:A1:75:6A:C9:59:91:C7:4A
 *	Valid from=10 November 2006
 *	Valid to=10 November 2031
 *	Download url=https://cacerts.digicert.com/DigiCertGlobalRootCA.crt.pem
 */
static const char tls_certificate[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"
	"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
	"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
	"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"
	"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
	"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"
	"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"
	"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"
	"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"
	"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"
	"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"
	"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"
	"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"
	"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"
	"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"
	"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"
	"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"
	"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"
	"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"
	"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"
	"-----END CERTIFICATE-----\n";

const char *location_service_get_certificate_skyhook(void)
{
	return tls_certificate;
}

static int adjust_rsrp(int input)
{
	if (input <= 0) {
		return input - 140;
	}

	return input - 141;
}

int location_service_generate_request(
	const struct lte_lc_cells_info *cell_data,
	char *buf,
	size_t buf_len)
{
	int len;
	enum lte_lc_lte_mode mode;
	int err;
	char timing_advance[30];
	size_t neighbors_to_use =
		MIN(CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS, cell_data->ncells_count);

	if ((cell_data == NULL) || (buf == NULL) || (buf_len == 0)) {
		return -EINVAL;
	}

	if (cell_data->current_cell.id == 0) {
		LOG_WRN("No cells were found");
		return -ENOENT;
	}

	err = lte_lc_lte_mode_get(&mode);
	if (err) {
		LOG_ERR("Failed to get current LTE mode (error %d), fallback to LTE-M", err);
		mode = LTE_LC_LTE_MODE_LTEM;
	}

	if (cell_data->current_cell.timing_advance == UINT16_MAX) {
		timing_advance[0] = '\0';
	} else {
		len = snprintk(timing_advance, sizeof(timing_advance),
			       "\"timing_advance\":%d,",
			       cell_data->current_cell.timing_advance / TA_DIVIDER);
		if ((len < 0) || (len >= sizeof(timing_advance))) {
			LOG_ERR("Too small buffer for timing advance");
			return -ENOMEM;
		}
	}

	if (neighbors_to_use == 0) {
		len = snprintk(buf, buf_len, HTTP_REQUEST_BODY_NO_NEIGHBORS,
			 mode == LTE_LC_LTE_MODE_LTEM ? "lte" : "nbiot",
			 cell_data->current_cell.mcc,
			 cell_data->current_cell.mnc,
			 cell_data->current_cell.tac,
			 cell_data->current_cell.id,
			 cell_data->current_cell.phys_cell_id,
			 timing_advance,
			 adjust_rsrp(cell_data->current_cell.rsrp),
			 cell_data->current_cell.earfcn);
		if ((len < 0) || (len >= buf_len)) {
			LOG_ERR("Too small buffer for HTTP request body");
			return -ENOMEM;
		}

		return 0;
	}

	neighbors[0] = '\0';

	for (size_t i = 0; i < neighbors_to_use; i++) {
		char element[NEIGHBOR_ELEMENT_SIZE];

		len = snprintk(element, sizeof(element) - 1 /* ',' */,
			HTTP_REQUEST_BODY_NEIGHBOR_ELEMENT,
			 mode == LTE_LC_LTE_MODE_LTEM ? "lte" : "nbiot",
			 cell_data->neighbor_cells[i].phys_cell_id,
			 adjust_rsrp(cell_data->neighbor_cells[i].rsrp),
			 cell_data->neighbor_cells[i].earfcn);
		if ((len < 0) || (len >= sizeof(element))) {
			LOG_ERR("Too small buffer for cell element");
			return -ENOMEM;
		}

		if (strlen(element) > (sizeof(neighbors) - (strlen(neighbors) + 1 /* ',' */))) {
			LOG_ERR("Not enough space in neighbor buffer to add cell");
			LOG_WRN("No more cells can be added, using %d of %d cells",
				i, neighbors_to_use);
			break;
		}

		/* Append ',' if this is not the last element in the array */
		if (i < (neighbors_to_use - 1)) {
			strncat(element, ",", sizeof(element) - strlen(element));
		}

		strncat(neighbors, element, sizeof(neighbors) - strlen(neighbors));
	}

	len = snprintk(buf, buf_len, HTTP_REQUEST_BODY,
		 mode == LTE_LC_LTE_MODE_LTEM ? "lte" : "nbiot",
		 cell_data->current_cell.mcc,
		 cell_data->current_cell.mnc,
		 cell_data->current_cell.tac,
		 cell_data->current_cell.id,
		 cell_data->current_cell.phys_cell_id,
		 timing_advance,
		 adjust_rsrp(cell_data->current_cell.rsrp),
		 cell_data->current_cell.earfcn,
		 neighbors);
	if ((len < 0) || (len >= buf_len)) {
		LOG_ERR("Too small buffer for HTTP request body");
		return -ENOMEM;
	}

	return 0;
}

static int location_service_parse_response(
	const char *response,
	struct multicell_location *location)
{
	int err;
	struct cJSON *root_obj, *location_obj, *lat_obj, *lng_obj, *accuracy_obj;
	static bool cjson_is_init;

	if ((response == NULL) || (location == NULL)) {
		return -EINVAL;
	}

	if (!cjson_is_init) {
		cJSON_Init();

		cjson_is_init = true;
	}

	/* The expected body format of the response is the following:
	 *
	 * {"location":{"lat":<double>,"lng":<double>},"accuracy":<double>}
	 */

	root_obj = cJSON_Parse(response);
	if (root_obj == NULL) {
		LOG_ERR("Could not parse JSON in payload");
		return -1;
	}

	location_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "location");
	if (location_obj == NULL) {
		LOG_DBG("No 'location' object found");

		err = -1;
		goto clean_exit;
	}

	lat_obj = cJSON_GetObjectItemCaseSensitive(location_obj, "lat");
	if (lat_obj == NULL) {
		LOG_DBG("No 'lat' object found");

		err = -1;
		goto clean_exit;
	}

	lng_obj = cJSON_GetObjectItemCaseSensitive(location_obj, "lng");
	if (lng_obj == NULL) {
		LOG_DBG("No 'lng' object found");

		err = -1;
		goto clean_exit;
	}

	accuracy_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "accuracy");
	if (accuracy_obj == NULL) {
		LOG_DBG("No 'accuracy' object found");

		err = -1;
		goto clean_exit;
	}

	location->latitude = lat_obj->valuedouble;
	location->longitude = lng_obj->valuedouble;
	location->accuracy = accuracy_obj->valuedouble;

	err = 0;

clean_exit:
	cJSON_Delete(root_obj);

	return err;
}

static int location_service_generate_url(char * const request_url, const size_t request_url_len)
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

int location_service_get_cell_location_skyhook(
	const struct lte_lc_cells_info *cell_data,
	char * const rcv_buf,
	const size_t rcv_buf_len,
	struct multicell_location *const location)
{
	int err;
	/* Reserving NRF_MODEM_FW_UUID_STR_LEN bytes for UUID */
	char request_url[sizeof(REQUEST_URL) + NRF_MODEM_FW_UUID_STR_LEN + 1];
	struct rest_client_req_context req_ctx = { 0 };
	struct rest_client_resp_context resp_ctx = { 0 };
	char *const headers[] = {
		HEADER_CONTENT_TYPE,
		HEADER_CONNECTION,
		/* Note: Content-length set according to payload in HTTP library */
		NULL
	};

	err = location_service_generate_url(request_url, sizeof(request_url));
	if (err) {
		LOG_ERR("location_service_generate_url failed, error: %d", err);
		return err;
	}

	LOG_DBG("Generated request URL:\r\n%s", log_strdup(request_url));

	err = location_service_generate_request(cell_data, body, sizeof(body));
	if (err) {
		LOG_ERR("Failed to generate HTTP request, error: %d", err);
		return err;
	}

	LOG_DBG("Generated request body:\r\n%s", log_strdup(body));

	/* Set the defaults: */
	rest_client_request_defaults_set(&req_ctx);
	req_ctx.http_method = HTTP_POST;
	req_ctx.url = request_url;
	req_ctx.sec_tag = CONFIG_MULTICELL_LOCATION_SKYHOOK_TLS_SEC_TAG;
	req_ctx.port = CONFIG_MULTICELL_LOCATION_SKYHOOK_HTTPS_PORT;
	req_ctx.host = CONFIG_MULTICELL_LOCATION_SKYHOOK_HOSTNAME;
	req_ctx.header_fields = (const char **)headers;
	req_ctx.resp_buff = rcv_buf;
	req_ctx.resp_buff_len = rcv_buf_len;

	/* Get the body/payload to request: */
	req_ctx.body = body;

	err = rest_client_request(&req_ctx, &resp_ctx);
	if (err) {
		LOG_ERR("Error from rest client lib, err: %d", err);
		return err;
	}

	if (resp_ctx.http_status_code != REST_CLIENT_HTTP_STATUS_OK) {
		LOG_ERR("HTTP status: %d", resp_ctx.http_status_code);
		/* Let it fail in parsing */
	}

	LOG_DBG("Received response body:\r\n%s", log_strdup(resp_ctx.response));

	err = location_service_parse_response(resp_ctx.response, location);
	if (err) {
		LOG_ERR("Failed to parse HTTP response");
		return -ENOMSG;
	}
	return err;
}
