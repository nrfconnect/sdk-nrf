/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_platform.h>
#include <suit_plat_decode_util.h>
#include <suit_memptr_storage.h>
#include <suit_sink.h>

/* Data for digest calculation */
static uint8_t data[] = {0xde, 0xad, 0xbe, 0xef};

/* Valid sha-256 hash of the data */
static uint8_t valid_digest_value[] = {0x5f, 0x78, 0xc3, 0x32, 0x74, 0xe4, 0x3f, 0xa9,
				       0xde, 0x56, 0x59, 0x26, 0x5c, 0x1d, 0x91, 0x7e,
				       0x25, 0xc0, 0x37, 0x22, 0xdc, 0xb0, 0xb8, 0xd2,
				       0x7d, 0xb8, 0xd5, 0xfe, 0xaa, 0x81, 0x39, 0x53};

/* Valid digest of the data */
static struct zcbor_string valid_digest = {
	.value = valid_digest_value,
	.len = sizeof(valid_digest_value),
};

/* Invalid sha-256 of the data */
static uint8_t invalid_digest_value[32] = {0x00};

/* Invalid digest of the data */
static struct zcbor_string invalid_digest = {
	.value = invalid_digest_value,
	.len = sizeof(invalid_digest_value),
};

/* Valid id value for cand_img component */
static uint8_t valid_cand_img_component_id_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
						      '_',  'I',  'M',	'G', 0x41, 0x02};

/* Valid cand_img component id */
static struct zcbor_string valid_cand_img_component_id = {
	.value = valid_cand_img_component_id_value,
	.len = sizeof(valid_cand_img_component_id_value),
};

