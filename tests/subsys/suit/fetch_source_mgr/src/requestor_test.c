/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <suit_ipc_streamer.h>

static K_THREAD_STACK_DEFINE(injector_stack, 1024);
static struct k_thread injector_thread;
#define INJECTOR_THREAD_PRIORITY 5

static uint32_t missing_image_notify_count;

static const char *requested_resource_id = "ExampleImageName.img";
static void *stream_sink_requested_ctx = (void *)0xabcd0001;
static void *missing_image_notify_requested_ctx = (void *)0xabcd0002;
static void *chunk_status_notify_requested_ctx = (void *)0xabcd0003;

/* Intentionally missaligned buffer size
 */
static uint8_t test_buf[32 * 1024 - 3];
#define ITERATIONS 16

static uint32_t received_bytes;
static uint32_t received_checksum;
static uint32_t expected_bytes;
static uint32_t expected_checksum;

#define BUFFER_SIZE  2048
#define BUFFER_COUNT 3

typedef enum {
	FREE = 0,
	READY_TO_ENQUEUE = 1,
	ENQUEUED = 2
} buffer_state_t;

typedef struct {
	buffer_state_t buffer_state;
	uint32_t offset_in_image;
	uint32_t chunk_size;
	uint32_t chunk_id;
} buffer_metadata_t;

typedef struct {
	buffer_metadata_t buffer_metadata[BUFFER_COUNT];
	uint8_t buffer[BUFFER_COUNT][BUFFER_SIZE];
} buffer_info_t;

typedef struct {
	uint32_t stream_session_id;
	size_t current_image_offset;
	size_t requested_image_offset;
	uint32_t last_chunk_id;
	buffer_info_t buffer_info;
} image_request_info_t;

static K_SEM_DEFINE(chunk_status_changed_sem, 0, 1);
static image_request_info_t request_info;

static bool chunk_released_check(image_request_info_t *ri,
				 suit_ipc_streamer_chunk_info_t *injected_chunks,
				 size_t chunk_info_count)
{
	bool chunk_released = false;

	for (int i = 0; i < BUFFER_COUNT; i++) {
		buffer_metadata_t *bm = &ri->buffer_info.buffer_metadata[i];

		if (bm->buffer_state == ENQUEUED) {
			bool still_enqueued = false;

			for (int j = 0; j < chunk_info_count; j++) {
				suit_ipc_streamer_chunk_info_t *ci = &injected_chunks[j];

				if (ci->chunk_id == bm->chunk_id && ci->status == PENDING) {
					still_enqueued = true;
					break;
				}
			}

			if (!still_enqueued) {
				bm->buffer_state = FREE;
				chunk_released = true;
			}
		}
	}

	return chunk_released;
}

static suit_plat_err_t wait_for_buffer_state_change(image_request_info_t *ri)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	suit_ipc_streamer_chunk_info_t injected_chunks[BUFFER_COUNT];
	size_t chunk_info_count = BUFFER_COUNT;

	while (true) {
		err = suit_ipc_streamer_chunk_status_req(ri->stream_session_id, injected_chunks,
							 &chunk_info_count);
		if (err == SUIT_PLAT_ERR_BUSY) {
			/* not enough space in injected_chunks. This is not a problem,
			 *  stream requestor most probably just released its space and
			 *  one of next calls will be successful.
			 */

		} else if (err == SUIT_PLAT_SUCCESS) {
			bool chunk_released =
				chunk_released_check(ri, injected_chunks, chunk_info_count);

			if (chunk_released) {
				return SUIT_PLAT_SUCCESS;
			}
		} else {
			return err;
		}

		/* 20 seconds to catch eventual timeout due to missing notification
		 */
		k_sem_take(&chunk_status_changed_sem, K_MSEC(20000));
	}

	return err;
}

static suit_plat_err_t find_buffer_for_enqueue(image_request_info_t *ri, buffer_metadata_t **bm,
					       uint8_t **buffer)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	*bm = NULL;
	*buffer = NULL;

	while (*bm == NULL) {
		for (int i = 0; i < BUFFER_COUNT; i++) {
			buffer_metadata_t *current = &ri->buffer_info.buffer_metadata[i];

			if (current->buffer_state == FREE) {
				*bm = current;
				*buffer = ri->buffer_info.buffer[i];
				break;
			}
		}

		if (*bm == NULL) {
			err = wait_for_buffer_state_change(ri);
			if (err != SUIT_PLAT_SUCCESS) {
				return err;
			}
		}
	}

	return err;
}

static suit_plat_err_t chunk_enqueue(image_request_info_t *ri, uint32_t chunk_id, uint32_t offset,
				     uint8_t *address, uint32_t size, bool last_chunk)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	while (true) {
		err = suit_ipc_streamer_chunk_enqueue(ri->stream_session_id, chunk_id, offset,
						      address, size, last_chunk);
		if (err == SUIT_PLAT_ERR_BUSY) {
			/* Not enough space in requestor, try again later
			 */
			err = wait_for_buffer_state_change(ri);
			if (err != SUIT_PLAT_SUCCESS) {
				return err;
			}
		} else {
			return err;
		}
	}
}

