/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_ram_sink.h>
#include <suit_sink.h>

static uint8_t test_data[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
			      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

#define SUIT_MAX_RAM_COMPONENTS 1 /* Currently only one component allowed */

#if (DT_NODE_EXISTS(DT_NODELABEL(ram0x))) /* nrf54H20, nrf54l15(?) */
	#define TEST_DST ((uint8_t *)DT_REG_ADDR(DT_NODELABEL(ram20)))
#elif (DT_NODE_EXISTS(DT_NODELABEL(ram20))) /* nrf54H20, nrf54l15(?) */
	#define TEST_DST_SPECIAL ((uint8_t *)DT_REG_ADDR(DT_NODELABEL(ram20)))
#elif (DT_NODE_EXISTS(DT_NODELABEL(sram0)))
	#if defined(CONFIG_BOARD_NATIVE_POSIX)
		#define TEST_DST ((uint8_t *)DT_REG_ADDR(DT_NODELABEL(sram0)))
	#else
		uint8_t test_mem[sizeof(test_data)];
		#define TEST_DST test_mem
	#endif
#else
	printk("Unrecognized platform");
	#define TEST_DST NULL
#endif

ZTEST_SUITE(ram_sink_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(ram_sink_tests, test_suit_ram_sink_get_OK)
{
	struct stream_sink ram_sink;

	int err = suit_ram_sink_get(&ram_sink, TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);
	zassert_not_equal(ram_sink.ctx, NULL, "suit_ram_sink_get failed - ctx is NULL");

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}

#if (DT_NODE_EXISTS(DT_NODELABEL(ram20))) /* nrf54H20, nrf54l15(?) */
ZTEST(ram_sink_tests, test_suit_ram_sink_get_OK)
{
	struct stream_sink ram_sink;

	int err = suit_ram_sink_get(&ram_sink, TEST_DST_SPECIAL, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);
	zassert_not_equal(ram_sink.ctx, NULL, "suit_ram_sink_get failed - ctx is NULL");

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}
#endif

ZTEST(ram_sink_tests, test_suit_ram_sink_get_NOK)
{
	struct stream_sink ram_sink;

	int err = suit_ram_sink_get(&ram_sink, NULL, sizeof(test_data));

	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_ram_sink_get should have failed - dst == NULL");

	err = suit_ram_sink_get(&ram_sink, TEST_DST, 0);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_ram_sink_get should have failed - offset_limit == 0");
}

ZTEST(ram_sink_tests, test_suit_ram_sink_get_out_of_contexts)
{
	struct stream_sink ram_sinks[SUIT_MAX_RAM_COMPONENTS + 1];
	suit_plat_err_t err;

	for (size_t i = 0; i < SUIT_MAX_RAM_COMPONENTS; i++) {
		err = suit_ram_sink_get(&ram_sinks[i], TEST_DST, sizeof(test_data));
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");
	}

	err = suit_ram_sink_get(&ram_sinks[SUIT_MAX_RAM_COMPONENTS], TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_ERR_NO_RESOURCES, "Unexpected error code");

	for (size_t i = 0; i < SUIT_MAX_RAM_COMPONENTS; i++) {
		err = ram_sinks[i].release(ram_sinks[i].ctx);
		zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
	}
}

ZTEST(ram_sink_tests, test_ram_sink_release_NOK)
{
	struct stream_sink ram_sink;

	int err = suit_ram_sink_get(&ram_sink, TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);

	err = ram_sink.release(NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "ram_sink.release should have failed - ctx == NULL");

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}

ZTEST(ram_sink_tests, test_ram_sink_seek_OK)
{
	struct stream_sink ram_sink;

	int err = suit_ram_sink_get(&ram_sink, TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);

	err = ram_sink.seek(ram_sink.ctx, 0);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.seek failed - error %i", err);

	err = ram_sink.seek(ram_sink.ctx, 9);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.seek failed - error %i", err);

	err = ram_sink.seek(ram_sink.ctx, 63);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.seek failed - error %i", err);

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}

ZTEST(ram_sink_tests, test_ram_sink_seek_NOK)
{
	struct stream_sink ram_sink;

	int err = suit_ram_sink_get(&ram_sink, TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);

	err = ram_sink.seek(ram_sink.ctx, sizeof(test_data));
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "ram_sink.seek should have failed - passed arg == offset_limit");

	err = ram_sink.seek(ram_sink.ctx, sizeof(test_data) + 1);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "ram_sink.seek should have failed - passed arg > offset_limit");

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}

ZTEST(ram_sink_tests, test_ram_sink_used_storage_OK)
{
	struct stream_sink ram_sink;
	size_t used_storage = 0;

	int err = suit_ram_sink_get(&ram_sink, TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);

	err = ram_sink.used_storage(ram_sink.ctx, &used_storage);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.use_storage failed - error %i", err);
	zassert_equal(used_storage, 0, "ram_sink.use_storage failed - not initialized to 0");

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}

ZTEST(ram_sink_tests, test_ram_sink_used_storage_NOK)
{
	struct stream_sink ram_sink;

	int err = suit_ram_sink_get(&ram_sink, TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);

	err = ram_sink.used_storage(ram_sink.ctx, NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "ram_sink.use_storage should have failed - arg size == NULL");

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}

ZTEST(ram_sink_tests, test_ram_sink_write_OK)
{
	struct stream_sink ram_sink;
	size_t used_storage = 0;
	size_t input_size = 21; /* Arbitrary value, chosen to be unaligned */

	int err = suit_ram_sink_get(&ram_sink, TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);

	err = ram_sink.write(ram_sink.ctx, test_data, input_size);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.write failed - error %i", err);

	err = ram_sink.used_storage(ram_sink.ctx, &used_storage);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.use_storage failed - error %i", err);
	zassert_equal(used_storage, input_size, "ram_sink.use_storage failed - value %d",
		      used_storage);

	err = ram_sink.seek(ram_sink.ctx, input_size + 7);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.seek failed - error %i", err);

	err = ram_sink.used_storage(ram_sink.ctx, &used_storage);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.use_storage failed - error %i", err);

	err = ram_sink.write(ram_sink.ctx, &test_data[input_size], input_size);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.write failed - error %i", err);

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}

ZTEST(ram_sink_tests, test_ram_sink_write_NOK)
{
	struct stream_sink ram_sink;

	int err = suit_ram_sink_get(&ram_sink, TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);

	err = ram_sink.write(ram_sink.ctx, test_data, 0);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.write should have failed - size == 0");

	err = ram_sink.write(NULL, test_data, sizeof(test_data));
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "ram_sink.write should have failed - ctx == NULL");

	err = ram_sink.write(ram_sink.ctx, NULL, sizeof(test_data));
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "ram_sink.write should have failed - buf == NULL");

	err = ram_sink.write(ram_sink.ctx, test_data, sizeof(test_data) + 1);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "ram_sink.write should have failed - size out of bounds");

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}

ZTEST(ram_sink_tests, test_ram_sink_erase_OK)
{
	struct stream_sink ram_sink;

	int err = suit_ram_sink_get(&ram_sink, TEST_DST, sizeof(test_data));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_ram_sink_get failed - error %i", err);

	err = ram_sink.erase(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.erase failed - error %i", err);

	err = ram_sink.erase(NULL);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.erase failed - error %i", err);

	err = ram_sink.release(ram_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "ram_sink.release failed - error %i", err);
}
