/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <date_time.h>
#include <dk_buttons_and_leds.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>
#if defined(CONFIG_NRF_CLOUD_AGNSS)
#include <net/nrf_cloud_agnss.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#include <net/nrf_cloud_coap.h>
#include "nrf_cloud_coap_transport.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_client_id.h"
#include "coap_codec.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf_cloud_coap, CONFIG_NRF_CLOUD_COAP_LOG_LEVEL);

#define COAP_AGNSS_RSC "loc/agnss"
#define COAP_PGPS_RSC "loc/pgps"
#define COAP_GND_FIX_RSC "loc/ground-fix"
#define COAP_FOTA_GET_RSC "fota/exec/current"
#define COAP_SHDW_RSC "state"
#define COAP_SHDW_REP_RSC "state/reported"
#define COAP_SHDW_DES_RSC "state/desired"
#define COAP_D2C_RSC "msg/d2c"
#define COAP_D2C_BULK_RSC COAP_D2C_RSC "/bulk"
#define COAP_D2C_RAW_RSC COAP_D2C_RSC "/raw"
#define COAP_D2C_BIN_RSC COAP_D2C_RSC "/bin"

#define MAX_COAP_PAYLOAD_SIZE (CONFIG_COAP_CLIENT_BLOCK_SIZE - \
			       CONFIG_COAP_CLIENT_MESSAGE_HEADER_SIZE)

/* Semaphore to guard CoAP client callback data/error codes for GET and FETCH operations.
 * A single semaphore is sufficient for all public functions since nrf_cloud_coap_transport
 * only allows one transfer at a time.
 */
static K_SEM_DEFINE(coap_transfer_sem, 1, 1);

static int64_t get_ts(void)
{
	int64_t ts;
	int err;

	err = date_time_now(&ts);
	if (err) {
		LOG_ERR("Error getting time: %d", err);
		ts = 0;
	}
	return ts;
}

#if defined(CONFIG_NRF_CLOUD_AGNSS)
static int agnss_err;

static void get_agnss_callback(const struct coap_client_response_data *data, void *user_data)
{
	struct nrf_cloud_rest_agnss_result *result = user_data;

	if (!result) {
		LOG_ERR("Cannot process result");
		agnss_err = -EINVAL;
		return;
	}
	if (data->result_code != COAP_RESPONSE_CODE_CONTENT) {
		agnss_err = data->result_code;
		if (data->payload_len) {
			LOG_ERR("Unexpected response: %.*s", data->payload_len, data->payload);
		}
		return;
	}
	if ((data->offset + data->payload_len) <= result->buf_sz && result->buf && data->payload) {
		memcpy(&result->buf[data->offset], data->payload, data->payload_len);
		result->agnss_sz += data->payload_len;
		agnss_err = 0;
	} else if (agnss_err != -ENOBUFS) {
		LOG_ERR("Received A-GNSS data cannot fit in result buffer");
		agnss_err = -ENOBUFS;
	}
}

int nrf_cloud_coap_agnss_data_get(struct nrf_cloud_rest_agnss_request const *const request,
				 struct nrf_cloud_rest_agnss_result *result)
{
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG(result != NULL);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}

	static uint8_t buffer[AGNSS_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buffer);
	int err;

	/* Take the semaphore before modifying the static buffer */
	(void)k_sem_take(&coap_transfer_sem, K_FOREVER);

	err = coap_codec_agnss_encode(request, buffer, &len,
				     COAP_CONTENT_FORMAT_APP_CBOR);
	if (err) {
		LOG_ERR("Unable to encode A-GNSS request: %d", err);
		goto give_and_return;
	}

	result->agnss_sz = 0;

	agnss_err = 0;

	err = nrf_cloud_coap_fetch(COAP_AGNSS_RSC, NULL,
				   buffer, len, COAP_CONTENT_FORMAT_APP_CBOR,
				   COAP_CONTENT_FORMAT_APP_CBOR, true, get_agnss_callback,
				   result);

	/* Check for CoAP transfer errors */
	if (err == -EAGAIN) {
		LOG_ERR("Timeout waiting for A-GNSS data");
		goto give_and_return;
	} else if (err) {
		LOG_ERR("CoAP client error: %d", err);
		goto give_and_return;
	}

	/* Use the callback error as the return value */
	err = agnss_err;

	/* Check for callback errors */
	if (agnss_err > 0) {
		LOG_RESULT_CODE_ERR("Unexpected A-GNSS result code:", agnss_err);
	} else if (agnss_err < 0) {
		LOG_ERR("A-GNSS response processing error: %d", agnss_err);
	} else {
		/* No callback error: success */
		LOG_DBG("Got A-GNSS data");
	}

