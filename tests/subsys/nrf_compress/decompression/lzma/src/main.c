/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <nrf_compress/implementation.h>
#include <mbedtls/sha256.h>

#define REDUCED_BUFFER_SIZE 512
#define SHA256_SIZE 32

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

/* Input valid lzma2 compressed data whereby the output is larger than the dictionary size */
const uint8_t dummy_data_too_large_input[] = {
#include "dummy_data_input_too_large.inc"
};

/* File size and sha256 hash of decompressed data for too large an output */
const uint32_t dummy_data_too_large_output_size = 134061;
const uint8_t dummy_data_too_large_output_sha256[] = {
	0xc0, 0xc4, 0xac, 0xc7, 0xac, 0x69, 0x37, 0x4b,
	0x60, 0xb4, 0x87, 0xe9, 0x3d, 0x65, 0xcf, 0xa2,
	0x4b, 0x2b, 0xef, 0xd0, 0xb9, 0xbf, 0xf9, 0xc9,
	0x2f, 0x61, 0x52, 0x17, 0xca, 0x55, 0x03, 0x77
};

#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC) && !defined(CONFIG_SOC_POSIX)
static uint8_t *large_malloc_object = NULL;
#endif

/* Input valid lzma1 compressed data */
const uint8_t dummy_data_lzma1_input[] = {
#include "dummy_data_input_lzma1.inc"
};

ZTEST(nrf_compress_decompression, test_valid_implementation)
{
	struct nrf_compress_implementation *implementation = NULL;

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	zassert_not_equal(implementation, NULL, "Expected implementation to not be NULL");
}

ZTEST(nrf_compress_decompression, test_valid_implementation_elements)
{
	struct nrf_compress_implementation *implementation = NULL;

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	zassert_equal(implementation->id, NRF_COMPRESS_TYPE_LZMA,
		      "Expected id element to have correct value");
	zassert_not_equal(implementation->init, NULL, "Expected init element to not be NULL");
	zassert_not_equal(implementation->deinit, NULL,
			  "Expected deinit element to not be NULL");
	zassert_not_equal(implementation->reset, NULL, "Expected reset element to not be NULL");
	zassert_equal(implementation->compress, NULL, "Expected compress element to be NULL");
	zassert_not_equal(implementation->decompress_bytes_needed, NULL,
			  "Expected decompress_bytes_needed element to not be NULL");
	zassert_not_equal(implementation->decompress, NULL,
			  "Expected decompress to not be NULL");
}

ZTEST(nrf_compress_decompression, test_invalid_implementation)
{
	struct nrf_compress_implementation *implementation = NULL;

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_COUNT);

	zassert_equal(implementation, NULL, "Expected implementation to be NULL");

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_COUNT + 300);

	zassert_equal(implementation, NULL, "Expected implementation to be NULL");

	implementation = nrf_compress_implementation_find(((uint16_t)-1));

	zassert_equal(implementation, NULL, "Expected implementation to be NULL");
}

ZTEST(nrf_compress_decompression, test_valid_data_decompression)
{
	int rc;
	uint32_t pos;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	uint32_t total_output_size = 0;
	uint8_t output_sha[SHA256_SIZE] = { 0 };
	struct nrf_compress_implementation *implementation;
	mbedtls_sha256_context ctx;

	mbedtls_sha256_init(&ctx);
        rc = mbedtls_sha256_starts(&ctx, false);
	zassert_ok(rc, "Expected mbedtls sha256 start to be successful");

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	pos = 0;

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	rc = implementation->decompress_bytes_needed(NULL);
	zassert_equal(rc, 2, "Expected to need 2 bytes for LZMA header");

	rc = implementation->decompress(NULL, &dummy_data_input[pos], rc, false, &offset, &output,
					&output_size);
	zassert_ok(rc, "Expected header decompress to be successful");
	pos += offset;

	while (pos < sizeof(dummy_data_input)) {
		rc = implementation->decompress_bytes_needed(NULL);
		zassert_equal(rc, CONFIG_NRF_COMPRESS_CHUNK_SIZE,
			      "Expected to need chunk size bytes for LZMA data");

		if ((pos + rc) >= sizeof(dummy_data_input)) {
			rc = implementation->decompress(NULL, &dummy_data_input[pos],
							(sizeof(dummy_data_input) - pos), true,
							&offset, &output, &output_size);
			pos += 1;
		} else {
			rc = implementation->decompress(NULL, &dummy_data_input[pos], rc, false,
							&offset, &output, &output_size);
		}

		zassert_ok(rc, "Expected data decompress to be successful");

		total_output_size += output_size;

		if (output_size > 0) {
			rc = mbedtls_sha256_update(&ctx, output, output_size);
			zassert_ok(rc, "Expected hash update to be successful");
		}

		pos += offset;
	}

	rc = implementation->deinit(NULL);
	zassert_ok(rc, "Expected deinit to be successful");

	zassert_equal(total_output_size, dummy_data_output_size,
		      "Expected decompressed data size to match");

	rc = mbedtls_sha256_finish(&ctx, output_sha);
	mbedtls_sha256_free(&ctx);
	zassert_ok(rc, "Expected mbedtls sha256 finish to be successful");

	zassert_mem_equal(output_sha, dummy_data_output_sha256, SHA256_SIZE,
			  "Expected hash to match");
}

