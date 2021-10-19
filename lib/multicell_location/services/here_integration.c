/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <modem/lte_lc.h>
#include <net/rest_client.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <net/multicell_location.h>

#include "location_service.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(multicell_location_here, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

#define HOSTNAME	CONFIG_MULTICELL_LOCATION_HERE_HOSTNAME

/* Estimated size requirements for HTTP header */
#define HTTP_HEADER_SIZE	200
/* Estimated size for current cell information */
#define CURRENT_CELL_SIZE	100
/* Buffer size for neighbor element objects */
#define NEIGHBOR_ELEMENT_SIZE	36
/* Neighbor buffer size that will hold all neighbor cell objects */
#define NEIGHBOR_BUFFER_SIZE	(CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS * NEIGHBOR_ELEMENT_SIZE)
/* Estimated size for HTTP request body */
#define HTTP_BODY_SIZE		(CURRENT_CELL_SIZE + NEIGHBOR_BUFFER_SIZE)

#if IS_ENABLED(CONFIG_MULTICELL_LOCATION_HERE_V1)
#define API_LOCATE_PATH		"/positioning/v1/locate"
#elif IS_ENABLED(CONFIG_MULTICELL_LOCATION_HERE_V2)
#define API_LOCATE_PATH		"/v2/locate"
#endif

#if IS_ENABLED(CONFIG_MULTICELL_LOCATION_HERE_USE_API_KEY)
BUILD_ASSERT(sizeof(CONFIG_MULTICELL_LOCATION_HERE_API_KEY) > 1, "API key must be configured");
#define AUTHENTICATION	"apiKey="CONFIG_MULTICELL_LOCATION_HERE_API_KEY
#elif IS_ENABLED(CONFIG_MULTICELL_LOCATION_HERE_USE_APP_CODE_ID)
BUILD_ASSERT(sizeof(API_APP_CODE) > 1, "App code must be configured");
BUILD_ASSERT(sizeof(API_APP_ID) > 1, "App ID must be configured");
#define AUTHENTICATION							\
	"app_code="CONFIG_MULTICELL_LOCATION_HERE_APP_CODE		\
	"&app_id="CONFIG_MULTICELL_LOCATION_HERE_APP_ID
#endif

#define REQUEST_URL		API_LOCATE_PATH"?"AUTHENTICATION

#define HEADER_CONTENT_TYPE    "Content-Type: application/json\r\n"
#define HEADER_CONNECTION      "Connection: close\r\n"

#define HTTP_REQUEST_BODY						\
	"{"								\
		"\"lte\":["						\
			"{"						\
				"\"mcc\": %d,"				\
				"\"mnc\": %d,"				\
				"\"cid\": %d,"				\
				"\"nmr\":["				\
					"%s"				\
				"]"					\
			"}"						\
		"]"							\
	"}"

#define HTTP_REQUEST_BODY_NO_NEIGHBORS					\
	"{"								\
		"\"lte\":["						\
			"{"						\
				"\"mcc\": %d,"				\
				"\"mnc\": %d,"				\
				"\"cid\": %d"				\
			"}"						\
		"]"							\
	"}"

#define HTTP_REQUEST_BODY_NEIGHBOR_ELEMENT				\
	"{"								\
		"\"earfcn\": %d,"						\
		"\"pci\": %d"						\
	"}"

BUILD_ASSERT(sizeof(HOSTNAME) > 1, "Hostname must be configured");

static char body[HTTP_BODY_SIZE];
static char neighbors[NEIGHBOR_BUFFER_SIZE];

/* TLS certificate:
 *	GlobalSign Root CA - R3
 *	CN=GlobalSign
 *	O=GlobalSign
 *	OU=GlobalSign Root CA - R3
 *	Serial number=04:00:00:00:00:01:21:58:53:08:A2
 *	Valid from=18 March 2009
 *	Valid to=18 March 2029
 *	Download url=https://secure.globalsign.com/cacert/root-r3.crt
 */
static const char tls_certificate[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\n"
	"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\n"
	"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\n"
	"MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\n"
	"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
	"hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\n"
	"RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\n"
	"gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\n"
	"KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\n"
	"QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\n"
	"XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\n"
	"DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\n"
	"LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\n"
	"RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\n"
	"jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\n"
	"6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\n"
	"mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\n"
	"Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\n"
	"WD9f\n"
	"-----END CERTIFICATE-----\n";

const char *location_service_get_certificate_here(void)
{
	return tls_certificate;
}

static int location_service_generate_request(
	const struct lte_lc_cells_info *cell_data,
	char *buf,
	size_t buf_len)
{
	int len;
	size_t neighbors_to_use =
		MIN(CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS, cell_data->ncells_count);

	if ((cell_data == NULL) || (buf == NULL) || (buf_len == 0)) {
		return -EINVAL;
	}

	if (neighbors_to_use == 0) {
		len = snprintk(buf, buf_len, HTTP_REQUEST_BODY_NO_NEIGHBORS,
			       cell_data->current_cell.mcc,
			       cell_data->current_cell.mnc,
			       cell_data->current_cell.id);
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
			       cell_data->neighbor_cells[i].earfcn,
			       cell_data->neighbor_cells[i].phys_cell_id);
		if ((len < 0) || (len >= sizeof(element))) {
			LOG_ERR("Too small buffer for cell element buffer");
			return -ENOMEM;
		}

		if (strlen(element) > (sizeof(neighbors) - strlen(neighbors))) {
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
		       cell_data->current_cell.mcc,
		       cell_data->current_cell.mnc,
		       cell_data->current_cell.id,
		       neighbors);
	if ((len < 0) || (len >= buf_len)) {
		LOG_ERR("Too small buffer for HTTP request body");
		return -ENOMEM;
	}

	return 0;
}

static int location_service_parse_response(const char *response,
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
	 * {"location":{"lat":<latitude>,"lng":<longitude>,"accuracy":<accuracy>}}
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

	accuracy_obj = cJSON_GetObjectItemCaseSensitive(location_obj, "accuracy");
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

int location_service_get_cell_location_here(
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
		/* Note: Content-length set according to payload in HTTP library */
		NULL
	};

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
	req_ctx.sec_tag = CONFIG_MULTICELL_LOCATION_HERE_TLS_SEC_TAG;
	req_ctx.port = CONFIG_MULTICELL_LOCATION_HERE_HTTPS_PORT;
	req_ctx.host = CONFIG_MULTICELL_LOCATION_HERE_HOSTNAME;
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
