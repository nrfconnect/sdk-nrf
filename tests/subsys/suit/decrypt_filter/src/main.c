/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <psa/crypto.h>
#include <suit_decrypt_filter.h>
#include <suit_ram_sink.h>
#include <suit_memptr_streamer.h>
#include <suit_mci.h>
#include <decrypt_test_utils.h>

/* Forward declaration of the internal, temporary AES key-unwrap implementation. */
psa_status_t suit_aes_key_unwrap_manual(psa_key_id_t kek_key_id, const uint8_t *wrapped_cek,
					size_t cek_bits, psa_key_type_t cek_key_type,
					psa_algorithm_t cek_key_alg,
					mbedtls_svc_key_id_t *unwrapped_cek_key_id);

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, suit_mci_fw_encryption_key_id_validate, const suit_manifest_class_id_t *,
		uint32_t);

static uint8_t kek_key_id_cbor[] = {
	0x1A, 0x00, 0x00, 0x00, 0x00,
};
static psa_key_id_t kek_key_id;

struct suit_decrypt_filter_tests_fixture {
	psa_key_id_t key_id;
};

static uint8_t output_buffer[128] = {0};

static void *test_suite_setup(void)
{
	static struct suit_decrypt_filter_tests_fixture fixture = {0};
	psa_status_t status;

	status = psa_crypto_init();
	zassert_equal(status, PSA_SUCCESS, "Failed to init psa crypto");

	/* Init the KEK key */
	status = decrypt_test_init_encryption_key(decrypt_test_key_data,
				sizeof(decrypt_test_key_data), &fixture.key_id,
				PSA_ALG_ECB_NO_PADDING, kek_key_id_cbor);
	zassert_equal(status, PSA_SUCCESS, "Failed to import key");

	kek_key_id = fixture.key_id;

	return &fixture;
}

static void test_suite_teardown(void *f)
{
	struct suit_decrypt_filter_tests_fixture *fixture =
		(struct suit_decrypt_filter_tests_fixture *)f;

	if (fixture != NULL) {
		psa_destroy_key(fixture->key_id);
	}
}

static void test_before(void *f)
{
	(void) f;
	memset(output_buffer, 0, sizeof(output_buffer));
	RESET_FAKE(suit_mci_fw_encryption_key_id_validate);
}


ZTEST_SUITE(suit_decrypt_filter_tests, NULL, test_suite_setup, test_before, NULL,
	    test_suite_teardown);

ZTEST_F(suit_decrypt_filter_tests, test_aes_unwrap_smoke)
{
	mbedtls_svc_key_id_t unwrapped_cek_key_id;

	psa_status_t status =
		suit_aes_key_unwrap_manual(fixture->key_id, decrypt_test_wrapped_cek,
					256, PSA_KEY_TYPE_AES, PSA_ALG_GCM, &unwrapped_cek_key_id);

	zassert_equal(status, PSA_SUCCESS, "Failed to unwrap CEK");
}

ZTEST_F(suit_decrypt_filter_tests, test_filter_smoke_aes_kw)
{
	struct stream_sink dec_sink;
	struct stream_sink ram_sink;
	struct suit_encryption_info enc_info = {
		.enc_alg_id = suit_cose_aes256_gcm,
		.IV = {
				.value = decrypt_test_iv_aes_kw,
				.len = sizeof(decrypt_test_iv_aes_kw),
			},
		.aad = {
				.value = decrypt_test_aad,
				.len = strlen(decrypt_test_aad),
			},
		.kw_alg_id = suit_cose_aes256_kw,
		.kw_key.aes = {.key_id = {.value = kek_key_id_cbor, .len = sizeof(kek_key_id_cbor)},
			       .ciphertext = {
						.value = decrypt_test_wrapped_cek,
						.len = sizeof(decrypt_test_wrapped_cek),
					}},
	};
	suit_mci_fw_encryption_key_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;

	suit_plat_err_t err = suit_ram_sink_get(&ram_sink, output_buffer, sizeof(output_buffer));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unable to create RAM sink");

	err = suit_decrypt_filter_get(&dec_sink, &enc_info,
				&decrypt_test_sample_class_id, &ram_sink);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to create decrypt filter");
	zassert_equal(suit_mci_fw_encryption_key_id_validate_fake.call_count, 1,
		      "Invalid number of calls to suit_mci_fw_encryption_key_id_validate");
	zassert_equal_ptr(suit_mci_fw_encryption_key_id_validate_fake.arg0_val,
			   &decrypt_test_sample_class_id,
			   "Invalid class ID passed to suit_mci_fw_encryption_key_id_validate");
	zassert_equal(suit_mci_fw_encryption_key_id_validate_fake.arg1_val, kek_key_id,
			   "Invalid key ID passed to suit_mci_fw_encryption_key_id_validate");

	err = suit_memptr_streamer_stream(decrypt_test_ciphertext_aes_kw,
					sizeof(decrypt_test_ciphertext_aes_kw), &dec_sink);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to decrypt ciphertext");

	err = dec_sink.flush(dec_sink.ctx);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to flush decrypt filter");

	err = dec_sink.release(dec_sink.ctx);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to release decrypt filter");

	zassert_equal(memcmp(output_buffer, decrypt_test_plaintext,
					strlen(decrypt_test_plaintext)), 0,
		      "Decrypted plaintext does not match");
}

