/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <dfu/dfu_multi_image.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <string.h>
#include <stdint.h>
#include <stddef.h>

/*
 * Expected properties of an update image that is supposed to be written while downloading
 * and unpacking a given DFU Multi Image package.
 */
struct expected_image {
	int image_id;
	const char *content;
	size_t content_size;
};

#define EXPECTED_IMAGE(id, contentstr)                                                             \
	{                                                                                          \
		.image_id = id, .content = contentstr, .content_size = strlen(contentstr)          \
	}

/*
 * Expected results of downloading and unpacking a given DFU Multi Image package.
 */
struct expected {
	struct expected_image images[CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT];
	size_t image_count;
};

/*
 * Implement fake image writer which compares written data with expected content instead of
 * actually storing it.
 */

struct comparison_context {
	struct expected expected;
	size_t current_image_no;
	size_t current_image_offset;
};

static struct comparison_context ctx;

static int image_comparator_open(int image_id, size_t image_size)
{
	zassert_true(ctx.current_image_no < ctx.expected.image_count, "Too many images written");
	zassert_equal(ctx.expected.images[ctx.current_image_no].image_id, image_id,
		      "Unexpected image id");
	zassert_equal(ctx.expected.images[ctx.current_image_no].content_size, image_size,
		      "Unexpected image size");
	zassert_equal(ctx.current_image_offset, 0, "Opening image while already in progress");

	return 0;
}

static int image_comparator_write(const uint8_t *chunk, size_t chunk_size)
{
	const struct expected_image *image = &ctx.expected.images[ctx.current_image_no];

	zassert_true(ctx.current_image_offset + chunk_size <= image->content_size,
		     "Too large image written");
	zassert_ok(memcmp(image->content + ctx.current_image_offset, chunk, chunk_size),
		   "Unexpected image content");

	ctx.current_image_offset += chunk_size;

	return 0;
}

static int image_comparator_close(bool success)
{
	zassert_true(success, "Closing image with failure");

	ctx.current_image_no++;
	ctx.current_image_offset = 0;

	return 0;
}

/*
 * Generic test that verifies that expected image data is written while downloading and
 * unpacking a given DFU Multi Image package.
 */

static int comparison_test(const uint8_t *package, size_t package_size,
			   const struct expected *expected, uint8_t *buffer, size_t buffer_size,
			   size_t chunk_size)
{
	int err;

	ctx.expected = *expected;
	ctx.current_image_no = 0;
	ctx.current_image_offset = 0;

	/* Initialize DFU and register image writers for all expected images */

	err = dfu_multi_image_init(buffer, buffer_size);

	if (err) {
		return err;
	}

	for (size_t i = 0; i < expected->image_count; ++i) {
		struct dfu_image_writer writer = { .image_id = expected->images[i].image_id,
						   .open = image_comparator_open,
						   .write = image_comparator_write,
						   .close = image_comparator_close };

		err = dfu_multi_image_register_writer(&writer);

		if (err) {
			return err;
		}
	}

	/* Write the package using the requested chunk size */

	for (size_t i = 0; i < package_size; i += chunk_size) {
		err = dfu_multi_image_write(i, package + i, MIN(chunk_size, package_size));

		if (err) {
			return err;
		}
	}

	err = dfu_multi_image_done(true);

	if (err) {
		return err;
	}

	zassert_equal(ctx.current_image_no, expected->image_count, "Too few images written");
	return 0;
}

static const uint8_t two_image_package[] = {
	0x1e, 0x00, 0xa1, 0x63, 0x69, 0x6d, 0x67, 0x82, 0xa2, 0x62, 0x69, 0x64, 0x00,
	0x64, 0x73, 0x69, 0x7a, 0x65, 0x0f, 0xa2, 0x62, 0x69, 0x64, 0x19, 0x01, 0x00,
	0x64, 0x73, 0x69, 0x7a, 0x65, 0x11, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x20, 0x30,
	0x20, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x69, 0x6d, 0x61, 0x67, 0x65,
	0x20, 0x32, 0x35, 0x36, 0x20, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74
};

