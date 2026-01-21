/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "psa_core_lite.h"
#include <psa/crypto.h>
#include <psa_crypto_driver_wrappers.h>

#ifdef CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
#include <cracen_psa_kmu.h>
#endif

#ifdef CONFIG_PSA_CRYPTO_DRIVER_CRACEN
#include <cracen/mem_helpers.h>
#include <cracen_psa_eddsa.h>
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
	return psa_driver_wrapper_verify_hash(&attr, pub_key, pub_key_length,
					      alg, hash, hash_length,
					      signature, signature_length);
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
	return psa_driver_wrapper_verify_message(&attr, pub_key, pub_key_size,
						 alg, input, input_length,
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

/* Ensure that the largest key size is supported */
#if PSA_WANT_AES_KEY_SIZE_256
#define ENC_KEY_MAX_SIZE	(32)
#elif PSA_WANT_AES_KEY_SIZE_192
#define ENC_KEY_MAX_SIZE	(24)
#elif PSA_WANT_AES_KEY_SIZE_128
#define ENC_KEY_MAX_SIZE	(16)
#else
#error "FW encryption requires either AES-256, AES-192 or AES-128 being enabled"
#endif

/** @brief Buffer containing the established/derived encryption key */
uint8_t g_enc_key[ENC_KEY_MAX_SIZE];

static psa_status_t get_enc_key(
	mbedtls_svc_key_id_t key_id, psa_algorithm_t alg,
	psa_key_attributes_t *attributes, size_t *key_length)
{
	if (attributes == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!VERIFY_ALG_CTR(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return get_key_buffer(key_id, attributes, g_enc_key, ENC_KEY_MAX_SIZE, key_length);
}

static inline void clear_enc_key(void)
{
	safe_memzero(g_enc_key, ENC_KEY_MAX_SIZE);
}

psa_status_t psa_cipher_encrypt_setup(
	psa_cipher_operation_t *operation, mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_key_attributes_t attributes;
	size_t key_length;

	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = get_enc_key(key, alg, &attributes, &key_length);
	if (status != PSA_SUCCESS) {
		clear_enc_key();
		return status;
	}

	status = psa_driver_wrapper_cipher_encrypt_setup(operation, &attributes, g_enc_key,
							 key_length, alg);
	if (status != PSA_SUCCESS) {
		clear_enc_key();
	}

	return status;
}

psa_status_t psa_cipher_decrypt_setup(
	psa_cipher_operation_t *operation, mbedtls_svc_key_id_t key, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	psa_key_attributes_t attributes;
	size_t key_length;

	if (operation == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = get_enc_key(key, alg, &attributes, &key_length);
	if (status != PSA_SUCCESS) {
		clear_enc_key();
		return status;
	}

	status = psa_driver_wrapper_cipher_decrypt_setup(operation, &attributes, g_enc_key,
							 key_length, alg);
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
