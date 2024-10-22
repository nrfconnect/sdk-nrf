/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "lc3_streamer.h"
#include "lc3_file.h"

#include <errno.h>
#include <data_fifo.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lc3_streamer, CONFIG_MODULE_SD_CARD_LC3_STREAMER_LOG_LEVEL);

K_THREAD_STACK_DEFINE(lc3_streamer_work_q_stack_area, CONFIG_SD_CARD_LC3_STREAMER_STACK_SIZE);

struct k_work_q lc3_streamer_work_q;

#define LC3_STREAMER_BUFFER_NUM_FRAMES 2

#if CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS > UINT8_MAX
#error "CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS must be less than or equal to UINT8_MAX"
#endif

enum lc3_stream_states {
	/* Stream ready to load file and start streaming */
	STREAM_IDLE = 0,

	/* Stream currently playing */
	STREAM_PLAYING,

	/* The last frame in the file is loaded and accessible for the caller */
	STREAM_PLAYING_LAST_FRAME,

	/* Stream has ended. Resources need to be cleaned for stream to be restarted. */
	STREAM_ENDED,
};

struct lc3_stream {
	/* State of the stream */
	enum lc3_stream_states state;

	/* Flag set at initialization to restart a stream when it reaches end. */
	bool loop_stream;

	/* Pointer to the data_fifo buffer that holds valid, readable LC3 data */
	char *active_buffer;

	/* Filename of the file being streamed */
	char filename[CONFIG_FS_FATFS_MAX_LFN];

	/* LC3 file context */
	struct lc3_file_ctx file;

	/* Work queue context */
	struct k_work work;

	/* data_fifo context */
	struct data_fifo fifo;

	/* Buffers used by data_fifo for allocating memory*/
	char msgq_buffer[LC3_STREAMER_BUFFER_NUM_FRAMES * sizeof(struct data_fifo_msgq)];
	char slab_buffer[LC3_STREAMER_BUFFER_NUM_FRAMES *
			 CONFIG_SD_CARD_LC3_STREAMER_MAX_FRAME_SIZE];
};

static struct lc3_stream streams[CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS];

static bool initialized;

/**
 * @brief Close the stream and free all resources.
 *
 * @param[in]	stream	Pointer to the stream to close.
 *
 * @retval	0	Success, negative value otherwise.
 */
static int stream_close(struct lc3_stream *stream)
{
	int ret;

	if (stream == NULL) {
		LOG_ERR("Nullptr received for stream");
		return -EINVAL;
	}

	if (stream->active_buffer != NULL) {
		data_fifo_block_free(&stream->fifo, (void *)stream->active_buffer);
		stream->active_buffer = NULL;
	}

	ret = lc3_file_close(&stream->file);
	if (ret) {
		LOG_ERR("Failed to close file %d", ret);
	}

	if (stream->fifo.initialized) {
		ret = data_fifo_uninit(&stream->fifo);
		if (ret) {
			LOG_ERR("Failed to empty data fifo %d", ret);
		}
	}

	stream->state = STREAM_IDLE;
	memset(stream->filename, 0, sizeof(stream->filename));

	return 0;
}

/**
 * @brief Get the next frame from the file and put it in the fifo.
 *
 * @param[in]	stream	Pointer to the stream to get the frame for.
 *
 * @retval	0	Success, negative value otherwise.
 */
