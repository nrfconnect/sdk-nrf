/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <mocks.h>

#define TEST_COMPONENT_HANDLE ((suit_component_t)0x123)
#define TEST_COMPONENT_ID     ((struct zcbor_string *)0x456)
#define TEST_CLASS_ID	      ((suit_manifest_class_id_t *)0x789)
#define TEST_MANIFEST_VERSION                                                                      \
	{                                                                                          \
		.len = 3, .raw = { 1, 2, 3, 0, 0 }                                                 \
	}

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(suit_platform_version_app_tests, NULL, NULL, test_before, NULL, NULL);

static int suit_plat_component_id_get_valid_component_fake_func(suit_component_t handle,
								struct zcbor_string **component_id)
{
	zassert_equal(handle, TEST_COMPONENT_HANDLE, "Unexpected component handle value");
	zassert_not_equal(component_id, NULL,
			  "API must provide valid address to get component ID value");

	*component_id = TEST_COMPONENT_ID;

	return SUIT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_component_type_instld_fake_func(struct zcbor_string *component_id,
						 suit_component_type_t *type)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(type, NULL, "API must provide valid address to get component type value");

	*type = SUIT_COMPONENT_TYPE_INSTLD_MFST;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_invalid_fake_func(struct zcbor_string *component_id,
						     suit_manifest_class_id_t **class_id)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(class_id, NULL,
			  "API must provide valid address to get manifest class ID value");

	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_valid_fake_func(struct zcbor_string *component_id,
						   suit_manifest_class_id_t **class_id)
{
	zassert_equal(component_id, TEST_COMPONENT_ID, "Unexpected component ID value");
	zassert_not_equal(class_id, NULL,
			  "API must provide valid address to get manifest class ID value");

	*class_id = TEST_CLASS_ID;

	return SUIT_PLAT_SUCCESS;
}

static suit_ssf_err_t suit_get_installed_manifest_info_ipc_error_fake_func(
	suit_manifest_class_id_t *manifest_class_id, unsigned int *seq_num,
	suit_semver_raw_t *version, suit_digest_status_t *status, int *alg_id,
	suit_plat_mreg_t *digest)
{
	zassert_equal(manifest_class_id, TEST_CLASS_ID,
		      "Unexpected manifest class ID request value");
	zassert_equal(seq_num, NULL, "Unexpected manifest sequence number request value");
	zassert_equal(status, NULL, "Unexpected manifest digest status request value");
	zassert_equal(alg_id, NULL, "Unexpected manifest digest algorithm request value");
	zassert_equal(digest, NULL, "Unexpected manifest digest request value");

	zassert_not_equal(version, NULL,
			  "API must provide valid address to get manifest version value");

	return SUIT_PLAT_ERR_IPC;
}

static suit_ssf_err_t suit_get_installed_manifest_info_unknown_version_fake_func(
	suit_manifest_class_id_t *manifest_class_id, unsigned int *seq_num,
	suit_semver_raw_t *version, suit_digest_status_t *status, int *alg_id,
	suit_plat_mreg_t *digest)
{
	zassert_equal(manifest_class_id, TEST_CLASS_ID,
		      "Unexpected manifest class ID request value");
	zassert_equal(seq_num, NULL, "Unexpected manifest sequence number request value");
	zassert_equal(status, NULL, "Unexpected manifest digest status request value");
	zassert_equal(alg_id, NULL, "Unexpected manifest digest algorithm request value");
	zassert_equal(digest, NULL, "Unexpected manifest digest request value");

	zassert_not_equal(version, NULL,
			  "API must provide valid address to get manifest version value");

	version->len = 0;

	return SUIT_SUCCESS;
}

static suit_ssf_err_t suit_get_installed_manifest_info_too_long_version_fake_func(
	suit_manifest_class_id_t *manifest_class_id, unsigned int *seq_num,
	suit_semver_raw_t *version, suit_digest_status_t *status, int *alg_id,
	suit_plat_mreg_t *digest)
{
	zassert_equal(manifest_class_id, TEST_CLASS_ID,
		      "Unexpected manifest class ID request value");
	zassert_equal(seq_num, NULL, "Unexpected manifest sequence number request value");
	zassert_equal(status, NULL, "Unexpected manifest digest status request value");
	zassert_equal(alg_id, NULL, "Unexpected manifest digest algorithm request value");
	zassert_equal(digest, NULL, "Unexpected manifest digest request value");

	zassert_not_equal(version, NULL,
			  "API must provide valid address to get manifest version value");

	version->len = ARRAY_SIZE(version->raw) + 1;

	return SUIT_SUCCESS;
}

static suit_ssf_err_t suit_get_installed_manifest_info_valid_version_fake_func(
	suit_manifest_class_id_t *manifest_class_id, unsigned int *seq_num,
	suit_semver_raw_t *version, suit_digest_status_t *status, int *alg_id,
	suit_plat_mreg_t *digest)
{
	suit_semver_raw_t sample_version = TEST_MANIFEST_VERSION;

	zassert_equal(manifest_class_id, TEST_CLASS_ID,
		      "Unexpected manifest class ID request value");
	zassert_equal(seq_num, NULL, "Unexpected manifest sequence number request value");
	zassert_equal(status, NULL, "Unexpected manifest digest status request value");
	zassert_equal(alg_id, NULL, "Unexpected manifest digest algorithm request value");
	zassert_equal(digest, NULL, "Unexpected manifest digest request value");

	zassert_not_equal(version, NULL,
			  "API must provide valid address to get manifest version value");

	version->len = sample_version.len;
	for (size_t i = 0; i < sample_version.len; i++) {
		version->raw[i] = sample_version.raw[i];
	}

	return SUIT_SUCCESS;
}

ZTEST(suit_platform_version_app_tests, test_instld_invalid_class)
{
	suit_semver_raw_t version;

	version.len = ARRAY_SIZE(version.raw);

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is invalid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_invalid_fake_func;

	/* WHEN platform is asked to return manifest version */
	int ret = suit_plat_component_version_get(TEST_COMPONENT_HANDLE, version.raw, &version.len);

	/* THEN manifest version is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Invalid manifest version retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");

	/* ... and other checks were not performed */
	zassert_equal(suit_get_installed_manifest_info_fake.call_count, 0,
		      "Incorrect number of suit_get_installed_manifest_info() calls");
}

ZTEST(suit_platform_version_app_tests, test_instld_manifest_ipc_error)
{
	suit_semver_raw_t version;

	version.len = ARRAY_SIZE(version.raw);

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is valid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_valid_fake_func;
	/* ... and the manifest version cannot be fetched due to IPC communcation issue */
	suit_get_installed_manifest_info_fake.custom_fake =
		suit_get_installed_manifest_info_ipc_error_fake_func;

	/* WHEN platform is asked to return manifest version */
	int ret = suit_plat_component_version_get(TEST_COMPONENT_HANDLE, version.raw, &version.len);

	/* THEN manifest version is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_PARAMETER, ret, "Invalid manifest version retrieved");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest version for given class ID was fetched */
	zassert_equal(suit_get_installed_manifest_info_fake.call_count, 1,
		      "Incorrect number of suit_get_installed_manifest_info_fake() calls");
}

ZTEST(suit_platform_version_app_tests, test_instld_manifest_unknown_version)
{
	suit_semver_raw_t version;

	version.len = ARRAY_SIZE(version.raw);

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is valid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_valid_fake_func;
	/* ... and the manifest version is unknown */
	suit_get_installed_manifest_info_fake.custom_fake =
		suit_get_installed_manifest_info_unknown_version_fake_func;

	/* WHEN platform is asked to return manifest version */
	int ret = suit_plat_component_version_get(TEST_COMPONENT_HANDLE, version.raw, &version.len);

	/* THEN manifest version is not returned... */
	zassert_equal(SUIT_FAIL_CONDITION, ret, "Unknown manifest version returned");
	zassert_equal(version.len, 0, "Invalid manifest version received");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest version for given class ID was fetched */
	zassert_equal(suit_get_installed_manifest_info_fake.call_count, 1,
		      "Incorrect number of suit_get_installed_manifest_info_fake() calls");
}

ZTEST(suit_platform_version_app_tests, test_instld_manifest_too_long_version)
{
	suit_semver_raw_t version;

	version.len = ARRAY_SIZE(version.raw);

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is valid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_valid_fake_func;
	/* ... and the manifest version is too long */
	suit_get_installed_manifest_info_fake.custom_fake =
		suit_get_installed_manifest_info_too_long_version_fake_func;

	/* WHEN platform is asked to return manifest version */
	int ret = suit_plat_component_version_get(TEST_COMPONENT_HANDLE, version.raw, &version.len);

	/* THEN manifest version is not returned... */
	zassert_equal(SUIT_ERR_UNSUPPORTED_PARAMETER, ret, "Too long manifest version returned");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest version for given class ID was fetched */
	zassert_equal(suit_get_installed_manifest_info_fake.call_count, 1,
		      "Incorrect number of suit_get_installed_manifest_info_fake() calls");
}

ZTEST(suit_platform_version_app_tests, test_instld_manifest_valid_version)
{
	suit_semver_raw_t test_version = TEST_MANIFEST_VERSION;
	suit_semver_raw_t version;

	version.len = ARRAY_SIZE(version.raw);

	/* GIVEN the component handle value is valid. */
	suit_plat_component_id_get_fake.custom_fake =
		suit_plat_component_id_get_valid_component_fake_func;
	/* ... and the component type is installed manifest component */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_instld_fake_func;
	/* ... and the manifest class ID for given component is valid */
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_valid_fake_func;
	/* ... and the manifest version is valid */
	suit_get_installed_manifest_info_fake.custom_fake =
		suit_get_installed_manifest_info_valid_version_fake_func;

	/* WHEN platform is asked to return manifest version */
	int ret = suit_plat_component_version_get(TEST_COMPONENT_HANDLE, version.raw, &version.len);

	/* THEN manifest version is returned... */
	zassert_equal(SUIT_SUCCESS, ret, "Valid manifest version not returned");
	zassert_equal(version.len, test_version.len, "Invalid manifest version length received");
	zassert_mem_equal(version.raw, test_version.raw, sizeof(test_version.raw),
			  "Invalid manifest version value received");

	/* ... and component ID for given component handle was fetched */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	/* ... and component type for given component ID was fetched */
	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	/* ... and manifest class ID for given component ID was fetched */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	/* ... and manifest version for given class ID was fetched */
	zassert_equal(suit_get_installed_manifest_info_fake.call_count, 1,
		      "Incorrect number of suit_get_installed_manifest_info_fake() calls");
}
