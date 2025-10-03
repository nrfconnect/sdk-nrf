/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/fcb.h>
#include <zephyr/drivers/flash.h>

#include <modem/trace_backend.h>

extern int unity_main(void);

extern struct nrf_modem_lib_trace_backend trace_backend;

/* The flash backend expects this semaphore to exist */
K_SEM_DEFINE(trace_clear_sem, 0, 1);

/* Callback for processed traces - not used in these tests */
static int processed_cb(size_t len)
{
	/* No-op for testing */
	return 0;
}

void setUp(void)
{
	/* Clear the trace backend before each test */
	trace_backend.clear();

	/* Ensure backend is deinitialized before each test */
	trace_backend.deinit();
}

void tearDown(void)
{
	/* Clear the trace backend */
	trace_backend.clear();

	/* Ensure backend is deinitialized after each test */
	trace_backend.deinit();
}

/* Test basic backend lifecycle - init/deinit for modem trace collection */
void test_init_and_deinit(void)
{
	int ret;
	/* Test initialization */
	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	ret = trace_backend.deinit();
	TEST_ASSERT_EQUAL(0, ret);
}

/* Test write operation fails when backend is not initialized */
void test_write_fails_when_not_initialized(void)
{
	int ret;
	uint8_t data[200];

	memset(data, 0x11, sizeof(data));

	ret = trace_backend.write(data, sizeof(data));
	TEST_ASSERT_EQUAL(-EPERM, ret);
}

/* Test read operation fails when backend is not initialized */
void test_read_fails_when_not_initialized(void)
{
	int ret;
	uint8_t out[16];

	ret = trace_backend.read(out, sizeof(out));
	TEST_ASSERT_EQUAL(-EPERM, ret);
}

/* Test basic write operation and data size tracking for modem traces */
void test_basic_write_and_size(void)
{
	int ret;
	uint8_t data[200];
	size_t initial_size;

	memset(data, 0xAA, sizeof(data));

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	initial_size = trace_backend.data_size();
	TEST_ASSERT_EQUAL(0, initial_size);

	ret = trace_backend.write(data, sizeof(data));
	TEST_ASSERT_EQUAL(sizeof(data), ret);

	ret = (int)trace_backend.data_size();
	TEST_ASSERT_EQUAL(sizeof(data), ret);
}

/* Test writing more than the flash partition size */
void test_write_more_than_flash_partition_size(void)
{
	int ret;
	static uint8_t data[CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_PARTITION_SIZE + 400];
	uint8_t read_buffer[1024];
	size_t data_available;

	memset(data, 0x11, sizeof(data));

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	ret = trace_backend.write(data, sizeof(data));
	/* When writing more data than the partition can hold, FCB rotation occurs.
	 * The write function returns the amount actually stored after rotation.
	 */
	TEST_ASSERT_TRUE(ret > 0);
	TEST_ASSERT_TRUE(ret <= sizeof(data));

	data_available = trace_backend.data_size();
	TEST_ASSERT_EQUAL(ret, data_available); /* Available should match what was written */
	TEST_ASSERT_TRUE(data_available <= CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_PARTITION_SIZE);
	/* Should retain at least 70% of partition size to be useful */
	TEST_ASSERT_TRUE(
		data_available >=
		(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_PARTITION_SIZE * 7) / 10);

	/* Verify we can actually read the data back */
	ret = trace_backend.read(read_buffer, sizeof(read_buffer));
	TEST_ASSERT_TRUE(ret > 0); /* Should be able to read something */
	TEST_ASSERT_TRUE(ret <= sizeof(read_buffer)); /* Shouldn't read more than requested */
}

