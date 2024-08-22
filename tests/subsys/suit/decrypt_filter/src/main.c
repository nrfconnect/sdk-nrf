/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <suit_decrypt_filter.h>
#include <suit_ram_sink.h>
#include <suit_memptr_streamer.h>

/* Forward declaration of the internal, temporary AES key-unwrap implementation. */
psa_status_t suit_aes_key_unwrap_manual(psa_key_id_t kek_key_id, const uint8_t *wrapped_cek,
					size_t cek_bits, psa_key_type_t cek_key_type,
					psa_algorithm_t cek_key_alg,
					mbedtls_svc_key_id_t *unwrapped_cek_key_id);

/**
 * The master key used by these tests can be imported into the local KMS backend by running:
 *
 * nrfkms import_keyvalue -k TEST_AES_KEY -t aes -v aHWJdIkl5hdXw4SS1nTdVYE/q7ycMOZm2mR6qx/KvKw=
 *
 * The KEK below is derived from context "test"
 * To acquire ut run:
 * nrfkms export_derived -k TEST_AES_KEY -c test --format native
 * hexdump -e '16/1 "0x%02x, " "\n"' kms_output/derived_key_native_test_from_TEST_AES_KEY.bin
 */
static const uint8_t test_kek_key[] = {
	0xf8, 0xfa, 0x8e, 0x7b, 0xed, 0x32, 0xd0, 0xc7, 0x15, 0x1f, 0xd9, 0xab, 0x0d,
	0x8d, 0xed, 0x95, 0x26, 0xa8, 0x6a, 0x15, 0x34, 0x16, 0x01, 0xcf, 0x9c, 0x6b,
	0xba, 0x00, 0x6a, 0xab, 0xaa, 0x9a,
};

static uint8_t kek_key_id_cbor[] = {
	0x1A, 0x00, 0x00, 0x00, 0x00,
};

struct suit_decrypt_filter_tests_fixture {
	psa_key_id_t key_id;
};

static const uint8_t plaintext[] = {
	"This is a sample plaintext for testing the decryption filter",
};

static const uint8_t aad[] = {
	"sample aad"
};

/**
 * Encryption and using wrapped CEK achieved by running:
 *
 * echo "This is a sample plaintext for testing the decryption filter" > plaintext.txt
 * nrfkms wrap -k TEST_AES_KEY -c test -f plaintext.txt --format native -t aes --aad "sample aad"
 *
 * Wrapped CEK stored in the resulting wrapped_aek-aes-... file
 *
 * Ciphertext and NONCE (IV) taken from the encrypted_asset-... file, which is in format
 * |nonce (12 bytes)|ciphertext|tag (16 bytes)|
 *
*/
static const uint8_t wrapped_cek[] = {
	0x7d, 0xd6, 0xf4, 0xd3, 0x52, 0x44, 0x5a, 0x3a, 0x67, 0xb8, 0xcc,
	0x74, 0x5b, 0x4b, 0x6f, 0x70, 0x62, 0xc3, 0xf2, 0x7b, 0x6b, 0x14,
	0xf1, 0x06, 0x57, 0xa3, 0x68, 0x32, 0x44, 0xc3, 0x85, 0x77, 0x86,
	0xe7, 0xda, 0x15, 0xbf, 0xf8, 0x9e, 0x63
};

static const uint8_t ciphertext_aes_kw[] = {
	/* tag (16 bytes) */
	0xdc, 0xe6, 0x95, 0xac, 0x0f, 0x61, 0x87, 0x17, 0x51, 0x48, 0xb4, 0xa1,
	0x8e, 0x09, 0x89, 0xb4,
	/* ciphertext */
	0x8b, 0xfb, 0xd9, 0xe4, 0xcf, 0xde, 0xf8, 0xcf, 0xe5, 0x69, 0x9d, 0x6d,
	0x92, 0x8a, 0x04, 0xf8, 0x26, 0x22, 0xd5, 0xd8, 0xe8, 0x77, 0x18, 0x5a,
	0x01, 0x13, 0xba, 0xd5, 0x23, 0x72, 0xae, 0x80, 0x44, 0xed, 0xea, 0xdf,
	0x74, 0x79, 0x8a, 0x83, 0x52, 0x72, 0x2f, 0x43, 0x06, 0xe9, 0xd4, 0xbb,
	0x54, 0x8a, 0x0d, 0xea, 0x7f, 0xe6, 0x48, 0xf0, 0xfd, 0x0e, 0xbb, 0xaa,
	0xa3,
};

