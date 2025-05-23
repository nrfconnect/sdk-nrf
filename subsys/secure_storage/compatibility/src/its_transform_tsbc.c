/* Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Trusted Storage Backward Compatible (TSBC) ITS transform module implementation
 *
 * Heavily based on Zephyr's AEAD implementation of the ITS transform module.
 * Made to fit the secure storage subsystem's API and use the trusted storage library's format
 * for stored entries in order to achieve compatibility.
 */

#include <zephyr/secure_storage/its/transform.h>
#include <mbedtls/platform_util.h>
#include <../library/psa_crypto_driver_wrappers.h>

BUILD_ASSERT(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_OUTPUT_OVERHEAD ==
	     sizeof(uint32_t) - sizeof(secure_storage_packed_create_flags_t) + sizeof(size_t) + 28);

BUILD_ASSERT(sizeof(psa_storage_uid_t) == sizeof(uint64_t));

enum {
	AEAD_KEY_SIZE = 32,
	AEAD_NONCE_SIZE = PSA_AEAD_NONCE_LENGTH(PSA_KEY_TYPE_CHACHA20, PSA_ALG_CHACHA20_POLY1305),
	CIPHERTEXT_MAX_SIZE = PSA_AEAD_ENCRYPT_OUTPUT_SIZE(PSA_KEY_TYPE_CHACHA20,
							   PSA_ALG_CHACHA20_POLY1305,
							   CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE),
};

static psa_status_t get_nonce(uint8_t nonce[static AEAD_NONCE_SIZE])
{
	psa_status_t ret;
	static bool s_nonce_initialized;
	static struct {
		uint64_t low;
		uint32_t high;
	} __packed s_nonce;
	BUILD_ASSERT(sizeof(s_nonce) == AEAD_NONCE_SIZE);

	if (!s_nonce_initialized) {
		ret = psa_generate_random((uint8_t *)&s_nonce, sizeof(s_nonce));
		if (ret != PSA_SUCCESS) {
			return ret;
		}
		s_nonce_initialized = true;
	} else {
		++s_nonce.low;
		if (s_nonce.low == 0) {
			++s_nonce.high;
		}
	}
	memcpy(nonce, &s_nonce, AEAD_NONCE_SIZE);
	return PSA_SUCCESS;
}

#if defined(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_TSBC_KEY_PROVIDER_ENTRY_UID_HASH)

static psa_status_t get_key(psa_storage_uid_t uid, uint8_t key[static AEAD_KEY_SIZE])
{
	BUILD_ASSERT(AEAD_KEY_SIZE == PSA_HASH_LENGTH(PSA_ALG_SHA_256));
	size_t hash_length;

	return psa_hash_compute(PSA_ALG_SHA_256, (uint8_t *)&uid, sizeof(uid), key, AEAD_KEY_SIZE,
				&hash_length);
}

#elif defined(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_TSBC_KEY_PROVIDER_HUK_LIBRARY)
#include <hw_unique_key.h>

static psa_status_t get_key(psa_storage_uid_t uid, uint8_t key[static AEAD_KEY_SIZE])
{
	int result;
	enum hw_unique_key_slot key_slot;

#ifdef HUK_HAS_KMU
	key_slot = HUK_KEYSLOT_MKEK;
#else
	key_slot = HUK_KEYSLOT_KDR;
#endif

	result = hw_unique_key_derive_key(key_slot, NULL, 0, (uint8_t *)&uid, sizeof(uid), key,
					  AEAD_KEY_SIZE);
	if (result != HW_UNIQUE_KEY_SUCCESS) {
		return PSA_ERROR_BAD_STATE;
	}

	return PSA_SUCCESS;
}

#endif /* CONFIG_SECURE_STORAGE_ITS_TRANSFORM_TSBC_KEY_PROVIDER */

struct stored_entry_header {
	psa_storage_create_flags_t create_flags;
	size_t data_len;
};

