/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <suit_ipc_streamer.h>

typedef enum {
	STAGE_IDLE,
	STAGE_PENDING_FIRST_RESPONSE,
	STAGE_IN_PROGRESS,
	STAGE_CLOSING

} image_request_stage_t;

typedef enum {
	CHUNK_SLOT_EMPTY = 0,
	CHUNK_PENDING = 1,
	CHUNK_PROCESSED_SUCCESS = 2,
	CHUNK_PROCESSED_FAIL = 3
} chunk_processing_status_t;

/**
 * @brief Metadata for image chunk
 *
 * @var	chunk_id	Chunk Identifier assigned and provided by ipc streamer provider.
 *			Not interpreted by ipc streamer requestor
 *
 * @var	offset		Offset (in bytes) in target image where chunk shall be placed,
 *			provided by ipc streamer provider
 *
 * @var	address		Address in RAM, where chunk raw data is placed.
 *
 * @var	size		Size of chunk raw data in bytes.
 *
 * @var	arrival_number	Assigned by ipc streamer requestor upon chunk arrival. Utilized by
 *			ipc streamer provider to pass chunks to sink in arrival order.
 *
 */
typedef struct {
	/* Initialized with CHUNK_SLOT_EMPTY at image request and modified during image download
	 */
	chunk_processing_status_t status;

	/* Filled in at time of chunk arrival
	 */
	uint32_t chunk_id;
	uint32_t offset;
	uint8_t *address;
	size_t size;

	uint32_t arrival_number;

} chunk_processing_state_t;

typedef struct {

	struct k_sem *data_loop_kick_sem;

	/* Initialized with STAGE_IDLE at image request and modified during image download
	 */
	image_request_stage_t stage;

	/*Initialized at image request, stay unmodified till image request is completed
	 */
	const uint8_t *resource_id;
	size_t resource_id_length;
	struct stream_sink *sink;
	uint32_t inter_chunk_timeout_ms;
	uint32_t requesting_period_ms;
	uint32_t stream_session_id;

	/*Initialized at image request and modified during image download
	 */
	int64_t last_request_ts;
	int64_t last_response_ts;
	uint32_t last_arrival_number;
	uint32_t last_processed_number;
	uint32_t current_sink_write_offset;
	uint32_t last_chunk_arrival_number;
	suit_plat_err_t completion_error_code;

	chunk_processing_state_t
		chunk_processing_state[CONFIG_SUIT_STREAM_IPC_REQUESTOR_MAX_CHUNKS];
} image_request_state_t;

static uint32_t last_used_stream_session_id;
static K_MUTEX_DEFINE(image_request_state_mutex);
static K_SEM_DEFINE(image_request_data_loop_kick_sem, 0, 1);

static image_request_state_t image_request_state = {
	.data_loop_kick_sem = &image_request_data_loop_kick_sem,
	.stage = STAGE_IDLE,
};

static suit_ipc_streamer_chunk_status_notify_fn chunk_status_notify_fn;
static void *chunk_status_notify_context;

static suit_ipc_streamer_missing_image_notify_fn missing_image_notify_fn;
static void *missing_image_notify_context;

static inline void image_request_state_lock(void)
{
	k_mutex_lock(&image_request_state_mutex, K_FOREVER);
}

static inline void image_request_state_unlock(void)
{
	k_mutex_unlock(&image_request_state_mutex);
}

/* Assumption - access to image_request_state locked on entry/exit
 */
static chunk_processing_state_t *pending_chunk_get_next(image_request_state_t *irs)
{

	for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_REQUESTOR_MAX_CHUNKS; i++) {
		chunk_processing_state_t *cps = &irs->chunk_processing_state[i];

		if (cps->status == CHUNK_PENDING &&
		    cps->arrival_number == irs->last_processed_number + 1) {
			return cps;
		}
	}
	return NULL;
}

/* Assumption - access to image_request_state locked on entry/exit
 */
static chunk_processing_state_t *empty_chunk_slot_get(image_request_state_t *irs)
{

	for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_REQUESTOR_MAX_CHUNKS; i++) {
		chunk_processing_state_t *cps = &irs->chunk_processing_state[i];

		if (cps->status == CHUNK_SLOT_EMPTY) {
			return cps;
		}
	}
	return NULL;
}

