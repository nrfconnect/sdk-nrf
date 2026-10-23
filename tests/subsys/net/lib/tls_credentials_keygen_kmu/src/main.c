/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/tls_credentials_keygen.h>

#include <psa/crypto.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_csr.h>
#include <cracen_psa_kmu.h>

#define TEST_SEC_TAG   42
#define TEST_SEC_TAG_2 43
#define TEST_SEC_TAG_3 44

#define P256 TLS_CREDENTIAL_KEYGEN_ECDSA_SECP256R1
#define P384 TLS_CREDENTIAL_KEYGEN_ECDSA_SECP384R1

#if defined(CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SCHEME_ENCRYPTED)
#define SCHEME      CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED
#define BLOCK_SLOTS (IS_ENABLED(CONFIG_TLS_CREDENTIALS_KEYGEN_SECP384R1) ? 5 : 4)
#else
#define SCHEME      CRACEN_KMU_KEY_USAGE_SCHEME_RAW
#define BLOCK_SLOTS (IS_ENABLED(CONFIG_TLS_CREDENTIALS_KEYGEN_SECP384R1) ? 3 : 2)
#endif

#define SLOT_FIRST CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SLOT_FIRST
#define KEY_COUNT  CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_KEY_COUNT

static const sec_tag_t test_tags[] = {TEST_SEC_TAG, TEST_SEC_TAG_2, TEST_SEC_TAG_3};

static psa_key_id_t window_key_id(size_t index)
{
	return PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(SCHEME, SLOT_FIRST + index * BLOCK_SLOTS);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_equal(psa_crypto_init(), PSA_SUCCESS);

	for (size_t i = 0; i < ARRAY_SIZE(test_tags); i++) {
		(void)tls_credential_delete(test_tags[i], TLS_CREDENTIAL_PRIVATE_KEY_PSA);
	}

	/* Only the slots of the key store are touched. */
	for (size_t i = 0; i < KEY_COUNT; i++) {
		(void)psa_destroy_key(window_key_id(i));
	}
}

static psa_key_id_t key_id_get(sec_tag_t tag)
{
	psa_key_id_t id = PSA_KEY_ID_NULL;

	zassert_ok(tls_credential_keygen_key_id_get(tag, &id));

	return id;
}

/* Generate a CSR and verify its signature with the exported public key. */
static void csr_check(sec_tag_t tag, mbedtls_md_type_t md)
{
	static uint8_t der[512];
	uint8_t pub[TLS_CREDENTIAL_KEYGEN_PUBKEY_MAX_SIZE];
	uint8_t hash[PSA_HASH_MAX_SIZE];
	mbedtls_x509_csr csr;
	mbedtls_pk_context pk;
	size_t hash_len;
	size_t len;

	int err = tls_credential_keygen_csr(tag, "CN=test", der, sizeof(der), &len);

	zassert_equal(err, 0, "CSR failed: %d", err);

	mbedtls_x509_csr_init(&csr);
	zassert_equal(mbedtls_x509_csr_parse_der(&csr, der, len), 0, "CSR does not parse");
	zassert_equal(csr.MBEDTLS_PRIVATE(sig_md), md, "unexpected CSR signature hash");
	zassert_equal(psa_hash_compute(md == MBEDTLS_MD_SHA384 ? PSA_ALG_SHA_384 : PSA_ALG_SHA_256,
				       csr.cri.p, csr.cri.len, hash, sizeof(hash), &hash_len),
		      PSA_SUCCESS);

	zassert_ok(tls_credential_keygen_pubkey_get(tag, pub, sizeof(pub), &len));
	mbedtls_pk_init(&pk);
	zassert_equal(mbedtls_pk_parse_public_key(&pk, pub, len), 0, "public key does not parse");
	zassert_equal(mbedtls_pk_verify(&pk, md, hash, hash_len, csr.MBEDTLS_PRIVATE(sig).p,
					csr.MBEDTLS_PRIVATE(sig).len),
		      0, "CSR signature does not verify with the exported public key");
	mbedtls_pk_free(&pk);
	mbedtls_x509_csr_free(&csr);
}

ZTEST_SUITE(tls_credentials_keygen_kmu, NULL, NULL, before, NULL, NULL);

