/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_ipc_streamer.h>

static uint32_t write_chunk_count;
static uint32_t missing_image_notify_count;

static const char *requested_resource_id = "ExampleImageName.img";
static void *stream_sink_requested_ctx = (void *)0xabcd0001;
static void *missing_image_notify_requested_ctx = (void *)0xabcd0002;

static suit_plat_err_t ipc_stream_write_chunk(void *ctx, const uint8_t *buf, size_t size)
{
	write_chunk_count++;
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t missing_image_notify_fn(const uint8_t *resource_id,
					       size_t resource_id_length,
					       uint32_t stream_session_id, void *context)
{
	zassert_equal(context, missing_image_notify_requested_ctx,
		      "missing_image_notify_fn context (%08x)", context);
	zassert_equal(resource_id_length, strlen(requested_resource_id), "resource_id_length (%d)",
		      resource_id_length);
	zassert_mem_equal(resource_id, requested_resource_id, strlen(requested_resource_id));

	missing_image_notify_count++;
	return SUIT_PLAT_SUCCESS;
}

void test_ipc_streamer_requestor_no_response(void)
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
	int rc = 0;

	suit_ipc_streamer_missing_image_evt_unsubscribe();
	rc = suit_ipc_streamer_missing_image_evt_subscribe(missing_image_notify_fn,
							   missing_image_notify_requested_ctx);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_ipc_streamer_missing_image_evt_subscribe returned (%d)", rc);

	rc = suit_ipc_streamer_stream(requested_resource_id, strlen(requested_resource_id),
				      &test_sink, inter_chunk_timeout_ms, requesting_period_ms);
	zassert_equal(rc, SUIT_PLAT_ERR_TIME, "suit_ipc_streamer_stream returned (%d)", rc);
	zassert_equal(write_chunk_count, 0, "write_chunk_count (%d)", write_chunk_count);

	/* taking requesting_period_ms = 1s and inter_chunk_timeout_ms = 10s
	 * ~10 missing_image_notify_fn() calls is expected.
	 */
	zassert_between_inclusive(missing_image_notify_count, 9, 11,
				  "missing_image_notify_count (%d)", missing_image_notify_count);
}
