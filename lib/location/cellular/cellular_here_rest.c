/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <net/rest_client.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <modem/modem_info.h>

#include "cellular_service.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

/* Here API references:
 * v1: https://developer.here.com/documentation/positioning/api-reference-swagger.html
 * v2: https://developer.here.com/documentation/positioning-api/api-reference.html
 */

#define HOSTNAME	CONFIG_LOCATION_SERVICE_HERE_HOSTNAME

#define API_LOCATE_PATH		"/v2/locate"

BUILD_ASSERT(sizeof(CONFIG_LOCATION_SERVICE_HERE_API_KEY) > 1,
	     "API key must be configured");
#define AUTHENTICATION	"apiKey="CONFIG_LOCATION_SERVICE_HERE_API_KEY

#define REQUEST_URL		API_LOCATE_PATH"?"AUTHENTICATION

#define HEADER_CONTENT_TYPE    "Content-Type: application/json\r\n"
#define HEADER_CONNECTION      "Connection: close\r\n"

/* Estimated size requirements for HTTP header */
#define HTTP_HEADER_SIZE	200

/* Estimated size for current cell information */
#define CURRENT_CELL_SIZE	100
/* Buffer size for neighbor element objects */
#define NEIGHBOR_ELEMENT_SIZE	72
/* Neighbor buffer size that will hold all neighbor cell objects */
#define NEIGHBOR_BUFFER_SIZE	(CONFIG_LTE_NEIGHBOR_CELLS_MAX * NEIGHBOR_ELEMENT_SIZE)
/* Estimated size for HTTP request body */
#define HTTP_BODY_SIZE		(CURRENT_CELL_SIZE + NEIGHBOR_BUFFER_SIZE)
#define HTTPS_PORT              443

#define HTTP_REQUEST_BODY						\
	"{"								\
		"\"lte\":["						\
			"{"						\
				"\"mcc\":%d,"				\
				"\"mnc\":%d,"				\
				"\"cid\":%d,"				\
				"\"tac\":%d,"				\
				"\"rsrp\":%d,"				\
				"\"rsrq\":%.1f,"			\
				"\"ta\":%d,"				\
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
				"\"mcc\":%d,"				\
				"\"mnc\":%d,"				\
				"\"cid\":%d,"				\
				"\"tac\":%d,"				\
				"\"rsrp\":%d,"				\
				"\"rsrq\":%.1f,"			\
				"\"ta\":%d"				\
			"}"						\
		"]"							\
	"}"

#define HTTP_REQUEST_BODY_NEIGHBOR_ELEMENT				\
	"{"								\
		"\"earfcn\":%d,"					\
		"\"pci\":%d,"						\
		"\"rsrp\":%d,"						\
		"\"rsrq\":%.1f"						\
	"}"

BUILD_ASSERT(sizeof(HOSTNAME) > 1, "Hostname must be configured");

static char body[HTTP_BODY_SIZE];
static char neighbors[NEIGHBOR_BUFFER_SIZE];

#define HERE_MIN_RSRP -140
#define HERE_MAX_RSRP -44

static int here_adjust_rsrp(int input)
{
	int return_value;

	return_value = RSRP_IDX_TO_DBM(input);

	if (return_value < HERE_MIN_RSRP) {
		return_value = HERE_MIN_RSRP;
	} else if (return_value > HERE_MAX_RSRP) {
		return_value = HERE_MAX_RSRP;
	}

	return return_value;
}

#define HERE_MIN_RSRQ -19.5
#define HERE_MAX_RSRQ -3

static double here_adjust_rsrq(int input)
{
	double return_value;

	return_value = RSRQ_IDX_TO_DB(input);

	if (return_value < HERE_MIN_RSRQ) {
		return_value = HERE_MIN_RSRQ;
	} else if (return_value > HERE_MAX_RSRQ) {
		return_value = HERE_MAX_RSRQ;
	}

	return return_value;
}

#define HERE_TA_ADJ(ta) (ta / 16)

#define HERE_MIN_TA 0
#define HERE_MAX_TA 1282

static int here_adjust_ta(int input)
{
	int return_value;

	return_value = HERE_TA_ADJ(input);

	if (return_value < HERE_MIN_TA) {
		return_value = HERE_MIN_TA;
	} else if (return_value > HERE_MAX_TA) {
		return_value = HERE_MAX_TA;
	}

	return return_value;
}

