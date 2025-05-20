/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <suit_decompress_filter.h>
#include <suit_ram_sink.h>
#include <suit_flash_sink.h>
#include <suit_memptr_streamer.h>
#include <suit_plat_mem_util.h>
#include <psa/crypto.h>

#define FLASH_AREA_SIZE	  (128 * 0x400)
#define WRITE_ADDR		  suit_plat_mem_nvm_ptr_get(IMAGE_STORAGE_PARTITION_OFFSET)

#define IMAGE_STORAGE_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(cpuapp_slot0_partition)
#define IMAGE_STORAGE_PARTITION_SIZE	FIXED_PARTITION_SIZE(cpuapp_slot0_partition)

#ifdef CONFIG_SOC_POSIX
static uint8_t output_buffer[128*1024] = {0};
#else
static uint8_t output_buffer[1024] = {0};
#endif

/* Input valid lzma2 compressed data */
const uint8_t dummy_data_input[] = {
#include "dummy_data_input.inc"
};

/* File size and sha256 hash of decompressed data */
const uint32_t dummy_data_output_size = 66477;

const uint8_t dummy_data_output_sha256[] = {
	0x87, 0xee, 0x2e, 0x17, 0xa5, 0xdb, 0x98, 0xbe,
	0x8c, 0xcb, 0xfe, 0xc9, 0x70, 0x8c, 0x7a, 0x43,
	0x66, 0xda, 0x63, 0xff, 0x48, 0x15, 0x48, 0x88,
	0xd7, 0xed, 0x64, 0x87, 0xba, 0xb9, 0xef, 0xc5
};

struct suit_decompress_filter_tests_fixture {
	/* Empty for now. */
};

static void *test_suite_setup(void)
{
	static struct suit_decompress_filter_tests_fixture fixture;

	return &fixture;
}

static void test_suite_teardown(void *f)
{
	ARG_UNUSED(f);
}

static void test_before(void *f)
{
	ARG_UNUSED(f);
}

ZTEST_SUITE(suit_decompress_filter_tests, NULL, test_suite_setup, test_before, NULL,
	    test_suite_teardown);

