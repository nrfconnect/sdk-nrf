/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "psa_core_lite.h"
#include "psa_core_lite_volatile_key_storage.h"
#include <psa/crypto.h>
#include <psa_crypto_driver_wrappers.h>

#ifdef CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
#include <cracen_psa_kmu.h>
#endif

#ifdef CONFIG_PSA_CRYPTO_DRIVER_CRACEN
#include <cracen/mem_helpers.h>
#include <internal/ecc/cracen_eddsa.h>
#endif

/* Needed for AES-KW (RFC3394) */
#define PSA_CORE_LITE_KEY_WRAP_BLOCK_SIZE			8u
#define PSA_CORE_LITE_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE		PSA_CORE_LITE_KEY_WRAP_BLOCK_SIZE

#if defined(CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS)
#include "cracen_psa.h"
psa_status_t silex_statuscodes_to_psa(int ret);
#endif

#ifdef CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
psa_status_t cracen_kmu_get_builtin_key(psa_drv_slot_number_t slot_number,
	psa_key_attributes_t *attributes, uint8_t *key_buffer,
	size_t key_buffer_size, size_t *key_buffer_length);

static psa_status_t get_kmu_key(mbedtls_svc_key_id_t key_id, psa_lite_key_slot_t *key_slot)
{
	psa_status_t status;
	psa_key_lifetime_t lifetime;
	psa_drv_slot_number_t slot_number;

	status = cracen_kmu_get_key_slot(key_id, &lifetime, &slot_number);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return cracen_kmu_get_builtin_key(slot_number, &key_slot->key_attributes,
					  key_slot->key, sizeof(key_slot->key),
					  &key_slot->key_size);
}
#endif

/* Signature validation algorithms */

#if defined(CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS) && \
	(defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA))
/* We don't support Ed25519 + ECDSA as the only supported verification is
 * Ed25519 when PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS is used
 */
#error PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS with ECDSA is invalid!
#endif

#if defined(CONFIG_PSA_CORE_LITE_HAS_VERIFY_HASH)