ZTEST(tls_credentials_keygen_kmu, test_key_in_kmu)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t id;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	id = key_id_get(TEST_SEC_TAG);

	zassert_equal(id, window_key_id(0), "unexpected key ID 0x%08x", id);
	zassert_equal(psa_get_key_attributes(id, &attr), PSA_SUCCESS);
	zassert_equal(PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(&attr)),
		      PSA_KEY_LOCATION_CRACEN_KMU, "key must be in the KMU");
	zassert_equal(psa_get_key_algorithm(&attr), PSA_ALG_ECDSA(PSA_ALG_ANY_HASH));
	psa_reset_key_attributes(&attr);
}

ZTEST(tls_credentials_keygen_kmu, test_private_key_not_exportable)
{
	uint8_t buf[PSA_EXPORT_KEY_PAIR_MAX_SIZE];
	size_t len;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	zassert_not_equal(psa_export_key(key_id_get(TEST_SEC_TAG), buf, sizeof(buf), &len),
			  PSA_SUCCESS, "the private key must not be exportable");
}

ZTEST(tls_credentials_keygen_kmu, test_csr_p256)
{
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	csr_check(TEST_SEC_TAG, MBEDTLS_MD_SHA256);
}

ZTEST(tls_credentials_keygen_kmu, test_csr_p384)
{
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P384));
	csr_check(TEST_SEC_TAG, MBEDTLS_MD_SHA384);
}

ZTEST(tls_credentials_keygen_kmu, test_jwt_sign_and_verify)
{
	static const uint8_t msg[] = "header.payload";
	uint8_t sig[PSA_SIGNATURE_MAX_SIZE];
	psa_key_id_t id;
	size_t sig_len;

	/* A JWT signer signs the message and checks the signature. */
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	id = key_id_get(TEST_SEC_TAG);

	zassert_equal(psa_sign_message(id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), msg, sizeof(msg), sig,
				       sizeof(sig), &sig_len),
		      PSA_SUCCESS);
	zassert_equal(psa_verify_message(id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), msg, sizeof(msg), sig,
					 sig_len),
		      PSA_SUCCESS);
}

ZTEST(tls_credentials_keygen_kmu, test_window_full)
{
	BUILD_ASSERT(KEY_COUNT == 2, "the test expects a window of two keys");

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	int err = tls_credential_keygen_generate(TEST_SEC_TAG_2, P384);

	zassert_equal(err, 0, "second key failed: %d", err);
	zassert_equal(key_id_get(TEST_SEC_TAG_2), window_key_id(1));
	zassert_equal(tls_credential_keygen_generate(TEST_SEC_TAG_3, P256), -ENOSPC,
		      "a full window must be reported");
}

ZTEST(tls_credentials_keygen_kmu, test_delete_and_reuse)
{
	psa_key_id_t id;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	zassert_ok(tls_credential_keygen_delete(TEST_SEC_TAG));
	zassert_equal(tls_credential_keygen_key_id_get(TEST_SEC_TAG, &id), -ENOENT);

	/* The slots are free again. */
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG_2, P256));
	zassert_equal(key_id_get(TEST_SEC_TAG_2), window_key_id(0));
}

ZTEST(tls_credentials_keygen_kmu, test_unregistered_key_destroyed)
{
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));

	/* Remove only the registration, so the key is left over in the KMU. */
	zassert_ok(tls_credential_delete(TEST_SEC_TAG, TLS_CREDENTIAL_PRIVATE_KEY_PSA));

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG_2, P256));
	zassert_equal(key_id_get(TEST_SEC_TAG_2), window_key_id(0),
		      "the slots of the left over key must be used again");
}

ZTEST(tls_credentials_keygen_kmu, test_foreign_kmu_key_rejected)
{
	psa_key_id_t foreign = PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(SCHEME, SLOT_FIRST + 1);
	psa_key_id_t id;

	/* A registration that points into a block, not to the first slot of a key. */
	zassert_ok(tls_credential_add(TEST_SEC_TAG, TLS_CREDENTIAL_PRIVATE_KEY_PSA, &foreign,
				      sizeof(foreign)));
	zassert_equal(tls_credential_keygen_key_id_get(TEST_SEC_TAG, &id), -EPERM);
	zassert_equal(tls_credential_keygen_delete(TEST_SEC_TAG), -EPERM);
}
