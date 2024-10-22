/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <nrf_compress/implementation.h>

/* Input valid ARM thumb filter compressed data */
static const uint8_t input_compressed[] = {
#include "arm_thumb_compressed.inc"
};

/* Output expected data */
static const uint8_t output_expected[] = {
#include "arm_thumb.inc"
};

#define PLUS_MINUS_OUTPUT_SIZE 2

ZTEST(nrf_compress_decompression, test_valid_implementation)
{
	int rc;
	uint32_t pos = 0;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	uint32_t total_output_size = 0;
	struct nrf_compress_implementation *implementation = NULL;

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_ARM_THUMB);

	zassert_not_equal(implementation, NULL, "Expected implementation to not be NULL");

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	while (pos < sizeof(input_compressed)) {
		uint32_t input_data_size;
		bool last = false;

		input_data_size = implementation->decompress_bytes_needed(NULL);
		zassert_equal(input_data_size, CONFIG_NRF_COMPRESS_CHUNK_SIZE,
			"Expected to need chunk size bytes for LZMA data");

		if ((pos + input_data_size) >= sizeof(input_compressed)) {
			input_data_size = sizeof(input_compressed) - pos;
			last = true;
		}

		rc = implementation->decompress(NULL, &input_compressed[pos], input_data_size,
						last, &offset, &output, &output_size);

		zassert_ok(rc, "Expected data decompress to be successful");
		zassert_mem_equal(output, &output_expected[pos], output_size);

		if (!(output_size == input_data_size || output_size ==
		      (input_data_size + PLUS_MINUS_OUTPUT_SIZE) || output_size ==
		      (input_data_size - PLUS_MINUS_OUTPUT_SIZE))) {
			zassert_ok(1, "Expected data output size is not valid");
		}

		pos += offset;
		total_output_size += output_size;
	}

	zassert_equal(total_output_size, sizeof(output_expected),
		      "Expected data decompress output size to match data input size");

	rc = implementation->deinit(NULL);
	zassert_ok(rc, "Expected deinit to be successful");
}

ZTEST_SUITE(nrf_compress_decompression, NULL, NULL, NULL, NULL, NULL);
