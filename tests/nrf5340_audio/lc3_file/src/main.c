/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/fs/fs.h>

#include "modules/lc3_file.h"

#include "sd_card/sd_card_fake.h"
#include "sd_card/lc3_file_data.h"

#define FRAME_BUFFER_SIZE 40

static void test_setup(void *f)
{
	ARG_UNUSED(f);

	DO_FOREACH_FAKE(RESET_FAKE);

	FFF_RESET_HISTORY();

	sd_card_fake_reset_counter();
}

ZTEST(lc3_file, test_lc3_file_frame_get_valid)
{
	int ret;
	struct lc3_file_ctx file;
	int8_t frame_buffer[FRAME_BUFFER_SIZE];

	sd_card_read_fake.custom_fake = sd_card_read_lc3_file_fake_valid;

	ret = lc3_file_open(&file, "test.lc3");
	zassert_equal(0, ret, "lc3_file_open() should return 0");

	ret = lc3_file_frame_get(&file, frame_buffer, sizeof(frame_buffer));
	zassert_equal(0, ret, "lc3_file_frame_get() should return 0");
	zassert_mem_equal(lc3_file_dataset1_valid_frame1, frame_buffer,
			  lc3_file_dataset1_valid_frame1_size, "Frame 1 data should match");

	ret = lc3_file_frame_get(&file, frame_buffer, sizeof(frame_buffer));
	zassert_equal(0, ret, "lc3_file_frame_get() should return 0");
	zassert_mem_equal(lc3_file_dataset1_valid_frame2, frame_buffer,
			  lc3_file_dataset1_valid_frame2_size, "Frame 2 data should match");

	ret = lc3_file_frame_get(&file, frame_buffer, sizeof(frame_buffer));
	zassert_equal(0, ret, "lc3_file_frame_get() should return 0");
	zassert_mem_equal(lc3_file_dataset1_valid_frame3, frame_buffer,
			  lc3_file_dataset1_valid_frame3_size, "Frame 3 data should match");

	ret = lc3_file_frame_get(&file, frame_buffer, sizeof(frame_buffer));
	zassert_equal(0, ret, "lc3_file_frame_get() should return 0");
	zassert_mem_equal(lc3_file_dataset1_valid_frame4, frame_buffer,
			  lc3_file_dataset1_valid_frame4_size, "Frame 4 data should match");

	ret = lc3_file_frame_get(&file, frame_buffer, sizeof(frame_buffer));
	zassert_equal(0, ret, "lc3_file_frame_get() should return 0");
	zassert_mem_equal(lc3_file_dataset1_valid_frame5, frame_buffer,
			  lc3_file_dataset1_valid_frame5_size, "Frame 5 data should match");

	ret = lc3_file_frame_get(&file, frame_buffer, sizeof(frame_buffer));
	zassert_equal(-EINVAL, ret, "lc3_file_frame_get() should return 0");
}

ZTEST(lc3_file, test_lc3_file_frame_get_invalid_sd_card_read_header_failure)
{
	int ret;
	struct lc3_file_ctx file;
	int8_t frame_buffer[FRAME_BUFFER_SIZE];

	sd_card_read_fake.return_val = -EINVAL;

	ret = lc3_file_frame_get(&file, frame_buffer, sizeof(frame_buffer));
	zassert_equal(-EINVAL, ret, "lc3_file_frame_get() should return -EINVAL");
}

ZTEST(lc3_file, test_lc3_file_frame_get_invalid_sd_card_read_frame_failure)
{
	int ret;
	struct lc3_file_ctx file;
	int sd_card_read_return_values[] = {0, -EINVAL};

	SET_RETURN_SEQ(sd_card_read, sd_card_read_return_values, 2);

	ret = lc3_file_frame_get(&file, NULL, 0);
	zassert_equal(-EINVAL, ret, "lc3_file_frame_get() should return -EINVAL");
}

ZTEST(lc3_file, test_lc3_file_frame_get_invalid_frame_size_mismatch)
{
	int ret;
	struct lc3_file_ctx file;
	int8_t frame_buffer[FRAME_BUFFER_SIZE];

	sd_card_read_fake.custom_fake = sd_card_read_lc3_file_fake_invalid_frame;

	ret = lc3_file_open(&file, "test.lc3");
	zassert_equal(0, ret, "lc3_file_open() should return 0");

	ret = lc3_file_frame_get(&file, frame_buffer, sizeof(frame_buffer));
	zassert_equal(-EIO, ret, "lc3_file_frame_get() should return -EIO");
}

ZTEST(lc3_file, test_lc3_file_frame_get_invalid_buf_size_too_small)
{
	int ret;
	struct lc3_file_ctx file;
	int8_t frame_buffer[FRAME_BUFFER_SIZE];

	sd_card_read_fake.custom_fake = sd_card_read_lc3_file_fake_valid;

	ret = lc3_file_open(&file, "test.lc3");
	zassert_equal(0, ret, "lc3_file_open() should return 0");

	ret = lc3_file_frame_get(&file, frame_buffer, 10);
	zassert_equal(-ENOMEM, ret, "lc3_file_frame_get() should return -ENOMEM");
}