ZTEST_F(suit_decrypt_filter_tests, test_filter_smoke_direct)
{
	struct stream_sink dec_sink;
	struct stream_sink ram_sink;
	uint8_t cek_key_id_cbor[] = {
		0x1A, 0x00, 0x00, 0x00, 0x00,
	};

	psa_key_id_t cek_key_id;

	psa_status_t const status =
		decrypt_test_init_encryption_key(decrypt_test_key_data,
				sizeof(decrypt_test_key_data), &cek_key_id,
				PSA_ALG_GCM, cek_key_id_cbor);

	zassert_equal(status, PSA_SUCCESS, "Failed to import key");

	struct suit_encryption_info enc_info =
			DECRYPT_TEST_ENC_INFO_DEFAULT_INIT(cek_key_id_cbor);

	suit_mci_fw_encryption_key_id_validate_fake.return_val = SUIT_PLAT_SUCCESS;

	suit_plat_err_t err = suit_ram_sink_get(&ram_sink, output_buffer, sizeof(output_buffer));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unable to create RAM sink");

	err = suit_decrypt_filter_get(&dec_sink, &enc_info,
				&decrypt_test_sample_class_id, &ram_sink);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to create decrypt filter");

	zassert_equal(suit_mci_fw_encryption_key_id_validate_fake.call_count, 1,
		      "Invalid number of calls to suit_mci_fw_encryption_key_id_validate");
	zassert_equal_ptr(suit_mci_fw_encryption_key_id_validate_fake.arg0_val,
			   &decrypt_test_sample_class_id,
			   "Invalid class ID passed to suit_mci_fw_encryption_key_id_validate");
	zassert_equal(suit_mci_fw_encryption_key_id_validate_fake.arg1_val, cek_key_id,
			   "Invalid key ID passed to suit_mci_fw_encryption_key_id_validate");


	err = suit_memptr_streamer_stream(decrypt_test_ciphertext_direct,
						sizeof(decrypt_test_ciphertext_direct), &dec_sink);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to decrypt ciphertext");

	err = dec_sink.flush(dec_sink.ctx);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to flush decrypt filter");

	err = dec_sink.release(dec_sink.ctx);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to release decrypt filter");

	zassert_equal(memcmp(output_buffer, decrypt_test_plaintext,
					strlen(decrypt_test_plaintext)), 0,
		      "Decrypted plaintext does not match");

	psa_destroy_key(cek_key_id);
}

ZTEST_F(suit_decrypt_filter_tests, test_key_id_validation_fail)
{
	struct stream_sink dec_sink;
	struct stream_sink ram_sink;
	uint8_t cek_key_id_cbor[] = {
		0x1A, 0x00, 0x00, 0x00, 0x00,
	};

	psa_key_id_t cek_key_id;

	psa_status_t const status =
		decrypt_test_init_encryption_key(decrypt_test_key_data,
				sizeof(decrypt_test_key_data), &cek_key_id,
				PSA_ALG_GCM, cek_key_id_cbor);

	zassert_equal(status, PSA_SUCCESS, "Failed to import key");

	struct suit_encryption_info enc_info =
			DECRYPT_TEST_ENC_INFO_DEFAULT_INIT(cek_key_id_cbor);

	suit_mci_fw_encryption_key_id_validate_fake.return_val = MCI_ERR_WRONGKEYID;

	suit_plat_err_t err = suit_ram_sink_get(&ram_sink, output_buffer, sizeof(output_buffer));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unable to create RAM sink");

	err = suit_decrypt_filter_get(&dec_sink, &enc_info,
				&decrypt_test_sample_class_id, &ram_sink);
	zassert_equal(err, SUIT_PLAT_ERR_AUTHENTICATION,
		      "Incorrect error code when getting decrypt filter");

	zassert_equal(suit_mci_fw_encryption_key_id_validate_fake.call_count, 1,
		      "Invalid number of calls to suit_mci_fw_encryption_key_id_validate");
	zassert_equal_ptr(suit_mci_fw_encryption_key_id_validate_fake.arg0_val,
			   &decrypt_test_sample_class_id,
			   "Invalid class ID passed to suit_mci_fw_encryption_key_id_validate");
	zassert_equal(suit_mci_fw_encryption_key_id_validate_fake.arg1_val, cek_key_id,
			   "Invalid key ID passed to suit_mci_fw_encryption_key_id_validate");
}
