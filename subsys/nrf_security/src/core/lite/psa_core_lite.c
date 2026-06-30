/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "psa_core_lite.h"
#include <zephyr/sys/util.h>
#include <psa/crypto.h>
#include <psa_crypto_driver_wrappers.h>
#include <nrf_security_mem_helpers.h>

#if CONFIG_PSA_CORE_LITE_HAS_VOLATILE_KEY_STORAGE
#include "psa_core_lite_volatile_key_storage.h"
#endif

#if PSA_NEED_CRACEN_KMU_DRIVER
#include <cracen_psa_kmu.h>
#include <internal/ecc/cracen_eddsa.h>
#endif

/* Needed for AES-KW (RFC3394) */
#define PSA_CORE_LITE_KEY_WRAP_BLOCK_SIZE			8u
#define PSA_CORE_LITE_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE		PSA_CORE_LITE_KEY_WRAP_BLOCK_SIZE

#define PSA_CORE_LITE_KEY_DERIVATION_OUTPUT			-1

#if CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS
#include "cracen_psa.h"
psa_status_t silex_statuscodes_to_psa(int ret);
#endif

#if PSA_NEED_CRACEN_KMU_DRIVER
psa_status_t cracen_kmu_get_builtin_key(psa_drv_slot_number_t slot_number,
	psa_key_attributes_t *attributes, uint8_t *key_buffer,
	size_t key_buffer_size, size_t *key_buffer_length);
#endif

static psa_status_t get_key_buffer(mbedtls_svc_key_id_t key_id,
				   psa_core_lite_key_slot_t *key_slot,
				   size_t key_buffer_size)
{
	psa_status_t status;

#if CONFIG_PSA_CORE_LITE_HAS_VOLATILE_KEY_STORAGE
	psa_core_lite_key_slot_t *temp_slot;

	if (psa_core_lite_key_id_is_volatile(key_id)) {
		/* The key is an RSA key imported and stored in volatile key store */
		status = psa_core_lite_get_key_slot(&key_id, &temp_slot);
		if (status != PSA_SUCCESS) {
			return status;
		}

		memcpy(key_slot, temp_slot, sizeof(psa_core_lite_key_slot_t));

		return PSA_SUCCESS;
	}
#endif /* CONFIG_PSA_CORE_LITE_HAS_VOLATILE_KEY_STORAGE */

#if PSA_NEED_CRACEN_KMU_DRIVER
	psa_key_lifetime_t lifetime;
	psa_drv_slot_number_t slot_number;
	status = cracen_kmu_get_key_slot(key_id, &lifetime, &slot_number);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/** Note: it is not possible to use sizeof(key_slot->key) here since
	 *  buffer size can be larger than KMU push area size for public keys,
	 *  so using it for private keys leads to PSA_ERROR_BUFFER_TOO_SMALL error.
	 */
	return cracen_kmu_get_builtin_key(slot_number, &key_slot->key_attributes,
					  key_slot->key, key_buffer_size,
					  &key_slot->key_size);
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */

	return PSA_ERROR_DOES_NOT_EXIST;
}

#if CONFIG_PSA_CORE_LITE_HAS_VOLATILE_KEY_STORAGE
static inline void clear_all_volatile_keys(void)
{
	psa_core_lite_free_all_key_slots();
}
#endif

/* Signature validation algorithms */

#if CONFIG_PSA_CORE_LITE_HAS_VERIFY_HASH