/* Test handling different trace data sizes */
void test_write_different_sizes(void)
{
	int ret;
	uint8_t small_data[8];
	uint8_t medium_data[32];
	uint8_t large_data[128];
	size_t initial_size;
	size_t size_after_small;
	size_t size_after_medium;
	size_t final_size;

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	/* Record initial size (may have data from previous tests) */
	initial_size = trace_backend.data_size();
	TEST_ASSERT_EQUAL(0, initial_size);

	/* Test small write */
	memset(small_data, 0xAA, sizeof(small_data));
	ret = trace_backend.write(small_data, sizeof(small_data));
	TEST_ASSERT_EQUAL(sizeof(small_data), ret);

	size_after_small = trace_backend.data_size();
	TEST_ASSERT_TRUE(size_after_small >= initial_size + sizeof(small_data));

	/* Test medium write */
	memset(medium_data, 0xBB, sizeof(medium_data));

	ret = trace_backend.write(medium_data, sizeof(medium_data));
	TEST_ASSERT_EQUAL(sizeof(medium_data), ret);

	size_after_medium = trace_backend.data_size();
	TEST_ASSERT_TRUE(size_after_medium >= size_after_small + sizeof(medium_data));

	/* Test large write */
	memset(large_data, 0xCC, sizeof(large_data));

	ret = trace_backend.write(large_data, sizeof(large_data));
	TEST_ASSERT_EQUAL(sizeof(large_data), ret);

	/* Verify total accumulated size */
	final_size = trace_backend.data_size();
	TEST_ASSERT_TRUE(final_size >= size_after_medium + sizeof(large_data));
}

/* Test continuous modem trace streaming with multiple write operations */
void test_multiple_write_operations(void)
{
	int ret;
	size_t initial_size;
	uint8_t data[1024] = {0x10};
	const uint8_t iterations = 5;
	size_t total_data;
	size_t available_data;
	size_t to_read_total;
	size_t read_total = 0;
	uint8_t read_buf[256];

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	/* Record initial size (accumulates from previous tests) */
	initial_size = trace_backend.data_size();
	TEST_ASSERT_EQUAL(0, initial_size);

	/* Simulate continuous modem trace fragments */
	for (int i = 0; i < iterations; i++) {
		size_t expected_min_size = (i + 1) * sizeof(data);
		size_t current_size;

		memset(data, 0x10 + i, sizeof(data));

		ret = trace_backend.write(data, sizeof(data));
		TEST_ASSERT_EQUAL(sizeof(data), ret);

		/* Verify data size increases incrementally */
		current_size = trace_backend.data_size();
		TEST_ASSERT_TRUE(current_size >= expected_min_size);
	}

	/* Verify we can read back everything written, in order */
	total_data = (size_t)iterations * sizeof(data);

	available_data = trace_backend.data_size();
	TEST_ASSERT_TRUE(available_data >= total_data);

	/* Read back in chunks and verify pattern across all bytes */
	to_read_total = total_data;

	while (read_total < to_read_total) {
		size_t request_size;
		int ret;

		request_size = MIN(sizeof(read_buf), to_read_total - read_total);
		ret = trace_backend.read(read_buf, request_size);

		TEST_ASSERT_TRUE(ret > 0);
		TEST_ASSERT_TRUE(ret <= request_size);

		for (size_t j = 0; j < (size_t)ret; j++) {
			uint8_t expected;

			expected = 0x10 + ((read_total + (size_t)j) / sizeof(data));
			TEST_ASSERT_EQUAL_HEX8(expected, read_buf[j]);
		}

		read_total += (size_t)ret;
	}

	TEST_ASSERT_EQUAL(to_read_total, read_total);
}

