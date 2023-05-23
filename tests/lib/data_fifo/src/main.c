/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include "data_fifo.h"

/* Catch asserts to fail test */
void assert_post_action(const char *file, unsigned int line)
{
	zassert_unreachable("reached assert file %s %x", file, line);
}

static void internal_test_remaining_elements(struct data_fifo *data_fifo, uint32_t num_alloced_tgt,
					     uint32_t num_locked_tgt, uint32_t line)
{
	uint32_t num_alloced;
	uint32_t num_locked;
	int ret;

	ret = data_fifo_num_used_get(data_fifo, &num_alloced, &num_locked);
	zassert_equal(ret, 0, "data_fifo_num_used_get did not return 0");
	zassert_equal(num_alloced, num_alloced_tgt,
		      "num_alloced target %d actual val %d. call from line: %d", num_alloced_tgt,
		      num_alloced, line);
	zassert_equal(num_locked, num_locked_tgt,
		      "num_locked target %d actual val %x call from line: %d", num_locked_tgt,
		      num_locked, line);
}

ZTEST(suite_data_fifo, test_data_fifo_init_ok)
{
	DATA_FIFO_DEFINE(data_fifo, 8, 128);

	int ret;

	ret = data_fifo_init(&data_fifo);
	zassert_equal(ret, 0, "init did not return 0");
}

ZTEST(suite_data_fifo, test_data_fifo_data_put_get_ok)
{
#define DATA_SIZE 5
	DATA_FIFO_DEFINE(data_fifo, 8, 128);

	int ret;

	ret = data_fifo_init(&data_fifo);
	zassert_equal(ret, 0, "init did not return 0");

	uint8_t *data_ptr;

	ret = data_fifo_pointer_first_vacant_get(&data_fifo, (void **)&data_ptr, K_NO_WAIT);
	zassert_equal(ret, 0, "first_vacant_get did not return 0");

	data_ptr[0] = 0xa1;
	data_ptr[1] = 0xa2;
	data_ptr[2] = 0xa3;
	data_ptr[3] = 0xa4;
	data_ptr[4] = 0xa5;
	uint8_t data_1[DATA_SIZE] = { 0xa1, 0xa2, 0xa3, 0xa4, 0xa5 };

	internal_test_remaining_elements(&data_fifo, 1, 0, __LINE__);

	ret = data_fifo_block_lock(&data_fifo, (void **)&data_ptr, DATA_SIZE);
	zassert_equal(ret, 0, "block_lock did not return 0");

	internal_test_remaining_elements(&data_fifo, 1, 1, __LINE__);

	ret = data_fifo_pointer_first_vacant_get(&data_fifo, (void **)&data_ptr, K_NO_WAIT);
	zassert_equal(ret, 0, "first_vacant_get did not return 0");

	data_ptr[0] = 0xb1;
	data_ptr[1] = 0xb2;
	data_ptr[2] = 0xb3;
	data_ptr[3] = 0xb4;
	data_ptr[4] = 0xb5;
	data_ptr[5] = 0xb6;
	uint8_t data_2[DATA_SIZE + 1] = { 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6 };

	internal_test_remaining_elements(&data_fifo, 2, 1, __LINE__);

	ret = data_fifo_block_lock(&data_fifo, (void **)&data_ptr, DATA_SIZE + 1);
	zassert_equal(ret, 0, "block_lock did not return 0");

	internal_test_remaining_elements(&data_fifo, 2, 2, __LINE__);

	void *data_ptr_read;
	size_t data_size;

	ret = data_fifo_pointer_last_filled_get(&data_fifo, &data_ptr_read, &data_size, K_NO_WAIT);
	zassert_equal(ret, 0, "_last_filled_get did not return 0");
	zassert_equal(memcmp(data_ptr_read, data_1, DATA_SIZE), 0,
		      "data contents are not identical");
	zassert_equal(data_size, DATA_SIZE, "data size incorrect");

	internal_test_remaining_elements(&data_fifo, 2, 1, __LINE__);

	data_fifo_block_free(&data_fifo, &data_ptr_read);

	internal_test_remaining_elements(&data_fifo, 1, 1, __LINE__);

	ret = data_fifo_pointer_last_filled_get(&data_fifo, &data_ptr_read, &data_size, K_NO_WAIT);
	zassert_equal(ret, 0, "_last_filled_get did not return 0");
	zassert_equal(memcmp(data_ptr_read, data_2, DATA_SIZE), 0,
		      "data contents are not identical");
	zassert_equal(data_size, DATA_SIZE + 1, "data size incorrect");

	internal_test_remaining_elements(&data_fifo, 1, 0, __LINE__);

	data_fifo_block_free(&data_fifo, &data_ptr_read);

	internal_test_remaining_elements(&data_fifo, 0, 0, __LINE__);
}

