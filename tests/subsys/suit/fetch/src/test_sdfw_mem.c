/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef CONFIG_TEST_SUIT_PLATFORM_FETCH_VARIANT_SDFW
#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <suit_memptr_storage.h>
#include <suit_platform_internal.h>
#include <suit_dfu_cache.h>
#include <suit_ipuc_sdfw.h>
#include <mocks_sdfw.h>
#include <psa/crypto.h>
#include <suit_decrypt_filter.h>
#include <decrypt_test_utils.h>

static uint8_t test_data[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
			      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

/* clang-format off */
static const uint8_t valid_manifest_component[] = {
	0x82, /* array: 2 elements */
		0x4c, /* byte string: 12 bytes */
			0x6b, /* string: 11 characters */
				'I', 'N', 'S', 'T', 'L', 'D', '_', 'M', 'F', 'S', 'T',
		0x50, /* byte string: 16 bytes */
			0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53, 0x9c,
			0xa3, 0x18, 0x68, 0x1b, 0x03, 0x69, 0x5e, 0x36
};
/* clang-format on */

static struct zcbor_string valid_manifest_component_id = {
	.value = valid_manifest_component,
	.len = sizeof(valid_manifest_component),
};

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_sdfw_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(fetch_sdfw_mem_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(fetch_sdfw_mem_tests, test_integrated_fetch_to_msink_OK)
{
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ipc_client_id = 1234;
	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);
	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &valid_dst_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "in-place updateable component found");

	ret = suit_ipuc_sdfw_declare(dst_handle, SUIT_MANIFEST_UNKNOWN);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - error %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &valid_dst_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write_setup failed - error %i", ret);

	ret = suit_plat_fetch_integrated(dst_handle, &source, &valid_dst_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &valid_dst_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "in-place updateable component found");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
}

ZTEST(fetch_tests, test_integrated_fetch_to_msink_encrypted_OK)
{
	suit_component_t component_handle;
	memptr_storage_handle_t handle = NULL;
	struct zcbor_string source = {.value = decrypt_test_ciphertext_direct,
				      .len = sizeof(decrypt_test_ciphertext_direct)};

	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x02, 0x45,
				 0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	psa_key_id_t cek_key_id;
	uint8_t cek_key_id_cbor[] = {
		0x1A, 0x00, 0x00, 0x00, 0x00,
	};

	psa_status_t status = psa_crypto_init();

	zassert_equal(status, PSA_SUCCESS, "Failed to init psa crypto");

	status = decrypt_test_init_encryption_key(decrypt_test_key_data,
						  sizeof(decrypt_test_key_data), &cek_key_id,
						  PSA_ALG_GCM, cek_key_id_cbor);
	zassert_equal(status, PSA_SUCCESS, "Failed to import key");

	struct suit_encryption_info enc_info = DECRYPT_TEST_ENC_INFO_DEFAULT_INIT(cek_key_id_cbor);

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(component_handle, &source, &valid_manifest_component_id,
					 &enc_info);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_plat_component_impl_data_get(component_handle, &handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_component_impl_data_get failed - error %i",
		      ret);

	const uint8_t *payload;
	size_t payload_size = 0;

	ret = suit_memptr_storage_ptr_get(handle, &payload, &payload_size);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "storage.get failed - error %i", ret);
	zassert_equal(memcmp(decrypt_test_plaintext, payload, strlen(decrypt_test_plaintext)), 0,
		      "Retrieved decrypted payload doesn't mach decrypt_test_plaintext");
	zassert_equal(sizeof(decrypt_test_plaintext), payload_size,
		      "Retrieved payload_size doesn't mach size of decrypt_test_plaintext");

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);

	psa_destroy_key(cek_key_id);
}

