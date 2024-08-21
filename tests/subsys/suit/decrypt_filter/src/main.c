/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <suit_decrypt_filter.h>
#include <suit_digest_sink.h>
#include <suit_memptr_streamer.h>

/* Forward declaration of the internal, temporary AES key-unwrap implementation. */
psa_status_t suit_aes_key_unwrap_manual(psa_key_id_t kek_key_id, const uint8_t *wrapped_cek,
					size_t cek_bits, psa_key_type_t cek_key_type,
					psa_algorithm_t cek_key_alg,
					mbedtls_svc_key_id_t *unwrapped_cek_key_id);

static const uint8_t test_kek_key[] = {
	0x7b, 0xf2, 0x67, 0xbe, 0x5c, 0x57, 0x35, 0x77, 0xb6, 0xe0, 0x6d,
	0xa3, 0x61, 0xc0, 0x88, 0x6b, 0x38, 0x91, 0x8a, 0x76, 0xf4, 0x72,
	0x02, 0xac, 0xf1, 0x38, 0x19, 0x5f, 0x48, 0xd3, 0x60, 0x59,
};

static uint8_t kek_key_id_cbor[] = {
	0x1A, 0x00, 0x00, 0x00, 0x00,
};

struct suit_decrypt_filter_tests_fixture {
	psa_key_id_t key_id;
};

const static uint8_t ciphertext[] = {
	0x27, 0x3c, 0xa8, 0x1f, 0x01, 0x6b, 0x76, 0xbe, 0x22, 0xab, 0x9f, 0x76,
	0xab, 0x33, 0x2a, 0x24, 0x08, 0x3b, 0x98, 0xd4, 0x8f, 0x26, 0xa3, 0xdc,
	0x4f, 0x35, 0xfd, 0x0b, 0x38, 0xb2, 0xd2, 0xd2, 0xc9, 0x4c, 0xde, 0xc9,
	0x9c, 0xca, 0x5f, 0x56, 0xda, 0x01, 0xe3, 0x66, 0x50, 0xaf, 0x9c,
};

static const uint8_t valid_digest[] = {
	0xfa, 0xf2, 0xf9, 0xb2, 0x44, 0xdc, 0xc7, 0xaa, 0x53, 0x1e, 0x06,
	0x49, 0x86, 0x6f, 0xfb, 0x3f, 0xf0, 0x8f, 0x86, 0xad, 0xf4, 0xda,
	0x5e, 0x2c, 0x15, 0xc9, 0xf5, 0xac, 0x06, 0x6b, 0x9f, 0xde,
};

static const psa_algorithm_t valid_algorithm = PSA_ALG_SHA_256;

static const uint8_t suit_aad_aes256_gcm[] = {
	0x83,					   /* array (3 elements) */
	0x67,					   /* context: text (7 characters) */
	'E',  'n',  'c', 'r', 'y', 'p', 't', 0x43, /* protected: bstr encoded map (3 elements) */
	0xA1,					   /* map (1 element) */
	0x01, 0x03,				   /* alg_id: A256GCM */
	0x40					   /* external_aad: h'' */
};

static const uint8_t iv_aes256_gcm[] = {
	0x61, 0x61, 0x75, 0x1C, 0x47, 0x79, 0x33, 0x2F, 0xFC, 0xBE, 0x0A, 0xA9,
};

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

ZTEST_SUITE(suit_decrypt_filter_tests, NULL, test_suite_setup, NULL, NULL, test_suite_teardown);

ZTEST_F(suit_decrypt_filter_tests, test_aes_unwrap_smoke)
{
	mbedtls_svc_key_id_t unwrapped_cek_key_id;
	const uint8_t wrapped_cek[] = {0xb2, 0x43, 0x88, 0x9a, 0x6a, 0x4a, 0x91, 0xc4, 0xf0, 0xb0,
				       0x9b, 0xe8, 0xc5, 0xbc, 0x54, 0x60, 0xb9, 0x38, 0x99, 0xa0,
				       0x1a, 0xdd, 0xa7, 0xd3, 0x87, 0x9f, 0xc7, 0x0a, 0xd8, 0xbf,
				       0x53, 0x28, 0xfa, 0x64, 0xea, 0x44, 0xaf, 0xb5, 0xbb, 0x92};

	psa_status_t status =
		suit_aes_key_unwrap_manual(fixture->key_id, wrapped_cek, 256, PSA_KEY_TYPE_AES,
					   PSA_ALG_GCM, &unwrapped_cek_key_id);

	zassert_equal(status, PSA_SUCCESS, "Failed to unwrap CEK");
}

ZTEST_F(suit_decrypt_filter_tests, test_filter_smoke)
{
	struct stream_sink dec_sink;
	struct stream_sink digest_sink;
	const uint8_t wrapped_cek[] = {0x50, 0x0A, 0xC9, 0x37, 0x2F, 0xA0, 0x34, 0x14, 0x8D, 0xB3,
				       0xE6, 0x59, 0x50, 0xED, 0x37, 0xE4, 0x76, 0xBE, 0x30, 0x18,
				       0x58, 0x81, 0xEA, 0xFA, 0xE5, 0x8A, 0xD1, 0x44, 0x1E, 0xD1,
				       0xAB, 0x3C, 0x6E, 0xBD, 0x31, 0xDD, 0x33, 0x61, 0x13, 0x49};
	struct suit_encryption_info enc_info = {
		.enc_alg_id = suit_cose_aes256_gcm,
		.IV = {
				.value = iv_aes256_gcm,
				.len = sizeof(iv_aes256_gcm),
			},
		.aad = {
				.value = suit_aad_aes256_gcm,
				.len = sizeof(suit_aad_aes256_gcm),
			},
		.kw_alg_id = suit_cose_aes256_kw,
		.kw_key.aes = {.key_id = {.value = kek_key_id_cbor, .len = sizeof(kek_key_id_cbor)},
			       .ciphertext = {
						.value = wrapped_cek,
						.len = sizeof(wrapped_cek),
					}},
	};

	suit_plat_err_t err = suit_digest_sink_get(&digest_sink, valid_algorithm, valid_digest);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unable to create digest sink");

	err = suit_decrypt_filter_get(&dec_sink, &enc_info, &digest_sink);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to create decrypt filter");

	err = suit_memptr_streamer_stream(ciphertext, sizeof(ciphertext), &dec_sink);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to decrypt ciphertext");

	err = dec_sink.flush(dec_sink.ctx);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to flush decrypt filter");

	err = suit_digest_sink_digest_match(digest_sink.ctx);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Decrypted content digest mismatch");

	err = dec_sink.release(dec_sink.ctx);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to release decrypt filter");
}