struct stored_entry {
	struct stored_entry_header header;
	uint8_t nonce[AEAD_NONCE_SIZE];
	uint8_t ciphertext[CIPHERTEXT_MAX_SIZE];
} __packed;
BUILD_ASSERT(sizeof(struct stored_entry) == SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE);

/** @return The length of a `struct stored_entry` whose `ciphertext` is `len` bytes long. */
#define STORED_ENTRY_LEN(len) (sizeof(struct stored_entry) - CIPHERTEXT_MAX_SIZE + len)

static psa_status_t crypt(psa_key_usage_t operation, psa_storage_uid_t uid,
			  const struct stored_entry *stored_entry,
			  size_t input_len, const void *input,
			  size_t output_size, void *output, size_t *output_len)
{
	psa_status_t ret;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t key[AEAD_KEY_SIZE];
	psa_status_t (*aead_crypt)(const psa_key_attributes_t *attributes, const uint8_t *key,
				   size_t key_size, psa_algorithm_t alg, const uint8_t *nonce,
				   size_t nonce_length, const uint8_t *add_data,
				   size_t add_data_len, const uint8_t *input, size_t input_len,
				   uint8_t *output, size_t output_size, size_t *output_len);

	psa_set_key_usage_flags(&key_attributes, operation);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_CHACHA20);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CHACHA20_POLY1305);
	psa_set_key_bits(&key_attributes, PSA_BYTES_TO_BITS(AEAD_KEY_SIZE));

	ret = get_key(uid, key);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	/* Avoid calling psa_aead_*crypt() because that would require importing keys into
	 * PSA Crypto. This gets called from PSA Crypto for storing persistent keys so,
	 * even if using PSA_KEY_LIFETIME_VOLATILE, it would corrupt the global key store
	 * which holds all the active keys in the PSA Crypto core.
	 */
	aead_crypt = (operation == PSA_KEY_USAGE_ENCRYPT) ? psa_driver_wrapper_aead_encrypt :
							    psa_driver_wrapper_aead_decrypt;

	ret = aead_crypt(&key_attributes, key, AEAD_KEY_SIZE, PSA_ALG_CHACHA20_POLY1305,
			 stored_entry->nonce, AEAD_NONCE_SIZE,
			 (uint8_t *)&stored_entry->header, sizeof(stored_entry->header),
			 input, input_len, output, output_size, output_len);

	mbedtls_platform_zeroize(key, AEAD_KEY_SIZE);
	return ret;
}

psa_status_t secure_storage_its_transform_to_store(
		secure_storage_its_uid_t uid, size_t data_len, const void *data,
		secure_storage_packed_create_flags_t create_flags,
		uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len)
{
	psa_status_t ret;
	struct stored_entry *stored_entry = (struct stored_entry *)stored_data;
	size_t ciphertext_len;

	stored_entry->header.create_flags = create_flags;
	stored_entry->header.data_len = data_len;

	ret = get_nonce(stored_entry->nonce);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	ret = crypt(PSA_KEY_USAGE_ENCRYPT, uid.uid, stored_entry, data_len, data,
		    CIPHERTEXT_MAX_SIZE, stored_entry->ciphertext, &ciphertext_len);
	if (ret == PSA_SUCCESS) {
		*stored_data_len = STORED_ENTRY_LEN(ciphertext_len);
	}
	return ret;
}

psa_status_t secure_storage_its_transform_from_store(
		secure_storage_its_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t data_size, void *data, size_t *data_len,
		psa_storage_create_flags_t *create_flags)
{
	if (stored_data_len < STORED_ENTRY_LEN(0)) {
		return PSA_ERROR_DATA_CORRUPT;
	}

	psa_status_t ret;
	struct stored_entry *stored_entry = (struct stored_entry *)stored_data;
	const size_t ciphertext_len = stored_data_len - STORED_ENTRY_LEN(0);

	ret = crypt(PSA_KEY_USAGE_DECRYPT, uid.uid, stored_entry,
		    ciphertext_len, stored_entry->ciphertext, data_size, data, data_len);
	if (ret == PSA_SUCCESS) {
		*create_flags = stored_entry->header.create_flags;
	}
	return ret;
}
