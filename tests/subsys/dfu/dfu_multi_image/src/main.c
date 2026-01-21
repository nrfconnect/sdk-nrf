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

#include <zephyr/settings/settings.h>

#define SETTINGS_MODULE "dfumi"
#define CBOR_HEADER_SETTING_NAME "h"
#define FULL_CBOR_HEADER_SETTING_NAME SETTINGS_MODULE "/" CBOR_HEADER_SETTING_NAME
#define IMAGE_FINISHED_SETTING_NAME "i"
#define FULL_IMAGE_FINISHED_SETTING_NAME SETTINGS_MODULE "/" IMAGE_FINISHED_SETTING_NAME

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
	size_t header_size;
};

/*
 * Implement fake image writer which compares written data with expected content instead of
 * actually storing it.
 */

struct comparison_context {
	struct expected expected;
	size_t current_image_no;
	size_t current_image_offset;
	size_t saved_image_offsets[CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT];
	bool ignore_image_size;
	bool reset_current_image_on_next_call;
};

static struct comparison_context ctx;

static void cleanup(void *fixture)
{
	(void) fixture;

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	int err;

	err = settings_delete(FULL_CBOR_HEADER_SETTING_NAME);
	zassert_ok(err, "Error deleting " FULL_CBOR_HEADER_SETTING_NAME
		   " from settings %d", err);

	err = settings_delete(FULL_IMAGE_FINISHED_SETTING_NAME);
	zassert_ok(err, "Error deleting " FULL_IMAGE_FINISHED_SETTING_NAME
		   " from settings %d", err);
#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */

	memset(&ctx, 0, sizeof(ctx));
}

static int image_comparator_open(int image_id, size_t image_size)
{
	zassert_true(ctx.current_image_no < ctx.expected.image_count, "Too many images written");
	zassert_equal(ctx.expected.images[ctx.current_image_no].image_id, image_id,
		      "Unexpected image id");
	if (!ctx.ignore_image_size) {
		zassert_equal(ctx.expected.images[ctx.current_image_no].content_size, image_size,
			      "Unexpected image size");
	}
	zassert_equal(ctx.current_image_offset, 0, "Opening image while already in progress");

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	ctx.current_image_offset = ctx.saved_image_offsets[ctx.current_image_no];
#endif

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
#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	ctx.saved_image_offsets[ctx.current_image_no] = ctx.current_image_offset;
#endif

	return 0;
}

static int image_comparator_close(bool success)
{
	zassert_true(success, "Closing image with failure");

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	if (success) {
		ctx.saved_image_offsets[ctx.current_image_no] = 0;
	}
#endif
	ctx.current_image_no++;
	ctx.current_image_offset = 0;

	return 0;
}

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
static int image_comparator_offset(size_t *offset)
{
	zassert_not_null(offset, "Null offset pointer");

	*offset = ctx.current_image_offset;

	return 0;
}
#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */

static int image_comparator_reset(void)
{
#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	ctx.saved_image_offsets[ctx.current_image_no] = 0;
#endif
	ctx.current_image_offset = 0;
	if (ctx.reset_current_image_on_next_call) {
		ctx.current_image_no = 0;
		ctx.reset_current_image_on_next_call = false;
	} else {
		ctx.current_image_no++;
	}

	return 0;
}

