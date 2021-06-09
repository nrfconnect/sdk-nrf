/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <modem/lte_lc.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <modem/at_cmd.h>

#include "location_service.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(multicell_location_nrf_cloud, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

#define HOSTNAME CONFIG_MULTICELL_LOCATION_HOSTNAME

/* Length of nrf9160 device IMEI */
#define IMEI_LEN 15

BUILD_ASSERT(sizeof(CONFIG_MULTICELL_LOCATION_NRF_CLOUD_API_KEY) > 1,
	     "API key must be configured");
#define API_KEY CONFIG_MULTICELL_LOCATION_NRF_CLOUD_API_KEY

#define REQUEST_PARAMETERS							\
	"?deviceIdentifier=%s"							\
	"&mcc=%d"								\
	"&mnc=%d"								\
	"&tac=%d"								\
	"&eci=%d"								\
	"&format=json"

#define HTTP_REQUEST_HEADER							\
	"GET /v1/location/single-cell" REQUEST_PARAMETERS " HTTP/1.1\r\n"	\
	"Host: "HOSTNAME"\r\n"							\
	"Authorization: Bearer "API_KEY"\r\n"					\
	"Connection: close\r\n"							\
	"Content-Type: application/json\r\n\r\n"

BUILD_ASSERT(sizeof(HOSTNAME) > 1, "Hostname must be configured");

/* TLS certificate:
 *	CN=Starfield Services Root Certificate Authority - G2
 *	O=Starfield Technologies, Inc.
 *	OU=Starfield Class 2 Certification Authority
 *	Serial number=a7:0e:4a:4c:34:82:b7:7f
 *	Valid from=Sep 2 00:00:00 2009 GMT
 *	Valid to=Jun 28 17:39:16 2034 GMT
 *	Download url=https://www.amazontrust.com/repository/SFSRootCAG2.pem
 */