give_and_return:
	k_sem_give(&coap_transfer_sem);
	return err;
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
static int pgps_err;

static void get_pgps_callback(const struct coap_client_response_data *data, void *user)
{
	if (data->result_code != COAP_RESPONSE_CODE_CONTENT) {
		pgps_err = data->result_code;
		if (data->payload_len) {
			LOG_ERR("Unexpected response: %.*s", data->payload_len, data->payload);
		}
	} else {
		pgps_err = coap_codec_pgps_resp_decode(user, data->payload, data->payload_len,
						       COAP_CONTENT_FORMAT_APP_CBOR);
	}
}

int nrf_cloud_coap_pgps_url_get(struct nrf_cloud_rest_pgps_request const *const request,
				struct nrf_cloud_pgps_result *file_location)
{
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG(file_location != NULL);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}

	static uint8_t buffer[PGPS_URL_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buffer);
	int err;

	/* Take the semaphore before modifying the static buffer */
	(void)k_sem_take(&coap_transfer_sem, K_FOREVER);

	err = coap_codec_pgps_encode(request, buffer, &len,
				     COAP_CONTENT_FORMAT_APP_CBOR);
	if (err) {
		LOG_ERR("Unable to encode P-GPS request: %d", err);
		goto give_and_return;
	}

	pgps_err = 0;

	err = nrf_cloud_coap_fetch(COAP_PGPS_RSC, NULL,
				   buffer, len, COAP_CONTENT_FORMAT_APP_CBOR,
				   COAP_CONTENT_FORMAT_APP_CBOR, true, get_pgps_callback,
				   file_location);

	/* Check for CoAP transfer errors */
	if (err == -EAGAIN) {
		LOG_ERR("Timeout waiting for P-GPS URL");
		goto give_and_return;
	} else if (err) {
		LOG_ERR("CoAP client error: %d", err);
		goto give_and_return;
	}

	/* Use the callback error as the return value */
	err = pgps_err;

	/* Check for callback errors */
	if (pgps_err > 0) {
		LOG_RESULT_CODE_ERR("Error getting P-GPS URL; result code:", pgps_err);
	} else if (pgps_err < 0) {
		LOG_ERR("P-GPS response decoding error: %d", pgps_err);
	} else {
		/* No callback error: success */
		LOG_DBG("Got P-GPS URL");
	}

give_and_return:
	k_sem_give(&coap_transfer_sem);
	return err;
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

int nrf_cloud_coap_bytes_send(uint8_t *buf, size_t buf_len, bool confirmable)
{
	int err = 0;

	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}

	err = nrf_cloud_coap_post(COAP_D2C_RAW_RSC, NULL, buf, buf_len,
				  COAP_CONTENT_FORMAT_APP_OCTET_STREAM, confirmable, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send POST request: %d", err);
	}
	return err;
}

int nrf_cloud_coap_bin_log_send(const uint8_t * const buf, size_t buf_len, bool confirmable)
{
	int err = 0;

	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}

	err = nrf_cloud_coap_post(COAP_D2C_BIN_RSC, NULL, buf, buf_len,
				  COAP_CONTENT_FORMAT_APP_OCTET_STREAM, confirmable, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send POST request: %d", err);
	}
	return err;
}

