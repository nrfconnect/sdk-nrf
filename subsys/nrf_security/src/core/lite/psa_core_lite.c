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

#if defined(CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS)
#include "cracen_psa.h"
psa_status_t silex_statuscodes_to_psa(int ret);
#endif

#if defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA) || \
	defined(PSA_WANT_ALG_PURE_EDDSA)

#if (defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA)) && \
	defined(PSA_WANT_ECC_SECP_R1_384)
	const size_t pub_key_max_size = 97;
#elif (defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA)) && \
	defined(PSA_WANT_ECC_SECP_R1_256)
	const size_t pub_key_max_size = 65;
#elif (defined(PSA_WANT_ALG_ED25519PH) || defined(PSA_WANT_ALG_PURE_EDDSA)) && \
	defined(PSA_WANT_ECC_TWISTED_EDWARDS_255)
	const size_t pub_key_max_size = 32;
#else
#error No valid curve for signature validation
#endif

#endif

#ifdef CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
psa_status_t cracen_kmu_get_builtin_key(psa_drv_slot_number_t slot_number,
	psa_key_attributes_t *attributes, uint8_t *key_buffer,
	size_t key_buffer_size, size_t *key_buffer_length);

static psa_status_t get_key_buffer(
	mbedtls_svc_key_id_t key_id, psa_key_attributes_t *attributes,
	uint8_t *key, size_t key_size, size_t *key_length)
{
	psa_status_t status;
	psa_key_lifetime_t lifetime;
	psa_drv_slot_number_t slot_number;

	status = cracen_kmu_get_key_slot(key_id, &lifetime, &slot_number);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return cracen_kmu_get_builtin_key(slot_number, attributes, key, key_size, key_length);
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

#if defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA) || \
	defined(PSA_WANT_ALG_ED25519PH)

psa_status_t psa_verify_hash(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
	const uint8_t *signature, size_t signature_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_key_attributes_t attr;
	uint8_t pub_key[pub_key_max_size];
	size_t pub_key_length;

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

	status = get_key_buffer(key_id, &attr, pub_key, pub_key_max_size, &pub_key_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

#if defined(CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS)
	int cracen_status = cracen_ed25519ph_verify(pub_key, hash, hash_length, signature, false);

	return silex_statuscodes_to_psa(cracen_status);
#else
	return psa_driver_wrapper_verify_hash_with_context(&attr, pub_key, pub_key_length, alg,
							   hash, hash_length, NULL, 0, signature,
							   signature_length);
#endif
}

#endif /* PSA_WANT_ALG_ECDSA || PSA_WANT_ALG_DETERMINISTIC_ECDSA || PSA_WANT_ALG_ED25519PH  */

#if defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA) || \
	defined(PSA_WANT_ALG_PURE_EDDSA)

psa_status_t psa_verify_message(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg,
	const uint8_t *input, size_t input_length,
	const uint8_t *signature, size_t signature_length)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_key_attributes_t attr;
	uint8_t pub_key[pub_key_max_size];
	size_t pub_key_size = 0;

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

	status = get_key_buffer(key_id, &attr, pub_key, pub_key_max_size, &pub_key_size);
	if (status != PSA_SUCCESS) {
		return status;
	}
#if defined(CONFIG_PSA_CORE_LITE_NSIB_ED25519_OPTIMIZATIONS)
	int cracen_status = cracen_ed25519_verify(pub_key, input, input_length, signature);

	return silex_statuscodes_to_psa(cracen_status);
#else
	return psa_driver_wrapper_verify_message_with_context(&attr, pub_key, pub_key_size, alg,
							      input, input_length, NULL, 0,
							      signature, signature_length);
#endif
}

#endif /* PSA_WANT_ALG_ECDSA || PSA_WANT_ALG_DETERMINISTIC_ECDSA || PSA_WANT_ALG_PURE_EDDSA  */

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

/** @brief Id of the volatile key that is created by extracting key material from the KMU */
static mbedtls_svc_key_id_t g_volatile_enc_key_id;

static psa_status_t get_enc_key(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg,
	psa_lite_key_slot_t **key_slot)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (key_slot == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!VERIFY_ALG_CTR(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (psa_lite_key_id_is_volatile(key_id)) {
		return psa_lite_get_key_slot(&key_id, key_slot);
	}

	/* Allocating volatile key slot to store key material from the KMU */
	status = psa_lite_get_key_slot(&g_volatile_enc_key_id, key_slot);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return get_key_buffer(key_id, &(*key_slot)->key_attributes,
			      (*key_slot)->key, sizeof((*key_slot)->key), &(*key_slot)->key_size);
}

static inline void clear_enc_key(void)
{
	psa_lite_free_key_slot(g_volatile_enc_key_id);
	/* Resetting key ID here to avoid clearing volatile key that may have the same ID */
	g_volatile_enc_key_id = PSA_LITE_KEY_ID_NULL;
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
		clear_enc_key();
		return status;
	}

	status = psa_driver_wrapper_cipher_encrypt_setup(operation,
							 &key_slot->key_attributes,
							 key_slot->key,
							 key_slot->key_size,
							 alg);
	if (status != PSA_SUCCESS) {
		clear_enc_key();
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
		clear_enc_key();
		return status;
	}

	status = psa_driver_wrapper_cipher_decrypt_setup(operation,
							 &key_slot->key_attributes,
							 key_slot->key,
							 key_slot->key_size,
							 alg);
	if (status != PSA_SUCCESS) {
		clear_enc_key();
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
		clear_enc_key();
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
		clear_enc_key();
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

	/* Always clear key when finishing operation */
	clear_enc_key();

	return status;
}

psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation)
{
	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	clear_enc_key();

	return psa_driver_wrapper_cipher_abort(operation);
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

#if defined(PSA_WANT_ALG_CTR)
	if (psa_lite_key_id_is_volatile(key_id)) {
		psa_lite_free_key_slot(key_id);
		return PSA_SUCCESS;
	}
#endif /* PSA_WANT_ALG_CTR */

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

/* Initialization function */

psa_status_t psa_crypto_init(void)
{
	return psa_driver_wrapper_init();
}