static const struct expected two_image_package_expected = {
	.images = { EXPECTED_IMAGE(0, "image 0 content"),
		    EXPECTED_IMAGE(256, "image 256 content") },
	.image_count = 2,
};

ZTEST(dfu_multi_image_test, test_two_image_package)
{
	uint8_t buffer[128];

	zassert_ok(comparison_test(two_image_package, sizeof(two_image_package),
				   &two_image_package_expected, buffer, sizeof(buffer), 100),
		   "DFU failed");
}

ZTEST(dfu_multi_image_test, test_two_image_package_small_chunk)
{
	uint8_t buffer[128];

	zassert_ok(comparison_test(two_image_package, sizeof(two_image_package),
				   &two_image_package_expected, buffer, sizeof(buffer), 1),
		   "DFU failed");
}

ZTEST(dfu_multi_image_test, test_too_small_buffer)
{
	int err;
	uint8_t buffer[31];

	/*
	 * Test that the DFU fails gracefully when the provided buffer is too small
	 * to store the package header.
	 */
	err = comparison_test(two_image_package, sizeof(two_image_package),
			      &two_image_package_expected, buffer, sizeof(buffer), 100);
	zassert_equal(err, -ENOMEM, "Memory error expected but not occurred");

	/*
	 * Test that the DFU fails with "invalid argument" error when the provided buffer
	 * is too small to store the package header size.
	 */
	err = comparison_test(two_image_package, sizeof(two_image_package),
			      &two_image_package_expected, buffer, 1, 100);
	zassert_equal(err, -EINVAL, "Invalid argument error expected but not occurred");
}

ZTEST(dfu_multi_image_test, test_too_small_package)
{
	int err;
	uint8_t buffer[32];
	struct expected expected;

	/*
	 * Test that the DFU fails gracefully when the provided package is truncated.
	 */

	expected = two_image_package_expected;
	err = comparison_test(two_image_package, sizeof(two_image_package) - 1, &expected, buffer,
			      sizeof(buffer), 100);
	zassert_equal(err, -ESPIPE, "DFU passed despite of truncated package");
}

ZTEST(dfu_multi_image_test, test_too_large_package)
{
	int err;
	uint8_t buffer[32];
	uint8_t package[sizeof(two_image_package) + 1];

	/*
	 * Test that the DFU fails gracefully when the provided package contains trailing data
	 * (not listed in the header).
	 */

	memcpy(package, two_image_package, sizeof(two_image_package));
	err = comparison_test(package, sizeof(package), &two_image_package_expected, buffer,
			      sizeof(buffer), 100);
	zassert_equal(err, -ESPIPE, "DFU passed despite of trailing data");
}

ZTEST(dfu_multi_image_test, test_skipped_image)
{
	uint8_t buffer[128];
	struct expected expected = two_image_package_expected;

	/*
	 * Test that if an application only registers a writer for the second of two
	 * images, the first one is silently ignored and the DFU still passes.
	 */
	expected.images[0] = expected.images[1];
	expected.image_count = 1;
	zassert_ok(comparison_test(two_image_package, sizeof(two_image_package), &expected, buffer,
				   sizeof(buffer), 100),
		   "DFU failed");
}

/*
 * See CMakeLists.txt of the test project for parameters passed to the script generating
 * the DFU Multi Image package. The expected values below should match the parameters.
 */

static const struct expected generated_dfu_package_expected = {
	.images = { EXPECTED_IMAGE(-1, "somecontent"), EXPECTED_IMAGE(1000000, "anothercontent") },
	.image_count = 2,
};

ZTEST(dfu_multi_image_test, test_generated_dfu_package)
{
	uint8_t buffer[128];
	uint8_t package[strlen(DFU_PACKAGE_HEX) / 2];
	size_t package_len;

	package_len = hex2bin(DFU_PACKAGE_HEX, strlen(DFU_PACKAGE_HEX), package, sizeof(package));
	zassert_true(package_len > 0, "Failed to convert package from hex string");

	zassert_ok(comparison_test(package, package_len, &generated_dfu_package_expected, buffer,
				   sizeof(buffer), 100),
		   "DFU failed");
}

ZTEST_SUITE(dfu_multi_image_test, NULL, NULL, NULL, NULL, NULL);