int nrf_cloud_coap_obj_send(struct nrf_cloud_obj *const obj, bool confirmable)
{
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}

	if (!obj) {
		return -EINVAL;
	}

	/* Only support sending of the CoAP CBOR or JSON type or a pre-encoded CBOR buffer. */
	if ((obj->type != NRF_CLOUD_OBJ_TYPE_COAP_CBOR) &&
	    (obj->type != NRF_CLOUD_OBJ_TYPE_JSON) &&
	    (obj->enc_src != NRF_CLOUD_ENC_SRC_PRE_ENCODED)) {
		return -ENOTSUP;
	}

	bool bulk = nrf_cloud_obj_bulk_check(obj);

	if (bulk && (obj->type == NRF_CLOUD_OBJ_TYPE_COAP_CBOR)) {
		return -ENOTSUP;
	}

	int err = 0;
	bool enc = false;
	const char *resource = bulk ? COAP_D2C_BULK_RSC : COAP_D2C_RSC;

	if (!resource) {
		return -EINVAL;
	}

	if (obj->enc_src == NRF_CLOUD_ENC_SRC_NONE) {
		err = nrf_cloud_obj_cloud_encode(obj);
		if (err) {
			LOG_ERR("Unable to encode data: %d", err);
			return err;
		}
		enc = true;
	}

	err = nrf_cloud_coap_post(resource, NULL,
				  obj->encoded_data.ptr, obj->encoded_data.len,
				  (obj->type == NRF_CLOUD_OBJ_TYPE_COAP_CBOR) ?
				   COAP_CONTENT_FORMAT_APP_CBOR : COAP_CONTENT_FORMAT_APP_JSON,
				  confirmable, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send POST request: %d", err);
	}

	if (enc) {
		nrf_cloud_obj_cloud_encoded_free(obj);
	}

	return err;
}

int nrf_cloud_coap_sensor_send(const char *app_id, double value, int64_t ts_ms, bool confirmable)
{
	__ASSERT_NO_MSG(app_id != NULL);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}
	int64_t ts = (ts_ms == NRF_CLOUD_NO_TIMESTAMP) ? get_ts() : ts_ms;
	static uint8_t buffer[SENSOR_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buffer);
	int err;

	err = coap_codec_sensor_encode(app_id, value, ts, buffer, &len,
				       COAP_CONTENT_FORMAT_APP_CBOR);
	if (err) {
		LOG_ERR("Unable to encode sensor data: %d", err);
		return err;
	}
	err = nrf_cloud_coap_post(COAP_D2C_RSC, NULL, buffer, len,
				  COAP_CONTENT_FORMAT_APP_CBOR, confirmable, NULL, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send POST request: %d", err);
	} else if (err > 0) {
		LOG_RESULT_CODE_ERR("Error from server:", err);
	}
	return err;
}

int nrf_cloud_coap_message_send(const char *app_id, const char *message, bool json, int64_t ts_ms,
				bool confirmable)
{
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}
	int64_t ts = (ts_ms == NRF_CLOUD_NO_TIMESTAMP) ? get_ts() : ts_ms;
	uint8_t buffer[MESSAGE_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buffer);
	int err;
	struct nrf_cloud_obj_coap_cbor msg = {
		.app_id		= (char *)app_id,
		.type		= NRF_CLOUD_DATA_TYPE_STR,
		.str_val	= (char *)message,
		.ts		= ts
	};

	err = coap_codec_message_encode(&msg, buffer, &len,
					json ? COAP_CONTENT_FORMAT_APP_JSON :
					       COAP_CONTENT_FORMAT_APP_CBOR);
	if (err) {
		LOG_ERR("Unable to encode sensor data: %d", err);
		return err;
	}
	err = nrf_cloud_coap_post(COAP_D2C_RSC, NULL, buffer, len,
				  json ? COAP_CONTENT_FORMAT_APP_JSON :
					 COAP_CONTENT_FORMAT_APP_CBOR,
				  confirmable, NULL, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send POST request: %d", err);
	} else if (err > 0) {
		LOG_RESULT_CODE_ERR("Error from server:", err);
	}
	return err;
}

int nrf_cloud_coap_json_message_send(const char *message, bool bulk, bool confirmable)
{
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}
	size_t len = strlen(message);
	const char *resource = bulk ? COAP_D2C_BULK_RSC : COAP_D2C_RSC;
	int err;

	if (!resource) {
		return -EINVAL;
	}
	err = nrf_cloud_coap_post(resource,
				  NULL, message, len,
				  COAP_CONTENT_FORMAT_APP_JSON,
				  confirmable, NULL, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send POST request: %d", err);
	} else if (err > 0) {
		LOG_RESULT_CODE_ERR("Error from server:", err);
	}
	return err;
}