ZTEST(nrf_compress_decompression, test_valid_data_too_large_decompression)
{
	int rc;
	uint32_t pos;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	struct nrf_compress_implementation *implementation;

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	pos = 0;

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	rc = implementation->decompress_bytes_needed(NULL);
	zassert_equal(rc, 2, "Expected to need 2 bytes for LZMA header");

	rc = implementation->decompress(NULL, &dummy_data_too_large_input[pos], rc, false, &offset,
					&output, &output_size);
	zassert_ok(rc, "Expected header decompress to be successful");
	pos += offset;

	while (pos < sizeof(dummy_data_too_large_input)) {
		rc = implementation->decompress_bytes_needed(NULL);
		zassert_equal(rc, CONFIG_NRF_COMPRESS_CHUNK_SIZE,
			      "Expected to need chunk size bytes for LZMA data");

		/* Limit input buffer size */
		if (rc > REDUCED_BUFFER_SIZE) {
			rc = REDUCED_BUFFER_SIZE;
		}

		if ((pos + rc) > sizeof(dummy_data_too_large_input)) {
			rc = implementation->decompress(NULL, &dummy_data_too_large_input[pos],
							(sizeof(dummy_data_too_large_input) - pos),
							true, &offset, &output, &output_size);
			pos += 1;
		} else {
			rc = implementation->decompress(NULL, &dummy_data_too_large_input[pos], rc,
							false, &offset, &output, &output_size);
		}

		if (rc != -EINVAL) {
			zassert_ok(rc, "Expected data decompress to be successful");
		}

		pos += offset;
	}

	(void)implementation->deinit(NULL);

	zassert_equal(rc, -EINVAL, "Expected data decompress with too large an output to fail");
}

ZTEST(nrf_compress_decompression, test_invalid_data_decompression)
{
	int rc;
	uint32_t pos;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	struct nrf_compress_implementation *implementation;

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	/* Random offset to make data input invalid */
	pos = 38;

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	rc = implementation->decompress_bytes_needed(NULL);
	zassert_equal(rc, 2, "Expected to need 2 bytes for LZMA header");

	rc = implementation->decompress(NULL, &dummy_data_input[pos], rc, false, &offset, &output,
					&output_size);
	(void)implementation->deinit(NULL);
	zassert_not_ok(rc, "Expected header decompress to be fail");
}