psa_status_t psa_verify_hash(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
	const uint8_t *signature, size_t signature_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_lite_key_slot_t pub_key_slot;

	if (hash == NULL || signature == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!UTIL_CONCAT_OR(
		VERIFY_ALG_ED25519PH(alg),
		VERIFY_ALG_ECDSA_SECP_R1_256(alg),
		VERIFY_ALG_ECDSA_SECP_R1_384(alg),
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_256(alg),
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_384(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = get_kmu_key(key_id, &pub_key_slot);
	if (status != PSA_SUCCESS) {
		return status;
	}

#if defined(CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS)
	int cracen_status = cracen_ed25519ph_verify(pub_key_slot.key, hash, hash_length,
						    signature, false);

	return silex_statuscodes_to_psa(cracen_status);
#else
	return psa_driver_wrapper_verify_hash_with_context(&pub_key_slot.key_attributes,
							   pub_key_slot.key,
							   pub_key_slot.key_size, alg,
							   hash, hash_length, NULL, 0, signature,
							   signature_length);
#endif
}

#endif /* CONFIG_PSA_CORE_LITE_HAS_VERIFY_HASH  */

#if defined(CONFIG_PSA_CORE_LITE_HAS_VERIFY_MESSAGE)

psa_status_t psa_verify_message(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg,
	const uint8_t *input, size_t input_length,
	const uint8_t *signature, size_t signature_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_lite_key_slot_t pub_key_slot;

	if (input == NULL || signature == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!UTIL_CONCAT_OR(
		VERIFY_ALG_ED25519(alg),
		VERIFY_ALG_ECDSA_SECP_R1_256(alg),
		VERIFY_ALG_ECDSA_SECP_R1_384(alg),
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_256(alg),
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_384(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = get_kmu_key(key_id, &pub_key_slot);
	if (status != PSA_SUCCESS) {
		return status;
	}
#if defined(CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS)
	int cracen_status = cracen_ed25519_verify(pub_key_slot.key, input, input_length, signature);

	return silex_statuscodes_to_psa(cracen_status);
#else
	return psa_driver_wrapper_verify_message_with_context(&pub_key_slot.key_attributes,
							      pub_key_slot.key,
							      pub_key_slot.key_size, alg,
							      input, input_length, NULL, 0,
							      signature, signature_length);
#endif
}

#endif /* CONFIG_PSA_CORE_LITE_HAS_VERIFY_MESSAGE  */

/* Hash algorithms */

#if defined(PSA_WANT_ALG_SHA_256) || defined(PSA_WANT_ALG_SHA_384) || defined(PSA_WANT_ALG_SHA_512)

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

#endif /* PSA_WANT_ALG_SHA_256 || PSA_WANT_ALG_SHA_384 || PSA_WANT_ALG_SHA_512 */

/* Encryption algorithms */

#if defined(PSA_WANT_ALG_CTR)

static psa_status_t get_enc_key(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg,
	psa_lite_key_slot_t **key_slot)
{
	if (!VERIFY_ALG_CTR(alg) || !psa_lite_key_id_is_volatile(key_id)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return psa_lite_get_key_slot(&key_id, key_slot);
}

static inline void clear_all_enc_keys(void)
{
	psa_lite_free_all_key_slots();
}

psa_status_t psa_cipher_encrypt_setup(
	psa_cipher_operation_t *operation, mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_lite_key_slot_t *key_slot;

	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = get_enc_key(key, alg, &key_slot);
	if (status != PSA_SUCCESS) {
		clear_all_enc_keys();
		return status;
	}

	status = psa_driver_wrapper_cipher_encrypt_setup(operation,
							 &key_slot->key_attributes,
							 key_slot->key,
							 key_slot->key_size,
							 alg);
	if (status != PSA_SUCCESS) {
		clear_all_enc_keys();
	}

	return status;
}

psa_status_t psa_cipher_decrypt_setup(
	psa_cipher_operation_t *operation, mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_lite_key_slot_t *key_slot;

	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = get_enc_key(key, alg, &key_slot);
	if (status != PSA_SUCCESS) {
		clear_all_enc_keys();
		return status;
	}

	status = psa_driver_wrapper_cipher_decrypt_setup(operation,
							 &key_slot->key_attributes,
							 key_slot->key,
							 key_slot->key_size,
							 alg);
	if (status != PSA_SUCCESS) {
		clear_all_enc_keys();
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
		clear_all_enc_keys();
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
		clear_all_enc_keys();
	}

	return status;
}

psa_status_t psa_cipher_finish(
	psa_cipher_operation_t *operation,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

	if (operation ==  NULL || output == NULL || output_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_driver_wrapper_cipher_finish(operation, output, output_size, output_length);
	if (status != PSA_SUCCESS) {
		clear_all_enc_keys();
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
		clear_all_enc_keys();
	}

	return status;
}

#endif /* PSA_WANT_ALG_CTR */

/* Random function */

#if defined(PSA_WANT_GENERATE_RANDOM)

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

#if defined(PSA_NEED_CRACEN_KMU_DRIVER)

psa_status_t psa_get_key_attributes(
	mbedtls_svc_key_id_t key_id, psa_key_attributes_t *attributes)
{

	psa_status_t status;
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
}

psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key_id)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_key_attributes_t attr;

#if defined(CONFIG_PSA_CORE_LITE_HAS_VOLATILE_KEY_STORAGE)
	if (psa_lite_key_id_is_volatile(key_id)) {
		psa_lite_free_key_slot(key_id);
		return PSA_SUCCESS;
	}
#endif /* CONFIG_PSA_CORE_LITE_HAS_VOLATILE_KEY_STORAGE */

	status = psa_get_key_attributes(key_id, &attr);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return psa_driver_wrapper_destroy_builtin_key(&attr);
}

psa_status_t psa_lock_key(mbedtls_svc_key_id_t key_id)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_key_attributes_t attr;

	status = psa_get_key_attributes(key_id, &attr);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Generalize to psa_driver_wrapper_lock_key */
	return cracen_kmu_block(&attr);
}

psa_status_t psa_purge_key(mbedtls_svc_key_id_t key_id)
{
	(void) key_id;
	/* Do nothing */
	return PSA_SUCCESS;
}

#endif /* PSA_NEED_CRACEN_KMU_DRIVER */

/* Key wrapping */

#if defined(PSA_WANT_ALG_AES_KW)

psa_status_t psa_unwrap_key(const psa_key_attributes_t *attributes,
			    mbedtls_svc_key_id_t wrapping_key,
			    psa_algorithm_t alg,
			    const uint8_t *data,
			    size_t data_length,
			    mbedtls_svc_key_id_t *key)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_lite_key_slot_t wrapping_key_slot;
	psa_lite_key_slot_t *key_slot;
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

	status = get_kmu_key(wrapping_key, &wrapping_key_slot);
	if (status != PSA_SUCCESS) {
		safe_memzero(&wrapping_key_slot, sizeof(wrapping_key_slot));
		return status;
	}

	status = psa_lite_get_key_slot(key, &key_slot);
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
	psa_lite_free_key_slot(*key);
	return status;
}

#endif /* PSA_WANT_ALG_AES_KW */

/* Initialization function */

psa_status_t psa_crypto_init(void)
{
	return psa_driver_wrapper_init();
}