ZTEST_F(suit_decompress_filter_tests, test_RAM_sink_successful_decompress)
{
#ifdef CONFIG_SOC_POSIX
	struct stream_sink compress_sink;
	struct stream_sink ram_sink;
	struct suit_compression_info compression_info = {
		.compression_alg_id = suit_lzma2,
		.arm_thumb_filter = false,
		.decompressed_image_size = dummy_data_output_size,
	};
	size_t output_size;
	const psa_algorithm_t psa_alg = PSA_ALG_SHA_256;
	psa_hash_operation_t operation = {0};

	psa_status_t status = psa_crypto_init();

	zassert_ok(status, "Expected crypto init to be successful");

	status = psa_hash_setup(&operation, psa_alg);

	zassert_ok(status, "Expected psa_hash_setup to be successful");

	suit_plat_err_t err = suit_ram_sink_get(&ram_sink, output_buffer, sizeof(output_buffer));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unable to create RAM sink");

	err = suit_decompress_filter_get(&compress_sink, &compression_info, &ram_sink,
					 suit_ram_sink_readback);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to create decompress filter, err = %d", err);

	err = suit_memptr_streamer_stream(dummy_data_input, sizeof(dummy_data_input),
					  &compress_sink);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to decompress binary blob");

	err = compress_sink.flush(compress_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to flush decompress filter");

	err = compress_sink.used_storage(compress_sink.ctx, &output_size);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to get binary image size");

	status = psa_hash_update(&operation, output_buffer, output_size);
	zassert_ok(status, "Expected hash update to be successful");

	status = psa_hash_verify(&operation, dummy_data_output_sha256,
				sizeof(dummy_data_output_sha256));
	zassert_ok(status, "Expected hash to match");

	err = compress_sink.release(compress_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to release decompress filter");
#else
	ztest_test_skip();
#endif
}

ZTEST_F(suit_decompress_filter_tests, test_flash_sink_successful_decompress)
{
	struct stream_sink compress_sink;
	struct stream_sink flash_sink;
	struct suit_compression_info compression_info = {
		.compression_alg_id = suit_lzma2,
		.arm_thumb_filter = false,
		.decompressed_image_size = dummy_data_output_size,
	};
	size_t output_size;
	const psa_algorithm_t psa_alg = PSA_ALG_SHA_256;
	psa_hash_operation_t operation = {0};

	psa_status_t status = psa_crypto_init();

	zassert_ok(status, "Expected crypto init to be successful");

	status = psa_hash_setup(&operation, psa_alg);

	zassert_ok(status, "Expected psa_hash_setup to be successful");

	suit_plat_err_t err = suit_flash_sink_get(&flash_sink, WRITE_ADDR, FLASH_AREA_SIZE);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unable to create flash sink");

	err = suit_decompress_filter_get(&compress_sink, &compression_info, &flash_sink,
					 suit_flash_sink_readback);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to create decompress filter, err = %d", err);

	err = suit_memptr_streamer_stream(dummy_data_input, sizeof(dummy_data_input),
					  &compress_sink);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to decompress binary blob");

	err = compress_sink.flush(compress_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to flush decompress filter");

	err = compress_sink.used_storage(compress_sink.ctx, &output_size);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to get binary image size");

	size_t read_offset = 0;

	while (output_size > 0) {
		size_t read_size = sizeof(output_buffer) < output_size ?
				sizeof(output_buffer) : output_size;
		err = suit_flash_sink_readback(flash_sink.ctx, read_offset,
					       output_buffer, read_size);
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to get binary image data");

		status = psa_hash_update(&operation, output_buffer, read_size);
		zassert_ok(status, "Expected hash update to be successful");
		read_offset += read_size;
		output_size -= read_size;
	}

	status = psa_hash_verify(&operation, dummy_data_output_sha256,
				 sizeof(dummy_data_output_sha256));
	zassert_ok(status, "Expected hash to match");

	err = compress_sink.release(compress_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to release decompress filter");
}

ZTEST_F(suit_decompress_filter_tests, test_flash_sink_unfinished_stream)
{
	struct stream_sink compress_sink;
	struct stream_sink flash_sink;
	struct suit_compression_info compression_info = {
		.compression_alg_id = suit_lzma2,
		.arm_thumb_filter = false,
		.decompressed_image_size = dummy_data_output_size,
	};
	size_t output_size;
	size_t read_offset = 0;
	static const char empty[sizeof(output_buffer)] = {
					[0 ... sizeof(output_buffer) - 1] = 0xff};

	/* Some random streaming size, less than sizeof(dummy_data_input)*/
	size_t const stream_size = 500;

	suit_plat_err_t err = suit_flash_sink_get(&flash_sink, WRITE_ADDR, FLASH_AREA_SIZE);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unable to create flash sink");

	err = flash_sink.erase(flash_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to erase flash sink");

	err = suit_decompress_filter_get(&compress_sink, &compression_info, &flash_sink,
					 suit_flash_sink_readback);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to create decompress filter");

	err = suit_memptr_streamer_stream(dummy_data_input,
					  stream_size, &compress_sink);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to decompress binary blob");

	err = compress_sink.flush(compress_sink.ctx);
	zassert_equal(err, SUIT_PLAT_ERR_CRASH, "Expected flush to crash");

	err = compress_sink.used_storage(compress_sink.ctx, &output_size);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to get binary image size");
	zassert_equal(output_size, 0, "Expected output size to be 0");

	/* Lets check that flash area was erased as a result of unsuccesfull decompression.*/
	output_size = FLASH_AREA_SIZE;

	while (output_size > 0) {
		size_t read_size = sizeof(output_buffer) < output_size ?
				sizeof(output_buffer) : output_size;
		err = suit_flash_sink_readback(flash_sink.ctx, read_offset,
					       output_buffer, read_size);
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to readback from flash");

		zassert_mem_equal(empty, output_buffer, read_size,
				  "Expected flash memory to be empty");

		read_offset += read_size;
		output_size -= read_size;
	}

	err = compress_sink.release(compress_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to release decompress filter");
}
