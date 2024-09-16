/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <modules/lc3_streamer.h>
#include <stdint.h>

#include "lc3_file/lc3_file_fake.h"
#include "lc3_file/lc3_file_fake_data.h"
#include "k_work/k_work_fake.h"

DEFINE_FFF_GLOBALS;

static void *suite_setup(void)
{
	DO_FOREACH_LC3_FILE_FAKE(RESET_FAKE);
	DO_FOREACH_K_WORK_FAKE(RESET_FAKE);

	FFF_RESET_HISTORY();

	int ret = lc3_streamer_init();

	zassert_equal(0, ret, "lc3_streamer_init should return success");

	return NULL;
}

static void test_setup(void *f)
{
	ARG_UNUSED(f);

	DO_FOREACH_LC3_FILE_FAKE(RESET_FAKE);
	DO_FOREACH_K_WORK_FAKE(RESET_FAKE);

	FFF_RESET_HISTORY();
}

static void test_teardown(void *f)
{
	int ret;

	ret = lc3_streamer_close_all_streams();
	zassert_equal(0, ret, "lc3_streamer_close_all_streams should return success");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_register_valid)
{
	int ret;
	uint8_t streamer_idx;

	static const char test_filename[] = "test";

	ret = lc3_streamer_stream_register(test_filename, &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");
	zassert_mem_equal(test_filename, lc3_file_open_fake.arg1_val, sizeof(test_filename),
			  "lc3_file_open called with wrong filename");

	int num_active = lc3_streamer_num_active_streams();

	zassert_equal(1, num_active, "Number of active streams should be 1");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_register_valid_register_closed)
{
	int ret;
	int num_active;
	uint8_t streamer_idx;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	num_active = lc3_streamer_num_active_streams();
	zassert_equal(1, num_active, "Number of active streams should be 1");

	ret = lc3_streamer_stream_close(streamer_idx);
	zassert_equal(0, ret, "lc3_streamer_stream_close should return success");

	num_active = lc3_streamer_num_active_streams();
	zassert_equal(0, num_active, "Number of active streams should be 0");

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	num_active = lc3_streamer_num_active_streams();
	zassert_equal(1, num_active, "Number of active streams should be 1");
}

ZTEST(lc3_streamer, test_lc3_streamer_register_valid_used_slot)
{
	int ret;
	uint8_t streamer_idx;

	char result_filename[CONFIG_FS_FATFS_MAX_LFN];
	static const char test_filename[] = "test";
	static const char long_filename[] = "this_is_a_long_filename_for_testing";
	static const char short_filename[] = "short";

	/* Fill all three available slots */
	ret = lc3_streamer_stream_register(long_filename, &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	ret = lc3_streamer_stream_register(long_filename, &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(1, streamer_idx, "lc3_streamer_stream_register should return index 1");

	ret = lc3_streamer_stream_register(long_filename, &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(2, streamer_idx, "lc3_streamer_stream_register should return index 2");

	/* Close second slot */
	ret = lc3_streamer_stream_close(1);
	zassert_equal(0, ret, "lc3_streamer_stream_close should return success");

	/* Open new slot */
	ret = lc3_streamer_stream_register(short_filename, &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(1, streamer_idx, "lc3_streamer_stream_register should return index 1");

	zassert_mem_equal(short_filename, lc3_file_open_fake.arg1_val, sizeof(test_filename),
			  "lc3_file_open called with wrong filename (%s)",
			  lc3_file_open_fake.arg1_val);

	ret = lc3_streamer_file_path_get(streamer_idx, result_filename, sizeof(result_filename));
	zassert_equal(0, ret, "lc3_streamer_file_path_get should return success");
	zassert_mem_equal(short_filename, result_filename, strlen(result_filename),
			  "lc3_streamer_file_path_get should return the correct filename");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_register_invalid_nullptr)
{
	int ret;
	uint8_t streamer_idx;

	ret = lc3_streamer_stream_register(NULL, &streamer_idx, false);
	zassert_equal(-EINVAL, ret,
		      "lc3_streamer_stream_register should return -EINVAL on nullptr");

	ret = lc3_streamer_stream_register("test", NULL, false);
	zassert_equal(-EINVAL, ret,
		      "lc3_streamer_stream_register should return -EINVAL on nullptr");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_register_invalid_long_filename)
{
	int ret;
	uint8_t streamer_idx;
	char test_string_valid[CONFIG_FS_FATFS_MAX_LFN];
	char test_string_invalid[CONFIG_FS_FATFS_MAX_LFN + 1];

	memset(test_string_valid, 'a', CONFIG_FS_FATFS_MAX_LFN);
	test_string_valid[CONFIG_FS_FATFS_MAX_LFN - 1] = '\0';

	memset(test_string_invalid, 'a', CONFIG_FS_FATFS_MAX_LFN + 1);
	test_string_invalid[CONFIG_FS_FATFS_MAX_LFN] = '\0';

	ret = lc3_streamer_stream_register(test_string_valid, &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");

	ret = lc3_streamer_stream_register(test_string_invalid, &streamer_idx, false);
	zassert_equal(-EINVAL, ret,
		      "lc3_streamer_stream_register should return -EINVAL on too long filename");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_register_invalid_too_many_streams)
{
	int ret;
	uint8_t streamer_idx;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(1, streamer_idx, "lc3_streamer_stream_register should return index 1");

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(2, streamer_idx, "lc3_streamer_stream_register should return index 2");

	int num_active = lc3_streamer_num_active_streams();

	zassert_equal(CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS, num_active,
		      "lc3_streamer_num_active_streams should return maximum number of streams");

	/* Fourth stream should fail */
	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(-EAGAIN, ret, "lc3_streamer_stream_register should return success");

	zassert_equal(3, lc3_file_open_fake.call_count,
		      "lc3_file_open should be called 3 times (once for each valid stream)");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_register_invalid_file_open)
{
	int ret;
	uint8_t streamer_idx;

	lc3_file_open_fake.return_val = -ENODEV;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(-ENODEV, ret,
		      "lc3_streamer_stream_register should return -ENODEV from lc3_file");
	zassert_equal(1, lc3_file_open_fake.call_count, "lc3_file_open should be called once");
	zassert_equal(0, lc3_file_frame_get_fake.call_count,
		      "lc3_file_frame_get should not be called");
	zassert_equal(0, k_work_init_fake.call_count, "k_work_init should not be called");

	int num_active = lc3_streamer_num_active_streams();

	zassert_equal(0, num_active, "Number of active streams should be 0");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_register_invalid_frame_get)
{
	int ret;
	uint8_t streamer_idx;

	lc3_file_frame_get_fake.return_val = -ENODEV;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(-ENODEV, ret,
		      "lc3_streamer_stream_register should return -ENODEV from lc3_file");
	zassert_equal(1, lc3_file_frame_get_fake.call_count,
		      "lc3_file_frame_get should be called once");
	zassert_equal(1, lc3_file_open_fake.call_count, "lc3_file_open should be called once");
	zassert_equal(0, lc3_file_close_fake.call_count, "lc3_file_close should NOT be called");

	int num_active = lc3_streamer_num_active_streams();

	zassert_equal(0, num_active, "Number of active streams should be 0");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_register_invalid_no_frames)
{
	int ret;
	uint8_t streamer_idx;

	lc3_file_frame_get_fake.return_val = -ENODATA;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(-ENODATA, ret, "lc3_streamer_stream_register should return -ENODATA");
	zassert_equal(1, lc3_file_frame_get_fake.call_count,
		      "lc3_file_frame_get should be called once");
	zassert_equal(1, lc3_file_open_fake.call_count, "lc3_file_open should be called once");
	zassert_equal(0, lc3_file_close_fake.call_count, "lc3_file_close should NOT be called");

	int num_active = lc3_streamer_num_active_streams();

	zassert_equal(0, num_active, "Number of active streams should be 0");
}

ZTEST(lc3_streamer, test_lc3_streamer_next_frame_get_valid)
{
	int ret;
	uint8_t streamer_idx;
	const uint8_t *frame_buffer_1 = NULL;
	const uint8_t *frame_buffer_2 = NULL;
	const uint8_t *frame_buffer_3 = NULL;

	lc3_file_frame_get_fake.custom_fake = lc3_file_frame_get_fake_valid;
	k_work_submit_to_queue_fake.custom_fake = k_work_submit_to_queue_valid_fake;
	k_work_init_fake.custom_fake = k_work_init_valid_fake;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_1);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_1, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_2);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_2, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_3);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_3, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	zassert_not_equal(frame_buffer_1, frame_buffer_2,
			  "First and second buffer pointer should be different");
	zassert_equal(frame_buffer_1, frame_buffer_3,
		      "First and third buffer pointer should be the same");
}

ZTEST(lc3_streamer, test_lc3_streamer_next_frame_get_valid_stream_end)
{
	int ret;
	uint8_t streamer_idx;
	const uint8_t *frame_buffer_1 = NULL;
	const uint8_t *frame_buffer_2 = NULL;
	const uint8_t *frame_buffer_3 = NULL;

	int (*lc3_file_frame_get_custom_fakes[])(struct lc3_file_ctx *, uint8_t *, size_t) = {
		lc3_file_frame_get_fake_valid, lc3_file_frame_get_fake_valid,
		lc3_file_frame_get_fake_enodata, lc3_file_frame_get_fake_valid};

	SET_CUSTOM_FAKE_SEQ(lc3_file_frame_get, lc3_file_frame_get_custom_fakes,
			    ARRAY_SIZE(lc3_file_frame_get_custom_fakes));
	k_work_submit_to_queue_fake.custom_fake = k_work_submit_to_queue_valid_fake;
	k_work_init_fake.custom_fake = k_work_init_valid_fake;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_1);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_1, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	/* Work item submitted by this call will get -ENODATA from lc3_file_frame_get, setting state
	 * to PLAYING_LAST_FRAME.
	 */
	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_2);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_2, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	/* Stream is in state PLAYING_LAST_FRAME */
	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_3);
	zassert_equal(-ENODATA, ret, "lc3_streamer_next_frame_get should return success");
	zassert_equal(NULL, frame_buffer_3, "Frame data ptr should be NULL");
}