int nrf_cloud_coap_location_send(const struct nrf_cloud_gnss_data *gnss, bool confirmable)
{
	__ASSERT_NO_MSG(gnss != NULL);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}
	int64_t ts = (gnss->ts_ms == NRF_CLOUD_NO_TIMESTAMP) ? get_ts() : gnss->ts_ms;
	static uint8_t buffer[LOCATION_SEND_CBOR_MAX_SIZE];
	size_t len = sizeof(buffer);
	int err;

	if (gnss->type != NRF_CLOUD_GNSS_TYPE_PVT) {
		LOG_ERR("Only PVT format is supported");
		return -ENOTSUP;
	}
	err = coap_codec_pvt_encode("GNSS", &gnss->pvt, ts, buffer, &len,
				    COAP_CONTENT_FORMAT_APP_CBOR);
	if (err) {
		LOG_ERR("Unable to encode GNSS PVT data: %d", err);
		return err;
	}
	err = nrf_cloud_coap_post(COAP_D2C_RSC, NULL, buffer, len,
				  COAP_CONTENT_FORMAT_APP_CBOR, confirmable, NULL, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send POST request: %d", err);
	} else if (err > 0) {
		LOG_RESULT_CODE_ERR("Error from server:", err);
	}
	return err;
}

static int loc_err;

static void get_location_callback(const struct coap_client_response_data *data, void *user)
{
	if (data->result_code != COAP_RESPONSE_CODE_CONTENT) {
		loc_err = data->result_code;
		if (data->payload_len) {
			LOG_ERR("Unexpected response: %.*s", data->payload_len, data->payload);
		}
	} else if (data->payload_len) {
		loc_err = coap_codec_ground_fix_resp_decode(user, data->payload, data->payload_len,
							    COAP_CONTENT_FORMAT_APP_CBOR);
	} else {
		struct nrf_cloud_location_result *result = user;

		loc_err = 0;
		if (result) {
			result->type = LOCATION_TYPE__INVALID;
		}
	}
}

int nrf_cloud_coap_location_get(struct nrf_cloud_rest_location_request const *const request,
				struct nrf_cloud_location_result *const result)
{
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG((request->cell_info != NULL) || (request->wifi_info != NULL));
	__ASSERT_NO_MSG(result != NULL);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}
	static uint8_t buffer[LOCATION_GET_CBOR_MAX_SIZE];
	size_t len = sizeof(buffer);
	int err;
	const struct nrf_cloud_location_config *conf = request->config;
	size_t url_size = nrf_cloud_ground_fix_url_encode(NULL, 0, COAP_GND_FIX_RSC, conf);

	__ASSERT_NO_MSG(url_size > 0);
	char url[url_size + 1];

	(void)nrf_cloud_ground_fix_url_encode(url, url_size, COAP_GND_FIX_RSC, conf);

	/* Take the semaphore before modifying the static buffer */
	(void)k_sem_take(&coap_transfer_sem, K_FOREVER);

	loc_err = 0;

	err = coap_codec_ground_fix_req_encode(request->cell_info,
					       request->wifi_info,
					       buffer, &len,
					       COAP_CONTENT_FORMAT_APP_CBOR);
	if (err) {
		LOG_ERR("Unable to encode location data: %d", err);
		goto give_and_return;
	}

	err = nrf_cloud_coap_fetch(url, NULL, buffer, len,
				   COAP_CONTENT_FORMAT_APP_CBOR,
				   COAP_CONTENT_FORMAT_APP_CBOR, true,
				   get_location_callback, result);

	/* Check for CoAP transfer errors */
	if (err == -EAGAIN) {
		LOG_ERR("Timeout waiting for location");
		goto give_and_return;
	} else if (err) {
		LOG_ERR("CoAP client error: %d", err);
		goto give_and_return;
	}

	/* Use the callback error as the return value */
	err = loc_err;

	/* Check for callback errors */
	if (loc_err > 0) {
		LOG_RESULT_CODE_ERR("Error getting location; result code:", loc_err);
	} else if (loc_err < 0) {
		LOG_ERR("Location response decoding error: %d", loc_err);
	} else {
		/* No callback error: success */
		if (result->type != LOCATION_TYPE__INVALID) {
			LOG_DBG("Location: %d, %.12g, %.12g, %d", result->type,
				result->lat, result->lon, result->unc);
		} else {
			LOG_DBG("No location returned");
		}
	}

