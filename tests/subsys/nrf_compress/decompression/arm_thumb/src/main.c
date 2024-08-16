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

ZTEST(nrf_compress_decompression, test_valid_implementation)
{
	int rc;
	uint32_t pos = 0;
	uint32_t offset;
	uint8_t *output;
	uint32_t output_size;
	struct nrf_compress_implementation *implementation = NULL;

	implementation = nrf_compress_implementation_find(NRF_COMPRESS_TYPE_ARM_THUMB);

	zassert_not_equal(implementation, NULL, "Expected implementation to not be NULL");

	rc = implementation->init(NULL);
	zassert_ok(rc, "Expected init to be successful");

	while (pos < sizeof(input_compressed)) {
		uint32_t input_data_size;

		input_data_size = implementation->decompress_bytes_needed(NULL);
		zassert_equal(input_data_size, CONFIG_NRF_COMPRESS_CHUNK_SIZE,
			"Expected to need chunk size bytes for LZMA data");

		if ((pos + input_data_size) >= sizeof(input_compressed)) {
			input_data_size = sizeof(input_compressed) - pos;
		}

		rc = implementation->decompress(NULL, &input_compressed[pos], input_data_size,
						false, &offset, &output, &output_size);

		zassert_ok(rc, "Expected data decompress to be successful");
		zassert_equal(output_size, input_data_size,
			      "Expected data decompress to be successful");
		zassert_mem_equal(output, &output_expected[pos], output_size);

		pos += offset;
	}

	rc = implementation->deinit(NULL);
	zassert_ok(rc, "Expected deinit to be successful");
}

ZTEST_SUITE(nrf_compress_decompression, NULL, NULL, NULL, NULL, NULL);