ZTEST(lc3_streamer, test_lc3_streamer_next_frame_get_invalid_index)
{
	int ret;
	const uint8_t *frame_buffer = NULL;

	ret = lc3_streamer_next_frame_get(CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS,
					  &frame_buffer);
	zassert_equal(-EINVAL, ret, "lc3_streamer_next_frame_get should return -EINVAL");
}

ZTEST(lc3_streamer, test_lc3_streamer_next_frame_get_invalid_not_playing)
{
	int ret;
	const uint8_t *frame_buffer = NULL;

	ret = lc3_streamer_next_frame_get(0, &frame_buffer);
	zassert_equal(-EFAULT, ret, "lc3_streamer_next_frame_get should return -EFAULT");
}

ZTEST(lc3_streamer, test_lc3_streamer_next_frame_get_invalid_queue_submit_error)
{
	int ret;
	uint8_t streamer_idx;
	const uint8_t *frame_buffer = NULL;

	lc3_file_frame_get_fake.custom_fake = lc3_file_frame_get_fake_valid;
	k_work_submit_to_queue_fake.return_val = -EINVAL;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer);
	zassert_equal(
		-EINVAL, ret,
		"lc3_streamer_next_frame_get should return -EINVAL from k_work_submit_to_queue");
}

ZTEST(lc3_streamer, test_lc3_streamer_next_frame_get_valid_loop_stream)
{
	int ret;
	uint8_t streamer_idx;
	const uint8_t *frame_buffer_1 = NULL;
	const uint8_t *frame_buffer_2 = NULL;

	char test_string[] = "test_filename";

	int (*lc3_file_frame_get_custom_fakes[])(struct lc3_file_ctx *, uint8_t *, size_t) = {
		lc3_file_frame_get_fake_valid, lc3_file_frame_get_fake_enodata,
		lc3_file_frame_get_fake_valid};

	SET_CUSTOM_FAKE_SEQ(lc3_file_frame_get, lc3_file_frame_get_custom_fakes,
			    ARRAY_SIZE(lc3_file_frame_get_custom_fakes));
	k_work_submit_to_queue_fake.custom_fake = k_work_submit_to_queue_valid_fake;
	k_work_init_fake.custom_fake = k_work_init_valid_fake;

	ret = lc3_streamer_stream_register(test_string, &streamer_idx, true);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	/* Work item submitted by this call will get -ENODATA from lc3_file_frame_get, which will
	 * trigger loop_stream()
	 */
	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_1);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_1, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	/* Verify stream is looped by fetching the next frame */
	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_2);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_2, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	zassert_equal(lc3_file_close_fake.call_count, 1, "lc3_file_close should be called once");
	zassert_equal(lc3_file_open_fake.call_count, 2, "lc3_file_open should be called twice");
	zassert_mem_equal(test_string, lc3_file_open_fake.arg1_val, sizeof(test_string),
			  "lc3_file_open should be called with test");
}