give_and_return:
	k_sem_give(&coap_transfer_sem);
	return err;
}

static int fota_err;

static void get_fota_callback(const struct coap_client_response_data *data, void *user)
{
	if (data->result_code != COAP_RESPONSE_CODE_CONTENT) {
		fota_err = data->result_code;
		if (data->payload_len) {
			LOG_ERR("Unexpected response: %.*s", data->payload_len, data->payload);
		}
	} else if (data->payload && data->payload_len) {
		LOG_DBG("Got FOTA response: %.*s", data->payload_len, (const char *)data->payload);
		fota_err = coap_codec_fota_resp_decode(user, data->payload, data->payload_len,
						       COAP_CONTENT_FORMAT_APP_JSON);
	} else {
		fota_err = -ENOMSG;
	}
}

int nrf_cloud_coap_fota_job_get(struct nrf_cloud_fota_job_info *const job)
{
	__ASSERT_NO_MSG(job != NULL);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}
	int err;

	job->type = NRF_CLOUD_FOTA_TYPE__INVALID;

	(void)k_sem_take(&coap_transfer_sem, K_FOREVER);

	fota_err = 0;

	err = nrf_cloud_coap_get(COAP_FOTA_GET_RSC, NULL, NULL, 0,
				 COAP_CONTENT_FORMAT_APP_CBOR,
				 COAP_CONTENT_FORMAT_APP_JSON, true, get_fota_callback, job);

	/* Check for CoAP transfer errors */
	if (err == -EAGAIN) {
		LOG_ERR("Timeout waiting for FOTA job");
		goto give_and_return;
	} else if (err) {
		LOG_ERR("CoAP client error: %d", err);
		goto give_and_return;
	}

	/* Use the callback error as the return value */
	err = fota_err;

	/* Check for callback errors */
	if (fota_err == COAP_RESPONSE_CODE_NOT_FOUND) {
		/* A CoAP error response code, but not an actual error */
		err = 0;
		LOG_DBG("No pending FOTA job");
	} else if (fota_err > 0) {
		LOG_RESULT_CODE_ERR("Error getting FOTA job info; result code:", fota_err);
	} else if (fota_err == -ETIMEDOUT) {
		LOG_ERR("FOTA job request timed out: %d", fota_err);
	} else if (fota_err < 0) {
		LOG_ERR("FOTA request failed: %d", fota_err);
	} else {
		/* No callback error: success */
		LOG_DBG("FOTA job received; type:%d, id:%s, host:%s, path:%s, size:%d",
			job->type, job->id, job->host, job->path, job->file_size);
	}

give_and_return:
	k_sem_give(&coap_transfer_sem);
	return err;
}

void nrf_cloud_coap_fota_job_free(struct nrf_cloud_fota_job_info *const job)
{
	nrf_cloud_fota_job_free(job);
}

int nrf_cloud_coap_fota_job_update(const char *const job_id,
	const enum nrf_cloud_fota_status status, const char * const details)
{
	__ASSERT_NO_MSG(job_id != NULL);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}

	struct nrf_cloud_fota_job_update update;
	int err;

	err = nrf_cloud_fota_job_update_create(NULL, job_id, status, details, &update);
	if (err) {
		LOG_ERR("Error creating FOTA job update structure: %d", err);
		return err;
	}
	err = nrf_cloud_coap_patch(update.url, NULL, update.payload, strlen(update.payload),
				   COAP_CONTENT_FORMAT_APP_JSON, true, NULL, NULL);

	nrf_cloud_fota_job_update_free(&update);

	return err;
}

static struct get_shadow_data  {
	enum coap_content_format format;
	char *buf;
	size_t buf_len;
	int err;
} shadow_data;

