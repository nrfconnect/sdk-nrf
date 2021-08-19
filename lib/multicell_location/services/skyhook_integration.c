/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <cJSON.h>
#include <cJSON_os.h>

#include "location_service.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(multicell_location_skyhook, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

#define API_KEY		CONFIG_MULTICELL_LOCATION_SKYHOOK_API_KEY
#define HOSTNAME	CONFIG_MULTICELL_LOCATION_HOSTNAME

/* The timing advance returned by the nRF9160 modem must be divided by 16
 * to have the range expected by Skyhook.
 */
#define TA_DIVIDER		16
/* Buffer size for neighbor element objects */
#define NEIGHBOR_ELEMENT_SIZE	100
/* Neighbor buffer size that will hold all neighbor cell objects */
#define NEIGHBOR_BUFFER_SIZE	(CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS * NEIGHBOR_ELEMENT_SIZE)

/* URL and query string format:
 * https://global.skyhookwireless.com/wps2/json/location?key=<API KEY>&user=<DEVICE ID>
 */

#define HTTP_REQUEST_HEADER						\
	"POST /wps2/json/location?key="API_KEY"&user=%s HTTP/1.1\r\n"	\
	"Host: "HOSTNAME"\r\n"					        \
	"Content-Type: application/json\r\n"				\
	"Connection: close\r\n"						\
	"Content-Length: %d\r\n\r\n"

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

BUILD_ASSERT(sizeof(API_KEY) > 1, "API key must be configured");
BUILD_ASSERT(sizeof(HOSTNAME) > 1, "Hostname must be configured");

static char body[1536];
static char neighbors[NEIGHBOR_BUFFER_SIZE];

const char *location_service_get_hostname(void)
{
	return HOSTNAME;
}

const char *location_service_get_certificate(void)
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

int location_service_generate_request(const struct lte_lc_cells_info *cell_data,
				      const char *const device_id,
				      char *buf, size_t buf_len)
{
	int len;
	enum lte_lc_lte_mode mode;
	int err;
	char imei[20];
	char timing_advance[30];
	char *dev_id = device_id ? (char *)device_id : imei;
	size_t neighbors_to_use =
		MIN(CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS, cell_data->ncells_count);

	if ((cell_data == NULL) || (buf == NULL) || (buf_len == 0)) {
		return -EINVAL;
	}

	if (cell_data->current_cell.id == 0) {
		LOG_WRN("No cells were found");
		return -ENOENT;
	}

	err = modem_info_init();
	if (err) {
		LOG_ERR("modem_info_init failed, error: %d", err);
		return err;
	}

	if (device_id == NULL) {
		err = modem_info_string_get(MODEM_INFO_IMEI, imei, sizeof(imei));
		if (err < 0) {
			LOG_ERR("Failed to get IMEI, error: %d", err);
			LOG_WRN("Falling back to uptime as user ID");

			len = snprintk(imei, sizeof(imei), "%d", k_cycle_get_32());
			if ((len < 0) || (len >= sizeof(imei))) {
				LOG_ERR("Too small buffer for IMEI buffer");
				return -ENOMEM;
			}

		} else {
			/* Null-terminate the IMEI. */
			imei[15] = '\0';
		}
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
		len = snprintk(body, sizeof(body), HTTP_REQUEST_BODY_NO_NEIGHBORS,
			 mode == LTE_LC_LTE_MODE_LTEM ? "lte" : "nbiot",
			 cell_data->current_cell.mcc,
			 cell_data->current_cell.mnc,
			 cell_data->current_cell.tac,
			 cell_data->current_cell.id,
			 cell_data->current_cell.phys_cell_id,
			 timing_advance,
			 adjust_rsrp(cell_data->current_cell.rsrp),
			 cell_data->current_cell.earfcn);
		if ((len < 0) || (len >= sizeof(body))) {
			LOG_ERR("Too small buffer for HTTP request body");
			return -ENOMEM;
		}

		len = snprintk(buf, buf_len, HTTP_REQUEST_HEADER "%s", dev_id, strlen(body), body);
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

	len = snprintk(body, sizeof(body), HTTP_REQUEST_BODY,
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
	if ((len < 0) || (len >= sizeof(body))) {
		LOG_ERR("Too small buffer for HTTP request body");
		return -ENOMEM;
	}

	len = snprintk(buf, buf_len, HTTP_REQUEST_HEADER "%s", dev_id, strlen(body), body);
	if ((len < 0) || (len >= buf_len)) {
		LOG_ERR("Too small buffer for HTTP request");
		return -ENOMEM;
	}

	return 0;
}

int location_service_parse_response(const char *response, struct multicell_location *location)
{
	int err;
	struct cJSON *root_obj, *location_obj, *lat_obj, *lng_obj, *accuracy_obj;
	char *json_start, *http_status;
	static bool cjson_is_init;

	if ((response == NULL) || (location == NULL)) {
		return -EINVAL;
	}

	if (!cjson_is_init) {
		cJSON_Init();

		cjson_is_init = true;
	}

	/* The expected response format is the following:
	 *
	 * HTTP/1.1 <HTTP status, 200 OK if successful>
	 * <Additional HTTP header elements>
	 * <\r\n\r\n>
	 * {"location":{"lat":<double>,"lng":<double>},"accuracy":<double>}
	 */

	http_status = strstr(response, "HTTP/1.1 200");
	if (http_status == NULL) {
		LOG_ERR("HTTP status was not 200");
		return -1;
	}

	json_start = strstr(response, "\r\n\r\n");
	if (json_start == NULL) {
		LOG_ERR("No payload found");
		return -1;
	}

	root_obj = cJSON_Parse(json_start);
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