/* Test error handling and edge cases for robust modem trace operation */
void test_error_conditions(void)
{
	int ret;
	uint8_t data[16];
	uint8_t out[16];

	/* Test NULL callback */
	ret = trace_backend.init(NULL);
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	/* Test zero-length operations */
	ret = trace_backend.write(data, 0);
	TEST_ASSERT_EQUAL(0, ret);

	/* Zero-length read: backend may return 0 or -ENODATA when empty */
	ret = trace_backend.read(out, 0);
	TEST_ASSERT_TRUE(ret == 0 || ret == -ENODATA);

	/* Read on empty backend should return -ENODATA */
	ret = trace_backend.read(out, sizeof(out));
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

/* Test trace data clearing for modem trace management and analysis */
void test_clear_operation(void)
{
	int ret;
	uint8_t test_data[32];
	size_t size_after;
	size_t size_before;

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	/* Write some data first */
	memset(test_data, 0xDD, sizeof(test_data));

	ret = trace_backend.write(test_data, sizeof(test_data));
	TEST_ASSERT_EQUAL(sizeof(test_data), ret);

	size_before = trace_backend.data_size();
	TEST_ASSERT_EQUAL(sizeof(test_data), size_before);

	/* Clear the data */
	ret = trace_backend.clear();
	TEST_ASSERT_EQUAL(0, ret);

	size_after = trace_backend.data_size();
	TEST_ASSERT_EQUAL(0, size_after);
}

/* Test large buffer write that triggers flash flush for high-volume traces */
void test_large_buffer_write(void)
{
	int ret;
	static uint8_t large_data[CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE * 3];
	size_t initial_size;
	size_t final_size;

	memset(large_data, 0xEE, sizeof(large_data));

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	initial_size = trace_backend.data_size();
	TEST_ASSERT_EQUAL(0, initial_size);

	ret = trace_backend.write(large_data, sizeof(large_data));
	TEST_ASSERT_EQUAL(sizeof(large_data), ret);

	final_size = trace_backend.data_size();
	/* With RAM tail possible, final size equals total written */
	TEST_ASSERT_EQUAL(sizeof(large_data), final_size);
}

/* Test that peek_at returns -EINVAL for NULL or zero length buffer */
void test_peek_at_invalid_parameters(void)
{
	int ret;
	uint8_t data[8] = { 0xAB };

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	ret = trace_backend.write(data, sizeof(data));
	TEST_ASSERT_EQUAL((int)sizeof(data), ret);

	ret = trace_backend.peek_at(0, NULL, 1);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = trace_backend.peek_at(0, data, 0);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/* Test that resume fails when no peek at offset mechanism is used and data is dropped */
void test_resume_fails_without_peek_at(void)
{
	int ret;
	uint8_t block[256];
	uint8_t read_buf[256];
	size_t first_read;
	size_t retry_offset;
	size_t read_total;
	int i;
	const size_t iterations = 8;

	/* Initialize backend */
	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	/* Write enough to span multiple entries/sectors to ensure rotation on read */
	for (i = 0; i < iterations; i++) {
		memset(block, 0xA0 + i, sizeof(block));

		ret = trace_backend.write(block, sizeof(block));
		TEST_ASSERT_EQUAL(sizeof(block), ret);
	}

	TEST_ASSERT_EQUAL((size_t)(iterations * sizeof(block)), trace_backend.data_size());

	/* Simulate upload attempt: read an initial chunk */
	first_read = 1024;
	read_total = 0;

	while (read_total < first_read) {
		size_t request_size;

		request_size = MIN(sizeof(read_buf), first_read - read_total);

		ret = trace_backend.read(read_buf, request_size);
		TEST_ASSERT_TRUE(ret > 0);

		TEST_ASSERT_EQUAL(request_size, (size_t)ret);

		/* Verify content matches the expected pattern across block boundaries */
		for (size_t j = 0; j < (size_t)ret; j++) {
			uint8_t expected;

			expected = 0xA0 + ((read_total + j) / sizeof(block));
			TEST_ASSERT_EQUAL_HEX8(expected, read_buf[j]);
		}

		read_total += (size_t)ret;
	}

	TEST_ASSERT_EQUAL(first_read, read_total);

	/* Upload fails, device resets and most recent read data is lost from caller's RAM buffer */
	retry_offset = first_read - sizeof(read_buf);

	ret = trace_backend.read(read_buf, sizeof(read_buf));
	TEST_ASSERT_EQUAL(sizeof(read_buf), (size_t)ret);

	/* Verify remaining content is as expected while draining, ie flash has been rotated,
	 * and we are reading from the next sector. Data was lost.
	 */
	for (size_t j = 0; j < (size_t)ret; j++) {
		uint8_t expected;

		expected = 0xA0 + ((retry_offset + j) / sizeof(block));
		TEST_ASSERT_NOT_EQUAL_HEX8(expected, read_buf[j]);
	}
}

/* Test peek_at basic functionality and that it does not consume data.
 * Data is still available in the flash buffer after the peek_at call.
 */
void test_peek_at_does_not_consume(void)
{
	int ret;
	uint8_t data[CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE - 16];
	uint8_t peek_at_buf[30];
	uint8_t read_buf[30];
	size_t size_before;

	for (size_t i = 0; i < sizeof(data); i++) {
		data[i] = (uint8_t)i;
	}

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	/* Write a small buffer that stays in RAM buffer, ie doesn't trigger flash flush */
	ret = trace_backend.write(data, sizeof(data));
	TEST_ASSERT_EQUAL((int)sizeof(data), ret);

	size_before = trace_backend.data_size();
	TEST_ASSERT_EQUAL(sizeof(data), size_before);

	/* Read the first 30 bytes at offset 0 */
	ret = trace_backend.peek_at(0, peek_at_buf, sizeof(peek_at_buf));
	TEST_ASSERT_EQUAL((int)sizeof(peek_at_buf), ret);

	TEST_ASSERT_EQUAL_HEX8_ARRAY(peek_at_buf, data, sizeof(peek_at_buf));

	/* Ensure size unchanged: peek_at does not consume */
	TEST_ASSERT_EQUAL(size_before, trace_backend.data_size());

	/* Read the same data again and verify that the size has not changed */
	ret = trace_backend.peek_at(0, peek_at_buf, sizeof(peek_at_buf));
	TEST_ASSERT_EQUAL((int)sizeof(peek_at_buf), ret);

	TEST_ASSERT_EQUAL(size_before, trace_backend.data_size());

	/* And that the data is the same */
	TEST_ASSERT_EQUAL_HEX8_ARRAY(peek_at_buf, data, sizeof(peek_at_buf));

	/* Consume via read() and verify equals the peek_at data */
	ret = trace_backend.read(read_buf, sizeof(read_buf));
	TEST_ASSERT_EQUAL((int)sizeof(read_buf), ret);

	TEST_ASSERT_EQUAL_HEX8_ARRAY(peek_at_buf, read_buf, sizeof(read_buf));

	/* Verify that the size has decreased after consuming */
	TEST_ASSERT_EQUAL(size_before - sizeof(read_buf), trace_backend.data_size());
}

/* Test peek_at spanning multiple FCB entries and into arbitrary offsets */
void test_peek_at_spanning_multiple_entries(void)
{
	int ret;
	/* Large buffer to ensure more than two FCB entries are created */
	static uint8_t large_buffer[(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE * 3) + 128];
	uint8_t read_buf[CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE];
	size_t offset;

	for (size_t i = 0; i < sizeof(large_buffer); i++) {
		/* Fill the buffer with a known pattern */
		large_buffer[i] = (uint8_t)(i & 0xFF);
	}

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	ret = trace_backend.write(large_buffer, sizeof(large_buffer));
	TEST_ASSERT_EQUAL((int)sizeof(large_buffer), ret);

	/* Verify that the data size is as expected */
	TEST_ASSERT_EQUAL(sizeof(large_buffer), trace_backend.data_size());

	/* Pick an offset that crosses the first FCB entry boundary */
	offset = (size_t)CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE + 128;

	ret = trace_backend.peek_at((size_t)offset, read_buf, sizeof(read_buf));
	TEST_ASSERT_EQUAL((int)sizeof(read_buf), ret);

	/* Verify the data read is correct */
	TEST_ASSERT_EQUAL_HEX8_ARRAY(read_buf, &large_buffer[offset], sizeof(read_buf));

	/* Move on to next chunk */
	offset += sizeof(read_buf);

	ret = trace_backend.peek_at((size_t)offset, read_buf, sizeof(read_buf));
	TEST_ASSERT_EQUAL((int)sizeof(read_buf), ret);

	/* Verify the data read is correct */
	TEST_ASSERT_EQUAL_HEX8_ARRAY(read_buf, &large_buffer[offset], sizeof(read_buf));

	/* Out of range offset should return -EFAULT */
	ret = trace_backend.peek_at(sizeof(large_buffer), read_buf, sizeof(read_buf));
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

/* Test that repeated peek_at calls do not advance cursor and that later read() sees same bytes */
void test_repeated_peek_at_and_read(void)
{
	int ret;
	static uint8_t buf[(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE) + 64];
	uint8_t peek_at_buf_1[32];
	uint8_t peek_at_buf_2[32];
	uint8_t read_buf[32];
	size_t read_offset = 0;

	for (size_t i = 0; i < sizeof(buf); i++) {
		buf[i] = (uint8_t)(i & 0xFF);
	}

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	ret = trace_backend.write(buf, sizeof(buf));
	TEST_ASSERT_EQUAL((int)sizeof(buf), ret);

	/* Read the first 32 bytes twice */
	ret = trace_backend.peek_at(read_offset, peek_at_buf_1, sizeof(peek_at_buf_1));
	TEST_ASSERT_EQUAL((int)sizeof(peek_at_buf_1), ret);

	ret = trace_backend.peek_at(read_offset, peek_at_buf_2, sizeof(peek_at_buf_2));
	TEST_ASSERT_EQUAL((int)sizeof(peek_at_buf_2), ret);

	TEST_ASSERT_EQUAL_HEX8_ARRAY(peek_at_buf_1, peek_at_buf_2, sizeof(peek_at_buf_1));

	/* Now do read() and verify that it matches the peek_at data */
	ret = trace_backend.read(read_buf, sizeof(read_buf));
	TEST_ASSERT_EQUAL((int)sizeof(read_buf), ret);

	TEST_ASSERT_EQUAL_HEX8_ARRAY(peek_at_buf_1, read_buf, sizeof(peek_at_buf_1));

	/* The next read should now be different from the content in peek_at_buf_1
	 * as we have advanced the read cursor.
	 */
	ret = trace_backend.read(read_buf, sizeof(read_buf));
	TEST_ASSERT_EQUAL((int)sizeof(read_buf), ret);

	for (size_t i = 0; i < sizeof(read_buf); i++) {
		TEST_ASSERT_NOT_EQUAL_HEX8(read_buf[i], peek_at_buf_1[i]);
	}

	/* The peek_at should not be affected */
	ret = trace_backend.peek_at(read_offset, peek_at_buf_1, sizeof(peek_at_buf_1));
	TEST_ASSERT_EQUAL((int)sizeof(peek_at_buf_1), ret);

	TEST_ASSERT_EQUAL_HEX8_ARRAY(peek_at_buf_1, peek_at_buf_2, sizeof(peek_at_buf_1));
}

/* Test reading small amount of data multiple times */
void test_read_small_amount_of_data_multiple_times(void)
{
	int ret;
	uint8_t data[128];
	uint8_t read_buf[sizeof(data) / 4];
	size_t read_offset = 0;

	for (size_t i = 0; i < sizeof(data); i++) {
		data[i] = (uint8_t)(i & 0xFF);
	}

	ret = trace_backend.init(processed_cb);
	TEST_ASSERT_EQUAL(0, ret);

	ret = trace_backend.write(data, sizeof(data));
	TEST_ASSERT_EQUAL((int)sizeof(data), ret);

	TEST_ASSERT_EQUAL(sizeof(data), trace_backend.data_size());

	while (read_offset < sizeof(data)) {
		size_t request_size;

		/* data buffer should always be divisible by request_size without remainder,
		 * and equal to the read_buf size.
		 */
		request_size = MIN(sizeof(read_buf), sizeof(data) - read_offset);
		TEST_ASSERT_EQUAL(request_size, sizeof(read_buf));

		ret = trace_backend.peek_at(read_offset, read_buf, request_size);
		TEST_ASSERT_EQUAL((int)request_size, ret);

		TEST_ASSERT_EQUAL_HEX8_ARRAY(read_buf, &data[read_offset], sizeof(read_buf));

		read_offset += request_size;
	}

	TEST_ASSERT_EQUAL(sizeof(data), read_offset);

	/* A subsequent read should return -EFAULT */
	ret = trace_backend.peek_at(read_offset, read_buf, sizeof(read_buf));
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

int main(void)
{
	(void)unity_main();
	return 0;
}