ZTEST(fetch_tests, test_integrated_fetch_to_memptr_encrypted_NOK)
{
	suit_component_t component_handle;
	memptr_storage_handle_t handle = NULL;
	struct zcbor_string source = {.value = decrypt_test_ciphertext_direct,
				      .len = sizeof(decrypt_test_ciphertext_direct)};

	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	psa_key_id_t cek_key_id;
	uint8_t cek_key_id_cbor[] = {
		0x1A, 0x00, 0x00, 0x00, 0x00,
	};

	psa_status_t status = psa_crypto_init();

	zassert_equal(status, PSA_SUCCESS, "Failed to init psa crypto");

	status = decrypt_test_init_encryption_key(decrypt_test_key_data,
						  sizeof(decrypt_test_key_data), &cek_key_id,
						  PSA_ALG_GCM, cek_key_id_cbor);
	zassert_equal(status, PSA_SUCCESS, "Failed to import key");

	struct suit_encryption_info enc_info = DECRYPT_TEST_ENC_INFO_DEFAULT_INIT(cek_key_id_cbor);

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(component_handle, &source, &valid_manifest_component_id,
					 &enc_info);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_plat_component_impl_data_get(component_handle, &handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_component_impl_data_get failed - error %i",
		      ret);

	const uint8_t *payload;
	size_t payload_size = 0;

	ret = suit_memptr_storage_ptr_get(handle, &payload, &payload_size);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "storage.get failed - error %i", ret);
	zassert_not_equal(memcmp(decrypt_test_plaintext, payload, strlen(decrypt_test_plaintext)),
			  0, "Retrieved decrypted payload should not mach decrypt_test_plaintext");
	zassert_equal(sizeof(decrypt_test_plaintext), payload_size,
		      "Retrieved payload_size doesn't mach size of decrypt_test_plaintext");

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);

	psa_destroy_key(cek_key_id);
}

ZTEST(fetch_tests, test_integrated_fetch_to_msink_encrypted_wrong_key_NOK)
{
	suit_component_t component_handle;
	memptr_storage_handle_t handle = NULL;
	struct zcbor_string source = {.value = decrypt_test_ciphertext_direct,
				      .len = sizeof(decrypt_test_ciphertext_direct)};

	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x02, 0x45,
				 0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	psa_key_id_t cek_key_id;
	uint8_t cek_key_id_cbor[] = {
		0x1A, 0x00, 0x00, 0x00, 0x00,
	};

	psa_status_t status = psa_crypto_init();

	zassert_equal(status, PSA_SUCCESS, "Failed to init psa crypto");

	uint8_t wrong_test_key_data[sizeof(decrypt_test_key_data)];

	memcpy(wrong_test_key_data, decrypt_test_key_data, sizeof(wrong_test_key_data));
	/* Corrupt CEK that we store.*/
	wrong_test_key_data[10] = 0x00;

	status = decrypt_test_init_encryption_key(wrong_test_key_data, sizeof(wrong_test_key_data),
						  &cek_key_id, PSA_ALG_GCM, cek_key_id_cbor);
	zassert_equal(status, PSA_SUCCESS, "Failed to import key");

	struct suit_encryption_info enc_info = DECRYPT_TEST_ENC_INFO_DEFAULT_INIT(cek_key_id_cbor);

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(component_handle, &source, &valid_manifest_component_id,
					 &enc_info);
	zassert_equal(ret, SUIT_ERR_AUTHENTICATION,
		      "suit_plat_fetch should have failed with SUIT_ERR_AUTHENTICATION");

	ret = suit_plat_component_impl_data_get(component_handle, &handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_component_impl_data_get failed - error %i",
		      ret);

	const uint8_t *payload;
	size_t payload_size = 0;

	ret = suit_memptr_storage_ptr_get(handle, &payload, &payload_size);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "storage.get failed - error %i", ret);
	zassert_not_equal(memcmp(decrypt_test_plaintext, payload, strlen(decrypt_test_plaintext)),
			  0, "Retrieved decrypted payload should not mach decrypt_test_plaintext");
	zassert_equal(payload_size, 0, "Retrieved payload_size is not 0");

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);

	psa_destroy_key(cek_key_id);
}
#endif /* CONFIG_TEST_SUIT_PLATFORM_FETCH_VARIANT_SDFW */
