/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <net/rest_client.h>
#include <cJSON.h>
#include <cJSON_os.h>

#include "location_service.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(multicell_location_polte, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

#define API_KEY		CONFIG_MULTICELL_LOCATION_POLTE_API_TOKEN
#define HOSTNAME	CONFIG_MULTICELL_LOCATION_POLTE_HOSTNAME

/* The timing advance returned by the nRF9160 modem must be divided by 8
 * to have the range expected by Polte.
 */
#define TA_DIVIDER	8

#define CURRENT_CELL_SIZE	110
/* Buffer size for neighbor element objects */
#define NEIGHBOR_ELEMENT_SIZE	16
/* Neighbor buffer size that will hold all neighbor cell objects */
#define NEIGHBOR_BUFFER_SIZE	(CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS * NEIGHBOR_ELEMENT_SIZE)
/* Estimated size for HTTP request body */
#define HTTP_BODY_SIZE		(CURRENT_CELL_SIZE + NEIGHBOR_BUFFER_SIZE)

#define API_LOCATE_PATH		"/api/v1/customer/"CONFIG_MULTICELL_LOCATION_POLTE_CUSTOMER_ID \
				"/locate-core"

#define REQUEST_URL		API_LOCATE_PATH"?excludeLocationMetrics=excludeLocationMetrics"

#define HEADER_CONTENT_TYPE	"Content-Type: application/json\r\n"
#define HEADER_CONNECTION	"Connection: close\r\n"
#define HEADER_AUTHORIZATION	"Authorization: Polte-API " API_KEY "\r\n"

/* TLS certificate:
 *	ISRG Root X1
 *	CN=ISRG Root X1
 *	O=Internet Security Research Group
 *	Serial number=91:2b:08:4a:cf:0c:18:a7:53:f6:d6:2e:25:a7:5f:5a
 *	Valid from=Sep 4 00:00:00 2020 GMT
 *	Valid to=Sep 15 16:00:00 2025 GMT
 *	Download url=https://letsencrypt.org/certs/isrgrootx1.pem
 */