psa_status_t psa_verify_hash(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
	const uint8_t *signature, size_t signature_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_core_lite_key_slot_t pub_key_slot = {0};

	if (hash == NULL || signature == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!UTIL_CONCAT_OR(
		VERIFY_ALG_ED25519PH(alg),
		VERIFY_ALG_ECDSA_SECP_R1_256(alg),
		VERIFY_ALG_ECDSA_SECP_R1_384(alg),
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_256(alg),
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_384(alg),
		VERIFY_ALG_RSA_PSS(alg),
		VERIFY_ALG_RSA_PKCS1V15(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = get_key_buffer(key_id, &pub_key_slot, CONFIG_PSA_CORE_LITE_PUB_KEY_MAX_SIZE);
	if (status != PSA_SUCCESS) {
		return status;
	}

#if CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS
	int cracen_status = cracen_ed25519ph_verify(pub_key_slot.key, hash, hash_length,
						    signature, false);

	return silex_statuscodes_to_psa(cracen_status);
#else
	return psa_driver_wrapper_verify_hash_with_context(&pub_key_slot.key_attributes,
							   pub_key_slot.key,
							   pub_key_slot.key_size, alg,
							   hash, hash_length, NULL, 0, signature,
							   signature_length);
#endif /* CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS */
}

#endif /* CONFIG_PSA_CORE_LITE_HAS_VERIFY_HASH  */

#if CONFIG_PSA_CORE_LITE_HAS_VERIFY_MESSAGE

psa_status_t psa_verify_message(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg,
	const uint8_t *input, size_t input_length,
	const uint8_t *signature, size_t signature_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_core_lite_key_slot_t pub_key_slot = {0};

	if (input == NULL || signature == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!UTIL_CONCAT_OR(
		VERIFY_ALG_ED25519(alg),
		VERIFY_ALG_ECDSA_SECP_R1_256(alg),
		VERIFY_ALG_ECDSA_SECP_R1_384(alg),
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_256(alg),
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_384(alg),
		VERIFY_ALG_RSA_PSS(alg),
		VERIFY_ALG_RSA_PKCS1V15(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = get_key_buffer(key_id, &pub_key_slot, CONFIG_PSA_CORE_LITE_PUB_KEY_MAX_SIZE);
	if (status != PSA_SUCCESS) {
		return status;
	}
#if CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS
	int cracen_status = cracen_ed25519_verify(pub_key_slot.key, input, input_length, signature);

	return silex_statuscodes_to_psa(cracen_status);
#else
	return psa_driver_wrapper_verify_message_with_context(&pub_key_slot.key_attributes,
							      pub_key_slot.key,
							      pub_key_slot.key_size, alg,
							      input, input_length, NULL, 0,
							      signature, signature_length);
#endif /* CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS */
}

#endif /* CONFIG_PSA_CORE_LITE_HAS_VERIFY_MESSAGE  */

/* Hash algorithms */

#if CONFIG_PSA_CORE_LITE_HAS_HASH

psa_status_t psa_hash_setup(psa_hash_operation_t *operation, psa_algorithm_t alg)
{
	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!UTIL_CONCAT_OR(VERIFY_ALG_SHA_256(alg),
			    VERIFY_ALG_SHA_384(alg),
			    VERIFY_ALG_SHA_512(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return psa_driver_wrapper_hash_setup(operation, alg);
}

psa_status_t psa_hash_update(
	psa_hash_operation_t *operation, const uint8_t *input, size_t input_length)
{
	if (operation == NULL || input == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return psa_driver_wrapper_hash_update(operation, input, input_length);
}

psa_status_t psa_hash_finish(
	psa_hash_operation_t *operation, uint8_t *hash, size_t hash_size, size_t *hash_length)
{
	if (operation == NULL || hash == NULL || hash_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return psa_driver_wrapper_hash_finish(operation, hash, hash_size, hash_length);
}

psa_status_t psa_hash_abort(psa_hash_operation_t *operation)
{
	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return psa_driver_wrapper_hash_abort(operation);
}

psa_status_t psa_hash_compute(
	psa_algorithm_t alg, const uint8_t *input, size_t input_length,
	uint8_t *hash, size_t hash_size, size_t *hash_length)
{
	if (input == NULL || hash_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!UTIL_CONCAT_OR(VERIFY_ALG_SHA_256(alg),
			    VERIFY_ALG_SHA_384(alg),
			    VERIFY_ALG_SHA_512(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return psa_driver_wrapper_hash_compute(alg, input, input_length,
					       hash, hash_size, hash_length);
}

#endif /* PSA_CORE_LITE_HAS_HASH */

/* Encryption algorithms */

#if PSA_WANT_ALG_CTR

static psa_status_t get_enc_key(mbedtls_svc_key_id_t key_id,
				     psa_algorithm_t alg,
				     psa_core_lite_key_slot_t **key_slot)
{
	if (!VERIFY_ALG_CTR(alg) ||
	    !psa_core_lite_key_id_is_volatile(key_id)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return psa_core_lite_get_key_slot(&key_id, key_slot);
}

psa_status_t psa_cipher_encrypt_setup(
	psa_cipher_operation_t *operation, mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_core_lite_key_slot_t *key_slot;

	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = get_enc_key(key, alg, &key_slot);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
		return status;
	}

	status = psa_driver_wrapper_cipher_encrypt_setup(operation,
							 &key_slot->key_attributes,
							 key_slot->key,
							 key_slot->key_size,
							 alg);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}

	return status;
}

psa_status_t psa_cipher_decrypt_setup(
	psa_cipher_operation_t *operation, mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_core_lite_key_slot_t *key_slot;

	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = get_enc_key(key, alg, &key_slot);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
		return status;
	}

	status = psa_driver_wrapper_cipher_decrypt_setup(operation,
							 &key_slot->key_attributes,
							 key_slot->key,
							 key_slot->key_size,
							 alg);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}

	return status;
}

psa_status_t psa_cipher_set_iv(
	psa_cipher_operation_t *operation, const uint8_t *iv, size_t iv_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

	if (operation == NULL || iv == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_driver_wrapper_cipher_set_iv(operation, iv, iv_length);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}

	return status;
}

psa_status_t psa_cipher_update(
	psa_cipher_operation_t *operation, const uint8_t *input, size_t input_length,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

	if (operation == NULL || input == NULL || output == NULL || output_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_driver_wrapper_cipher_update(operation, input, input_length,
						  output, output_size, output_length);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}

	return status;
}

psa_status_t psa_cipher_finish(
	psa_cipher_operation_t *operation,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_status_t abort_status = PSA_ERROR_NOT_SUPPORTED;

	if (operation ==  NULL || output == NULL || output_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_driver_wrapper_cipher_finish(operation, output, output_size, output_length);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}

	/** Calling psa_cipher_abort() here to make it possible to use another cipher operation
	 *  after this finalization call.
	 *  This approach follows the one used in PSA Core Oberon.
	 */
	abort_status = psa_cipher_abort(operation);
	if (abort_status != PSA_SUCCESS) {
		status = abort_status;
	}

	return status;
}

psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_driver_wrapper_cipher_abort(operation);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}

	return status;
}

#endif /* PSA_WANT_ALG_CTR */

#if CONFIG_PSA_CORE_LITE_HAS_RSA_DECRYPT

psa_status_t psa_asymmetric_decrypt(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg,
	const uint8_t *input, size_t input_length, const uint8_t *salt, size_t salt_length,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_core_lite_key_slot_t key_slot = {0};

	if (input == NULL || output == NULL || output_length == NULL ||
	    (salt == NULL && salt_length != 0)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!VERIFY_ALG_RSA_OAEP(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = get_key_buffer(key_id, &key_slot, CONFIG_PSA_CORE_LITE_PRIV_KEY_MAX_SIZE);
	if (status != PSA_SUCCESS) {
		safe_memzero(&key_slot, sizeof(key_slot));
		clear_all_volatile_keys();
		return status;
	}

	status = psa_driver_wrapper_asymmetric_decrypt(&key_slot.key_attributes, key_slot.key,
						       key_slot.key_size, alg,
						       input, input_length, salt, salt_length,
						       output, output_size, output_length);
	safe_memzero(&key_slot, sizeof(key_slot));
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}

	return status;
}

#endif /* CONFIG_PSA_CORE_LITE_HAS_RSA_DECRYPT */

/* Random function */

#if PSA_WANT_GENERATE_RANDOM

psa_status_t psa_generate_random(uint8_t *output, size_t output_size)
{
	psa_driver_random_context_t rng_ctx = {0};

	if (output == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return psa_driver_wrapper_get_random(&rng_ctx, output, output_size);
}

#endif /* PSA_WANT_GENERATE_RANDOM */

/* Key management */

psa_status_t psa_get_key_attributes(
	mbedtls_svc_key_id_t key_id, psa_key_attributes_t *attributes)
{
	psa_status_t status;

#if CONFIG_PSA_CORE_LITE_HAS_VOLATILE_KEY_STORAGE
	if (psa_core_lite_key_id_is_volatile(key_id)) {
		psa_core_lite_key_slot_t *key_slot;

		status = psa_core_lite_get_key_slot(&key_id, &key_slot);
		if (status != PSA_SUCCESS) {
			return status;
		}

		memcpy(attributes, &key_slot->key_attributes, sizeof(psa_key_attributes_t));
		return PSA_SUCCESS;
	}
#endif

#if PSA_NEED_CRACEN_KMU_DRIVER
	psa_key_lifetime_t lifetime;
	psa_drv_slot_number_t slot_number;
	status = cracen_kmu_get_key_slot(key_id, &lifetime, &slot_number);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* We know the key exists due to cracen_kmu_get_key_slot, feeding
	 * zero for buffers to retrieve only the attributes
	 */
	status = cracen_kmu_get_builtin_key(slot_number, attributes, NULL, 0, NULL);
	psa_set_key_id(attributes, key_id);

	if (status == PSA_ERROR_BUFFER_TOO_SMALL) {
		return PSA_SUCCESS;
	}

	return status;
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */

	return PSA_ERROR_NOT_SUPPORTED;
}

void psa_reset_key_attributes(psa_key_attributes_t *attributes)
{
	safe_memzero(attributes, sizeof(psa_key_attributes_t));
}

#if CONFIG_PSA_CORE_LITE_HAS_RSA

psa_status_t psa_import_key(const psa_key_attributes_t *attributes,
				const uint8_t *data, size_t data_length,
				mbedtls_svc_key_id_t *key)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_core_lite_key_slot_t *key_slot;
	psa_algorithm_t alg = psa_get_key_algorithm(attributes);
	*key = MBEDTLS_SVC_KEY_ID_INIT;

	/* Key import is limited to RSA keys which can't be stored in KMU */
	if (!UTIL_CONCAT_OR(VERIFY_ALG_RSA_PSS(alg),
			    VERIFY_ALG_RSA_PKCS1V15(alg),
			    VERIFY_ALG_RSA_OAEP(alg),
			    VERIFY_ALG_CTR(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = psa_core_lite_get_key_slot(key, &key_slot);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
		return status;
	}

	memcpy(&key_slot->key_attributes, attributes, sizeof(psa_key_attributes_t));
	memcpy(&key_slot->key, data, data_length);
	key_slot->key_size = data_length;

	return PSA_SUCCESS;
}

#endif /* CONFIG_PSA_CORE_LITE_HAS_RSA */

psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key_id)
{
#if CONFIG_PSA_CORE_LITE_HAS_VOLATILE_KEY_STORAGE
	if (psa_core_lite_key_id_is_volatile(key_id)) {
		psa_core_lite_free_key_slot(key_id);
		return PSA_SUCCESS;
	}
#endif /* CONFIG_PSA_CORE_LITE_HAS_VOLATILE_KEY_STORAGE */

#if PSA_NEED_CRACEN_KMU_DRIVER
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_key_attributes_t attr;
	status = psa_get_key_attributes(key_id, &attr);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return psa_driver_wrapper_destroy_builtin_key(&attr);
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_lock_key(mbedtls_svc_key_id_t key_id)
{
#if PSA_NEED_CRACEN_KMU_DRIVER
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_key_attributes_t attr;
	status = psa_get_key_attributes(key_id, &attr);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Generalize to psa_driver_wrapper_lock_key */
	return cracen_kmu_block(&attr);
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_purge_key(mbedtls_svc_key_id_t key_id)
{
	(void) key_id;
	/* Do nothing */
	return PSA_SUCCESS;
}

/* Key wrapping */

#if PSA_WANT_ALG_AES_KW

psa_status_t psa_unwrap_key(const psa_key_attributes_t *attributes,
			    mbedtls_svc_key_id_t wrapping_key,
			    psa_algorithm_t alg,
			    const uint8_t *data,
			    size_t data_length,
			    mbedtls_svc_key_id_t *key)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_core_lite_key_slot_t wrapping_key_slot;
	psa_core_lite_key_slot_t *key_slot;
	size_t storage_size;
	size_t required_key_size_bits;

	if (attributes == NULL || key == NULL || data == NULL || alg != PSA_ALG_KW ||
	    data_length < PSA_CORE_LITE_KEY_WRAP_BLOCK_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	storage_size = data_length - PSA_CORE_LITE_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE;
	if (storage_size > sizeof(wrapping_key_slot.key)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*key = MBEDTLS_SVC_KEY_ID_INIT;

	status = get_key_buffer(wrapping_key, &wrapping_key_slot,
			     CONFIG_PSA_CORE_LITE_AES_KEY_MAX_SIZE);
	if (status != PSA_SUCCESS) {
		safe_memzero(&wrapping_key_slot, sizeof(wrapping_key_slot));
		return status;
	}

	status = psa_core_lite_get_key_slot(key, &key_slot);
	if (status != PSA_SUCCESS) {
		safe_memzero(&wrapping_key_slot, sizeof(wrapping_key_slot));
		goto error;
	}

	status = psa_driver_wrapper_unwrap_key(attributes, &wrapping_key_slot.key_attributes,
					       wrapping_key_slot.key,
					       wrapping_key_slot.key_size, alg, data, data_length,
					       key_slot->key,
					       sizeof(key_slot->key),
					       &key_slot->key_size);

	safe_memzero(&wrapping_key_slot, sizeof(wrapping_key_slot));
	if (status != PSA_SUCCESS) {
		goto error;
	}

	key_slot->key_attributes = *attributes;
	required_key_size_bits = psa_get_key_bits(attributes);
	if (required_key_size_bits == 0) {
		psa_set_key_bits(&key_slot->key_attributes,
				 PSA_BYTES_TO_BITS(key_slot->key_size));
	} else if (required_key_size_bits != PSA_BYTES_TO_BITS(key_slot->key_size)) {
		status = PSA_ERROR_INVALID_ARGUMENT;
		goto error;
	} else {
		/* For compliance. Nothing to do here. */
	}

	return status;

error:
	psa_core_lite_free_key_slot(*key);
	return status;
}

#endif /* PSA_WANT_ALG_AES_KW */

/* Key derivation */

#if PSA_WANT_ALG_HKDF

psa_status_t psa_key_derivation_setup(psa_key_derivation_operation_t *operation,
				      psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_algorithm_t kdf_alg;

	if (operation == NULL ||
	    PSA_ALG_IS_RAW_KEY_AGREEMENT(alg)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (PSA_ALG_IS_KEY_AGREEMENT(alg)) {
		kdf_alg = PSA_ALG_KEY_AGREEMENT_GET_KDF(alg);
	} else if (PSA_ALG_IS_KEY_DERIVATION(alg)) {
		/* Pure key derivation algorithm (without key agreement is not supported) */
		return PSA_ERROR_NOT_SUPPORTED;
	} else {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_driver_wrapper_key_derivation_setup(operation, kdf_alg);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Note: the following algorithm will be used later (by psa_key_derivation_key_agreement) */
	operation->MBEDTLS_PRIVATE(alg) = alg;

	if (VERIFY_ALG_HKDF(kdf_alg)) {
		/* length of output keying material in octets (RFC5869) */
		operation->MBEDTLS_PRIVATE(capacity) = 255 * PSA_HASH_LENGTH(kdf_alg);
	} else {
		operation->MBEDTLS_PRIVATE(capacity) = PSA_KEY_DERIVATION_UNLIMITED_CAPACITY;
	}

	return PSA_SUCCESS;
}

static psa_status_t psa_key_derivation_check_state(psa_key_derivation_operation_t *operation,
						   int step)
{
	psa_algorithm_t alg = operation->MBEDTLS_PRIVATE(alg);

	if (!PSA_ALG_IS_KEY_AGREEMENT(alg) &&
	    !PSA_ALG_IS_HKDF(PSA_ALG_KEY_AGREEMENT_GET_KDF(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_SECRET:
		if (operation->MBEDTLS_PRIVATE(secret_set)) {
			return PSA_ERROR_BAD_STATE;
		}
		operation->MBEDTLS_PRIVATE(secret_set) = 1;
		break;

	case PSA_KEY_DERIVATION_INPUT_INFO:
		if (operation->MBEDTLS_PRIVATE(info_set)) {
			return PSA_ERROR_BAD_STATE;
		}
		operation->MBEDTLS_PRIVATE(info_set) = 1;
		break;

	case PSA_CORE_LITE_KEY_DERIVATION_OUTPUT:
		if (!operation->MBEDTLS_PRIVATE(secret_set) ||
		    !operation->MBEDTLS_PRIVATE(info_set)) {
			return PSA_ERROR_BAD_STATE;
		}
		operation->MBEDTLS_PRIVATE(no_input) = 1;
		break;

	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static inline int psa_key_lifetime_is_external(psa_key_lifetime_t lifetime)
{
	return PSA_KEY_LIFETIME_GET_LOCATION(lifetime) != PSA_KEY_LOCATION_LOCAL_STORAGE;
}

psa_status_t psa_key_derivation_key_agreement(psa_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step,
					      mbedtls_svc_key_id_t private_key,
					      const uint8_t *peer_key,
					      size_t peer_key_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_core_lite_key_slot_t key_slot;
	psa_algorithm_t ka_alg;
	uint8_t shared_secret[PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE] = {};
	size_t shared_secret_length = 0;

	if (operation == NULL || peer_key == NULL || step != PSA_KEY_DERIVATION_INPUT_SECRET) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_key_derivation_check_state(operation, step);
	if (status != PSA_SUCCESS) {
		return status;
	}
	ka_alg = PSA_ALG_KEY_AGREEMENT_GET_BASE(operation->MBEDTLS_PRIVATE(alg));

	if (!VERIFY_ALG_ECDH(ka_alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = get_key_buffer(private_key, &key_slot, CONFIG_PSA_CORE_LITE_PRIV_KEY_MAX_SIZE);
	if (status != PSA_SUCCESS) {
		safe_memzero(&key_slot, sizeof(key_slot));
		clear_all_volatile_keys();
		return status;
	}

	/* Step 1: run the secret agreement algorithm to generate the shared secret. */
	status = psa_driver_wrapper_key_agreement(
		&key_slot.key_attributes, key_slot.key, key_slot.key_size,
		ka_alg,
		peer_key, peer_key_length,
		shared_secret, sizeof(shared_secret), &shared_secret_length);

	safe_memzero(&key_slot, sizeof(key_slot));
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
		goto exit;
	}

	/* Step 2: set up the key derivation to generate key material from
	 * the shared secret. A shared secret is permitted wherever a key
	 * of type DERIVE is permitted.
	 */
	status = psa_driver_wrapper_key_derivation_input_bytes(operation, step,
							       shared_secret,
							       shared_secret_length);

	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}
exit:
	safe_memzero(shared_secret, sizeof(shared_secret));
	return status;
}

psa_status_t psa_key_derivation_input_bytes(psa_key_derivation_operation_t *operation,
					    psa_key_derivation_step_t step,
					    const uint8_t *data,
					    size_t data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (step != PSA_KEY_DERIVATION_INPUT_INFO) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_key_derivation_check_state(operation, step);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
		return status;
	}

	status = psa_driver_wrapper_key_derivation_input_bytes(operation, step, data, data_length);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}

	return status;
}

psa_status_t psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
					   psa_key_derivation_operation_t *operation,
					   mbedtls_svc_key_id_t *key)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_core_lite_key_slot_t *key_slot;
	psa_key_type_t key_type;
	size_t key_bits;
	size_t key_bytes;

	if (attributes == NULL || key == NULL || operation == NULL ||
	    psa_get_key_bits(attributes) == 0 ||
	    PSA_KEY_TYPE_IS_PUBLIC_KEY(attributes->MBEDTLS_PRIVATE(type))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	key_bits = psa_get_key_bits(attributes);
	key_bytes = PSA_BITS_TO_BYTES(key_bits);
	key_type = attributes->MBEDTLS_PRIVATE(type);
	*key = MBEDTLS_SVC_KEY_ID_INIT;

	if (PSA_KEY_TYPE_IS_UNSTRUCTURED(key_type) &&
	    !psa_key_lifetime_is_external(attributes->MBEDTLS_PRIVATE(lifetime))) {
		if (key_bits % 8 != 0) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = psa_key_derivation_check_state(operation, PSA_CORE_LITE_KEY_DERIVATION_OUTPUT);
	if (status != PSA_SUCCESS) {
		goto error;
	}

	status = psa_core_lite_get_key_slot(key, &key_slot);
	if (status != PSA_SUCCESS) {
		goto error;
	}

	if (key_bytes <= operation->MBEDTLS_PRIVATE(capacity)) {
		status = psa_driver_wrapper_key_derivation_output_bytes(operation,
									key_slot->key,
									key_bytes);
		operation->MBEDTLS_PRIVATE(capacity) -= key_bytes;
		if (status != PSA_SUCCESS) {
			goto error;
		}
	} else {
		status = PSA_ERROR_INSUFFICIENT_DATA;
		operation->MBEDTLS_PRIVATE(capacity) = 0;
		goto error;
	}

	key_slot->key_attributes = *attributes;
	key_slot->key_size = key_bytes;

	return status;
error:
	clear_all_volatile_keys();
	return status;
}

psa_status_t psa_key_derivation_abort(psa_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_driver_wrapper_key_derivation_abort(operation);
	if (status != PSA_SUCCESS) {
		clear_all_volatile_keys();
	}
	safe_memzero(operation, sizeof(psa_key_derivation_operation_t));

	return status;
}

#endif /* PSA_WANT_ALG_HKDF */

/* MAC */

#if PSA_WANT_ALG_HMAC

static inline psa_status_t get_mac_key(mbedtls_svc_key_id_t key_id,
				       psa_algorithm_t alg,
				       psa_core_lite_key_slot_t **key_slot)
{
	if (!VERIFY_ALG_HMAC(alg) ||
	    !psa_core_lite_key_id_is_volatile(key_id)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return psa_core_lite_get_key_slot(&key_id, key_slot);
}

psa_status_t psa_mac_verify(mbedtls_svc_key_id_t key,
			    psa_algorithm_t alg,
			    const uint8_t *input,
			    size_t input_length,
			    const uint8_t *mac,
			    size_t mac_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_core_lite_key_slot_t *key_slot;
	psa_key_type_t key_type;
	size_t key_bits;
	size_t operation_mac_size;
	uint8_t actual_mac[PSA_MAC_MAX_SIZE];
	size_t actual_mac_length;

	if (input == NULL || mac == NULL || !PSA_ALG_IS_HMAC(alg)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = get_mac_key(key, alg, &key_slot);
	if (status != PSA_SUCCESS) {
		goto error;
	}

	key_type = psa_get_key_type(&key_slot->key_attributes);
	key_bits = psa_get_key_bits(&key_slot->key_attributes);

	if (key_type != PSA_KEY_TYPE_HMAC) {
		status = PSA_ERROR_INVALID_ARGUMENT;
		goto error;
	}
	operation_mac_size = PSA_MAC_LENGTH(key_type, key_bits, alg);

	if (operation_mac_size > PSA_MAC_MAX_SIZE) {
		/*  Note from Oberon PSA Core:
		 *
		 *  PSA_MAC_LENGTH returns the correct length even for a MAC algorithm
		 *  that is disabled in the compile-time configuration. The result can
		 *  therefore be larger than PSA_MAC_MAX_SIZE, which does take the
		 *  configuration into account. In this case, force a return of
		 *  PSA_ERROR_NOT_SUPPORTED here. Otherwise psa_mac_verify(), or
		 *  psa_mac_compute(mac_size=PSA_MAC_MAX_SIZE), would return
		 *  PSA_ERROR_BUFFER_TOO_SMALL for an unsupported algorithm whose MAC size
		 *  is larger than PSA_MAC_MAX_SIZE, which is misleading and which breaks
		 *  systematically generated tests.
		 */
		status = PSA_ERROR_NOT_SUPPORTED;
		goto error;
	}

	status = psa_driver_wrapper_mac_compute(&key_slot->key_attributes,
						key_slot->key, key_slot->key_size,
						alg,
						input, input_length,
						actual_mac, operation_mac_size,
						&actual_mac_length);

	if (status != PSA_SUCCESS) {
		goto error;
	}

	if (mac_length != actual_mac_length) {
		status = PSA_ERROR_INVALID_SIGNATURE;
		goto error;
	}

	if (constant_memcmp(mac, actual_mac, actual_mac_length) != 0) {
		status = PSA_ERROR_INVALID_SIGNATURE;
		goto error;
	}

	safe_memzero(actual_mac, sizeof(actual_mac));
	return status;

error:
	safe_memzero(actual_mac, sizeof(actual_mac));
	clear_all_volatile_keys();
	actual_mac_length = sizeof(actual_mac);
	operation_mac_size = 0;

	return status;
}

#endif /* PSA_WANT_ALG_HMAC */

/* Initialization function */

psa_status_t psa_crypto_init(void)
{
	return psa_driver_wrapper_init();
}