static const uint8_t iv_aes_kw[] = {
	0x61, 0xb4, 0x70, 0x53, 0xa5, 0xe2, 0x05, 0x68, 0xfe, 0x77, 0x12, 0x89,
};

static uint8_t output_buffer[128] = {0};

static void *test_suite_setup(void)
{
	static struct suit_decrypt_filter_tests_fixture fixture = {0};
	psa_status_t status;

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 256);

	status = psa_crypto_init();
	zassert_equal(status, PSA_SUCCESS, "Failed to init psa crypto");

	status = psa_import_key(&key_attributes, test_kek_key, sizeof(test_kek_key),
				&fixture.key_id);

	zassert_equal(status, PSA_SUCCESS, "Failed to import KEK");

	/* Encode KEK key ID as CBOR unsigned int */
	kek_key_id_cbor[1] = ((fixture.key_id >> 24) & 0xFF);
	kek_key_id_cbor[2] = ((fixture.key_id >> 16) & 0xFF);
	kek_key_id_cbor[3] = ((fixture.key_id >> 8) & 0xFF);
	kek_key_id_cbor[4] = ((fixture.key_id >> 0) & 0xFF);

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

static void test_before(void* f)
{
	(void) f;
	memset(output_buffer, 0, sizeof(output_buffer));
}


ZTEST_SUITE(suit_decrypt_filter_tests, NULL, test_suite_setup, test_before, NULL, test_suite_teardown);

ZTEST_F(suit_decrypt_filter_tests, test_aes_unwrap_smoke)
{
	mbedtls_svc_key_id_t unwrapped_cek_key_id;

	psa_status_t status =
		suit_aes_key_unwrap_manual(fixture->key_id, wrapped_cek, 256, PSA_KEY_TYPE_AES,
					   PSA_ALG_GCM, &unwrapped_cek_key_id);

	zassert_equal(status, PSA_SUCCESS, "Failed to unwrap CEK");
}

ZTEST_F(suit_decrypt_filter_tests, test_filter_smoke)
{
	struct stream_sink dec_sink;
	struct stream_sink ram_sink;
	struct suit_encryption_info enc_info = {
		.enc_alg_id = suit_cose_aes256_gcm,
		.IV = {
				.value = iv_aes_kw,
				.len = sizeof(iv_aes_kw),
			},
		.aad = {
				.value = aad,
				.len = strlen(aad),
			},
		.kw_alg_id = suit_cose_aes256_kw,
		.kw_key.aes = {.key_id = {.value = kek_key_id_cbor, .len = sizeof(kek_key_id_cbor)},
			       .ciphertext = {
						.value = wrapped_cek,
						.len = sizeof(wrapped_cek),
					}},
	};

	suit_plat_err_t err = suit_ram_sink_get(&ram_sink, output_buffer, sizeof(output_buffer));

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unable to create RAM sink");

	err = suit_decrypt_filter_get(&dec_sink, &enc_info, &ram_sink);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to create decrypt filter");

	err = suit_memptr_streamer_stream(ciphertext_aes_kw, sizeof(ciphertext_aes_kw), &dec_sink);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to decrypt ciphertext");

	err = dec_sink.flush(dec_sink.ctx);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to flush decrypt filter");

	err = dec_sink.release(dec_sink.ctx);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to release decrypt filter");

	zassert_equal(memcmp(output_buffer, plaintext, strlen(plaintext)), 0, "Decrypted plaintext does not match");
}