static const char tls_certificate[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIEdTCCA12gAwIBAgIJAKcOSkw0grd/MA0GCSqGSIb3DQEBCwUAMGgxCzAJBgNV\n"
	"BAYTAlVTMSUwIwYDVQQKExxTdGFyZmllbGQgVGVjaG5vbG9naWVzLCBJbmMuMTIw\n"
	"MAYDVQQLEylTdGFyZmllbGQgQ2xhc3MgMiBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0\n"
	"eTAeFw0wOTA5MDIwMDAwMDBaFw0zNDA2MjgxNzM5MTZaMIGYMQswCQYDVQQGEwJV\n"
	"UzEQMA4GA1UECBMHQXJpem9uYTETMBEGA1UEBxMKU2NvdHRzZGFsZTElMCMGA1UE\n"
	"ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjE7MDkGA1UEAxMyU3RhcmZp\n"
	"ZWxkIFNlcnZpY2VzIFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IC0gRzIwggEi\n"
	"MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDVDDrEKvlO4vW+GZdfjohTsR8/\n"
	"y8+fIBNtKTrID30892t2OGPZNmCom15cAICyL1l/9of5JUOG52kbUpqQ4XHj2C0N\n"
	"Tm/2yEnZtvMaVq4rtnQU68/7JuMauh2WLmo7WJSJR1b/JaCTcFOD2oR0FMNnngRo\n"
	"Ot+OQFodSk7PQ5E751bWAHDLUu57fa4657wx+UX2wmDPE1kCK4DMNEffud6QZW0C\n"
	"zyyRpqbn3oUYSXxmTqM6bam17jQuug0DuDPfR+uxa40l2ZvOgdFFRjKWcIfeAg5J\n"
	"Q4W2bHO7ZOphQazJ1FTfhy/HIrImzJ9ZVGif/L4qL8RVHHVAYBeFAlU5i38FAgMB\n"
	"AAGjgfAwge0wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0O\n"
	"BBYEFJxfAN+qAdcwKziIorhtSpzyEZGDMB8GA1UdIwQYMBaAFL9ft9HO3R+G9FtV\n"
	"rNzXEMIOqYjnME8GCCsGAQUFBwEBBEMwQTAcBggrBgEFBQcwAYYQaHR0cDovL28u\n"
	"c3MyLnVzLzAhBggrBgEFBQcwAoYVaHR0cDovL3guc3MyLnVzL3guY2VyMCYGA1Ud\n"
	"HwQfMB0wG6AZoBeGFWh0dHA6Ly9zLnNzMi51cy9yLmNybDARBgNVHSAECjAIMAYG\n"
	"BFUdIAAwDQYJKoZIhvcNAQELBQADggEBACMd44pXyn3pF3lM8R5V/cxTbj5HD9/G\n"
	"VfKyBDbtgB9TxF00KGu+x1X8Z+rLP3+QsjPNG1gQggL4+C/1E2DUBc7xgQjB3ad1\n"
	"l08YuW3e95ORCLp+QCztweq7dp4zBncdDQh/U90bZKuCJ/Fp1U1ervShw3WnWEQt\n"
	"8jxwmKy6abaVd38PMV4s/KCHOkdp8Hlf9BRUpJVeEXgSYCfOn8J3/yNTd126/+pZ\n"
	"59vPr5KW7ySaNRB6nJHGDn2Z9j8Z3/VyVOEVqQdZe4O/Ui5GjLIAZHYcSNPYeehu\n"
	"VsyuLAOQ1xk4meTKCRlb/weWsKh/NEnfVqn3sF/tM+2MR7cwA130A4w=\n"
	"-----END CERTIFICATE-----\n";

const char *location_service_get_hostname(void)
{
	return HOSTNAME;
}

const char *location_service_get_certificate(void)
{
	return tls_certificate;
}

int location_service_generate_request(const struct lte_lc_cells_info *cell_data,
				      char *buf, size_t buf_len)
{
	int len;
	int err;
	char device_id[20];

	if ((cell_data == NULL) || (buf == NULL) || (buf_len == 0)) {
		return -EINVAL;
	}

	if (cell_data->current_cell.id == 0) {
		LOG_WRN("No cells were found");
		return -ENOENT;
	}

	/* Retrieve device IMEI from modem. */
	err = at_cmd_write("AT+CGSN", device_id, sizeof(device_id), NULL);
	if (err) {
		LOG_ERR("Not able to return device IMEI from modem");
		return err;
	}

	/* Set null character at the end of the device IMEI. */
	device_id[IMEI_LEN] = 0;

	len = snprintk(buf, buf_len, HTTP_REQUEST_HEADER,
		       device_id,
		       cell_data->current_cell.mcc,
		       cell_data->current_cell.mnc,
		       cell_data->current_cell.tac,
		       cell_data->current_cell.id);
	if ((len < 0) || (len >= buf_len)) {
		LOG_ERR("Too small buffer for HTTP request");
		return -ENOMEM;
	}

	return 0;
}

int location_service_parse_response(const char *response, struct multicell_location *location)
{
	int err;
	struct cJSON *root_obj, *lat_obj, *lng_obj, *accuracy_obj;
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
	 * {"lat":<latitude>,"lon":<longitude>,"alt":<altitude>,"uncertainty":<uncertainty>}
	 */

	http_status = strstr(response, "HTTP/1.1 200");
	if (http_status == NULL) {
		LOG_ERR("HTTP status was not \"200\"");
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

	lat_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "lat");
	if (lat_obj == NULL) {
		LOG_DBG("No 'lat' object found");

		err = -1;
		goto clean_exit;
	}

	lng_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "lon");
	if (lng_obj == NULL) {
		LOG_DBG("No 'lon' object found");

		err = -1;
		goto clean_exit;
	}

	accuracy_obj = cJSON_GetObjectItemCaseSensitive(root_obj, "uncertainty");
	if (accuracy_obj == NULL) {
		LOG_DBG("No 'uncertainty' object found");

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
