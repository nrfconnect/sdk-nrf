/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Store AES keys in the KMU with each CCM-family policy and check that they
 * encrypt and decrypt like the same key held in local storage. The KMU key is
 * purged from the key cache before use so its attributes are reloaded from
 * the KMU metadata.
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include "cracen_psa_kmu.h"
#include "psa_tests_common.h"

#define SLOT_CCM	  100
#define SLOT_CCM_STAR	  101
#define SLOT_CCM_STAR_ANY 102

#define CCM_NONCE_SIZE	    13
#define CCM_STAR_IV_SIZE    13
#define CCM_SHORT_TAG_SIZE  4

static const uint8_t key_data[16] = {
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
};
static const uint8_t nonce[CCM_NONCE_SIZE] = {
	0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
};
static const uint8_t ad[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
static const uint8_t msg[] = "CCM and CCM* with a key stored in the KMU";

static psa_key_id_t kmu_key_id(uint32_t slot)
{
	return PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, slot);
}

static psa_key_id_t import_key(psa_algorithm_t alg, uint32_t slot, bool in_kmu)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t id = PSA_KEY_ID_NULL;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 128);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, alg);
	if (in_kmu) {
		psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
						    PSA_KEY_PERSISTENCE_DEFAULT,
						    PSA_KEY_LOCATION_CRACEN_KMU));
		psa_set_key_id(&attr, kmu_key_id(slot));
	}

	zassert_equal(psa_import_key(&attr, key_data, sizeof(key_data), &id), PSA_SUCCESS,
		      "import (kmu=%d, alg=0x%08x)", in_kmu, alg);
	return id;
}

/* Import the key into the KMU and a local reference copy, then drop the KMU key
 * from the key cache and check the policy that is read back from the KMU.
 */
static void setup_keys(psa_algorithm_t alg, psa_algorithm_t loaded_alg, uint32_t slot,
		       psa_key_id_t *kmu, psa_key_id_t *ref)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	zassert_equal(crypto_init(), APP_SUCCESS);

	/* Leftover from an aborted earlier run; the slot is erasable. */
	(void)psa_destroy_key(kmu_key_id(slot));

	*kmu = import_key(alg, slot, true);
	/* Give the reference key the policy the KMU key is reloaded with. */
	*ref = import_key(loaded_alg, 0, false);

	zassert_equal(psa_purge_key(*kmu), PSA_SUCCESS);
	zassert_equal(psa_get_key_attributes(*kmu, &attr), PSA_SUCCESS);
	zassert_equal(psa_get_key_algorithm(&attr), loaded_alg,
		      "KMU policy 0x%08x, expected 0x%08x", psa_get_key_algorithm(&attr),
		      loaded_alg);
	psa_reset_key_attributes(&attr);
}

static void destroy_keys(psa_key_id_t kmu, psa_key_id_t ref)
{
	zassert_equal(psa_destroy_key(kmu), PSA_SUCCESS);
	zassert_equal(psa_destroy_key(ref), PSA_SUCCESS);
}

static void check_aead(psa_key_id_t kmu, psa_key_id_t ref, size_t tag_size)
{
	psa_algorithm_t alg = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, tag_size);
	uint8_t ct[sizeof(msg) + 16];
	uint8_t ct_ref[sizeof(ct)];
	uint8_t pt[sizeof(msg)];
	size_t ct_len, ct_ref_len, pt_len;

	zassert_equal(psa_aead_encrypt(kmu, alg, nonce, sizeof(nonce), ad, sizeof(ad), msg,
				       sizeof(msg), ct, sizeof(ct), &ct_len),
		      PSA_SUCCESS, "KMU CCM encrypt, tag %u", tag_size);
	zassert_equal(ct_len, sizeof(msg) + tag_size);

	zassert_equal(psa_aead_encrypt(ref, alg, nonce, sizeof(nonce), ad, sizeof(ad), msg,
				       sizeof(msg), ct_ref, sizeof(ct_ref), &ct_ref_len),
		      PSA_SUCCESS);
	zassert_mem_equal(ct, ct_ref, ct_len, "KMU and local CCM ciphertext differ");

	zassert_equal(psa_aead_decrypt(kmu, alg, nonce, sizeof(nonce), ad, sizeof(ad), ct,
				       ct_len, pt, sizeof(pt), &pt_len),
		      PSA_SUCCESS, "KMU CCM decrypt, tag %u", tag_size);
	zassert_equal(pt_len, sizeof(msg));
	zassert_mem_equal(pt, msg, sizeof(msg));

	ct[0] ^= 1;
	zassert_equal(psa_aead_decrypt(kmu, alg, nonce, sizeof(nonce), ad, sizeof(ad), ct,
				       ct_len, pt, sizeof(pt), &pt_len),
		      PSA_ERROR_INVALID_SIGNATURE, "tampered CCM ciphertext accepted");
}