static int simulate_reset(uint8_t buffer[], size_t buffer_size)
{
	int err;

	/* Simulate reset */
	ctx.current_image_no = 0;
	ctx.current_image_offset = 0;
	err = dfu_multi_image_init(buffer, buffer_size);

	if (err) {
		return err;
	}


	for (size_t i = 0; i < ctx.expected.image_count; ++i) {
		struct dfu_image_writer writer = { .image_id = ctx.expected.images[i].image_id,
						   .open = image_comparator_open,
						   .write = image_comparator_write,
						   .close = image_comparator_close,
						   .reset = image_comparator_reset };
#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
		writer.offset = image_comparator_offset;
#endif

		err = dfu_multi_image_register_writer(&writer);
		if (err) {
			return err;
		}
	}

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
	err = simulate_reset(buffer, buffer_size);

	if (err) {
		return err;
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
	.header_size = 32,
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
	.header_size = 32,
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

static void verify_dfu_multi_image_reset_test(size_t bytes_to_write)
{
	int err;
	uint8_t buffer[32];

	ctx.current_image_no = 0;
	ctx.current_image_offset = 0;

	err = simulate_reset(buffer, sizeof(buffer));
	zassert_ok(err, "DFU init failed");

	err = dfu_multi_image_write(0, two_image_package, bytes_to_write);
	zassert_ok(err, "DFU write failed");
	zassert_equal(dfu_multi_image_offset(), bytes_to_write, "Incorrect offset");

	ctx.ignore_image_size = true;
	ctx.reset_current_image_on_next_call = true;
	err = dfu_multi_image_reset();
	zassert_ok(err, "DFU reset failed");

	zassert_equal(dfu_multi_image_offset(), 0, "Offset after dfu_multi_image_reset is not 0");
	zassert_equal(ctx.current_image_offset, 0, "Current image offset is not 0");

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	for (size_t i = 0; i < CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT; ++i) {
		zassert_equal(ctx.saved_image_offsets[i], 0,
			      "Saved image offset for image %d is not 0", i);
	}
#endif

}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_reset_first_image)
{
	ctx.expected = two_image_package_expected;

	size_t bytes_to_write = ctx.expected.header_size + 5;

	verify_dfu_multi_image_reset_test(bytes_to_write);
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_reset_second_image)
{
	ctx.expected = two_image_package_expected;

	size_t bytes_to_write = ctx.expected.header_size
				+ ctx.expected.images[0].content_size + 5;

	verify_dfu_multi_image_reset_test(bytes_to_write);
}

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS

static void verify_offset_after_reset_test(size_t bytes_to_write, size_t expected_offset)
{
	int err;
	uint8_t buffer[32];

	ctx.current_image_no = 0;
	ctx.current_image_offset = 0;

	err = simulate_reset(buffer, sizeof(buffer));
	zassert_ok(err, "DFU init failed");

	err = dfu_multi_image_write(0, two_image_package, bytes_to_write);
	zassert_ok(err, "DFU write failed");
	zassert_equal(dfu_multi_image_offset(), bytes_to_write, "Offset before reset incorrect");

	err = simulate_reset(buffer, sizeof(buffer));
	zassert_ok(err, "Reset simulation failed");

	zassert_equal(dfu_multi_image_offset(), expected_offset,
		      "Offset after reset does not match expected");
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_save_no_full_header)
{
	ctx.expected = two_image_package_expected;

	verify_offset_after_reset_test(ctx.expected.header_size - 8, 0);
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_save_header_only)
{
	ctx.expected = two_image_package_expected;

	verify_offset_after_reset_test(ctx.expected.header_size, ctx.expected.header_size);
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_save_first_image_part)
{
	ctx.expected = two_image_package_expected;

	size_t bytes_count = ctx.expected.header_size + 4;

	verify_offset_after_reset_test(bytes_count, bytes_count);
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_save_first_full)
{
	ctx.expected = two_image_package_expected;

	size_t bytes_count = ctx.expected.header_size + ctx.expected.images[0].content_size;

	verify_offset_after_reset_test(bytes_count, bytes_count);
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_save_second_image_part)
{
	ctx.expected = two_image_package_expected;

	size_t bytes_count = ctx.expected.header_size + ctx.expected.images[0].content_size + 4;

	verify_offset_after_reset_test(bytes_count, bytes_count);
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_save_full_package)
{
	ctx.expected = two_image_package_expected;

	size_t bytes_count = sizeof(two_image_package);

	verify_offset_after_reset_test(bytes_count, bytes_count);
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_save_write_loads_progress)
{
	ctx.expected = two_image_package_expected;

	int err;
	uint8_t buffer[32];
	size_t bytes_count = ctx.expected.header_size + 4;
	size_t additional_bytes = 4;

	verify_offset_after_reset_test(bytes_count, bytes_count);

	err = simulate_reset(buffer, sizeof(buffer));
	zassert_ok(err, "Reset simulation failed");

	err = dfu_multi_image_write(bytes_count, two_image_package + bytes_count, additional_bytes);
	zassert_ok(err, "DFU write failed");

	zassert_equal(dfu_multi_image_offset(), bytes_count + additional_bytes,
		      "Offset after reset does not match expected");
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_save_done_loads_progress)
{
	ctx.expected = two_image_package_expected;

	int err;
	uint8_t buffer[32];
	size_t bytes_count = sizeof(two_image_package);

	verify_offset_after_reset_test(bytes_count, bytes_count);

	err = simulate_reset(buffer, sizeof(buffer));
	zassert_ok(err, "reset simulation failed");

	/**
	 * If done did not load the offset properly it would fail due
	 * offset being less than expected.
	 * This way we confirm that done loads the saved offset and
	 * works properly if the device resets right before calling it.
	 */
	err = dfu_multi_image_done(true);
	zassert_ok(err, "DFU done failed");
}

ZTEST(dfu_multi_image_test, test_dfu_multi_image_save_done_resets_saved_progress)
{
	ctx.expected = two_image_package_expected;

	int err;
	uint8_t buffer[32];
	size_t bytes_count = sizeof(two_image_package);

	verify_offset_after_reset_test(bytes_count, bytes_count);

	err = dfu_multi_image_done(true);
	zassert_ok(err, "DFU done failed");

	err = simulate_reset(buffer, sizeof(buffer));
	zassert_ok(err, "reset simulation failed");

	zassert_equal(dfu_multi_image_offset(), 0,
		      "Offset after calling dfu_multi_image_done is not 0");
}

#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */

ZTEST_SUITE(dfu_multi_image_test, NULL, NULL, NULL, cleanup, NULL);