ZTEST(nrf_compress_decompression, test_invalid_data_data)
{
	int rc;
	uint32_t pos;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	uint32_t total_output_size = 0;
	struct nrf_compress_implementation *implementation;

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	pos = 0;

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	rc = implementation->decompress_bytes_needed(NULL);
	zassert_equal(rc, 2, "Expected to need 2 bytes for LZMA header");

	rc = implementation->decompress(NULL, &dummy_data_too_large_input[pos], rc, false, &offset,
					&output, &output_size);
	zassert_ok(rc, "Expected header decompress to be successful");
	pos += offset;

	while (pos < sizeof(dummy_data_input)) {
		rc = implementation->decompress_bytes_needed(NULL);
		zassert_equal(rc, CONFIG_NRF_COMPRESS_CHUNK_SIZE,
			      "Expected to need chunk size bytes for LZMA data");

		/* Limit input buffer size */
		if (rc > REDUCED_BUFFER_SIZE) {
			rc = REDUCED_BUFFER_SIZE;
		}

		if ((pos + rc) >= sizeof(dummy_data_input)) {
			rc = implementation->decompress(NULL, &dummy_data_input[pos],
							(sizeof(dummy_data_input) - pos), true,
							&offset, &output, &output_size);
			pos += 1;
		} else if (pos >= REDUCED_BUFFER_SIZE) {
			/* Read in manipulated bad data */
			uint8_t bad_data[REDUCED_BUFFER_SIZE];
			uint8_t swap_data;

			memcpy(bad_data, &dummy_data_input[pos], REDUCED_BUFFER_SIZE);
			swap_data = bad_data[3];
			bad_data[3] = bad_data[9];
			bad_data[9] = swap_data;

			rc = implementation->decompress(NULL, bad_data, rc, false, &offset,
							&output, &output_size);

			zassert_not_ok(rc, "Expected data decompress to fail");

			goto finish;
		} else {
			rc = implementation->decompress(NULL, &dummy_data_input[pos], rc, false,
							&offset, &output, &output_size);
		}

		zassert_ok(rc, "Expected data decompress to be successful");

		pos += offset;
		total_output_size += output_size;
	}

finish:
	(void)implementation->deinit(NULL);
}

ZTEST(nrf_compress_decompression, test_valid_data_decompression_random_sizes)
{
	int rc;
	uint32_t pos;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	uint32_t total_output_size = 0;
	uint8_t output_sha[SHA256_SIZE] = { 0 };
	struct nrf_compress_implementation *implementation;
	uint8_t loop = 0;
	uint16_t read_sizes[] = {
		384,
		512,
		64,
		32,
		192,
		256
	};

	mbedtls_sha256_context ctx;

	mbedtls_sha256_init(&ctx);
	rc = mbedtls_sha256_starts(&ctx, false);
	zassert_ok(rc, "Expected mbedtls sha256 start to be successful");

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	pos = 0;

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	rc = implementation->decompress_bytes_needed(NULL);
	zassert_equal(rc, 2, "Expected to need 2 bytes for LZMA header");

	rc = implementation->decompress(NULL, &dummy_data_input[pos], rc, false, &offset, &output,
					&output_size);
	zassert_ok(rc, "Expected header decompress to be successful");
	pos += offset;

	while (pos < sizeof(dummy_data_input)) {
		rc = read_sizes[loop % ARRAY_SIZE(read_sizes)];
		++loop;

		if ((pos + rc) >= sizeof(dummy_data_input)) {
			rc = implementation->decompress(NULL, &dummy_data_input[pos],
							(sizeof(dummy_data_input) - pos), true,
							&offset, &output, &output_size);
			pos += 1;
		} else {
			rc = implementation->decompress(NULL, &dummy_data_input[pos], rc, false,
							&offset, &output, &output_size);
		}

		zassert_ok(rc, "Expected data decompress to be successful");

		total_output_size += output_size;

		if (output_size > 0) {
			rc = mbedtls_sha256_update(&ctx, output, output_size);
			zassert_ok(rc, "Expected hash update to be successful");
		}

		pos += offset;
	}

	rc = implementation->deinit(NULL);
	zassert_ok(rc, "Expected deinit to be successful");

	zassert_equal(total_output_size, dummy_data_output_size,
		      "Expected decompressed data size to match");

	rc = mbedtls_sha256_finish(&ctx, output_sha);
	mbedtls_sha256_free(&ctx);
	zassert_ok(rc, "Expected mbedtls sha256 finish to be successful");

	zassert_mem_equal(output_sha, dummy_data_output_sha256, SHA256_SIZE,
			  "Expected hash to match");
}

