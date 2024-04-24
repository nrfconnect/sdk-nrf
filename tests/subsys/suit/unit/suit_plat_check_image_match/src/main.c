/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <mocks.h>
#include <suit_platform.h>

#define TEST_ENVELOPE_ADDRESS	   ((uint8_t *)0xabcd)
#define TEST_ENVELOPE_SIZE	   ((size_t)0xef01)
#define TEST_MEMPTR_STORAGE_HANDLE ((memptr_storage_handle_t)0x2345)
#define TEST_MEMPTR_COMP_ADDRESS   ((uint8_t *)0xabcd)
#define TEST_MEMPTR_COMP_SIZE	   ((size_t)0xef01)

static int valid_component_number = 1;
static int invalid_component_number = 2;
static intptr_t valid_component_handle = 0x1234;
static struct zcbor_string valid_component_id = {.value = (const uint8_t *)0x1234, .len = 123};
static uint8_t valid_manifest_digest[] = {
	0xE6, 0x34, 0x39, 0x3D, 0x82, 0x64, 0x93, 0xF4, 0x2C, 0x42, 0xCA,
	0x81, 0x08, 0x82, 0x7E, 0xF6, 0x10, 0x31, 0x47, 0x85, 0xB0, 0xA0,
	0xAF, 0xAE, 0xF4, 0x85, 0xA5, 0x05, 0x14, 0xE8, 0x38, 0x56,
};
static struct zcbor_string valid_digest = {.value = valid_manifest_digest,
					   .len = sizeof(valid_manifest_digest)};

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

static suit_plat_err_t release_fake_func(void *ctx)
{
	return SUIT_SUCCESS;
}

static int suit_plat_component_id_get_invalid_fake_func(suit_component_t handle,
							struct zcbor_string **component_id)
{
	zassert_equal(valid_component_handle, handle, "Invalid component handle value");
	zassert_not_null(component_id, "The API must provide a valid pointer");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

static int suit_plat_component_id_get_correct_fake_func(suit_component_t handle,
							struct zcbor_string **component_id)
{
	zassert_equal(valid_component_handle, handle, "Invalid component handle value");
	zassert_not_null(component_id, "The API must provide a valid pointer");

	*component_id = &valid_component_id;