static void check_cipher(psa_key_id_t kmu, psa_key_id_t ref)
{
	uint8_t ct[CCM_STAR_IV_SIZE + sizeof(msg)];
	uint8_t pt[sizeof(msg)];
	size_t ct_len, pt_len;

	/* psa_cipher_encrypt() prepends a random IV, so cross-check by decrypting
	 * with the other key in both directions.
	 */
	zassert_equal(psa_cipher_encrypt(kmu, PSA_ALG_CCM_STAR_NO_TAG, msg, sizeof(msg), ct,
					 sizeof(ct), &ct_len),
		      PSA_SUCCESS, "KMU CCM* encrypt");
	zassert_equal(ct_len, sizeof(ct));
	zassert_equal(psa_cipher_decrypt(ref, PSA_ALG_CCM_STAR_NO_TAG, ct, ct_len, pt,
					 sizeof(pt), &pt_len),
		      PSA_SUCCESS);
	zassert_equal(pt_len, sizeof(msg));
	zassert_mem_equal(pt, msg, sizeof(msg), "local key cannot decrypt KMU CCM*");

	zassert_equal(psa_cipher_encrypt(ref, PSA_ALG_CCM_STAR_NO_TAG, msg, sizeof(msg), ct,
					 sizeof(ct), &ct_len),
		      PSA_SUCCESS);
	zassert_equal(psa_cipher_decrypt(kmu, PSA_ALG_CCM_STAR_NO_TAG, ct, ct_len, pt,
					 sizeof(pt), &pt_len),
		      PSA_SUCCESS, "KMU CCM* decrypt");
	zassert_equal(pt_len, sizeof(msg));
	zassert_mem_equal(pt, msg, sizeof(msg), "KMU key cannot decrypt local CCM*");
}

static void check_cipher_not_permitted(psa_key_id_t kmu)
{
	uint8_t ct[CCM_STAR_IV_SIZE + sizeof(msg)];
	size_t ct_len;

	zassert_equal(psa_cipher_encrypt(kmu, PSA_ALG_CCM_STAR_NO_TAG, msg, sizeof(msg), ct,
					 sizeof(ct), &ct_len),
		      PSA_ERROR_NOT_PERMITTED);
}

static void check_aead_not_permitted(psa_key_id_t kmu)
{
	uint8_t ct[sizeof(msg) + 16];
	size_t ct_len;

	zassert_equal(psa_aead_encrypt(kmu, PSA_ALG_CCM, nonce, sizeof(nonce), ad, sizeof(ad),
				       msg, sizeof(msg), ct, sizeof(ct), &ct_len),
		      PSA_ERROR_NOT_PERMITTED);
}

ZTEST(test_suite_ikg, test_kmu_ccm)
{
	psa_key_id_t kmu, ref;

	/* The KMU does not store the tag length, so CCM keys load back as
	 * "at least 4 bytes of tag".
	 */
	setup_keys(PSA_ALG_CCM,
		   PSA_ALG_AEAD_WITH_AT_LEAST_THIS_LENGTH_TAG(PSA_ALG_CCM, CCM_SHORT_TAG_SIZE),
		   SLOT_CCM, &kmu, &ref);
	check_aead(kmu, ref, 16);
	check_aead(kmu, ref, CCM_SHORT_TAG_SIZE);
	check_cipher_not_permitted(kmu);
	destroy_keys(kmu, ref);
}

ZTEST(test_suite_ikg, test_kmu_ccm_star_no_tag)
{
	psa_key_id_t kmu, ref;

	setup_keys(PSA_ALG_CCM_STAR_NO_TAG, PSA_ALG_CCM_STAR_NO_TAG, SLOT_CCM_STAR, &kmu, &ref);
	check_cipher(kmu, ref);
	check_aead_not_permitted(kmu);
	destroy_keys(kmu, ref);
}

ZTEST(test_suite_ikg, test_kmu_ccm_star_any_tag)
{
	psa_key_id_t kmu, ref;

	setup_keys(PSA_ALG_CCM_STAR_ANY_TAG, PSA_ALG_CCM_STAR_ANY_TAG, SLOT_CCM_STAR_ANY, &kmu,
		   &ref);
	check_cipher(kmu, ref);
	check_aead(kmu, ref, 16);
	check_aead(kmu, ref, CCM_SHORT_TAG_SIZE);
	destroy_keys(kmu, ref);
}