ZTEST(lc3_streamer, test_lc3_streamer_next_frame_get_invalid_loop_stream_file_close_fail)
{
	int ret;
	uint8_t streamer_idx;
	const uint8_t *frame_buffer_1 = NULL;
	const uint8_t *frame_buffer_2 = NULL;

	char test_string[] = "test_filename";

	lc3_file_close_fake.return_val = -EINVAL;

	int (*lc3_file_frame_get_custom_fakes[])(struct lc3_file_ctx *, uint8_t *, size_t) = {
		lc3_file_frame_get_fake_valid, lc3_file_frame_get_fake_enodata,
		lc3_file_frame_get_fake_valid};

	SET_CUSTOM_FAKE_SEQ(lc3_file_frame_get, lc3_file_frame_get_custom_fakes,
			    ARRAY_SIZE(lc3_file_frame_get_custom_fakes));
	k_work_submit_to_queue_fake.custom_fake = k_work_submit_to_queue_valid_fake;
	k_work_init_fake.custom_fake = k_work_init_valid_fake;

	ret = lc3_streamer_stream_register(test_string, &streamer_idx, true);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	/* Work item submitted by this call will get -ENODATA from lc3_file_frame_get, which will
	 * trigger loop_stream(), which will get -EINVAL from lc3_file_close
	 */
	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_1);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_1, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_2);
	zassert_equal(-EFAULT, ret,
		      "lc3_streamer_next_frame_get should return -EFAULT as stream should not be "
		      "playing");
	zassert_equal(NULL, frame_buffer_2, "Frame buffer should be null");
}