/* Assumption - access to image_request_state locked on entry/exit
 */
static chunk_processing_state_t *non_empty_chunk_slot_get_next(image_request_state_t *irs,
							       uint32_t min_arrival_number)
{

	chunk_processing_state_t *found_cps = NULL;

	for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_REQUESTOR_MAX_CHUNKS; i++) {
		chunk_processing_state_t *cps = &irs->chunk_processing_state[i];

		if (cps->status != CHUNK_SLOT_EMPTY) {
			if (cps->arrival_number >= min_arrival_number) {
				if (found_cps == NULL ||
				    found_cps->arrival_number > cps->arrival_number) {
					found_cps = cps;
				}
			}
		}
	}
	return found_cps;
}

/* Assumption - access to image_request_state locked on entry/exit
 */
static int non_empty_chunk_slot_count_get(image_request_state_t *irs)
{

	int count = 0;

	for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_REQUESTOR_MAX_CHUNKS; i++) {
		chunk_processing_state_t *cps = &irs->chunk_processing_state[i];

		if (cps->status != CHUNK_SLOT_EMPTY) {
			count++;
		}
	}
	return count;
}

/* Assumption - access to image_request_state locked on entry/exit
 */
static void chunk_status_notify(image_request_state_t *irs)
{

	suit_ipc_streamer_chunk_status_notify_fn notify_fn = chunk_status_notify_fn;
	void *context = chunk_status_notify_context;
	uint32_t stream_session_id = irs->stream_session_id;

	if (NULL == notify_fn) {
		return;
	}

	image_request_state_unlock();
	notify_fn(stream_session_id, context);
	image_request_state_lock();
}

/* Assumption - access to image_request_state locked on entry/exit
 */
static void missing_image_notify(image_request_state_t *irs)
{
	suit_ipc_streamer_missing_image_notify_fn notify_fn = missing_image_notify_fn;
	void *context = missing_image_notify_context;
	const uint8_t *resource_id = irs->resource_id;
	size_t resource_id_length = irs->resource_id_length;
	uint32_t stream_session_id = irs->stream_session_id;

	if (NULL != notify_fn) {
		image_request_state_unlock();
		notify_fn(resource_id, resource_id_length, stream_session_id, context);
		image_request_state_lock();
	}
}

static suit_plat_err_t irs_seek(image_request_state_t *irs, size_t offset)
{
	suit_plat_err_t sink_error = SUIT_PLAT_SUCCESS;

	if (irs->sink->seek != NULL) {
		image_request_state_unlock();
		sink_error = irs->sink->seek(irs->sink->ctx, offset);
		image_request_state_lock();

		if (sink_error == SUIT_PLAT_SUCCESS) {
			irs->current_sink_write_offset = offset;
		}
	} else {
		/* seek is necessary but sink does not support it
		 */
		sink_error = SUIT_PLAT_ERR_UNSUPPORTED;
	}

	return sink_error;
}

/* Assumption - function is called with access to image_request_state initially locked.
 * image_request_state will also be locked on function exit.
 * Function will release image_request_state for blocking operation
 * and get it back once blocking operation is over. This is to avoid blocking
 * suit_ipc_streamer_chunk_enqueue while other chunk is being processed, i.e. decrypting,
 * storing in non-volatile memory, and also to avoid potential deadlock between
 * suit_ipc_streamer_chunk_enqueue,
 * and suit_ipc_streamer_missing_image_notify_fn, suit_ipc_streamer_chunk_status_notify_fn
 */