ZTEST(lc3_file, test_lc3_file_open)
{
	int ret;
	struct lc3_file_ctx file;

	sd_card_read_fake.custom_fake = sd_card_read_lc3_file_fake_valid;

	ret = lc3_file_open(&file, "test.lc3");

	zassert_equal(0, ret, "lc3_file_open() should return 0");
	zassert_equal(1, sd_card_read_fake.call_count, "sd_card_read() should be called once");

	zassert_equal(0xcc1c, file.lc3_header.file_id, "File ID is wrong (is 0x%2x)",
		      file.lc3_header.file_id);
	zassert_equal(0x0012, file.lc3_header.hdr_size, "Header size is wrong, is 0x%2x",
		      file.lc3_header.hdr_size);
	zassert_equal(0x01e0, file.lc3_header.sample_rate, "Sample rate is wrong, is 0x%2x",
		      file.lc3_header.sample_rate);
	zassert_equal(0x0140, file.lc3_header.bit_rate, "Bit rate is wrong, is 0x%2x",
		      file.lc3_header.bit_rate);
	zassert_equal(0x0001, file.lc3_header.channels, "Number of channels is wrong, is 0x%2x",
		      file.lc3_header.channels);
	zassert_equal(0x03e8, file.lc3_header.frame_duration, "Frame duration is wrong, is 0x%2x",
		      file.lc3_header.frame_duration);
	zassert_equal(0x0000, file.lc3_header.rfu, "RFU is wrong, is 0x%2x", file.lc3_header.rfu);
	zassert_equal(0x0780, file.lc3_header.signal_len_lsb,
		      "Signal length LSB is wrong, is 0x%2x", file.lc3_header.signal_len_lsb);
	zassert_equal(0x0000, file.lc3_header.signal_len_msb,
		      "Signal length MSB is wrong, is 0x%2x", file.lc3_header.signal_len_msb);
}

ZTEST(lc3_file, test_lc3_file_open_invalid_nullptr)
{
	int ret;
	struct lc3_file_ctx file;

	ret = lc3_file_open(NULL, "test.lc3");

	zassert_equal(-EINVAL, ret, "lc3_file_open() should return -EINVAL");
	zassert_equal(0, sd_card_read_fake.call_count, "sd_card_read() should not be called");

	ret = lc3_file_open(&file, NULL);

	zassert_equal(-EINVAL, ret, "lc3_file_open() should return -EINVAL");
	zassert_equal(0, sd_card_read_fake.call_count, "sd_card_read() should not be called");
}

ZTEST(lc3_file, test_lc3_file_open_invalid_header)
{
	int ret;
	struct lc3_file_ctx file;

	sd_card_read_fake.custom_fake = sd_card_read_fake_invalid_header;

	ret = lc3_file_open(&file, "test.lc3");

	zassert_equal(-EINVAL, ret, "lc3_file_open() should return -EINVAL");
	zassert_equal(1, sd_card_read_fake.call_count, "sd_card_read() should be called once");
}

ZTEST(lc3_file, test_lc3_file_open_invalid_sd_card_open_failure)
{
	int ret;
	struct lc3_file_ctx file;

	sd_card_open_fake.return_val = -EINVAL;

	ret = lc3_file_open(&file, "test.lc3");

	zassert_equal(-EINVAL, ret, "lc3_file_open() should return -EINVAL");
	zassert_equal(1, sd_card_open_fake.call_count, "sd_card_open() should be called once");
}

ZTEST(lc3_file, test_lc3_file_open_invalid_sd_card_read_failure)
{
	int ret;
	struct lc3_file_ctx file;

	sd_card_read_fake.return_val = -EINVAL;

	ret = lc3_file_open(&file, "test.lc3");

	zassert_equal(-EINVAL, ret, "lc3_file_open() should return -EINVAL");
	zassert_equal(1, sd_card_read_fake.call_count, "sd_card_read() should be called once");
}

ZTEST(lc3_file, test_lc3_file_close)
{
	int ret;
	struct lc3_file_ctx file;

	ret = lc3_file_close(&file);

	zassert_equal(0, ret, "lc3_file_close() should return success");
	zassert_equal(1, sd_card_close_fake.call_count, "sd_card_close() should be called once");
}

ZTEST(lc3_file, test_lc3_file_close_invalid_nullptr)
{
	int ret;

	ret = lc3_file_close(NULL);

	zassert_equal(-EINVAL, ret, "lc3_file_close() should return an error");
	zassert_equal(0, sd_card_close_fake.call_count, "sd_card_close() should be called once");
}

ZTEST(lc3_file, test_lc3_file_close_invalid)
{
	int ret;
	struct lc3_file_ctx file;

	sd_card_close_fake.return_val = -EINVAL;

	ret = lc3_file_close(&file);

	zassert_equal(-EINVAL, ret, "lc3_file_close() should return an error");
	zassert_equal(1, sd_card_close_fake.call_count, "sd_card_close() should be called once");
}

ZTEST(lc3_file, test_lc3_file_init)
{
	int ret;

	ret = lc3_file_init();

	zassert_equal(0, ret, "lc3_file_init() should return 0");
	zassert_equal(1, sd_card_init_fake.call_count, "sd_card_init() should be called once");
}

ZTEST(lc3_file, test_lc3_file_init_invalid_sd_card)
{
	sd_card_init_fake.return_val = -EINVAL;

	int ret = lc3_file_init();

	zassert_equal(-EINVAL, ret, "lc3_file_init() should return an error");
	zassert_equal(1, sd_card_init_fake.call_count, "sd_card_init() should be called once");
}

ZTEST_SUITE(lc3_file, NULL, NULL, test_setup, NULL, NULL);