static int location_service_generate_request(
	const struct lte_lc_cells_info *cell_data,
	char *buf,
	size_t buf_len)
{
	int len;

	if ((cell_data == NULL) || (buf == NULL) || (buf_len == 0)) {
		return -EINVAL;
	}

	if (cell_data->ncells_count == 0) {
		len = snprintk(buf, buf_len, HTTP_REQUEST_BODY_NO_NEIGHBORS,
			       cell_data->current_cell.mcc,
			       cell_data->current_cell.mnc,
			       cell_data->current_cell.id,
			       cell_data->current_cell.tac,
			       here_adjust_rsrp(cell_data->current_cell.rsrp),
			       here_adjust_rsrq(cell_data->current_cell.rsrq),
			       here_adjust_ta(cell_data->current_cell.timing_advance)
			       );
		if ((len < 0) || (len >= buf_len)) {
			LOG_ERR("Too small buffer for HTTP request body");
			return -ENOMEM;
		}

		return 0;
	}

	neighbors[0] = '\0';

	for (size_t i = 0; i < cell_data->ncells_count; i++) {
		char element[NEIGHBOR_ELEMENT_SIZE];

		len = snprintk(element, sizeof(element) - 1 /* ',' */,
			       HTTP_REQUEST_BODY_NEIGHBOR_ELEMENT,
			       cell_data->neighbor_cells[i].earfcn,
			       cell_data->neighbor_cells[i].phys_cell_id,
			       here_adjust_rsrp(cell_data->neighbor_cells[i].rsrp),
			       here_adjust_rsrq(cell_data->neighbor_cells[i].rsrq)
			       );
		if ((len < 0) || (len >= sizeof(element))) {
			LOG_ERR("Too small buffer for cell element buffer");
			return -ENOMEM;
		}

		if (strlen(element) > (sizeof(neighbors) - strlen(neighbors))) {
			LOG_ERR("Not enough space in neighbor buffer to add cell");
			LOG_WRN("No more cells can be added, using %d of %d cells",
				i, cell_data->ncells_count);
			break;
		}

		/* Append ',' if this is not the last element in the array */
		if (i < (cell_data->ncells_count - 1)) {
			strncat(element, ",", sizeof(element) - strlen(element));
		}

		strncat(neighbors, element, sizeof(neighbors) - strlen(neighbors));
	}

	len = snprintk(buf, buf_len, HTTP_REQUEST_BODY,
		       cell_data->current_cell.mcc,
		       cell_data->current_cell.mnc,
		       cell_data->current_cell.id,
		       cell_data->current_cell.tac,
		       here_adjust_rsrp(cell_data->current_cell.rsrp),
		       here_adjust_rsrq(cell_data->current_cell.rsrq),
		       here_adjust_ta(cell_data->current_cell.timing_advance),
		       neighbors);
	if ((len < 0) || (len >= buf_len)) {
		LOG_ERR("Too small buffer for HTTP request body");
		return -ENOMEM;
	}

	return 0;
}

static int location_service_parse_response(const char *response,
					   struct location_data *location)
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

int cellular_here_rest_pos_get(
	const struct location_cellular_serv_pos_req *params,
	char * const rcv_buf,
	const size_t rcv_buf_len,
	struct location_data *const location)
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

	err = location_service_generate_request(params->cell_data, body, sizeof(body));
	if (err) {
		LOG_ERR("Failed to generate HTTP request, error: %d", err);
		return err;
	}

	LOG_DBG("Generated request body:\r\n%s", body);

	/* Set the defaults: */
	rest_client_request_defaults_set(&req_ctx);
	req_ctx.http_method = HTTP_POST;
	req_ctx.url = REQUEST_URL;
	req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	req_ctx.port = HTTPS_PORT;
	req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;
	req_ctx.header_fields = (const char **)headers;
	req_ctx.resp_buff = rcv_buf;
	req_ctx.resp_buff_len = rcv_buf_len;
	req_ctx.timeout_ms = params->timeout;

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

	LOG_DBG("Received response body:\r\n%s", resp_ctx.response);

	err = location_service_parse_response(resp_ctx.response, location);
	if (err) {
		LOG_ERR("Failed to parse HTTP response");
		return -ENOMSG;
	}
	return err;
}