static suit_plat_err_t data_loop(image_request_state_t *irs)
{
	uint32_t sleep_period_ms = 0;
	struct k_sem *data_loop_kick_sem = irs->data_loop_kick_sem;

	while (true) {

		image_request_state_unlock();
		k_sem_take(data_loop_kick_sem, K_MSEC(sleep_period_ms));
		image_request_state_lock();

		int64_t current_ts = k_uptime_get();

		/* longest possible sleep time, will be decreased eventually
		 */
		sleep_period_ms = irs->inter_chunk_timeout_ms;

		/* Pushing the chunk to sink - if chunk is available
		 */
		if (irs->stage == STAGE_IN_PROGRESS) {
			chunk_processing_state_t *cps = pending_chunk_get_next(irs);

			if (cps != NULL) {
				suit_plat_err_t sink_error = SUIT_PLAT_SUCCESS;

				bool seek_needed = (irs->current_sink_write_offset != cps->offset)
							   ? true
							   : false;

				if (seek_needed) {
					sink_error = irs_seek(irs, cps->offset);
				}

				if (sink_error == SUIT_PLAT_SUCCESS && cps->address != NULL &&
				    cps->size != 0) {

					image_request_state_unlock();
					sink_error = irs->sink->write(irs->sink->ctx, cps->address,
								      cps->size);
					image_request_state_lock();

					if (sink_error == SUIT_PLAT_SUCCESS) {
						irs->current_sink_write_offset =
							cps->offset + cps->size;
					}
				}

				if (sink_error == SUIT_PLAT_SUCCESS) {

					irs->last_processed_number = cps->arrival_number;

					/* Let's check if it is time to transit to STAGE_CLOSING */
					if (irs->last_chunk_arrival_number == cps->arrival_number) {
						irs->stage = STAGE_CLOSING;
					}
					cps->status = CHUNK_PROCESSED_SUCCESS;
				} else {
					irs->completion_error_code = SUIT_PLAT_ERR_CRASH;
					irs->stage = STAGE_CLOSING;
					cps->status = CHUNK_PROCESSED_FAIL;
				}

				chunk_status_notify(irs);
				sleep_period_ms = 0;
			}
		}

		/* Let's check if it is time to transit to complete
		 */
		if (irs->stage == STAGE_CLOSING) {

			for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_REQUESTOR_MAX_CHUNKS; i++) {
				chunk_processing_state_t *cps = &irs->chunk_processing_state[i];

				if (cps->status == CHUNK_PENDING) {
					cps->status = CHUNK_PROCESSED_FAIL;
				}
			}

			chunk_status_notify(irs);

			suit_plat_err_t retval = irs->completion_error_code;

			irs->stage = STAGE_IDLE;
			return retval;
		}

		/* Managing image request notifications
		 */
		if (irs->stage == STAGE_PENDING_FIRST_RESPONSE) {
			if (irs->last_request_ts == 0 ||
			    current_ts - irs->last_request_ts >= irs->requesting_period_ms) {
				missing_image_notify(irs);
				irs->last_request_ts = current_ts;
			}

			uint32_t sleep_period_ms_candidate =
				irs->last_request_ts + irs->requesting_period_ms - current_ts;

			sleep_period_ms = (sleep_period_ms_candidate < sleep_period_ms)
						  ? sleep_period_ms_candidate
						  : sleep_period_ms;
		}

		/* Managing inter-chunk timeout
		 */
		if (current_ts - irs->last_response_ts >= irs->inter_chunk_timeout_ms) {
			irs->stage = STAGE_IDLE;
			return SUIT_PLAT_ERR_TIME;
		}

		uint32_t sleep_period_ms_candidate =
			irs->last_response_ts + irs->inter_chunk_timeout_ms - current_ts;

		sleep_period_ms = (sleep_period_ms_candidate < sleep_period_ms)
					  ? sleep_period_ms_candidate
					  : sleep_period_ms;
	}

	return SUIT_PLAT_ERR_UNREACHABLE_PATH;
}

/* Assumption - access to image_request_state locked on entry/exit
 */
