/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>

#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
#include <sdfw/sdfw_services/suit_service.h>
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */

#include "dfu/suit_dfu_fetch_source.h"

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include "suitfu_mgmt_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(suitfu_mgmt, CONFIG_MGMT_SUITFU_LOG_LEVEL);

static K_MUTEX_DEFINE(component_state_mutex);
static K_CONDVAR_DEFINE(transfer_completed_cvar);

static inline void component_lock(void)
{
	k_mutex_lock(&component_state_mutex, K_FOREVER);
}

static inline void component_unlock(void)
{
	k_mutex_unlock(&component_state_mutex);
}

typedef struct {

	const uint8_t *uri;
	size_t uri_length;
	uint32_t session_id;
	uint32_t fetch_src_session_id;

	int64_t last_notify_ts;
	suit_plat_err_t return_code;
	struct k_condvar *transfer_completed_cvar;

} stream_session_t;

static stream_session_t stream_session;

static suit_plat_err_t suitfu_mgmt_suit_missing_image_request(const uint8_t *uri, size_t uri_length,
							      uint32_t session_id)
{
	static uint32_t last_session_id;
	stream_session_t *session = &stream_session;
	int64_t current_ts = k_uptime_get();

	LOG_HEXDUMP_INF(uri, uri_length, "Request for image");

	if (strncmp(uri, "file://", strlen("file://"))) {
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	component_lock();
	if (session->uri) {
		/* Seems that there is a streaming in progress
		 */
		component_unlock();
		return SUIT_PLAT_ERR_NO_RESOURCES;
	}

	last_session_id++;
	if (last_session_id == 0) {
		last_session_id++;
	}

	session->uri = uri;
	session->uri_length = uri_length;
	session->session_id = last_session_id;
	session->fetch_src_session_id = session_id;
	session->last_notify_ts = current_ts;
	session->return_code = SUIT_PLAT_ERR_TIME;
	session->transfer_completed_cvar = &transfer_completed_cvar;

	while (true) {
		/* Looping till timeout or transfer completion
		 */
		uint32_t sleep_period_ms = session->last_notify_ts +
					   CONFIG_MGMT_SUITFU_GRP_SUIT_IMAGE_FETCH_TIMEOUT_MS -
					   current_ts;

		k_condvar_wait(&transfer_completed_cvar, &component_state_mutex,
			       K_MSEC(sleep_period_ms));

		current_ts = k_uptime_get();

		if (current_ts - session->last_notify_ts >=
			    CONFIG_MGMT_SUITFU_GRP_SUIT_IMAGE_FETCH_TIMEOUT_MS ||
		    session->return_code != SUIT_PLAT_ERR_TIME) {
			break;
		}
	}

	/* Cleanup
	 */
	int rc = session->return_code;

	session->uri = NULL;
	session->uri_length = 0;
	session->session_id = 0;
	session->fetch_src_session_id = 0;
	session->last_notify_ts = 0;
	session->return_code = SUIT_PLAT_SUCCESS;
	session->transfer_completed_cvar = NULL;

	component_unlock();

	return rc;
}

/** Represents an individual upload request. */
typedef struct {
	uint32_t stream_session_id;
	size_t off;  /* SIZE_MAX if unspecified. */
	size_t size; /* SIZE_MAX if unspecified. */
	struct zcbor_string img_data;
} suitfu_mgmt_missing_img_upload_req_t;

int suitfu_mgmt_suit_missing_image_upload(struct smp_streamer *ctx)
{
	stream_session_t *session = &stream_session;
	zcbor_state_t *zsd = ctx->reader->zs;
	zcbor_state_t *zse = ctx->writer->zs;
	static size_t image_size;
	static size_t offset_in_image;

	size_t decoded = 0;
	suitfu_mgmt_missing_img_upload_req_t req = {
		.stream_session_id = 0,
		.off = SIZE_MAX,
		.size = SIZE_MAX,
		.img_data = {0},
	};

	int rc = 0;

	struct zcbor_map_decode_key_val image_upload_decode[] = {

		ZCBOR_MAP_DECODE_KEY_VAL(stream_session_id, zcbor_uint32_decode,
					 &req.stream_session_id),
		ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &req.img_data),
		ZCBOR_MAP_DECODE_KEY_VAL(len, zcbor_size_decode, &req.size),
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_size_decode, &req.off)};

	if (zcbor_map_decode_bulk(zsd, image_upload_decode, ARRAY_SIZE(image_upload_decode),
				  &decoded) != 0) {
		LOG_ERR("Decoding image upload request failed");
		return MGMT_ERR_EINVAL;
	}

	if (req.off == 0) {
		LOG_INF("New image transfer started, stream_session_id: %d", req.stream_session_id);
		image_size = 0;
		offset_in_image = 0;

		if (req.size == 0) {
			LOG_ERR("Candidate image empty");
			return MGMT_ERR_EINVAL;
		}

		image_size = req.size;
	}

	if (image_size == 0) {
		LOG_ERR("No transfer in progress");
		return MGMT_ERR_EBADSTATE;
	}

	if (req.off + req.img_data.len > image_size) {
		LOG_ERR("Image boundaries reached");
		image_size = 0;
		return MGMT_ERR_ENOMEM;
	}

	if (req.off != offset_in_image) {
		LOG_WRN("Wrong offset in image, expected: %p, received: %p",
			(void *)offset_in_image, (void *)req.off);
		if (zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, MGMT_ERR_EUNKNOWN) &&
		    zcbor_tstr_put_lit(zse, "off") && zcbor_size_put(zse, offset_in_image)) {
			return MGMT_ERR_EOK;
		}
	}

	bool last = false;

	if ((req.off + req.img_data.len) == image_size) {
		last = true;
	}

	size_t chunk_size = req.img_data.len;

	component_lock();
	if (session->transfer_completed_cvar && session->uri && session->uri_length &&
	    session->session_id == req.stream_session_id) {

		session->last_notify_ts = k_uptime_get();

		component_unlock();
	} else {
		component_unlock();
		return MGMT_ERR_EBADSTATE;
	}

	rc = suit_dfu_fetch_source_seek(session->fetch_src_session_id, req.off);

	if (rc == 0) {
		rc = suit_dfu_fetch_source_write_fetched_data(session->fetch_src_session_id,
							      req.img_data.value, chunk_size);
	}

	if (rc == 0) {
		offset_in_image += req.img_data.len;
	}

	component_lock();
	if (session->transfer_completed_cvar && session->uri && session->uri_length &&
	    session->session_id == req.stream_session_id) {
		if (rc != 0) {
			session->return_code = SUIT_PLAT_ERR_IO;
			k_condvar_signal(session->transfer_completed_cvar);
			component_unlock();
			image_size = 0;
			return MGMT_ERR_EUNKNOWN;
		}

		if (last) {
			session->return_code = SUIT_PLAT_SUCCESS;
			k_condvar_signal(session->transfer_completed_cvar);
		}

		component_unlock();

	} else {
		component_unlock();
		image_size = 0;
		return MGMT_ERR_EBADSTATE;
	}

	rc = MGMT_ERR_EOK;
	if (zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, rc) &&
	    zcbor_tstr_put_lit(zse, "off") && zcbor_size_put(zse, offset_in_image)) {
		return MGMT_ERR_EOK;
	}

	return MGMT_ERR_EMSGSIZE;
}

int suitfu_mgmt_suit_missing_image_state_read(struct smp_streamer *ctx)
{
	stream_session_t *session = &stream_session;
	zcbor_state_t *zse = ctx->writer->zs;
	bool ok;
	struct zcbor_string zcs;

	component_lock();
	if (session->uri && session->uri_length) {
		ok = zcbor_tstr_put_lit(zse, "stream_session_id") &&
		     zcbor_uint32_put(zse, session->session_id);
		if (!ok) {
			component_unlock();
			return MGMT_ERR_EMSGSIZE;
		}

		zcs.value = session->uri;
		zcs.len = session->uri_length;
		ok = zcbor_tstr_put_lit(zse, "resource_id") && zcbor_bstr_encode(zse, &zcs);
		if (!ok) {
			component_unlock();
			return MGMT_ERR_EMSGSIZE;
		}
	}
	component_unlock();

	return MGMT_ERR_EOK;
}

void suitfu_mgmt_suit_image_fetch_init(void)
{
	suit_dfu_fetch_source_register(suitfu_mgmt_suit_missing_image_request);
}
