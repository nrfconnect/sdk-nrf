/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "lc3_streamer.h"
#include "lc3_file.h"
#include "data_fifo.h"

#include <errno.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lc3_streamer, 3);

K_THREAD_STACK_DEFINE(lc3_streamer_work_q_stack_area, CONFIG_SD_CARD_LC3_STREAMER_STACK_SIZE);

struct k_work_q lc3_streamer_work_q;

#define LC3_STREAMER_NUM_FRAMES 2

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
	char msgq_buffer[LC3_STREAMER_NUM_FRAMES * sizeof(struct data_fifo_msgq)];
	char slab_buffer[LC3_STREAMER_NUM_FRAMES * CONFIG_SD_CARD_LC3_STREAMER_MAX_FRAME_SIZE];
};

static struct lc3_stream streams[CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS];

/**
 * @brief Close the stream and free all resources.
 *
 * @param[in]	stream	Pointer to the stream to close.
 *
 * @retval	0	Success, negative value otherwise.
 */
static int close_stream(struct lc3_stream *stream)
{
	int ret;

	if (stream->active_buffer != NULL) {
		data_fifo_block_free(&stream->fifo, (void *)stream->active_buffer);
		stream->active_buffer = NULL;
	}

	ret = lc3_file_close(&stream->file);
	if (ret) {
		LOG_ERR("Failed to close file %d", ret);
	}

	ret = data_fifo_empty(&stream->fifo);
	if (ret) {
		LOG_ERR("Failed to empty data fifo %d", ret);
	}

	stream->state = STREAM_IDLE;

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
		LOG_ERR("Failed to get frame from file %d", ret);
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
static int loop_stream(struct lc3_stream *stream)
{
	int ret;

	ret = lc3_file_close(&stream->file);
	if (ret) {
		LOG_ERR("Failed to close file %d", ret);
		return ret;
	}

	ret = lc3_file_open(&stream->file, stream->filename);
	if (ret) {
		LOG_ERR("Failed to open file %d", ret);
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
static void load_next_frame(struct k_work *work)
{
	int ret;
	struct lc3_stream *stream = CONTAINER_OF(work, struct lc3_stream, work);

	ret = put_next_frame_to_fifo(stream);
	if (ret == -ENODATA) {
		LOG_DBG("End of stream");
		if (stream->loop_stream) {
			ret = loop_stream(stream);
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

int lc3_streamer_next_frame_get(int streamer_idx, uint8_t **frame_buffer)
{
	int ret;
	char *data_ptr;
	size_t data_len;

	if (streamer_idx < 0 || streamer_idx >= CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS) {
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
		LOG_ERR("Failed to get last filled block %d", ret);
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

int lc3_streamer_register_stream(char *filename, ssize_t *streamer_idx, bool loop)
{
	int ret;
	ssize_t idx = -1;

	if (filename == NULL) {
		LOG_ERR("Nullptr received for filename");
		*streamer_idx = idx;
		return -EINVAL;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename too long");
		*streamer_idx = idx;
		return -EINVAL;
	}

	if (streamer_idx == NULL) {
		LOG_ERR("Nullptr received for streamer_idx");
		return -EINVAL;
	}

	for (int i = 0; i < CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS; i++) {
		if (streams[i].state == STREAM_IDLE) {
			LOG_DBG("Found free stream slot %d", i);
			idx = i;
			break;
		}
	}

	if (idx == -1) {
		LOG_ERR("No stream slot is available");
		*streamer_idx = idx;
		return -EAGAIN;
	}

	strncpy(streams[idx].filename, filename, strlen(filename));

	ret = lc3_file_open(&streams[idx].file, streams[idx].filename);
	if (ret) {
		LOG_ERR("Failed to open file %d", ret);
		*streamer_idx = -1;
		return ret;
	}

	ret = data_fifo_init(&streams[idx].fifo);
	if (ret) {
		LOG_ERR("Failed to initialize data fifo %d", ret);
		int lc3_file_ret;

		*streamer_idx = -1;
		lc3_file_ret = lc3_file_close(&streams[idx].file);
		if (lc3_file_ret) {
			LOG_ERR("Failed to close file %d", lc3_file_ret);
		}

		return ret;
	}

	k_work_init(&streams[idx].work, load_next_frame);

	ret = put_next_frame_to_fifo(&streams[idx]);
	if (ret) {
		LOG_ERR("Failed to put next frame to fifo %d", ret);
		int lc3_file_ret;

		*streamer_idx = -1;
		lc3_file_ret = lc3_file_close(&streams[idx].file);
		if (lc3_file_ret) {
			LOG_ERR("Failed to close file %d", lc3_file_ret);
		}

		return ret;
	}

	streams[idx].state = STREAM_PLAYING;
	streams[idx].loop_stream = loop;
	*streamer_idx = idx;

	return 0;
}

unsigned int lc3_streamer_num_active_streams(void)
{
	unsigned int num_active = 0;

	for (int i = 0; i < CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS; i++) {
		if ((streams[i].state == STREAM_PLAYING) ||
		    (streams[i].state == STREAM_PLAYING_LAST_FRAME)) {
			num_active++;
		}
	}

	return num_active;
}

int lc3_streamer_end_stream(int streamer_idx)
{
	int ret;
	struct lc3_stream *stream;

	if (streamer_idx < 0 || streamer_idx >= CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS) {
		LOG_ERR("Invalid streamer index %d", streamer_idx);
		return -EINVAL;
	}

	stream = &streams[streamer_idx];

	ret = close_stream(stream);
	if (ret) {
		LOG_ERR("Failed to close stream %d", ret);
		return ret;
	}

	return 0;
}

int lc3_streamer_clear_all_streams(void)
{
	int ret;

	ret = k_work_queue_drain(&lc3_streamer_work_q, false);
	if (ret < 0) {
		LOG_ERR("Failed to drain work queue %d", ret);
		return ret;
	}

	for (int i = 0; i < CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS; i++) {
		if (streams[i].state == STREAM_PLAYING) {
			data_fifo_empty(&streams[i].fifo);
			streams[i].state = STREAM_IDLE;
		}
	}

	return 0;
}

int lc3_streamer_init(void)
{
	int ret;

	for (int i = 0; i < CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS; i++) {
		streams[i].fifo.msgq_buffer = streams[i].msgq_buffer;
		streams[i].fifo.slab_buffer = streams[i].slab_buffer;
		streams[i].fifo.block_size_max = CONFIG_SD_CARD_LC3_STREAMER_MAX_FRAME_SIZE;
		streams[i].fifo.elements_max = LC3_STREAMER_NUM_FRAMES;
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
			   CONFIG_SD_CARD_LC3_STREAMER_THREAD_PRIORITY, NULL);
	k_thread_name_set(&lc3_streamer_work_q.thread, "lc3_streamer_work_q");

	return 0;
}