static image_request_state_t *image_request_state_allocate(const uint8_t *resource_id,
							   size_t resource_id_length,
							   struct stream_sink *sink,
							   uint32_t inter_chunk_timeout_ms,
							   uint32_t requesting_period_ms)
{
	image_request_state_t *irs = &image_request_state;

	if (irs->stage != STAGE_IDLE) {
		return NULL;
	}

	irs->stage = STAGE_PENDING_FIRST_RESPONSE;
	irs->resource_id = resource_id;
	irs->resource_id_length = resource_id_length;
	irs->sink = sink;
	irs->inter_chunk_timeout_ms = inter_chunk_timeout_ms;
	irs->requesting_period_ms = requesting_period_ms;

	if (0 == ++last_used_stream_session_id) {
		++last_used_stream_session_id;
	}

	irs->stream_session_id = last_used_stream_session_id;

	/* Initialized with 0 so suit_ssf_server_missing_image_notify will be executed immediately
	 */
	irs->last_request_ts = 0;
	/* Initialized with current time stamp to avoid immediate inter-chunk timeout
	 */
	irs->last_response_ts = k_uptime_get();
	irs->last_arrival_number = 0;
	irs->last_processed_number = 0;
	irs->current_sink_write_offset = 0;
	irs->last_chunk_arrival_number = 0;
	irs->completion_error_code = SUIT_PLAT_SUCCESS;

	for (int i = 0; i < CONFIG_SUIT_STREAM_IPC_REQUESTOR_MAX_CHUNKS; i++) {
		chunk_processing_state_t *cps = &irs->chunk_processing_state[i];

		cps->status = CHUNK_SLOT_EMPTY;
	}

	return irs;
}

static size_t cache_size_adjust(size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();

	if (line_size == 0) {
		return size;
	}

	return ROUND_UP(size, line_size);
}

static bool check_cache_alignment(uintptr_t address)
{
	size_t line_size = sys_cache_data_line_size_get();

	if (line_size == 0) {
		return true;
	}

	return (address % line_size) == 0;
}

suit_plat_err_t suit_ipc_streamer_stream(const uint8_t *resource_id, size_t resource_id_length,
					 struct stream_sink *sink, uint32_t inter_chunk_timeout_ms,
					 uint32_t requesting_period_ms)
{
	if (resource_id == NULL || resource_id_length == 0 || sink == NULL || sink->write == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	image_request_state_lock();
	image_request_state_t *irs =
		image_request_state_allocate(resource_id, resource_id_length, sink,
					     inter_chunk_timeout_ms, requesting_period_ms);
	if (irs == NULL) {
		image_request_state_unlock();
		return SUIT_PLAT_ERR_NO_RESOURCES;
	}

	suit_plat_err_t err = data_loop(irs);

	image_request_state_unlock();

	return err;
}

suit_plat_err_t suit_ipc_streamer_chunk_enqueue(uint32_t stream_session_id, uint32_t chunk_id,
						uint32_t offset, uint8_t *address, uint32_t size,
						bool last_chunk)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	if (address != NULL && size != 0) {
		size_t aligned_size = cache_size_adjust(size);

		if (!check_cache_alignment((uintptr_t)address)) {
			/* The address is not aligned to cache line size. */
			err = SUIT_PLAT_ERR_INVAL;
		}

		int result = sys_cache_data_flush_range(address, aligned_size);

		/*
		 * EAGAIN means that the cache is not enabled.
		 * ENOTSUP means there is no cache in the system.
		 */
		if (result != 0 && result != -EAGAIN && result != -ENOTSUP) {
			err = SUIT_PLAT_ERR_IO;
		}
	} else if (address == NULL && size == 0) {

		/* that combination of partameters is valid, caller may use it to
		 * signalize the last chunk or to 'seek' to new offset in target image
		 */

	} else {
		err = SUIT_PLAT_ERR_INVAL;
	}

	image_request_state_lock();

	image_request_state_t *irs = &image_request_state;

	if (err == SUIT_PLAT_SUCCESS) {
		if (irs->stage == STAGE_IDLE || irs->stream_session_id != stream_session_id) {
			err = SUIT_PLAT_ERR_INCORRECT_STATE;
		}
	}

	if (err == SUIT_PLAT_SUCCESS) {
		if (irs->stage == STAGE_CLOSING || irs->last_chunk_arrival_number != 0) {
			/* connection is closing, no new chunks will be accepted
			 */
			err = SUIT_PLAT_ERR_INCORRECT_STATE;
		}
	}

	if (err == SUIT_PLAT_SUCCESS) {

		if (irs->stage == STAGE_PENDING_FIRST_RESPONSE) {
			irs->stage = STAGE_IN_PROGRESS;
		}

		irs->last_response_ts = k_uptime_get();

		chunk_processing_state_t *cps = empty_chunk_slot_get(irs);

		if (cps == NULL) {
			/* not enough space to store chunk info. Try again later
			 */
			err = SUIT_PLAT_ERR_BUSY;
		} else {
			cps->status = CHUNK_PENDING;
			cps->chunk_id = chunk_id;
			cps->offset = offset;
			cps->address = address;
			cps->size = size;

			cps->arrival_number = ++irs->last_arrival_number;

			if (last_chunk) {
				irs->last_chunk_arrival_number = cps->arrival_number;
			}

			struct k_sem *data_loop_kick_sem = irs->data_loop_kick_sem;

			k_sem_give(data_loop_kick_sem);
		}
	}

	image_request_state_unlock();

	return err;
}

