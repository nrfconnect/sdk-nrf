/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <suit_ipc_streamer.h>
#include <suit_fetch_source_streamer.h>

#define IMG_REQUEST_THREAD_STACK_SIZE 2048
#define IMG_REQUEST_THREAD_PRIORITY   5

#ifdef CONFIG_DCACHE_LINE_SIZE
#define CACHE_ALIGNMENT CONFIG_DCACHE_LINE_SIZE
#else
#define CACHE_ALIGNMENT 4
#endif

static K_THREAD_STACK_DEFINE(img_request_thread_stack_area, IMG_REQUEST_THREAD_STACK_SIZE);
static struct k_work_q img_request_work_q;

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
	buffer_metadata_t buffer_metadata[CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS];
	uint8_t buffer[CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS]
		      [CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE]
		__aligned(CACHE_ALIGNMENT);
} buffer_info_t;

typedef struct {
	struct k_work work;
	uint32_t stream_session_id;
	size_t current_image_offset;
	size_t requested_image_offset;
	uint32_t last_chunk_id;

	buffer_info_t buffer_info;
} image_request_info_t;

static image_request_info_t request_info = {
	.stream_session_id = 0,
};

static K_SEM_DEFINE(chunk_status_changed_sem, 0, 1);

static bool chunk_released_check(image_request_info_t *ri,
				 suit_ipc_streamer_chunk_info_t *injected_chunks,
				 size_t chunk_info_count)
{
	bool chunk_released = false;

	for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS; i++) {
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
	suit_ipc_streamer_chunk_info_t injected_chunks[CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS];
	size_t chunk_info_count = CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS;

	while (true) {
		err = suit_ipc_streamer_chunk_status_req(ri->stream_session_id, injected_chunks,
							 &chunk_info_count);
		if (err == SUIT_PLAT_ERR_BUSY) {
			/* not enough space in injected_chunks. This is not a problem,
			 * stream requestor most probably just released its space and
			 * one of next calls will be successful.
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

		/* timeout is a guard against chunk status notification miss.
		 *  20 ms limits amount of IPC calls and at the same time it does not
		 *  affect operation duration significantly
		 */
		k_sem_take(&chunk_status_changed_sem, K_MSEC(20));
	}

	return err;
}

static suit_plat_err_t find_buffer_for_enqueue(image_request_info_t *ri, buffer_metadata_t **bm,
					       uint8_t **buffer)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	*bm = NULL;
	*buffer = NULL;

	for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS; i++) {
		buffer_metadata_t *current = &ri->buffer_info.buffer_metadata[i];

		if (current->buffer_state == READY_TO_ENQUEUE) {
			/* READY_TO_ENQUEUE- means that new data shall be concatenated to this one
			 */
			*bm = current;
			*buffer = ri->buffer_info.buffer[i];
			return SUIT_PLAT_SUCCESS;
		}
	}

	while (*bm == NULL) {
		for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS; i++) {
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

static suit_plat_err_t chunk_enqueue(image_request_info_t *ri, buffer_metadata_t *bm,
				     uint8_t *buffer_address, bool last_chunk)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	while (true) {
		sys_cache_data_flush_range(buffer_address, bm->chunk_size);

		err = suit_ipc_streamer_chunk_enqueue(ri->stream_session_id, bm->chunk_id,
						      bm->offset_in_image, buffer_address,
						      bm->chunk_size, last_chunk);
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

	/* UNREACHABLE */
}

static suit_plat_err_t end_of_stream(image_request_info_t *ri)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	buffer_metadata_t *bm = NULL;
	uint8_t *buffer = NULL;

	err = find_buffer_for_enqueue(ri, &bm, &buffer);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	if (FREE == bm->buffer_state) {
		bm->offset_in_image = ri->requested_image_offset;
		bm->chunk_size = 0;
		bm->chunk_id = ++ri->last_chunk_id;
		bm->buffer_state = READY_TO_ENQUEUE;
	}

	if (READY_TO_ENQUEUE != bm->buffer_state) {
		/* This shall not happen. There must be a flaw in design. concurrency issue?
		 */
		return SUIT_PLAT_ERR_UNREACHABLE_PATH;
	}

	err = chunk_enqueue(ri, bm, buffer, true);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}
	ri->requested_image_offset = bm->offset_in_image + bm->chunk_size;
	bm->buffer_state = ENQUEUED;

	/* Let's wait till all buffers are given back or there is a
	 * 'unrecognized stream_session_id' error
	 */
	while (true) {
		bm = NULL;
		for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFERS; i++) {
			buffer_metadata_t *current = &ri->buffer_info.buffer_metadata[i];

			if (current->buffer_state == ENQUEUED) {
				bm = current;
				break;
			}
		}

		if (bm == NULL) {
			return SUIT_PLAT_SUCCESS;
		}

		err = wait_for_buffer_state_change(ri);
		if (err != SUIT_PLAT_SUCCESS) {
			return SUIT_PLAT_SUCCESS;
		}
	}

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t write_proxy(void *ctx, const uint8_t *source_buffer, size_t size)
{

	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	uint32_t stream_session_id = (uintptr_t)ctx;
	image_request_info_t *ri = &request_info;

	size_t source_offset = 0;
	size_t source_remaining = size;

	while (source_remaining) {

		if (ri->stream_session_id != stream_session_id) {
			return SUIT_PLAT_ERR_INCORRECT_STATE;
		}

		buffer_metadata_t *bm = NULL;
		uint8_t *buffer = NULL;

		err = find_buffer_for_enqueue(ri, &bm, &buffer);
		if (err != SUIT_PLAT_SUCCESS) {
			return err;
		}

		if (FREE == bm->buffer_state) {
			bm->offset_in_image = ri->requested_image_offset;
			bm->chunk_size = 0;
			bm->chunk_id = ++ri->last_chunk_id;
			bm->buffer_state = READY_TO_ENQUEUE;
		}

		if (READY_TO_ENQUEUE != bm->buffer_state) {
			/* This shall not happen. There must be a flaw in design. concurrency issue?
			 */
			return SUIT_PLAT_ERR_UNREACHABLE_PATH;
		}

		size_t to_be_copied = CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE - bm->chunk_size;

		if (source_remaining < to_be_copied) {
			to_be_copied = source_remaining;
		}

		memcpy(buffer + bm->chunk_size, source_buffer + source_offset, to_be_copied);
		source_offset += to_be_copied;
		source_remaining -= to_be_copied;
		bm->chunk_size += to_be_copied;

		if (CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE == bm->chunk_size) {
			/* buffer full, it is time to enqueue
			 */
			err = chunk_enqueue(ri, bm, buffer, false);
			if (err != SUIT_PLAT_SUCCESS) {
				return err;
			}
			ri->requested_image_offset = bm->offset_in_image + bm->chunk_size;
			bm->buffer_state = ENQUEUED;
		}
	}

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t seek_proxy(void *ctx, size_t offset)
{

	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	uint32_t stream_session_id = (uintptr_t)ctx;
	image_request_info_t *ri = &request_info;

	if (ri->stream_session_id != stream_session_id) {
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	buffer_metadata_t *bm = NULL;
	uint8_t *buffer = NULL;

	err = find_buffer_for_enqueue(ri, &bm, &buffer);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	if (READY_TO_ENQUEUE == bm->buffer_state) {

		if (bm->offset_in_image + bm->chunk_size == offset) {
			/* this operation moves write pointer exactly to location next to
			 *  last byte of this chunk, so there is no need to do anything
			 */
			return SUIT_PLAT_SUCCESS;

		} else {
			err = chunk_enqueue(ri, bm, buffer, false);
			if (err != SUIT_PLAT_SUCCESS) {
				return err;
			}
			ri->requested_image_offset = bm->offset_in_image + bm->chunk_size;
			bm->buffer_state = ENQUEUED;
		}
	}

	if (ri->requested_image_offset != offset) {
		err = find_buffer_for_enqueue(ri, &bm, &buffer);
		if (err != SUIT_PLAT_SUCCESS) {
			return err;
		}

		if (FREE != bm->buffer_state) {
			/* This shall not happen. There must be a flaw in design. concurrency issue?
			 *  as max one buffer can be READY_TO_ENQUEUE
			 */
			return SUIT_PLAT_ERR_UNREACHABLE_PATH;
		}

		bm->offset_in_image = offset;
		bm->chunk_size = 0;
		bm->chunk_id = ++ri->last_chunk_id;
		bm->buffer_state = READY_TO_ENQUEUE;
	}

	return SUIT_PLAT_SUCCESS;
}

static void image_request_worker(struct k_work *item)
{
	image_request_info_t *ri = CONTAINER_OF(item, image_request_info_t, work);

	struct stream_sink sink = {.write = write_proxy,
				   .seek = seek_proxy,
				   .flush = NULL,
				   .used_storage = NULL,
				   .release = NULL,
				   .ctx = (void *)(uintptr_t)ri->stream_session_id};

	uint8_t *resource_id = ri->buffer_info.buffer[0];
	suit_plat_err_t err = suit_fetch_source_stream(resource_id, strlen(resource_id), &sink);

	if (err == SUIT_PLAT_SUCCESS) {
		err = end_of_stream(ri);
	}

	ri->stream_session_id = 0;
}

static suit_plat_err_t missing_image_notify_fn(const uint8_t *resource_id,
					       size_t resource_id_length,
					       uint32_t stream_session_id, void *context)
{
	image_request_info_t *ri = &request_info;

	if (0 == ri->stream_session_id) {

		ri->current_image_offset = 0;
		ri->requested_image_offset = 0;
		ri->last_chunk_id = 0;
		memset(&ri->buffer_info, 0, sizeof(buffer_info_t));

		/* Buffer 0 is utilized to pass the URI
		 */
		strncpy(ri->buffer_info.buffer[0], resource_id,
			(resource_id_length < CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE
				 ? resource_id_length
				 : CONFIG_SUIT_STREAM_IPC_PROVIDER_BUFFER_SIZE - 1));

		ri->stream_session_id = stream_session_id;
		k_work_submit_to_queue(&img_request_work_q, &ri->work);
		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t chunk_status_notify_fn(uint32_t stream_session_id, void *context)
{
	k_sem_give(&chunk_status_changed_sem);
	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_ipc_streamer_provider_init(void)
{

	k_work_queue_init(&img_request_work_q);
	k_work_queue_start(&img_request_work_q, img_request_thread_stack_area,
			   K_THREAD_STACK_SIZEOF(img_request_thread_stack_area),
			   IMG_REQUEST_THREAD_PRIORITY, NULL);

	image_request_info_t *ri = &request_info;

	k_work_init(&ri->work, image_request_worker);

	suit_plat_err_t err = suit_ipc_streamer_requestor_init();

	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	err = suit_ipc_streamer_chunk_status_evt_subscribe(chunk_status_notify_fn, (void *)NULL);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	err = suit_ipc_streamer_missing_image_evt_subscribe(missing_image_notify_fn, (void *)NULL);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	return SUIT_PLAT_SUCCESS;
}