static void get_shadow_callback(const struct coap_client_response_data *data, void *user)
{
	if (data->result_code != COAP_RESPONSE_CODE_CONTENT) {
		shadow_data.err = data->result_code;
		if (data->payload_len) {
			LOG_ERR("Unexpected response: %.*s", data->payload_len, data->payload);
		}
		return;
	}

	/* If JSON, ensure buffer can be NULL-terminated */
	size_t cpy_len = (shadow_data.format == COAP_CONTENT_FORMAT_APP_JSON) ?
			 (shadow_data.buf_len - 1) : shadow_data.buf_len;

	if (data->payload_len > cpy_len) {
		LOG_WRN("Shadow data truncated from %zd to %zd bytes.", data->payload_len, cpy_len);
		shadow_data.err = -E2BIG;
	} else {
		/* Copy the entire received buffer */
		cpy_len = data->payload_len;
	}

	if (cpy_len) {
		memcpy(shadow_data.buf, data->payload, cpy_len);
	}

	if (shadow_data.format == COAP_CONTENT_FORMAT_APP_JSON) {
		shadow_data.buf[cpy_len] = '\0';
	}

	shadow_data.buf_len = cpy_len;
}

int nrf_cloud_coap_shadow_get(char *buf, size_t *buf_len, bool delta,
			      enum coap_content_format format)
{
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(buf_len != NULL);
	__ASSERT_NO_MSG(*buf_len != 0);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}

	if ((format != COAP_CONTENT_FORMAT_APP_JSON) &&
	    (format != COAP_CONTENT_FORMAT_APP_CBOR) &&
	    (format != COAP_CONTENT_FORMAT_APP_OCTET_STREAM)) {
		return -EINVAL;
	}

	int err;

	(void)k_sem_take(&coap_transfer_sem, K_FOREVER);

	shadow_data.buf		= buf;
	shadow_data.buf_len	= *buf_len;
	shadow_data.err		= 0;
	shadow_data.format	= format;

	err = nrf_cloud_coap_get(COAP_SHDW_RSC, delta ? NULL : "delta=false", NULL, 0,
				  0, format, true, get_shadow_callback, NULL);

	/* Check for CoAP transfer errors */
	if (err) {
		LOG_ERR("Failed to send shadow get request: %d", err);
		goto give_and_return;
	}

	/* Use the callback error as the return value */
	err = shadow_data.err;

	/* Check for callback errors */
	if (shadow_data.err > 0) {
		LOG_RESULT_CODE_ERR("Error getting shadow; result code:", shadow_data.err);
	} else if ((shadow_data.err < 0) && (shadow_data.err != -E2BIG)) {
		LOG_ERR("Shadow response processing error: %d", shadow_data.err);
	} else {
		/* Data received, a return value of -E2BIG indicates that data was truncated */
		LOG_DBG("Received shadow, size: %u", shadow_data.buf_len);
		*buf_len = shadow_data.buf_len;
	}

give_and_return:
	k_sem_give(&coap_transfer_sem);
	return err;
}

static int shadow_update(const char * const resource, const char * const shadow_json)
{
	int err;

	__ASSERT_NO_MSG(shadow_json != NULL);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}

	err = nrf_cloud_coap_patch(resource, NULL, (uint8_t *)shadow_json,
				   strlen(shadow_json),
				   COAP_CONTENT_FORMAT_APP_JSON, true, NULL, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send PATCH request: %d", err);
	} else if (err > 0) {
		LOG_RESULT_CODE_ERR("Error from server:", err);
	}
	return err;
}

int nrf_cloud_coap_shadow_state_update(const char * const shadow_json)
{
	return shadow_update(COAP_SHDW_REP_RSC, shadow_json);
}

int nrf_cloud_coap_shadow_desired_update(const char * const shadow_json)
{
	return shadow_update(COAP_SHDW_DES_RSC, shadow_json);
}