ZTEST(lc3_streamer, test_lc3_streamer_next_frame_get_invalid_loop_stream_file_open_fail)
{
	int ret;
	uint8_t streamer_idx;
	const uint8_t *frame_buffer_1 = NULL;
	const uint8_t *frame_buffer_2 = NULL;

	char test_string[] = "test_filename";

	int lc3_file_open_return_values[] = {0, -EINVAL};

	SET_RETURN_SEQ(lc3_file_open, lc3_file_open_return_values,
		       ARRAY_SIZE(lc3_file_open_return_values));

	int (*lc3_file_frame_get_custom_fakes[])(struct lc3_file_ctx *, uint8_t *, size_t) = {
		lc3_file_frame_get_fake_valid, lc3_file_frame_get_fake_enodata,
		lc3_file_frame_get_fake_valid};

	SET_CUSTOM_FAKE_SEQ(lc3_file_frame_get, lc3_file_frame_get_custom_fakes,
			    ARRAY_SIZE(lc3_file_frame_get_custom_fakes));
	k_work_submit_to_queue_fake.custom_fake = k_work_submit_to_queue_valid_fake;
	k_work_init_fake.custom_fake = k_work_init_valid_fake;

	ret = lc3_streamer_stream_register(test_string, &streamer_idx, true);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	/* Work item submitted by this call will get -ENODATA from lc3_file_frame_get, which will
	 * trigger loop_stream(), which will get -EINVAL from lc3_file_open
	 */
	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_1);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_1, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_2);
	zassert_equal(-EFAULT, ret,
		      "lc3_streamer_next_frame_get should return -EFAULT as stream should not be "
		      "playing");
	zassert_equal(NULL, frame_buffer_2, "Frame buffer should be null");
}

ZTEST(lc3_streamer, test_lc3_streamer_next_frame_get_invalid_loop_stream_frame_get_fail)
{
	int ret;
	uint8_t streamer_idx;
	const uint8_t *frame_buffer_1 = NULL;
	const uint8_t *frame_buffer_2 = NULL;

	char test_string[] = "test_filename";

	int (*lc3_file_frame_get_custom_fakes[])(struct lc3_file_ctx *, uint8_t *, size_t) = {
		lc3_file_frame_get_fake_valid, lc3_file_frame_get_fake_enodata,
		lc3_file_frame_get_fake_enodata};

	SET_CUSTOM_FAKE_SEQ(lc3_file_frame_get, lc3_file_frame_get_custom_fakes,
			    ARRAY_SIZE(lc3_file_frame_get_custom_fakes));
	k_work_submit_to_queue_fake.custom_fake = k_work_submit_to_queue_valid_fake;
	k_work_init_fake.custom_fake = k_work_init_valid_fake;

	ret = lc3_streamer_stream_register(test_string, &streamer_idx, true);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	/* Work item submitted by this call will get -ENODATA from lc3_file_frame_get, which will
	 * trigger loop_stream(), which will get -ENODATA from lc3_file_frame_get again.
	 */
	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_1);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer_1, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer_2);
	zassert_equal(-EFAULT, ret,
		      "lc3_streamer_next_frame_get should return -EFAULT as stream should not be "
		      "playing");
	zassert_equal(NULL, frame_buffer_2, "Frame buffer should be null");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_close_valid)
{
	int ret;
	uint8_t streamer_idx;
	const uint8_t *frame_buffer = NULL;

	lc3_file_frame_get_fake.custom_fake = lc3_file_frame_get_fake_valid;
	k_work_submit_to_queue_fake.custom_fake = k_work_submit_to_queue_valid_fake;
	k_work_init_fake.custom_fake = k_work_init_valid_fake;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	ret = lc3_streamer_next_frame_get(streamer_idx, &frame_buffer);
	zassert_equal(0, ret, "lc3_streamer_next_frame_get should return success");
	zassert_mem_equal(fake_dataset1_valid, frame_buffer, fake_dataset1_valid_size,
			  "Frame data was not as expected");

	ret = lc3_streamer_num_active_streams();
	zassert_equal(1, ret, "There should be no active streams");

	ret = lc3_streamer_stream_close(streamer_idx);
	zassert_equal(0, ret, "lc3_streamer_stream_close should return success");
	zassert_equal(1, lc3_file_close_fake.call_count, "lc3_file_close should be called once");

	ret = lc3_streamer_num_active_streams();
	zassert_equal(0, ret, "There should be no active streams");
}