suit_plat_err_t suit_ipc_streamer_chunk_status_req(uint32_t stream_session_id,
						   suit_ipc_streamer_chunk_info_t *chunk_info,
						   size_t *chunk_info_count)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	if (chunk_info == NULL || chunk_info_count == NULL) {
		err = SUIT_PLAT_ERR_INVAL;
	}

	image_request_state_lock();

	image_request_state_t *irs = &image_request_state;

	if (err == SUIT_PLAT_SUCCESS) {
		if (irs->stage == STAGE_IDLE || irs->stream_session_id != stream_session_id) {
			err = SUIT_PLAT_ERR_INCORRECT_STATE;
		}
	}

	uint32_t count = 0;

	if (err == SUIT_PLAT_SUCCESS) {

		uint32_t min_arrival_number = 1;

		count = non_empty_chunk_slot_count_get(irs);
		if (count > *chunk_info_count) {
			err = SUIT_PLAT_ERR_BUSY;
		}

		count = 0;
		while (count < CONFIG_SUIT_STREAM_IPC_REQUESTOR_MAX_CHUNKS) {

			chunk_processing_state_t *cps =
				non_empty_chunk_slot_get_next(irs, min_arrival_number);
			if (cps != NULL) {

				min_arrival_number = cps->arrival_number + 1;

				if (err == SUIT_PLAT_SUCCESS) {
					suit_ipc_streamer_chunk_info_t *entry = &chunk_info[count];
					*chunk_info_count = count + 1;
					entry->chunk_id = cps->chunk_id;

					if (cps->status == CHUNK_PENDING) {
						entry->status = PENDING;

					} else if (cps->status == CHUNK_PROCESSED_SUCCESS) {
						entry->status = PROCESSED;

					} else {
						entry->status = REFUSED;
					}
				}

				if (cps->status != CHUNK_PENDING) {
					cps->status = CHUNK_SLOT_EMPTY;
				}

				count++;
			} else {
				break;
			}
		}
	}

	image_request_state_unlock();

	return err;
}

suit_plat_err_t
suit_ipc_streamer_chunk_status_evt_subscribe(suit_ipc_streamer_chunk_status_notify_fn notify_fn,
					     void *context)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	image_request_state_lock();

	if (NULL == chunk_status_notify_fn) {
		chunk_status_notify_fn = notify_fn;
		chunk_status_notify_context = context;
	} else {
		err = SUIT_PLAT_ERR_NO_RESOURCES;
	}

	image_request_state_unlock();

	return err;
}

void suit_ipc_streamer_chunk_status_evt_unsubscribe(void)
{
	image_request_state_lock();
	chunk_status_notify_fn = NULL;
	chunk_status_notify_context = 0;
	image_request_state_unlock();
}

suit_plat_err_t
suit_ipc_streamer_missing_image_evt_subscribe(suit_ipc_streamer_missing_image_notify_fn notify_fn,
					      void *context)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	image_request_state_lock();

	if (NULL == missing_image_notify_fn) {
		missing_image_notify_fn = notify_fn;
		missing_image_notify_context = context;
	} else {
		err = SUIT_PLAT_ERR_NO_RESOURCES;
	}

	image_request_state_unlock();

	return err;
}

void suit_ipc_streamer_missing_image_evt_unsubscribe(void)
{
	image_request_state_lock();
	missing_image_notify_fn = NULL;
	missing_image_notify_context = 0;
	image_request_state_unlock();
}

suit_plat_err_t suit_ipc_streamer_requestor_init(void)
{
	return SUIT_PLAT_SUCCESS;
}