ZTEST(suite_data_fifo, test_data_fifo_data_put_too_many)
{
#define BLOCKS_NUM 10
	DATA_FIFO_DEFINE(data_fifo, 10, 128);

	int ret;

	ret = data_fifo_init(&data_fifo);
	zassert_equal(ret, 0, "init did not return 0");

	uint8_t *data_ptr;
	size_t data_size = 5;

	for (uint32_t i = 0; i < BLOCKS_NUM; i++) {
		ret = data_fifo_pointer_first_vacant_get(&data_fifo, (void **)&data_ptr, K_NO_WAIT);
		zassert_equal(ret, 0, "first_vacant_get did not return 0");
		data_ptr[0] = 0xa1;
		data_ptr[1] = 0xa2;
		data_ptr[2] = 0xa3;
		data_ptr[3] = 0xa4;
		data_ptr[4] = 0xa5;

		internal_test_remaining_elements(&data_fifo, i + 1, i, __LINE__);

		ret = data_fifo_block_lock(&data_fifo, (void **)&data_ptr, data_size);
		zassert_equal(ret, 0, "block_lock did not return 0");

		internal_test_remaining_elements(&data_fifo, i + 1, i + 1, __LINE__);
	}

	/* Add one too many elements */
	ret = data_fifo_pointer_first_vacant_get(&data_fifo, (void **)&data_ptr, K_NO_WAIT);
	zassert_equal(ret, -ENOMEM, "first_vacant_get did not ENOMEM");
}

ZTEST(suite_data_fifo, test_data_fifo_data_put_too_much_data)
{
	DATA_FIFO_DEFINE(data_fifo, 10, 128);

	int ret;

	ret = data_fifo_init(&data_fifo);
	zassert_equal(ret, 0, "init did not return 0");

	uint8_t *data_ptr;
	size_t data_size = 1025;

	ret = data_fifo_pointer_first_vacant_get(&data_fifo, (void **)&data_ptr, K_NO_WAIT);
	zassert_equal(ret, 0, "first_vacant_get did not return 0");

	ret = data_fifo_block_lock(&data_fifo, (void **)&data_ptr, data_size);
	zassert_equal(ret, -ENOMEM, "block_lock did not return -EACCESS");
}

ZTEST(suite_data_fifo, test_data_fifo_data_put_size_zero)
{
	DATA_FIFO_DEFINE(data_fifo, 10, 128);

	int ret;

	ret = data_fifo_init(&data_fifo);
	zassert_equal(ret, 0, "init did not return 0");

	uint8_t *data_ptr;
	size_t data_size = 0;

	ret = data_fifo_pointer_first_vacant_get(&data_fifo, (void **)&data_ptr, K_NO_WAIT);
	zassert_equal(ret, 0, "first_vacant_get did not return 0");

	ret = data_fifo_block_lock(&data_fifo, (void **)&data_ptr, data_size);
	zassert_equal(ret, -EINVAL, "block_lock did not return -EINVAL");
}

ZTEST_SUITE(suite_data_fifo, NULL, NULL, NULL, NULL, NULL);