ZTEST(lc3_streamer, test_lc3_streamer_stream_close_invalid_index)
{
	int ret;

	ret = lc3_streamer_stream_close(CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS);
	zassert_equal(-EINVAL, ret, "lc3_streamer_stream_close should return -EINVAL");
}

ZTEST(lc3_streamer, test_lc3_streamer_file_path_get_valid)
{
	int ret;
	uint8_t streamer_idx;
	char file_path[CONFIG_FS_FATFS_MAX_LFN];

	char test_string_1[] = "test_string_1";

	ret = lc3_streamer_stream_register(test_string_1, &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	ret = lc3_streamer_file_path_get(streamer_idx, file_path, sizeof(file_path));
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_mem_equal(test_string_1, file_path, sizeof(test_string_1),
			  "Strings for file path doesn't match");

	ret = lc3_streamer_stream_close(streamer_idx);
	zassert_equal(0, ret, "lc3_streamer_close_stream should return success");

	ret = lc3_streamer_file_path_get(streamer_idx, file_path, sizeof(file_path));
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, strlen(file_path), "File path should be empty");
}

ZTEST(lc3_streamer, test_lc3_streamer_file_path_get_invalid)
{
	int ret;
	uint8_t streamer_idx;
	char file_path[CONFIG_FS_FATFS_MAX_LFN];

	ret = lc3_streamer_file_path_get(CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS, file_path,
					 sizeof(file_path));
	zassert_equal(-EINVAL, ret,
		      "lc3_streamer_file_path_get should return -EINVAL on invalid index");

	char test_string_1[] = "test_string_1";

	ret = lc3_streamer_stream_register(test_string_1, &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	ret = lc3_streamer_file_path_get(streamer_idx, NULL, CONFIG_FS_FATFS_MAX_LFN);
	zassert_equal(-EINVAL, ret, "lc3_streamer_file_path_get should return -EINVAL on nullptr");

	size_t small_buf_size = 4;
	uint8_t too_small_buffer[sizeof(test_string_1)];

	ret = lc3_streamer_file_path_get(streamer_idx, too_small_buffer, small_buf_size);
	zassert_equal(0, ret, "lc3_streamer_file_path_get should return success");
	zassert_equal(small_buf_size, strlen(too_small_buffer),
		      "File path should be truncated to buffer size");
	zassert_mem_equal(test_string_1, too_small_buffer, small_buf_size,
			  "Strings for file path doesn't match");
}

ZTEST(lc3_streamer, test_lc3_streamer_is_looping_valid)
{
	int ret;
	bool is_looping;
	uint8_t streamer_idx;

	ret = lc3_streamer_stream_register("test", &streamer_idx, false);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(0, streamer_idx, "lc3_streamer_stream_register should return index 0");

	is_looping = lc3_streamer_is_looping(streamer_idx);
	zassert_equal(false, is_looping, "Stream should not be looping");

	ret = lc3_streamer_stream_register("test", &streamer_idx, true);
	zassert_equal(0, ret, "lc3_streamer_stream_register should return success");
	zassert_equal(1, streamer_idx, "lc3_streamer_stream_register should return index 1");

	is_looping = lc3_streamer_is_looping(streamer_idx);
	zassert_equal(true, is_looping, "Stream should be looping");
}

ZTEST(lc3_streamer, test_lc3_streamer_is_looping_invalid)
{
	bool is_looping;

	is_looping = lc3_streamer_is_looping(CONFIG_SD_CARD_LC3_STREAMER_MAX_NUM_STREAMS);
	zassert_equal(false, is_looping,
		      "lc3_streamer_is_looping should return false on an invalid index");
}

ZTEST_SUITE(lc3_streamer, NULL, suite_setup, test_setup, test_teardown, NULL);