	return SUIT_SUCCESS;
}

int suit_plat_retrieve_manifest_invalid_fake_func(suit_component_t component_handle,
						  const uint8_t **envelope_str,
						  size_t *envelope_len)
{
	zassert_equal(valid_component_handle, component_handle, "Invalid component handle value");
	zassert_not_null(envelope_str, "The API must provide a valid pointer");
	zassert_not_null(envelope_len, "The API must provide a valid pointer");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

int suit_plat_retrieve_manifest_correct_fake_func(suit_component_t component_handle,
						  const uint8_t **envelope_str,
						  size_t *envelope_len)
{
	zassert_equal(valid_component_handle, component_handle, "Invalid component handle value");
	zassert_not_null(envelope_str, "The API must provide a valid pointer");
	zassert_not_null(envelope_len, "The API must provide a valid pointer");

	*envelope_str = TEST_ENVELOPE_ADDRESS;
	*envelope_len = TEST_ENVELOPE_SIZE;

	return SUIT_SUCCESS;
}

int suit_processor_get_manifest_metadata_invalid_fake_func(
	const uint8_t *envelope_str, size_t envelope_len, bool authenticate,
	struct zcbor_string *manifest_component_id, struct zcbor_string *digest,
	enum suit_cose_alg *alg, unsigned int *seq_num)
{
	zassert_equal(TEST_ENVELOPE_ADDRESS, envelope_str, "Invalid envelope_str");
	zassert_equal(TEST_ENVELOPE_SIZE, envelope_len, "Invalid envelope_len");
	zassert_not_null(digest, "The API must provide a valid pointer");
	zassert_not_null(alg, "The API must provide a valid pointer");

	return SUIT_ERR_DECODING;
}

int suit_processor_get_manifest_metadata_correct_fake_func(
	const uint8_t *envelope_str, size_t envelope_len, bool authenticate,
	struct zcbor_string *manifest_component_id, struct zcbor_string *digest,
	enum suit_cose_alg *alg, unsigned int *seq_num)
{
	zassert_equal(TEST_ENVELOPE_ADDRESS, envelope_str, "Invalid envelope_str");
	zassert_equal(TEST_ENVELOPE_SIZE, envelope_len, "Invalid envelope_len");
	zassert_not_null(digest, "The API must provide a valid pointer");
	zassert_not_null(alg, "The API must provide a valid pointer");

	*alg = suit_cose_sha256;
	digest->value = valid_manifest_digest;
	digest->len = sizeof(valid_manifest_digest);

	return SUIT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_invalid_fake_func(struct zcbor_string *component_id,
						  suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");

	*type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_component_type_invalid_type_ret_fake_func(struct zcbor_string *component_id,
							   suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");

	*type = SUIT_COMPONENT_TYPE_CACHE_POOL +
		30; /* Arbitrary value added to get type out of bounds */

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_mem_correct_fake_func(struct zcbor_string *component_id,
						      suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_MEM;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_special_correct_fake_func(struct zcbor_string *component_id,
							  suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_SOC_SPEC;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_unsupported_correct_fake_func(struct zcbor_string *component_id,
							      suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_cand_mfst_correct_fake_func(struct zcbor_string *component_id,
							    suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_CAND_MFST;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_instld_mfst_correct_fake_func(struct zcbor_string *component_id,
							      suit_component_type_t *type)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(type, NULL,
			  "The API must provide a valid type pointer, to decode component ID");

	*type = SUIT_COMPONENT_TYPE_INSTLD_MFST;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_number_invalid_fake_func(struct zcbor_string *component_id,
						    uint32_t *number)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(number, NULL,
			  "The API must provide a valid number pointer, to decode component ID");

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_component_number_correct_fake_func(struct zcbor_string *component_id,
						    uint32_t *number)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(number, NULL,
			  "The API must provide a valid number pointer, to decode component ID");

	*number = valid_component_number;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_number_invalid_ret_number_fake_func(struct zcbor_string *component_id,
							       uint32_t *number)
{
	zassert_equal(&valid_component_id, component_id, "Invalid component ID value");
	zassert_not_equal(number, NULL,
			  "The API must provide a valid number pointer, to decode component ID");

	*number = invalid_component_number;

	return SUIT_PLAT_SUCCESS;
}

static int suit_plat_component_impl_data_get_invalid_fake_func(suit_component_t handle,
							       void **impl_data)
{
	zassert_equal(valid_component_handle, handle, "Unexpected component handle value");
	zassert_not_equal(impl_data, NULL,
			  "API must provide valid address to get imeplementation-specific data");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

static int suit_plat_component_impl_data_get_correct_fake_func(suit_component_t handle,
							       void **impl_data)
{
	zassert_equal(valid_component_handle, handle, "Unexpected component handle value");
	zassert_not_equal(impl_data, NULL,
			  "API must provide valid address to get imeplementation-specific data");

	*impl_data = TEST_MEMPTR_STORAGE_HANDLE;

	return SUIT_SUCCESS;
}

static suit_memptr_storage_err_t
suit_memptr_storage_ptr_get_invalid_fake_func(memptr_storage_handle_t handle,
					      const uint8_t **payload_ptr, size_t *payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(payload_size, NULL,
			  "API must provide valid address to get installed manifest size value");

	return SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;
}

static suit_memptr_storage_err_t
suit_memptr_storage_ptr_get_correct_fake_func(memptr_storage_handle_t handle,
					      const uint8_t **payload_ptr, size_t *payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(payload_size, NULL,
			  "API must provide valid address to get installed manifest size value");

	*payload_ptr = TEST_MEMPTR_COMP_ADDRESS;
	*payload_size = TEST_MEMPTR_COMP_SIZE;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_digest_sink_get_correct_fake_func(struct stream_sink *sink,
						       psa_algorithm_t algorithm,
						       const uint8_t *expected_digest)
{
	zassert_not_null(sink, "The API must provide a valid pointer");
	zassert_not_null(expected_digest, "The API must provide a valid pointer");

	sink->release = release_fake_func;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_digest_sink_get_invalid_fake_func(struct stream_sink *sink,
						       psa_algorithm_t algorithm,
						       const uint8_t *expected_digest)
{
	zassert_not_null(sink, "The API must provide a valid pointer");
	zassert_not_null(expected_digest, "The API must provide a valid pointer");

	return SUIT_PLAT_ERR_CRASH;
}

suit_plat_err_t suit_generic_address_streamer_stream_correct_fake_func(const uint8_t *payload,
								       size_t payload_size,
								       struct stream_sink *sink)
{
	zassert_not_null(payload, "The API must provide a valid pointer");
	zassert_not_equal(payload_size, 0, "The API must provide a valid payload size");
	zassert_not_null(sink, "The API must provide a valid pointer");

	return SUIT_SUCCESS;
}

suit_plat_err_t suit_generic_address_streamer_stream_invalid_fake_func(const uint8_t *payload,
								       size_t payload_size,
								       struct stream_sink *sink)
{
	zassert_not_null(payload, "The API must provide a valid pointer");
	zassert_not_equal(payload_size, 0, "The API must provide a valid payload size");
	zassert_not_null(sink, "The API must provide a valid pointer");

	return SUIT_PLAT_ERR_NOT_FOUND;
}

digest_sink_err_t suit_digest_sink_digest_match_invalid_fake_func(void *ctx)
{
	zassert_not_null(ctx, "The API must provide a valid pointer");

	return DIGEST_SINK_ERR_DIGEST_MISMATCH;
}

digest_sink_err_t suit_digest_sink_digest_match_correct_fake_func(void *ctx)
{
	zassert_not_null(ctx, "The API must provide a valid pointer");

	return SUIT_PLAT_SUCCESS;
}

ZTEST_SUITE(suit_plat_check_image_match_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_plat_check_image_match_tests, test_instld_mfst_OK)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_mfst_correct_fake_func;
	suit_plat_retrieve_manifest_fake.custom_fake =
		suit_plat_retrieve_manifest_correct_fake_func;
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_correct_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_SUCCESS, ret, "Check have failed: %i", ret);

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_retrieve_manifest_fake.call_count, 1,
		      "Incorrect number of suit_plat_retrieve_manifest() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 1,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_cand_mfst_OK)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_cand_mfst_correct_fake_func;
	suit_plat_retrieve_manifest_fake.custom_fake =
		suit_plat_retrieve_manifest_correct_fake_func;
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_correct_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_SUCCESS, ret, "Check have failed: %i", ret);

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_retrieve_manifest_fake.call_count, 1,
		      "Incorrect number of suit_plat_retrieve_manifest() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 1,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_instld_mfst_get_manifest_metadata_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_mfst_correct_fake_func;
	suit_plat_retrieve_manifest_fake.custom_fake =
		suit_plat_retrieve_manifest_correct_fake_func;
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(
		SUIT_ERR_DECODING, ret,
		"Check should have failed - suit_processor_get_manifest_metadata returned error");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_retrieve_manifest_fake.call_count, 1,
		      "Incorrect number of suit_plat_retrieve_manifest() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 1,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_instld_mfst_retrieve_manifest_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_mfst_correct_fake_func;
	suit_plat_retrieve_manifest_fake.custom_fake =
		suit_plat_retrieve_manifest_invalid_fake_func;
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Check should have failed - suit_plat_retrieve_manifest returned error");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_retrieve_manifest_fake.call_count, 1,
		      "Incorrect number of suit_plat_retrieve_manifest() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_decode_component_type_invalid_type_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_invalid_type_ret_fake_func;
	suit_plat_retrieve_manifest_fake.custom_fake =
		suit_plat_retrieve_manifest_invalid_fake_func;
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(
		SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		"Check should have failed - suit_plat_decode_component_type returned invalid type");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_retrieve_manifest_fake.call_count, 0,
		      "Incorrect number of suit_plat_retrieve_manifest() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_decode_component_type_unsupported_type_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_unsupported_correct_fake_func;
	suit_plat_retrieve_manifest_fake.custom_fake =
		suit_plat_retrieve_manifest_invalid_fake_func;
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(
		SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		"Check should have failed - suit_plat_decode_component_type returned invalid type");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_retrieve_manifest_fake.call_count, 0,
		      "Incorrect number of suit_plat_retrieve_manifest() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_decode_component_type_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_invalid_fake_func;
	suit_plat_retrieve_manifest_fake.custom_fake =
		suit_plat_retrieve_manifest_invalid_fake_func;
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Check should have failed - suit_plat_decode_component_type returned error");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_retrieve_manifest_fake.call_count, 0,
		      "Incorrect number of suit_plat_retrieve_manifest() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_component_id_get_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_invalid_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_invalid_fake_func;
	suit_plat_retrieve_manifest_fake.custom_fake =
		suit_plat_retrieve_manifest_invalid_fake_func;
	suit_processor_get_manifest_metadata_fake.custom_fake =
		suit_processor_get_manifest_metadata_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Check should have failed - suit_plat_component_id_get returned error");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_retrieve_manifest_fake.call_count, 0,
		      "Incorrect number of suit_plat_retrieve_manifest() calls");
	zassert_equal(suit_processor_get_manifest_metadata_fake.call_count, 0,
		      "Incorrect number of suit_processor_get_manifest_metadata() calls");
}

/**************************************************************************************************/
ZTEST(suit_plat_check_image_match_tests, test_soc_spec_OK)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_special_correct_fake_func;
	suit_plat_decode_component_number_fake.custom_fake =
		suit_plat_decode_component_number_correct_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

#ifdef CONFIG_SOC_SERIES_NRF54HX
	zassert_equal(SUIT_SUCCESS, ret, "Check have failed: %i", ret);
#else  /* CONFIG_SOC_SERIES_NRF54HX */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Check should have failed for HW other than NRF54H20");
#endif /* CONFIG_SOC_SERIES_NRF54HX */

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_number() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_soc_spec_invalid_component_number_ret)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_special_correct_fake_func;
	suit_plat_decode_component_number_fake.custom_fake =
		suit_plat_decode_component_number_invalid_ret_number_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Check should have failed - invalid component number");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_number() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_soc_spec_invalid_component_number)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_special_correct_fake_func;
	suit_plat_decode_component_number_fake.custom_fake =
		suit_plat_decode_component_number_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Check should have failed - invalid component number");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_component_number_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_number() calls");
}

/**************************************************************************************************/
ZTEST(suit_plat_check_image_match_tests, test_mem_OK)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_correct_fake_func;
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_correct_fake_func;
	suit_digest_sink_get_fake.custom_fake = suit_digest_sink_get_correct_fake_func;
	suit_generic_address_streamer_stream_fake.custom_fake =
		suit_generic_address_streamer_stream_correct_fake_func;
	suit_digest_sink_digest_match_fake.custom_fake =
		suit_digest_sink_digest_match_correct_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_SUCCESS, ret, "Check have failed: %i", ret);

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
	zassert_equal(suit_digest_sink_get_fake.call_count, 1,
		      "Incorrect number of suit_digest_sink_get() calls");
	zassert_equal(suit_generic_address_streamer_stream_fake.call_count, 1,
		      "Incorrect number of suit_generic_address_streamer_stream() calls");
	zassert_equal(suit_digest_sink_digest_match_fake.call_count, 1,
		      "Incorrect number of suit_digest_sink_digest_match() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_mem_sink_digest_match_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_correct_fake_func;
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_correct_fake_func;
	suit_digest_sink_get_fake.custom_fake = suit_digest_sink_get_correct_fake_func;
	suit_generic_address_streamer_stream_fake.custom_fake =
		suit_generic_address_streamer_stream_correct_fake_func;
	suit_digest_sink_digest_match_fake.custom_fake =
		suit_digest_sink_digest_match_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_FAIL_CONDITION, ret, "Check have failed: %i", ret);

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
	zassert_equal(suit_digest_sink_get_fake.call_count, 1,
		      "Incorrect number of suit_digest_sink_get() calls");
	zassert_equal(suit_generic_address_streamer_stream_fake.call_count, 1,
		      "Incorrect number of suit_generic_address_streamer_stream() calls");
	zassert_equal(suit_digest_sink_digest_match_fake.call_count, 1,
		      "Incorrect number of suit_digest_sink_digest_match() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_mem_generic_address_streamer_stream_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_correct_fake_func;
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_correct_fake_func;
	suit_digest_sink_get_fake.custom_fake = suit_digest_sink_get_correct_fake_func;
	suit_generic_address_streamer_stream_fake.custom_fake =
		suit_generic_address_streamer_stream_invalid_fake_func;
	suit_digest_sink_digest_match_fake.custom_fake =
		suit_digest_sink_digest_match_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_ERR_CRASH, ret, "Check have failed: %i", ret);

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
	zassert_equal(suit_digest_sink_get_fake.call_count, 1,
		      "Incorrect number of suit_digest_sink_get() calls");
	zassert_equal(suit_generic_address_streamer_stream_fake.call_count, 1,
		      "Incorrect number of suit_generic_address_streamer_stream() calls");
	zassert_equal(suit_digest_sink_digest_match_fake.call_count, 0,
		      "Incorrect number of suit_digest_sink_digest_match() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_mem_digest_sink_get_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_correct_fake_func;
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_correct_fake_func;
	suit_digest_sink_get_fake.custom_fake = suit_digest_sink_get_invalid_fake_func;
	suit_generic_address_streamer_stream_fake.custom_fake =
		suit_generic_address_streamer_stream_invalid_fake_func;
	suit_digest_sink_digest_match_fake.custom_fake =
		suit_digest_sink_digest_match_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_ERR_CRASH, ret, "Check have failed: %i", ret);

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
	zassert_equal(suit_digest_sink_get_fake.call_count, 1,
		      "Incorrect number of suit_digest_sink_get() calls");
	zassert_equal(suit_generic_address_streamer_stream_fake.call_count, 0,
		      "Incorrect number of suit_generic_address_streamer_stream() calls");
	zassert_equal(suit_digest_sink_digest_match_fake.call_count, 0,
		      "Incorrect number of suit_digest_sink_digest_match() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_mem_memptr_storage_ptr_get_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_correct_fake_func;
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_invalid_fake_func;
	suit_digest_sink_get_fake.custom_fake = suit_digest_sink_get_invalid_fake_func;
	suit_generic_address_streamer_stream_fake.custom_fake =
		suit_generic_address_streamer_stream_invalid_fake_func;
	suit_digest_sink_digest_match_fake.custom_fake =
		suit_digest_sink_digest_match_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_ERR_CRASH, ret, "Check have failed: %i", ret);

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
	zassert_equal(suit_digest_sink_get_fake.call_count, 0,
		      "Incorrect number of suit_digest_sink_get() calls");
	zassert_equal(suit_generic_address_streamer_stream_fake.call_count, 0,
		      "Incorrect number of suit_generic_address_streamer_stream() calls");
	zassert_equal(suit_digest_sink_digest_match_fake.call_count, 0,
		      "Incorrect number of suit_digest_sink_digest_match() calls");
}

ZTEST(suit_plat_check_image_match_tests, test_mem_component_impl_data_get_fail)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_mem_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_invalid_fake_func;
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_invalid_fake_func;
	suit_digest_sink_get_fake.custom_fake = suit_digest_sink_get_invalid_fake_func;
	suit_generic_address_streamer_stream_fake.custom_fake =
		suit_generic_address_streamer_stream_invalid_fake_func;
	suit_digest_sink_digest_match_fake.custom_fake =
		suit_digest_sink_digest_match_invalid_fake_func;

	int ret = suit_plat_check_image_match(valid_component_handle, suit_cose_sha256,
					      &valid_digest);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Check have failed: %i", ret);

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_impl_data_get() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_get() calls");
	zassert_equal(suit_digest_sink_get_fake.call_count, 0,
		      "Incorrect number of suit_digest_sink_get() calls");
	zassert_equal(suit_generic_address_streamer_stream_fake.call_count, 0,
		      "Incorrect number of suit_generic_address_streamer_stream() calls");
	zassert_equal(suit_digest_sink_digest_match_fake.call_count, 0,
		      "Incorrect number of suit_digest_sink_digest_match() calls");
}