static suit_plat_err_t chunk_enqueue_and_push(void *ctx, uint8_t *source_buffer, size_t *size)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	size_t source_offset = 0;
	size_t source_remaining = *size;
	image_request_info_t *ri = &request_info;

	if (ri->stream_session_id != (uintptr_t)ctx) {
		return SUIT_PLAT_ERR_CRASH;
	}

	while (source_remaining) {
		buffer_metadata_t *bm = NULL;
		uint8_t *buffer = NULL;

		err = find_buffer_for_enqueue(ri, &bm, &buffer);
		if (err != SUIT_PLAT_SUCCESS) {
			return err;
		}

		bm->offset_in_image = ri->requested_image_offset;
		bm->chunk_size = 0;
		bm->chunk_id = ++ri->last_chunk_id;

		size_t to_be_copied = BUFFER_SIZE - bm->chunk_size;

		if (source_remaining < to_be_copied) {
			to_be_copied = source_remaining;
		}

		memcpy(buffer + bm->chunk_size, source_buffer + source_offset, to_be_copied);
		source_offset += to_be_copied;
		source_remaining -= to_be_copied;
		bm->chunk_size += to_be_copied;
		bm->buffer_state = READY_TO_ENQUEUE;

		err = chunk_enqueue(ri, bm->chunk_id, bm->offset_in_image, buffer, bm->chunk_size,
				    false);
		if (err != SUIT_PLAT_SUCCESS) {
			break;
		}
		ri->requested_image_offset += bm->chunk_size;
		bm->buffer_state = ENQUEUED;
	}

	return err;
}

static suit_plat_err_t last_chunk_enqueue_and_push(void *ctx)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	image_request_info_t *ri = &request_info;

	if (ri->stream_session_id != (uintptr_t)ctx) {
		return SUIT_PLAT_ERR_CRASH;
	}

	err = chunk_enqueue(ri, 0, ri->requested_image_offset, 0, 0, true);
	return err;
}

static void injector_entry_point(void *stream_session_id, void *p2, void *p3)
{
	for (int i = 0; i < sizeof(test_buf); ++i) {
		test_buf[i] = (uint8_t)i;
	}

	for (int buf_iter = 0; buf_iter < ITERATIONS; buf_iter++) {

		for (int i = 0; i < sizeof(test_buf); ++i) {
			expected_bytes++;
			expected_checksum += test_buf[i];
		}
	}

	for (int buf_iter = 0; buf_iter < ITERATIONS; buf_iter++) {
		size_t size = sizeof(test_buf);

		chunk_enqueue_and_push(stream_session_id, test_buf, &size);
	}

	last_chunk_enqueue_and_push(stream_session_id);
}

static K_SEM_DEFINE(delay_simulator_sem, 0, 1);
static suit_plat_err_t ipc_stream_write_chunk(void *ctx, const uint8_t *buf, size_t size)
{
	received_bytes += size;

	for (int i = 0; i < size; ++i) {
		received_checksum += buf[i];
	}

	/* let's simulate 10 ms lasting operation.
	 *  cannot use k_sleep in posix tests!
	 */
	k_sem_take(&delay_simulator_sem, K_MSEC(10));
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t missing_image_notify_fn(const uint8_t *resource_id,
					       size_t resource_id_length,
					       uint32_t stream_session_id, void *context)
{
	zassert_equal(context, missing_image_notify_requested_ctx, "context (%08x)", context);
	zassert_equal(resource_id_length, strlen(requested_resource_id), "resource_id_length (%d)",
		      resource_id_length);
	zassert_mem_equal(resource_id, requested_resource_id, strlen(requested_resource_id));

	if (0 == missing_image_notify_count) {

		image_request_info_t *ri = &request_info;

		ri->stream_session_id = stream_session_id;

		k_thread_create(&injector_thread, injector_stack,
				K_THREAD_STACK_SIZEOF(injector_stack), injector_entry_point,
				(void *)(uintptr_t)stream_session_id, NULL, NULL,
				INJECTOR_THREAD_PRIORITY, 0, K_NO_WAIT);
	}
	missing_image_notify_count++;
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t chunk_status_notify_fn(uint32_t stream_session_id, void *context)
{
	zassert_equal(context, chunk_status_notify_requested_ctx, "context (%08x)", context);
	k_sem_give(&chunk_status_changed_sem);
	return SUIT_PLAT_SUCCESS;
}

void test_ipc_streamer_requestor(void)
{

	struct stream_sink test_sink = {
		.write = ipc_stream_write_chunk,
		.seek = NULL,
		.flush = NULL,
		.used_storage = NULL,
		.release = NULL,
		.ctx = stream_sink_requested_ctx,
	};

	uint32_t inter_chunk_timeout_ms = 10000;
	uint32_t requesting_period_ms = 1000;
	suit_plat_err_t rc = SUIT_PLAT_SUCCESS;

	rc = suit_ipc_streamer_requestor_init();
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "suit_ipc_streamer_requestor_init returned (%d)", rc);

	suit_ipc_streamer_chunk_status_evt_unsubscribe();
	rc = suit_ipc_streamer_chunk_status_evt_subscribe(chunk_status_notify_fn,
							  chunk_status_notify_requested_ctx);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_ipc_streamer_chunk_status_evt_subscribe returned (%d)", rc);

	suit_ipc_streamer_missing_image_evt_unsubscribe();
	rc = suit_ipc_streamer_missing_image_evt_subscribe(missing_image_notify_fn,
							   missing_image_notify_requested_ctx);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_ipc_streamer_missing_image_evt_unsubscribe returned (%d)", rc);

	rc = suit_ipc_streamer_stream(requested_resource_id, strlen(requested_resource_id),
				      &test_sink, inter_chunk_timeout_ms, requesting_period_ms);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "suit_ipc_streamer_stream returned (%d)", rc);
	zassert_equal(expected_bytes, received_bytes, "%d vs %d", expected_bytes, received_bytes);
	zassert_equal(expected_checksum, received_checksum, "%d vs %d", expected_checksum,
		      received_checksum);
}