ZTEST(nrf_compress_decompression, test_valid_data_decompression_reset)
{
	int rc;
	uint32_t pos;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	uint32_t total_output_size = 0;
	uint8_t output_sha[SHA256_SIZE] = { 0 };
	struct nrf_compress_implementation *implementation;
	mbedtls_sha256_context ctx;

	mbedtls_sha256_init(&ctx);
        rc = mbedtls_sha256_starts(&ctx, false);
	zassert_ok(rc, "Expected mbedtls sha256 start to be successful");

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	pos = 0;

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	rc = implementation->decompress_bytes_needed(NULL);
	zassert_equal(rc, 2, "Expected to need 2 bytes for LZMA header");

	rc = implementation->decompress(NULL, &dummy_data_input[pos], rc, false, &offset, &output,
					&output_size);
	zassert_ok(rc, "Expected header decompress to be successful");
	pos += offset;

	rc = implementation->decompress(NULL, &dummy_data_input[pos], REDUCED_BUFFER_SIZE, false,
					&offset, &output, &output_size);
	zassert_ok(rc, "Expected data decompress to be successful");
	pos += offset;

	rc = implementation->decompress(NULL, &dummy_data_input[pos], REDUCED_BUFFER_SIZE, false,
					&offset, &output, &output_size);
	zassert_ok(rc, "Expected data decompress to be successful");

	pos = 0;
	implementation->reset(NULL);

	rc = implementation->decompress_bytes_needed(NULL);
	zassert_equal(rc, 2, "Expected to need 2 bytes for LZMA header");

	rc = implementation->decompress(NULL, &dummy_data_input[pos], rc, false, &offset, &output,
					&output_size);
	zassert_ok(rc, "Expected header decompress to be successful");
	pos += offset;

	while (pos < sizeof(dummy_data_input)) {
		rc = implementation->decompress_bytes_needed(NULL);
		zassert_equal(rc, CONFIG_NRF_COMPRESS_CHUNK_SIZE,
			      "Expected to need chunk size bytes for LZMA data");

		if ((pos + rc) >= sizeof(dummy_data_input)) {
			rc = implementation->decompress(NULL, &dummy_data_input[pos],
							(sizeof(dummy_data_input) - pos), true,
							&offset, &output, &output_size);
			pos += 1;
		} else {
			rc = implementation->decompress(NULL, &dummy_data_input[pos], rc, false,
							&offset, &output, &output_size);
		}

		zassert_ok(rc, "Expected data decompress to be successful");

		total_output_size += output_size;

		if (output_size > 0) {
			rc = mbedtls_sha256_update(&ctx, output, output_size);
			zassert_ok(rc, "Expected hash update to be successful");
		}

		pos += offset;
	}

	rc = implementation->deinit(NULL);
	zassert_ok(rc, "Expected deinit to be successful");

	zassert_equal(total_output_size, dummy_data_output_size,
		      "Expected decompressed data size to match");

	rc = mbedtls_sha256_finish(&ctx, output_sha);
	mbedtls_sha256_free(&ctx);
	zassert_ok(rc, "Expected mbedtls sha256 finish to be successful");

	zassert_mem_equal(output_sha, dummy_data_output_sha256, SHA256_SIZE,
			  "Expected hash to match");
}

ZTEST(nrf_compress_decompression, test_invalid_lzma1)
{
	int rc;
	uint32_t pos;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	struct nrf_compress_implementation *implementation;

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	/* Random offset to make data input invalid */
	pos = 38;

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	rc = implementation->decompress_bytes_needed(NULL);
	zassert_equal(rc, 2, "Expected to need 2 bytes for LZMA header");

	rc = implementation->decompress(NULL, &dummy_data_lzma1_input[pos], rc, false, &offset,
					&output, &output_size);
	(void)implementation->deinit(NULL);
	zassert_not_ok(rc, "Expected header decompress to be fail");
}

ZTEST(nrf_compress_decompression, test_too_large_malloc)
{
#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC) && !defined(CONFIG_SOC_POSIX)
	int rc;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	struct nrf_compress_implementation *implementation;

	/* Malloc data to reduce available heap for decompression library */
	large_malloc_object = (uint8_t *)malloc(4264);
	zassert_not_null(large_malloc_object, "Expected large malloc to be successful");

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_LZMA);

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	rc = implementation->decompress_bytes_needed(NULL);
	zassert_equal(rc, 2, "Expected to need 2 bytes for LZMA header");

	rc = implementation->decompress(NULL, dummy_data_input, rc, false, &offset, &output,
					&output_size);
	zassert_not_ok(rc, "Expected header decompress to be fail");

	(void)implementation->deinit(NULL);
	zassert_not_ok(rc, "Expected header decompress to be fail");

	free(large_malloc_object);
	large_malloc_object = NULL;
#else
	ztest_test_skip();
#endif
}

static void cleanup_test(void *p)
{
#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC) && !defined(CONFIG_SOC_POSIX)
	if (large_malloc_object != NULL) {
		free(large_malloc_object);
		large_malloc_object = NULL;
	}
#endif
}

ZTEST_SUITE(nrf_compress_decompression, NULL, NULL, NULL, cleanup_test, NULL);