int nrf_cloud_coap_shadow_device_status_update(const struct nrf_cloud_device_status
					       *const dev_status)
{
	__ASSERT_NO_MSG(dev_status != NULL);
	if (!nrf_cloud_coap_is_connected()) {
		return -EACCES;
	}

	int err;
	struct nrf_cloud_data data_out;

	err = nrf_cloud_shadow_dev_status_encode(dev_status, &data_out, false, false);
	if (err) {
		LOG_ERR("Failed to encode device status, error: %d", err);
		return err;
	}

	err = nrf_cloud_coap_shadow_state_update(data_out.ptr);
	if (err) {
		LOG_ERR("Failed to update device shadow, error: %d", err);
	}

	nrf_cloud_device_status_free(&data_out);

	return err;
}

int nrf_cloud_coap_shadow_service_info_update(const struct nrf_cloud_svc_info * const svc_inf)
{
	if (svc_inf == NULL) {
		return -EINVAL;
	}

	const struct nrf_cloud_device_status dev_status = {
		.modem = NULL,
		.svc = (struct nrf_cloud_svc_info *)svc_inf
	};

	return nrf_cloud_coap_shadow_device_status_update(&dev_status);
}

int nrf_cloud_coap_shadow_delta_process(const struct nrf_cloud_data *in_data,
					struct nrf_cloud_obj *const delta_out)
{
	if (!in_data) {
		return -EINVAL;
	}

	int err;
	struct nrf_cloud_data out_data = {0};
	struct nrf_cloud_obj_shadow_delta shadow_delta = {0};
	struct nrf_cloud_obj_shadow_data shadow_data;

	/* CoAP delta data does not have a "state" object, so decode directly to
	 * the object in the nrf_cloud_obj_shadow_delta struct.
	 */
	shadow_delta.state.type = NRF_CLOUD_OBJ_TYPE_JSON;
	err = nrf_cloud_obj_input_decode(&shadow_delta.state, in_data);
	if (err) {
		LOG_ERR("Error decoding shadow delta data, error: %d", err);
		return -ENOMSG;
	}

	/* Set the delta data */
	shadow_data.type = NRF_CLOUD_OBJ_SHADOW_TYPE_DELTA,
	shadow_data.delta = &shadow_delta;

	/* Process the default nRF Cloud topic and pairing info, which is not used for CoAP */
	err = nrf_cloud_coap_shadow_default_process(&shadow_data, &out_data);
	if (err == 0) {
		/* Acknowledge it so we do not receive it again. */
		err = nrf_cloud_coap_shadow_state_update(out_data.ptr);
		if (err) {
			LOG_ERR("Failed to acknowledge default shadow data: %d", err);
		} else {
			LOG_DBG("Default shadow data acknowledged");
		}

		nrf_cloud_free((void *)out_data.ptr);
		out_data.ptr = NULL;
		out_data.len = 0;
	}

	/* Process the potential control section of the delta */
	err = nrf_cloud_shadow_control_process(&shadow_data, &out_data);
	if ((err == -ENODATA) || (err == -ENOMSG)) {
		/* No control data in the delta or no reply is needed */
	} else if ((err == -EINVAL) || (err == 0)) {
		const char *action = err ? "reject" : "acknowledge";

		LOG_DBG("delta: len:%zd, %s", out_data.len, (const char *)out_data.ptr);

		/* Acknowledge it or reject it so we do not receive it again. */
		if (!err) {
			err = nrf_cloud_coap_shadow_state_update(out_data.ptr);
		} else {
			err = nrf_cloud_coap_shadow_desired_update(out_data.ptr);
		}
		if (err) {
			LOG_ERR("Failed to %s control delta: %d", action, err);
		} else {
			LOG_DBG("Control delta: %s", action);
		}

		nrf_cloud_free((void *)out_data.ptr);
		out_data.ptr = NULL;
		out_data.len = 0;
	} else {
		LOG_ERR("Failed to process device control shadow update, error: %d", err);
	}

	/* Check if there is delta data to give to the caller */
	if ((delta_out != NULL) &&
	    nrf_cloud_shadow_app_send_check(&shadow_data)) {
		/* Caller is now responsible for the object's memory */
		*delta_out = shadow_data.delta->state;
		err = 1;
	} else {
		nrf_cloud_obj_free(&shadow_delta.state);
		err = 0;
	}

	return err;
}