ZTEST_SUITE(check_image_match_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(check_image_match_tests, test_mem_valid)
{
	/* GIVEN a MEM component pointing to the data */
	uint8_t component_id_value[] = {0x84, 0x44, 0x63, 'M',	'E',  'M',
					0x41, 0x02, 0x41, 0x00, 0x41, 0x00};

	struct zcbor_string valid_src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	void *impl_data = NULL;
	suit_component_t component;

	int err = suit_plat_create_component_handle(&valid_src_component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err, "test error - suit_plat_component_impl_data_get: %d", err);

	err = suit_memptr_storage_ptr_store((memptr_storage_handle_t)impl_data, data, sizeof(data));
	zassert_equal(SUIT_PLAT_SUCCESS, err, "test error - suit_memptr_storage_ptr_store: %d",
		      err);

	/* WHEN a check image match function is called */
	err = suit_plat_check_image_match(component, suit_cose_sha256, &valid_digest);

	/* THEN calculated digest matches valid digest value */
	zassert_equal(SUIT_SUCCESS, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(check_image_match_tests, test_mem_wrong_size)
{
	/* GIVEN a MEM component pointing to the data... */
	uint8_t component_id_value[] = {0x84, 0x44, 0x63, 'M',	'E',  'M',
					0x41, 0x02, 0x41, 0x00, 0x41, 0x00};

	struct zcbor_string valid_src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	void *impl_data = NULL;
	suit_component_t component;

	int err = suit_plat_create_component_handle(&valid_src_component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err, "test error - suit_plat_component_impl_data_get: %d", err);

	/* ... but indicating wrong data size */
	err = suit_memptr_storage_ptr_store((memptr_storage_handle_t)impl_data, data,
					    sizeof(data) - 1);
	zassert_equal(SUIT_PLAT_SUCCESS, err, "test error - suit_memptr_storage_ptr_store: %d",
		      err);

	/* WHEN a check image match function is called */
	err = suit_plat_check_image_match(component, suit_cose_sha256, &valid_digest);

	/* THEN appropriate error code is returned */
	zassert_equal(SUIT_FAIL_CONDITION, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(check_image_match_tests, test_mem_wrong_digest)
{
	/* GIVEN a MEM component pointing to the data */
	uint8_t component_id_value[] = {0x84, 0x44, 0x63, 'M',	'E',  'M',
					0x41, 0x02, 0x41, 0x00, 0x41, 0x00};

	struct zcbor_string valid_src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	void *impl_data = NULL;
	suit_component_t component;

	int err = suit_plat_create_component_handle(&valid_src_component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err, "test error - suit_plat_component_impl_data_get: %d", err);

	err = suit_memptr_storage_ptr_store((memptr_storage_handle_t)impl_data, data, sizeof(data));
	zassert_equal(SUIT_PLAT_SUCCESS, err, "test error - suit_memptr_storage_ptr_store: %d",
		      err);

	/* WHEN a check image match function is called with invalid digest */
	err = suit_plat_check_image_match(component, suit_cose_sha256, &invalid_digest);

	/* THEN appropriate error is returned */
	zassert_equal(SUIT_FAIL_CONDITION, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(check_image_match_tests, test_mem_invalid_component)
{
	/* GIVEN a MEM component with invalid component id, pointing to the data */
	uint8_t invalid_component_id_value[] = {0x84, 0x44, 0x63, 'M',	'E',  'M',  0x41, 0x02,
						0x41, 0x00, 0x45, 0xFF, 0x00, 0x00, 0x00, 0x00};

	struct zcbor_string invalid_src_component_id = {
		.value = invalid_component_id_value,
		.len = sizeof(invalid_component_id_value),
	};

	suit_component_t component;

	/* WHEN a component handle is created */
	int err = suit_plat_create_component_handle(&invalid_src_component_id, &component);

	/* THEN it is not supported */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, err,
		      "test error - create_component_handle failed: %d", err);
}

ZTEST(check_image_match_tests, test_cand_img_match)
{
	void *impl_data = NULL;

	/* GIVEN CAND_IMG component pointing to the data */
	suit_component_t component;

	int err = suit_plat_create_component_handle(&valid_cand_img_component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err,
		      "test error - suit_plat_component_impl_data_get failed: %d", err);

	err = suit_memptr_storage_ptr_store((memptr_storage_handle_t)impl_data, data, sizeof(data));
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "test error - suit_memptr_storage_ptr_store failed: %d", err);

	/* WHEN a check image match function is called */
	err = suit_plat_check_image_match(component, suit_cose_sha256, &valid_digest);

	/* THEN calculated digest matches valid digest value */
	zassert_equal(SUIT_SUCCESS, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(check_image_match_tests, test_cand_img_mismatch)
{
	void *impl_data = NULL;
	/* GIVEN CAND_IMG component pointing to the data */
	suit_component_t component;

	int err = suit_plat_create_component_handle(&valid_cand_img_component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err,
		      "test error - suit_plat_component_impl_data_get failed: %d", err);

	err = suit_memptr_storage_ptr_store((memptr_storage_handle_t)impl_data, data, sizeof(data));
	zassert_equal(SUIT_PLAT_SUCCESS, err,
		      "test error - suit_memptr_storage_ptr_store failed: %d", err);

	/* WHEN a check image match function is called with invalid digest */
	err = suit_plat_check_image_match(component, suit_cose_sha256, &invalid_digest);

	/* THEN appropriate error code is returned */
	zassert_equal(SUIT_FAIL_CONDITION, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

#ifndef CONFIG_SOC_SERIES_NRF54HX
ZTEST(check_image_match_tests, test_soc_spec_1)
{
	/* GIVEN SOC_SPEC component 1 - SDFW */
	uint8_t component_id_value[] = {0x82, 0x49, 0x68, 'S', 'O',  'C', '_',
					'S',  'P',  'E',  'C', 0x41, 0x01};
	struct zcbor_string component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	void *impl_data = NULL;
	suit_component_t component;

	int err = suit_plat_create_component_handle(&component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err,
		      "test error - suit_plat_component_impl_data_get failed: %d", err);

	/* WHEN a check image match function is called on a a non-nrf54h20 platform */
	err = suit_plat_check_image_match(component, suit_cose_sha256, &valid_digest);

	/* THEN appropriate error code is returned */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(check_image_match_tests, test_soc_spec_2)
{
	/* SOC_SPEC component 2 - SDFW recovery*/
	uint8_t component_id_value[] = {0x82, 0x49, 0x68, 'S', 'O',  'C', '_',
					'S',  'P',  'E',  'C', 0x41, 0x02};
	struct zcbor_string component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	void *impl_data = NULL;
	suit_component_t component;

	int err = suit_plat_create_component_handle(&component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	err = suit_plat_component_impl_data_get(component, &impl_data);

	zassert_equal(SUIT_SUCCESS, err,
		      "test error - suit_plat_component_impl_data_get failed: %d", err);

	/* WHEN a check image match function is called on a a non-nrf54h20 platform*/
	err = suit_plat_check_image_match(component, suit_cose_sha256, &valid_digest);

	/* THEN appropriate error code is returned */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(check_image_match_tests, test_soc_spec_3)
{
	/* SOC_SPEC component 3 - not supported */
	uint8_t component_id_value[] = {0x82, 0x49, 0x68, 'S', 'O',  'C', '_',
					'S',  'P',  'E',  'C', 0x41, 0x03};
	struct zcbor_string component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	void *impl_data = NULL;
	suit_component_t component;

	int err = suit_plat_create_component_handle(&component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err,
		      "test error - suit_plat_component_impl_data_get failed: %d", err);

	/* WHEN a check image match function is called on a a non-nrf54h20 platform*/
	err = suit_plat_check_image_match(component, suit_cose_sha256, &valid_digest);

	/* THEN appropriate error code is returned */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(check_image_match_tests, test_soc_spec_none)
{
	/* SOC_SPEC component without an index - not supported */
	uint8_t component_id_value[] = {0x81, 0x49, 0x68, 'S', 'O', 'C', '_', 'S', 'P', 'E', 'C'};
	struct zcbor_string component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	void *impl_data = NULL;
	suit_component_t component;

	int err = suit_plat_create_component_handle(&component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err,
		      "test error - suit_plat_component_impl_data_get failed: %d", err);

	/* WHEN a check image match function is called on a a non-nrf54h20 platform*/
	err = suit_plat_check_image_match(component, suit_cose_sha256, &valid_digest);

	/* THEN appropriate error code is returned */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}
#endif /* CONFIG_SOC_SERIES_NRF54HX */

ZTEST(check_image_match_tests, test_unhandled_component)
{
	/* NOTE: This test will need to be reworked once the installed manifest component will be
	 * handled
	 */

	/* GIVEN unhandled component */
	uint8_t component_id_value[] = {0x82, 0x4c, 0x6b, 'I',	'N',  'S',  'T',  'L',
					'D',  '_',  'M',  'F',	'S',  'T',  0x50, 0x56,
					0xdc, 0x9a, 0x14, 0x28, 0xd8, 0x52, 0xd3, 0xbd,
					0x62, 0xe7, 0x7a, 0x08, 0xbc, 0x8b, 0x91};

	struct zcbor_string valid_src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	suit_component_t component;

	int err = suit_plat_create_component_handle(&valid_src_component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	/* WHEN a check image match function is called */
	err = suit_plat_check_image_match(component, suit_cose_sha256, &valid_digest);

	/* THEN appropriate error code is returned */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}