static int put_next_frame_to_fifo(struct lc3_stream *stream)
{
	int ret;
	char *data_ptr;

	ret = data_fifo_pointer_first_vacant_get(&stream->fifo, (void **)&data_ptr, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to get first vacant block %d", ret);
		return ret;
	}

	ret = lc3_file_frame_get(&stream->file, data_ptr,
				 CONFIG_SD_CARD_LC3_STREAMER_MAX_FRAME_SIZE);
	if (ret) {
		if (ret != -ENODATA) {
			LOG_ERR("Failed to get frame from file %d", ret);
		}

		data_fifo_block_free(&stream->fifo, (void *)data_ptr);
		return ret;
	}

	ret = data_fifo_block_lock(&stream->fifo, (void **)&data_ptr,
				   CONFIG_SD_CARD_LC3_STREAMER_MAX_FRAME_SIZE);
	if (ret) {
		LOG_ERR("Failed to lock block %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Loop the stream by closing and re-opening the file, and loading the first frame.
 *
 * @param[in]	stream	Pointer to the stream to loop.
 *
 * @retval	0	Success, negative value otherwise.
 */
static int stream_loop(struct lc3_stream *stream)
{
	int ret;

	ret = lc3_file_close(&stream->file);
	if (ret) {
		LOG_ERR("Failed to close file %d", ret);
		return ret;
	}

	ret = lc3_file_open(&stream->file, stream->filename);
	if (ret) {
		LOG_ERR("Failed to open file %s: %d", stream->filename, ret);
		return ret;
	}

	ret = put_next_frame_to_fifo(stream);
	if (ret) {
		LOG_ERR("Failed to put first frame after loop to fifo %d", ret);

		int lc3_file_ret = lc3_file_close(&stream->file);

		if (lc3_file_ret) {
			LOG_ERR("Failed to close file %d", lc3_file_ret);
		}

		return ret;
	}

	return 0;
}

/**
 * @brief Load the next frame from the stream to the fifo. This is the work queue function.
 *
 * @param[in]	work	Pointer to the work queue item.
 */
static void next_frame_load(struct k_work *work)
{
	int ret;
	struct lc3_stream *stream = CONTAINER_OF(work, struct lc3_stream, work);

	ret = put_next_frame_to_fifo(stream);
	if (ret == -ENODATA) {
		LOG_DBG("End of stream");
		if (stream->loop_stream) {
			ret = stream_loop(stream);
			if (ret) {
				LOG_ERR("Failed to loop stream %d", ret);
				stream->state = STREAM_ENDED;
			}
		} else {
			stream->state = STREAM_PLAYING_LAST_FRAME;
		}
	} else if (ret) {
		LOG_ERR("Failed to put next frame to fifo %d", ret);
		stream->state = STREAM_ENDED;
	}
}

int lc3_streamer_next_frame_get(const uint8_t streamer_idx, const uint8_t **const frame_buffer)
{
	int ret;
	char *data_ptr;
	size_t data_len;

	if (!initialized) {
		LOG_ERR("LC3 streamer not initialized");
		return -EFAULT;
	}

	if (streamer_idx >= ARRAY_SIZE(streams)) {
		LOG_ERR("Invalid streamer index %d", streamer_idx);
		return -EINVAL;
	}

	struct lc3_stream *stream = &streams[streamer_idx];

	if ((stream->state != STREAM_PLAYING) && (stream->state != STREAM_PLAYING_LAST_FRAME)) {
		LOG_ERR("Stream not playing");
		return -EFAULT;
	}

	if (stream->active_buffer != NULL) {
		data_fifo_block_free(&stream->fifo, (void *)stream->active_buffer);
		stream->active_buffer = NULL;
	}

	if (stream->state == STREAM_PLAYING_LAST_FRAME) {
		LOG_INF("Stream ended");
		stream->state = STREAM_ENDED;
		return -ENODATA;
	}

	ret = data_fifo_pointer_last_filled_get(&stream->fifo, (void **)&data_ptr, &data_len,
						K_NO_WAIT);
	if (ret) {
		if (ret == -ENOMSG) {
			LOG_DBG("Next block is not ready %d", ret);
		} else {
			LOG_ERR("Failed to get last filled block %d", ret);
		}
		return ret;
	}

	*frame_buffer = (uint8_t *)data_ptr;
	stream->active_buffer = data_ptr;

	ret = k_work_submit_to_queue(&lc3_streamer_work_q, &stream->work);
	if (ret < 0) {
		LOG_ERR("Failed to submit work item %d", ret);
		return ret;
	}

	return 0;
}

bool lc3_streamer_file_compatible_check(const char *const filename,
					const struct lc3_stream_cfg *const cfg)
{
	int ret;
	bool result = true;

	if (filename == NULL || cfg == NULL) {
		LOG_ERR("NULL pointer received");
		return false;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN - 1) {
		LOG_ERR("Filename too long");
		return false;
	}

	struct lc3_file_ctx file;

	ret = lc3_file_open(&file, filename);
	if (ret) {
		LOG_ERR("Failed to open file %d", ret);
		return false;
	}

	struct lc3_file_header header;

	ret = lc3_header_get(&file, &header);
	if (ret) {
		LOG_WRN("Failed to get header %d", ret);
		return false;
	}

	if ((header.sample_rate * 100) != cfg->sample_rate_hz) {
		LOG_WRN("Sample rate mismatch %d Hz != %d Hz", (header.sample_rate * 100),
			cfg->sample_rate_hz);
		result = false;
	}

	if ((header.bit_rate * 100) != cfg->bit_rate_bps) {
		LOG_WRN("Bit rate mismatch %d bps != %d bps", (header.bit_rate * 100),
			cfg->bit_rate_bps);
		result = false;
	}

	if ((header.frame_duration * 10) != cfg->frame_duration_us) {
		LOG_WRN("Frame duration mismatch %d us != %d us", (header.frame_duration * 10),
			cfg->frame_duration_us);
		result = false;
	}

	ret = lc3_file_close(&file);
	if (ret) {
		LOG_ERR("Failed to close file %d", ret);
	}

	return result;
}

int lc3_streamer_stream_register(const char *const filename, uint8_t *const streamer_idx,
				 const bool loop)
{
	int ret;

	if (!initialized) {
		LOG_ERR("LC3 streamer not initialized");
		return -EFAULT;
	}

	if ((streamer_idx == NULL) || (filename == NULL)) {
		LOG_ERR("Nullptr received for streamer_idx or filename");
		return -EINVAL;
	}

	/* Check that there's room for the filename and a NULL terminating char */
	if (strlen(filename) > (ARRAY_SIZE(streams[*streamer_idx].filename) - 1)) {
		LOG_ERR("Filename too long");
		return -EINVAL;
	}

	bool free_slot_found = false;

	for (int i = 0; i < ARRAY_SIZE(streams); i++) {
		if (streams[i].state == STREAM_IDLE) {
			LOG_DBG("Found free stream slot %d", i);
			*streamer_idx = i;
			free_slot_found = true;
			break;
		}
	}

	if (!free_slot_found) {
		LOG_ERR("No stream slot is available");
		return -EAGAIN;
	}

	ret = lc3_file_open(&streams[*streamer_idx].file, filename);
	if (ret) {
		LOG_ERR("Failed to open file %d", ret);
		return ret;
	}

	strcpy(streams[*streamer_idx].filename, filename);

	ret = data_fifo_init(&streams[*streamer_idx].fifo);
	if (ret) {
		LOG_ERR("Failed to initialize data fifo %d", ret);
		int lc3_file_ret;

		lc3_file_ret = lc3_file_close(&streams[*streamer_idx].file);
		if (lc3_file_ret) {
			LOG_ERR("Failed to close file %d", lc3_file_ret);
		}

		return ret;
	}

	k_work_init(&streams[*streamer_idx].work, next_frame_load);

	ret = put_next_frame_to_fifo(&streams[*streamer_idx]);
	if (ret) {
		LOG_ERR("Failed to put next frame to fifo %d", ret);
		streams[*streamer_idx].state = STREAM_ENDED;
		return ret;
	}

	streams[*streamer_idx].state = STREAM_PLAYING;
	streams[*streamer_idx].loop_stream = loop;

	return 0;
}

uint8_t lc3_streamer_num_active_streams(void)
{
	uint8_t num_active = 0;

	if (!initialized) {
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(streams); i++) {
		if ((streams[i].state == STREAM_PLAYING) ||
		    (streams[i].state == STREAM_PLAYING_LAST_FRAME)) {
			num_active++;
		}
	}

	return num_active;
}

int lc3_streamer_file_path_get(const uint8_t streamer_idx, char *const path, const size_t path_len)
{
	if (streamer_idx >= ARRAY_SIZE(streams)) {
		LOG_ERR("Invalid streamer index %d", streamer_idx);
		return -EINVAL;
	}

	if (path == NULL) {
		LOG_ERR("Nullptr received for path");
		return -EINVAL;
	}

	if (path_len < strlen(streams[streamer_idx].filename)) {
		LOG_WRN("Path buffer too small");
	}

	strncpy(path, streams[streamer_idx].filename, path_len);

	return 0;
}

bool lc3_streamer_is_looping(const uint8_t streamer_idx)
{
	if (streamer_idx >= ARRAY_SIZE(streams)) {
		LOG_ERR("Invalid streamer index %d", streamer_idx);
		return false;
	}

	return streams[streamer_idx].loop_stream;
}

int lc3_streamer_stream_close(const uint8_t streamer_idx)
{
	int ret;

	if (!initialized) {
		LOG_ERR("LC3 streamer not initialized");
		return -EFAULT;
	}

	if (streamer_idx >= ARRAY_SIZE(streams)) {
		LOG_ERR("Invalid streamer index %d", streamer_idx);
		return -EINVAL;
	}

	ret = stream_close(&streams[streamer_idx]);
	if (ret) {
		LOG_ERR("Failed to close stream %d", ret);
		return ret;
	}

	return 0;
}

int lc3_streamer_close_all_streams(void)
{
	int ret;

	if (!initialized) {
		LOG_ERR("LC3 streamer not initialized");
		return -EFAULT;
	}

	ret = k_work_queue_drain(&lc3_streamer_work_q, false);
	if (ret < 0) {
		LOG_ERR("Failed to drain work queue %d", ret);
		return ret;
	}

	int first_error = 0;

	for (int i = 0; i < ARRAY_SIZE(streams); i++) {
		ret = stream_close(&streams[i]);
		if (ret) {
			LOG_ERR("Failed to close stream %d %d", i, ret);
			if (!first_error) {
				first_error = ret;
			}
		}
	}

	return first_error;
}

int lc3_streamer_init(void)
{
	int ret;

	if (initialized) {
		LOG_WRN("LC3 streamer already initialized");
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i].fifo.msgq_buffer = streams[i].msgq_buffer;
		streams[i].fifo.slab_buffer = streams[i].slab_buffer;
		streams[i].fifo.block_size_max = WB_UP(CONFIG_SD_CARD_LC3_STREAMER_MAX_FRAME_SIZE);
		streams[i].fifo.elements_max = LC3_STREAMER_BUFFER_NUM_FRAMES;
		streams[i].fifo.initialized = false;
		streams[i].active_buffer = NULL;
		streams[i].state = STREAM_IDLE;
	}

	ret = lc3_file_init();
	if (ret) {
		LOG_ERR("Failed to initialize LC3 file module %d", ret);
		return ret;
	}

	k_work_queue_init(&lc3_streamer_work_q);
	k_work_queue_start(&lc3_streamer_work_q, lc3_streamer_work_q_stack_area,
			   K_THREAD_STACK_SIZEOF(lc3_streamer_work_q_stack_area),
			   CONFIG_SD_CARD_LC3_STREAMER_THREAD_PRIO, NULL);
	k_thread_name_set(&lc3_streamer_work_q.thread, "lc3_streamer_work_q");

	initialized = true;

	return 0;
}
