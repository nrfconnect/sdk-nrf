/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "suit_fetch_source_streamer.h"
#include <zephyr/kernel.h>
#include <dfu/suit_dfu_fetch_source.h>
#include <suit_plat_err.h>

typedef enum {
	STAGE_IDLE,
	STAGE_PENDING_FIRST_RESPONSE,
	STAGE_IN_PROGRESS
} session_stage_t;

typedef struct {
	suit_dfu_fetch_source_request_fn request_fn;
} fetch_source_t;

typedef struct {
	session_stage_t stage;
	struct stream_sink client_sink;
	uint32_t session_id;

} stream_session_t;

static stream_session_t stream_session = {.stage = STAGE_IDLE};

static fetch_source_t sources[CONFIG_SUIT_STREAM_FETCH_MAX_SOURCES] = {0};

static uint32_t last_used_session_id;

static K_MUTEX_DEFINE(component_state_mutex);

static inline void component_lock(void)
{
	k_mutex_lock(&component_state_mutex, K_FOREVER);
}

static inline void component_unlock(void)
{
	k_mutex_unlock(&component_state_mutex);
}

static inline stream_session_t *open_session(const uint8_t *uri, size_t uri_length,
					     struct stream_sink *sink)
{
	component_lock();
	stream_session_t *session = &stream_session;

	if (session->stage != STAGE_IDLE) {
		component_unlock();
		return NULL;
	}

	session->stage = STAGE_PENDING_FIRST_RESPONSE;
	session->client_sink = *sink;
	session->session_id = 0;

	component_unlock();
	return session;
}

static inline void close_session(stream_session_t *session)
{
	component_lock();
	session->stage = STAGE_IDLE;
	session->session_id = 0;
	component_unlock();
}

static stream_session_t *find_session(uint32_t session_id)
{
	if (session_id == 0) {
		return NULL;
	}

	component_lock();
	stream_session_t *session = &stream_session;

	if (session->stage == STAGE_IDLE || session_id != session->session_id) {
		component_unlock();
		return NULL;
	}

	component_unlock();
	return session;
}

int suit_dfu_fetch_source_write_fetched_data(uint32_t session_id, const uint8_t *data, size_t len)
{
	component_lock();
	stream_session_t *session = find_session(session_id);

	if (session == NULL) {
		component_unlock();
		return -ENOENT;
	}

	if (session->stage == STAGE_PENDING_FIRST_RESPONSE) {
		session->stage = STAGE_IN_PROGRESS;
	}

	int err = session->client_sink.write(session->client_sink.ctx, data, len);

	if (err == SUIT_PLAT_SUCCESS) {
		err = 0;
	} else {
		err = -EIO;
	}

	component_unlock();
	return err;
}

int suit_dfu_fetch_source_seek(uint32_t session_id, size_t offset)
{
	component_lock();
	stream_session_t *session = find_session(session_id);

	if (session == NULL) {
		component_unlock();
		return -ENOENT;
	}

	if (session->stage == STAGE_PENDING_FIRST_RESPONSE) {
		session->stage = STAGE_IN_PROGRESS;
	}

	if (session->client_sink.seek == NULL) {
		return -EACCES;
	}

	suit_plat_err_t err = session->client_sink.seek(session->client_sink.ctx, offset);

	if (err == SUIT_PLAT_SUCCESS) {
		err = 0;
	} else {
		err = -EIO;
	}

	component_unlock();
	return err;
}

int suit_dfu_fetch_source_register(suit_dfu_fetch_source_request_fn request_fn)
{
	component_lock();

	for (int i = 0; i < sizeof(sources) / sizeof(fetch_source_t); i++) {
		fetch_source_t *source = &sources[i];

		if (source->request_fn == NULL) {
			source->request_fn = request_fn;
			component_unlock();
			return SUIT_PLAT_SUCCESS;
		}
	}

	component_unlock();
	return SUIT_PLAT_ERR_NO_RESOURCES;
}

suit_plat_err_t suit_fetch_source_stream(const uint8_t *uri, size_t uri_length,
					 struct stream_sink *sink)
{
	if (uri == NULL || uri_length == 0 || sink == NULL || sink->write == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	stream_session_t *session = open_session(uri, uri_length, sink);

	if (session == NULL) {
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	for (int i = 0; i < sizeof(sources) / sizeof(fetch_source_t); i++) {
		component_lock();

		fetch_source_t *source = &sources[i];

		suit_dfu_fetch_source_request_fn request_fn = source->request_fn;

		if (0 == ++last_used_session_id) {
			++last_used_session_id;
		}

		session->session_id = last_used_session_id;

		component_unlock();

		if (request_fn != NULL) {
			int err = request_fn(uri, uri_length, last_used_session_id);

			if (err == 0) {
				close_session(session);
				return SUIT_PLAT_SUCCESS;

			} else if (session->stage != STAGE_PENDING_FIRST_RESPONSE) {
				/* error while transfer has arleady started, unrecoverable
				 */
				close_session(session);
				return SUIT_PLAT_ERR_INCORRECT_STATE;
			}

			/* fetch source signalized an error immediately, means it does not
			 * support fetching from provided URI, let's try next fetch source
			 */
		}
	}

	close_session(session);
	return SUIT_PLAT_ERR_CRASH;
}
