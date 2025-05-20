/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <mocks.h>
#include <suit_plat_component_compatibility.h>

static suit_manifest_class_id_t sample_class_id = {{0xca, 0xd8, 0x52, 0x3a, 0xf8, 0x29, 0x5a, 0x9a,
						    0xba, 0x85, 0x2e, 0xa0, 0xb2, 0xf5, 0x77,
						    0xc9}};

static struct zcbor_string valid_manifest_component_id = {
	.value = (const uint8_t *)0x1234,
	.len = 123,
};

static struct zcbor_string valid_component_id = {
	.value = (const uint8_t *)0x4321,
	.len = 321,
};

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

static int
suit_plat_component_compatibility_check_invalid_fake_func(const suit_manifest_class_id_t *class_id,
							  struct zcbor_string *component_id)
{
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	zassert_equal(&valid_component_id, component_id, "Invalid manifest component ID value");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

static int
suit_plat_component_compatibility_check_correct_fake_func(const suit_manifest_class_id_t *class_id,
							  struct zcbor_string *component_id)
{
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	zassert_equal(&valid_component_id, component_id, "Invalid manifest component ID value");

	return SUIT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_correct_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(&valid_manifest_component_id, component_id,
		      "Invalid manifest component ID value");
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	*class_id = &sample_class_id;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_invalid_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(&valid_manifest_component_id, component_id,
		      "Invalid manifest component ID value");
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	*class_id = NULL;

	return SUIT_PLAT_ERR_CRASH;
}

ZTEST_SUITE(suit_platform_authorize_component_id_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_platform_authorize_component_id_tests, test_null_manifest_component_id)
{
	int ret = suit_plat_authorize_component_id(NULL, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
}

ZTEST(suit_platform_authorize_component_id_tests, test_invalid_manifest_component_id_null_value)
{
	struct zcbor_string invalid_manifest_component_id = {
		.value = NULL,
		.len = 8,
	};

	int ret = suit_plat_authorize_component_id(&invalid_manifest_component_id,
						   &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
}

ZTEST(suit_platform_authorize_component_id_tests, test_invalid_manifest_component_id_0_len)
{
	struct zcbor_string invalid_manifest_component_id = {
		.value = (const uint8_t *)0x1234,
		.len = 0,
	};

	int ret = suit_plat_authorize_component_id(&invalid_manifest_component_id,
						   &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
}

ZTEST(suit_platform_authorize_component_id_tests, test_null_component_id)
{
	int ret = suit_plat_authorize_component_id(&valid_manifest_component_id, NULL);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
}

ZTEST(suit_platform_authorize_component_id_tests, test_invalid_component_id_null_value)
{
	struct zcbor_string invalid_component_id = {
		.value = NULL,
		.len = 8,
	};

	int ret = suit_plat_authorize_component_id(&valid_manifest_component_id,
						   &invalid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
}

ZTEST(suit_platform_authorize_component_id_tests, test_invalid_component_id_0_len)
{
	struct zcbor_string invalid_component_id = {
		.value = (const uint8_t *)0x1234,
		.len = 0,
	};

	int ret = suit_plat_authorize_component_id(&valid_manifest_component_id,
						   &invalid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_DECODING, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
}

ZTEST(suit_platform_authorize_component_id_tests, test_invalid_decode_manifest_class_id)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_invalid_fake_func;

	int ret =
		suit_plat_authorize_component_id(&valid_manifest_component_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
}

ZTEST(suit_platform_authorize_component_id_tests, test_invalid_sut_plat_check_component_id)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_plat_component_compatibility_check_fake.custom_fake =
		suit_plat_component_compatibility_check_invalid_fake_func;

	int ret =
		suit_plat_authorize_component_id(&valid_manifest_component_id, &valid_component_id);

	/* Manifest authentication fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Authorization should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
}

ZTEST(suit_platform_authorize_component_id_tests, test_OK)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_correct_fake_func;
	suit_plat_component_compatibility_check_fake.custom_fake =
		suit_plat_component_compatibility_check_correct_fake_func;

	int ret =
		suit_plat_authorize_component_id(&valid_manifest_component_id, &valid_component_id);

	/* Manifest authentication succeeds */
	zassert_equal(SUIT_SUCCESS, ret, "Authorization should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
}