static const char tls_certificate[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIFFjCCAv6gAwIBAgIRAJErCErPDBinU/bWLiWnX1owDQYJKoZIhvcNAQELBQAw\n"
	"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
	"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjAwOTA0MDAwMDAw\n"
	"WhcNMjUwOTE1MTYwMDAwWjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\n"
	"RW5jcnlwdDELMAkGA1UEAxMCUjMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
	"AoIBAQC7AhUozPaglNMPEuyNVZLD+ILxmaZ6QoinXSaqtSu5xUyxr45r+XXIo9cP\n"
	"R5QUVTVXjJ6oojkZ9YI8QqlObvU7wy7bjcCwXPNZOOftz2nwWgsbvsCUJCWH+jdx\n"
	"sxPnHKzhm+/b5DtFUkWWqcFTzjTIUu61ru2P3mBw4qVUq7ZtDpelQDRrK9O8Zutm\n"
	"NHz6a4uPVymZ+DAXXbpyb/uBxa3Shlg9F8fnCbvxK/eG3MHacV3URuPMrSXBiLxg\n"
	"Z3Vms/EY96Jc5lP/Ooi2R6X/ExjqmAl3P51T+c8B5fWmcBcUr2Ok/5mzk53cU6cG\n"
	"/kiFHaFpriV1uxPMUgP17VGhi9sVAgMBAAGjggEIMIIBBDAOBgNVHQ8BAf8EBAMC\n"
	"AYYwHQYDVR0lBBYwFAYIKwYBBQUHAwIGCCsGAQUFBwMBMBIGA1UdEwEB/wQIMAYB\n"
	"Af8CAQAwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYfr52LFMLGMB8GA1UdIwQYMBaA\n"
	"FHm0WeZ7tuXkAXOACIjIGlj26ZtuMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcw\n"
	"AoYWaHR0cDovL3gxLmkubGVuY3Iub3JnLzAnBgNVHR8EIDAeMBygGqAYhhZodHRw\n"
	"Oi8veDEuYy5sZW5jci5vcmcvMCIGA1UdIAQbMBkwCAYGZ4EMAQIBMA0GCysGAQQB\n"
	"gt8TAQEBMA0GCSqGSIb3DQEBCwUAA4ICAQCFyk5HPqP3hUSFvNVneLKYY611TR6W\n"
	"PTNlclQtgaDqw+34IL9fzLdwALduO/ZelN7kIJ+m74uyA+eitRY8kc607TkC53wl\n"
	"ikfmZW4/RvTZ8M6UK+5UzhK8jCdLuMGYL6KvzXGRSgi3yLgjewQtCPkIVz6D2QQz\n"
	"CkcheAmCJ8MqyJu5zlzyZMjAvnnAT45tRAxekrsu94sQ4egdRCnbWSDtY7kh+BIm\n"
	"lJNXoB1lBMEKIq4QDUOXoRgffuDghje1WrG9ML+Hbisq/yFOGwXD9RiX8F6sw6W4\n"
	"avAuvDszue5L3sz85K+EC4Y/wFVDNvZo4TYXao6Z0f+lQKc0t8DQYzk1OXVu8rp2\n"
	"yJMC6alLbBfODALZvYH7n7do1AZls4I9d1P4jnkDrQoxB3UqQ9hVl3LEKQ73xF1O\n"
	"yK5GhDDX8oVfGKF5u+decIsH4YaTw7mP3GFxJSqv3+0lUFJoi5Lc5da149p90Ids\n"
	"hCExroL1+7mryIkXPeFM5TgO9r0rvZaBFOvV2z0gp35Z0+L4WPlbuEjN/lxPFin+\n"
	"HlUjr8gRsI3qfJOQFy/9rKIJR0Y/8Omwt/8oTWgy1mdeHmmjk7j1nYsvC9JSQ6Zv\n"
	"MldlTTKB3zhThV1+XWYp6rjd5JW1zbVWEkLNxE7GJThEUG3szgBVGP7pSWTUTsqX\n"
	"nLRbwHOoq7hHwg==\n"
	"-----END CERTIFICATE-----\n";

BUILD_ASSERT(sizeof(API_KEY) > 1, "API key must be configured");
BUILD_ASSERT(sizeof(HOSTNAME) > 1, "Hostname must be configured");

static bool cjson_is_init;

const char *location_service_get_certificate_polte(void)
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

static int location_service_generate_request(
	const struct lte_lc_cells_info *cell_data,
	char *buf,
	size_t buf_len)
{
	int err;
	cJSON *root_obj, *payload_obj, *earfcn_array, *pcid_array, *rsrp_array;
	size_t neighbors_to_use =
		MIN(CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS, cell_data->ncells_count);

	if ((cell_data == NULL) || (buf == NULL) || (buf_len == 0)) {
		return -EINVAL;
	}

	if (cell_data->current_cell.id == 0) {
		LOG_WRN("No cells were found");
		return -ENOENT;
	}

	if (!cjson_is_init) {
		cJSON_Init();
		cjson_is_init = true;
	}

	root_obj = cJSON_CreateObject();
	if (root_obj == NULL) {
		LOG_ERR("Failed to allocate root object");
		return -ENOMEM;
	}

	payload_obj = cJSON_AddObjectToObject(root_obj, "payload");
	if (payload_obj == NULL) {
		LOG_ERR("Failed to allocate payload object");
		err = -ENOMEM;
		goto cleanup;
	}

	pcid_array = cJSON_AddArrayToObject(payload_obj, "pcid");
	if (pcid_array == NULL) {
		LOG_ERR("Failed to allocate physical cell ID array");
		err = -ENOMEM;
		goto cleanup;
	}

	earfcn_array = cJSON_AddArrayToObject(payload_obj, "earfcn");
	if (earfcn_array == NULL) {
		LOG_ERR("Failed to allocate EARFCN array");
		err = -ENOMEM;
		goto cleanup;
	}

	rsrp_array = cJSON_AddArrayToObject(payload_obj, "rsrp");
	if (rsrp_array == NULL) {
		LOG_ERR("Failed to allocate RSRP array");
		err = -ENOMEM;
		goto cleanup;
	}

	if (!cJSON_AddNumberToObject(payload_obj, "gcid",
		cell_data->current_cell.id) ||
	    !cJSON_AddNumberToObject(payload_obj, "ta",
		(cell_data->current_cell.timing_advance / TA_DIVIDER)) ||
	    !cJSON_AddNumberToObject(payload_obj, "mcc",
		cell_data->current_cell.mcc) ||
	    !cJSON_AddNumberToObject(payload_obj, "mnc",
		cell_data->current_cell.mnc) ||
	    !cJSON_AddNumberToObject(payload_obj, "tac",
		cell_data->current_cell.tac) ||
	    !cJSON_AddItemToArray(pcid_array,
		cJSON_CreateNumber(cell_data->current_cell.phys_cell_id)) ||
	    !cJSON_AddItemToArray(earfcn_array,
		cJSON_CreateNumber(cell_data->current_cell.earfcn)) ||
	    !cJSON_AddItemToArray(rsrp_array,
		cJSON_CreateNumber(adjust_rsrp(cell_data->current_cell.rsrp)))) {
		LOG_ERR("Failed to add current cell information");
		err = -ENOMEM;
		goto cleanup;
	}

	if (cell_data->ncells_count > 0) {
		for (size_t i = 0; i < neighbors_to_use; i++) {
			if (!cJSON_AddItemToArray(pcid_array,
				cJSON_CreateNumber(cell_data->neighbor_cells[i].phys_cell_id)) ||
			    !cJSON_AddItemToArray(earfcn_array,
				cJSON_CreateNumber(cell_data->neighbor_cells[i].earfcn)) ||
			    !cJSON_AddItemToArray(rsrp_array,
				cJSON_CreateNumber(
					adjust_rsrp(cell_data->neighbor_cells[i].rsrp)))) {
				LOG_ERR("Failed to add neighbor cell");
				err = -ENOMEM;
				goto cleanup;
			}
		}
	}

	/* cJSON advises to keep a "safety margin" of 5 bytes in the buffer size. */
	if (cJSON_PrintPreallocated(root_obj, buf, buf_len - 5, 0) == 0) {
		LOG_ERR("Too small buffer for HTTP request body");
		err = -ENOMEM;
		goto cleanup;
	}

	err = 0;

cleanup:
	cJSON_Delete(root_obj);

	return err;
}

int location_service_parse_response(const char *response, struct multicell_location *location)
{
	int err;
	struct cJSON *root_obj, *location_obj, *lat_obj, *lng_obj, *accuracy_obj;

	if ((response == NULL) || (location == NULL)) {
		return -EINVAL;
	}

	if (!cjson_is_init) {
		cJSON_Init();
		cjson_is_init = true;
	}

	/* The expected response format is the following:
	 *	{"location":{"latitude":<double>,"longitude":<double>},"confidence":<integer>}
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

	lat_obj = cJSON_GetObjectItemCaseSensitive(location_obj, "latitude");
	if (lat_obj == NULL) {
		LOG_DBG("No 'latitude' object found");

		err = -1;
		goto clean_exit;
	}

	lng_obj = cJSON_GetObjectItemCaseSensitive(location_obj, "longitude");
	if (lng_obj == NULL) {
		LOG_DBG("No 'longitude' object found");

		err = -1;
		goto clean_exit;
	}

	accuracy_obj = cJSON_GetObjectItemCaseSensitive(location_obj, "confidence");
	if (accuracy_obj == NULL) {
		LOG_DBG("No 'confidence' object found");

		err = -1;
		goto clean_exit;
	}

	location->latitude = lat_obj->valuedouble;
	location->longitude = lng_obj->valuedouble;
	location->accuracy = (double)accuracy_obj->valueint;

	err = 0;

clean_exit:
	cJSON_Delete(root_obj);

	return err;
}

int location_service_get_cell_location_polte(
	const struct lte_lc_cells_info *cell_data,
	char * const rcv_buf,
	const size_t rcv_buf_len,
	struct multicell_location *const location)
{
	int err;
	struct rest_client_req_context req_ctx = { 0 };
	struct rest_client_resp_context resp_ctx = { 0 };
	char *const headers[] = {
		HEADER_CONTENT_TYPE,
		HEADER_CONNECTION,
		HEADER_AUTHORIZATION,
		/* Note: Content-length set according to payload in HTTP library */
		NULL
	};
	static char body[HTTP_BODY_SIZE];

	err = location_service_generate_request(cell_data, body, sizeof(body));
	if (err) {
		LOG_ERR("Failed to generate HTTP request, error: %d", err);
		return err;
	}

	LOG_DBG("Generated request body:\r\n%s", log_strdup(body));

	/* Set the defaults: */
	rest_client_request_defaults_set(&req_ctx);
	req_ctx.http_method = HTTP_POST;
	req_ctx.url = REQUEST_URL;
	req_ctx.sec_tag = CONFIG_MULTICELL_LOCATION_POLTE_TLS_SEC_TAG;
	req_ctx.port = CONFIG_MULTICELL_LOCATION_POLTE_HTTPS_PORT;
	req_ctx.host = CONFIG_MULTICELL_LOCATION_POLTE_HOSTNAME;
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
