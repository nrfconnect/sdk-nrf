/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 *  Functions to delegate cryptographic operations to an available
 *  and appropriate accelerator.
 *  Warning: This file will be auto-generated in the future.
 */

#include "common.h"
#include "psa_crypto_core.h"
#include "psa_crypto_driver_wrappers.h"
#include <string.h>

#include "mbedtls/platform_util.h"

#if defined(MBEDTLS_PSA_CRYPTO_C)

#if defined(CONFIG_HW_UNIQUE_KEY_WRITE_ON_CRYPTO_INIT)
#include <hw_unique_key.h>
#endif /* CONFIG_HW_UNIQUE_KEY_WRITE_ON_CRYPTO_INIT */

#if defined(PSA_NEED_CRACEN_TRNG_DRIVER)
#include <cracen_trng/cracen_trng.h>
#endif

#if defined(MBEDTLS_PSA_CRYPTO_DRIVERS)

#if defined(PSA_NEED_CC3XX_AEAD_DRIVER) || defined(PSA_NEED_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER) || \
	defined(PSA_NEED_CC3XX_CIPHER_DRIVER) || defined(PSA_NEED_CC3XX_KEY_AGREEMENT_DRIVER) ||   \
	defined(PSA_NEED_CC3XX_HASH_DRIVER) || defined(PSA_NEED_CC3XX_KEY_MANAGEMENT_DRIVER) ||    \
	defined(PSA_NEED_CC3XX_MAC_DRIVER) || defined(PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER)
#include "cc3xx.h"
#endif

#if defined(PSA_NEED_CC3XX_CTR_DRBG_DRIVER)
#include "nrf_cc3xx_platform_ctr_drbg.h"
#endif
#if defined(PSA_NEED_CC3XX_HMAC_DRBG_DRIVER)
#include "nrf_cc3xx_platform_hmac_drbg.h"
#endif

#ifdef PSA_NEED_OBERON_AEAD_DRIVER
#include "oberon_aead.h"
#endif

#ifdef PSA_NEED_OBERON_CIPHER_DRIVER
#include "oberon_cipher.h"
#endif

#ifdef PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER
#include "oberon_asymmetric_signature.h"
#endif

#ifdef PSA_NEED_OBERON_ASYMMETRIC_ENCRYPTION_DRIVER
#include "oberon_asymmetric_encrypt.h"
#endif

#ifdef PSA_NEED_OBERON_HASH_DRIVER
#include "oberon_hash.h"
#endif

#ifdef PSA_NEED_OBERON_KEY_AGREEMENT_DRIVER
#include "oberon_key_agreement.h"
#endif

#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER
#include "oberon_key_management.h"
#endif

#ifdef PSA_NEED_OBERON_MAC_DRIVER
#include "oberon_mac.h"
#endif

#ifdef PSA_NEED_OBERON_KEY_DERIVATION_DRIVER
#include "oberon_key_derivation.h"
#endif

#ifdef PSA_NEED_OBERON_CTR_DRBG_DRIVER
#include "oberon_ctr_drbg.h"
#endif

#ifdef PSA_NEED_OBERON_HMAC_DRBG_DRIVER
#include "oberon_hmac_drbg.h"
#endif

#ifdef PSA_NEED_OBERON_AEAD_DRIVER
#include "oberon_aead.h"
#endif

#ifdef PSA_NEED_OBERON_CIPHER_DRIVER
#include "oberon_cipher.h"
#endif

#ifdef PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER
#include "oberon_asymmetric_signature.h"
#endif

#ifdef PSA_NEED_OBERON_PAKE_DRIVER
#include "oberon_pake.h"
#endif

#ifdef PSA_NEED_OBERON_XOF_DRIVER
#include "oberon_xof.h"
#endif

#ifdef PSA_NEED_OBERON_KEY_WRAP_DRIVER
#include "oberon_key_wrap.h"
#endif

#if defined(PSA_CRYPTO_DRIVER_CRACEN)
#ifndef PSA_CRYPTO_DRIVER_PRESENT
#define PSA_CRYPTO_DRIVER_PRESENT
#endif
#include "cracen_psa.h"
#include "cracen_psa_ctr_drbg.h"
#include "cracen/hardware.h"
#endif /* PSA_CRYPTO_DRIVER_CRACEN */

/* Include TF-M builtin key driver */
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
#include "tfm_crypto_defs.h"
#include "tfm_builtin_key_loader.h"
#endif /* PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER */

#if defined(PSA_NEED_NRF_RNG_ENTROPY_DRIVER)
#include <psa/nrf_rng_entropy.h>
#endif

#if defined(PSA_CRYPTO_DRIVER_IRONSIDE)
#include "ironside_psa.h"
#endif

/* Repeat above block for each JSON-declared driver during autogeneration */
#endif /* MBEDTLS_PSA_CRYPTO_DRIVERS */

/* Auto-generated values depending on which drivers are registered.
 * ID 0 is reserved for unallocated operations.
 * ID 1 is reserved for the Mbed TLS software driver.
 * ID 5 is defined by a vanilla TF-M patch file.
 */

#define PSA_CRYPTO_CC3XX_DRIVER_ID  (4)
#define PSA_CRYPTO_CRACEN_DRIVER_ID (5)

#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
#define PSA_CRYPTO_TFM_BUILTIN_KEY_LOADER_DRIVER_ID (6)
#endif /* PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER */

#define PSA_CRYPTO_IRONSIDE_DRIVER_ID (7)

#define PSA_CRYPTO_OBERON_DRIVER_ID (28)

#if defined(PSA_CRYPTO_DRIVER_ALG_PRNG_TEST)
psa_status_t prng_test_generate_random(uint8_t *output, size_t output_size);
#endif

#if defined(CONFIG_HW_UNIQUE_KEY_WRITE_ON_CRYPTO_INIT)
static psa_status_t hw_unique_key_provisioning(void)
{
	if (!hw_unique_key_are_any_written()) {
		int result = hw_unique_key_write_random();

		if (result != HW_UNIQUE_KEY_SUCCESS) {
			return PSA_ERROR_GENERIC_ERROR;
		}
	}

	return PSA_SUCCESS;
}
#endif /* CONFIG_HW_UNIQUE_KEY_WRITE_ON_CRYPTO_INIT */

psa_status_t psa_driver_wrapper_init(void)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	(void)status;

#if defined(PSA_CRYPTO_DRIVER_CRACEN)
	status = cracen_init();
	if (status != PSA_SUCCESS) {
		return status;
	}
#endif

#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	status = tfm_builtin_key_loader_init();
	if (status != PSA_SUCCESS) {
		return status;
	}
#endif /* PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER */

#if defined(CONFIG_HW_UNIQUE_KEY_WRITE_ON_CRYPTO_INIT)
	return hw_unique_key_provisioning();
#endif /* CONFIG_HW_UNIQUE_KEY_WRITE_ON_CRYPTO_INIT */

	return PSA_SUCCESS;
}

void psa_driver_wrapper_free(void)
{
	/* Intentionally left empty */
}

/* Start delegation functions */
psa_status_t psa_driver_wrapper_sign_message_with_context(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length, const uint8_t *context,
	size_t context_length, uint8_t *signature, size_t signature_size, size_t *signature_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
	case PSA_KEY_LOCATION_CRACEN:
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */
		status = cracen_sign_message(attributes, key_buffer, key_buffer_size, alg, input,
					     input_length, signature, signature_size,
					     signature_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER */
#if defined(PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER)
		status = cc3xx_sign_message(attributes, key_buffer, key_buffer_size, alg, input,
					    input_length, signature, signature_size,
					    signature_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER */
#if defined(PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER)
		status = oberon_sign_message_with_context(
			attributes, key_buffer, key_buffer_size, alg, input, input_length, context,
			context_length, signature, signature_size, signature_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER */
		break;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		break;
	}

	/* Call back to the core with psa_sign_message_builtin.
	 * This will in turn forward this to use psa_crypto_driver_wrapper_sign_hash
	 */
	return psa_sign_message_with_context_builtin(attributes, key_buffer, key_buffer_size, alg,
						     input, input_length, context, context_length,
						     signature, signature_size, signature_length);
}

psa_status_t psa_driver_wrapper_verify_message_with_context(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length, const uint8_t *context,
	size_t context_length, const uint8_t *signature, size_t signature_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
	case PSA_KEY_LOCATION_CRACEN:
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */
		status = cracen_verify_message(attributes, key_buffer, key_buffer_size, alg, input,
					       input_length, signature, signature_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER */
#if defined(PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER)
		status = cc3xx_verify_message(attributes, key_buffer, key_buffer_size, alg, input,
					      input_length, signature, signature_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER */
#if defined(PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER)
		status = oberon_verify_message_with_context(
			attributes, key_buffer, key_buffer_size, alg, input, input_length, context,
			context_length, signature, signature_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER */
		break;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		break;
	}

#if defined(CONFIG_PSA_CORE_LITE)
	return PSA_ERROR_NOT_SUPPORTED;
#else
	/* Call back to the core with psa_verify_message_builtin.
	 * This will in turn forward this to use psa_crypto_driver_wrapper_verify_hash
	 */
	return psa_verify_message_with_context_builtin(attributes, key_buffer, key_buffer_size, alg,
						       input, input_length, context, context_length,
						       signature, signature_length);
#endif
}

psa_status_t psa_driver_wrapper_sign_hash_with_context(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *hash, size_t hash_length, const uint8_t *context,
	size_t context_length, uint8_t *signature, size_t signature_size, size_t *signature_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */

#if defined(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
	case PSA_KEY_LOCATION_CRACEN:
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */
		status = cracen_sign_hash(attributes, key_buffer, key_buffer_size, alg, hash,
					  hash_length, signature, signature_size, signature_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER */
#if defined(PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER)
		status = cc3xx_sign_hash(attributes, key_buffer, key_buffer_size, alg, hash,
					 hash_length, signature, signature_size, signature_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER */
#if defined(PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER)
		return oberon_sign_hash_with_context(attributes, key_buffer, key_buffer_size, alg,
						     hash, hash_length, context, context_length,
						     signature, signature_size, signature_length);
#endif /* PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER */
		/* Fell through, meaning nothing supports this operation */
		(void)attributes;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)hash;
		(void)hash_length;
		(void)context;
		(void)context_length;
		(void)signature;
		(void)signature_size;
		(void)signature_length;
		return PSA_ERROR_NOT_SUPPORTED;
		/* Add cases for opaque driver here */
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_verify_hash_with_context(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *hash, size_t hash_length, const uint8_t *context,
	size_t context_length, const uint8_t *signature, size_t signature_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
	case PSA_KEY_LOCATION_CRACEN:
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */
		status = cracen_verify_hash(attributes, key_buffer, key_buffer_size, alg, hash,
					    hash_length, signature, signature_length);

		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER */
#if defined(PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER)
		/* Do not call the cc3xx_verify_hash for RSA keys since it still in early
		 * development
		 */
		status = cc3xx_verify_hash(attributes, key_buffer, key_buffer_size, alg, hash,
					   hash_length, signature, signature_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER */
#if defined(PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER)
		return oberon_verify_hash_with_context(attributes, key_buffer, key_buffer_size, alg,
						       hash, hash_length, context, context_length,
						       signature, signature_length);
#endif /* PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER */
		/* Fell through, meaning nothing supports this operation */
		(void)attributes;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)hash;
		(void)hash_length;
		(void)context;
		(void)context_length;
		(void)signature;
		(void)signature_length;
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

/** Calculate the key buffer size required to store the key material of a key
 *  associated with an opaque driver from input key data.
 *
 * \param[in] attributes        The key attributes
 * \param[in] data              The input key data.
 * \param[in] data_length       The input data length.
 * \param[out] key_buffer_size  Minimum buffer size to contain the key material.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t
psa_driver_wrapper_get_key_buffer_size_from_key_data(const psa_key_attributes_t *attributes,
						     const uint8_t *data, size_t data_length,
						     size_t *key_buffer_size)
{
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);
	psa_key_type_t key_type = attributes->type;

	*key_buffer_size = 0;
	switch (location) {
#if defined(PSA_CRYPTO_DRIVER_CRACEN)
	case PSA_KEY_LOCATION_CRACEN:
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		return cracen_get_opaque_size(attributes, key_buffer_size);
#endif
	default:
		(void)key_type;
		(void)data;
		(void)data_length;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

/** Get the key buffer size required to store the key material of a key
 *  associated with an opaque driver.
 *
 * \param[in] attributes  The key attributes.
 * \param[out] key_buffer_size  Minimum buffer size to contain the key material
 *
 * \retval #PSA_SUCCESS
 *         The minimum size for a buffer to contain the key material has been
 *         returned successfully.
 * \retval #PSA_ERROR_NOT_SUPPORTED
 *         The type and/or the size in bits of the key or the combination of
 *         the two is not supported.
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 *         The key is declared with a lifetime not known to us.
 */
psa_status_t psa_driver_wrapper_get_key_buffer_size(const psa_key_attributes_t *attributes,
						    size_t *key_buffer_size)
{
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);
	psa_key_type_t key_type = attributes->type;
	size_t key_bits = attributes->bits;

	*key_buffer_size = 0;
	switch (location) {
#if defined(PSA_CRYPTO_DRIVER_IRONSIDE)
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
		return ironside_psa_get_key_buffer_size(attributes, key_buffer_size);
#endif
#if defined(PSA_CRYPTO_DRIVER_CRACEN)
	case PSA_KEY_LOCATION_CRACEN:
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		return cracen_get_opaque_size(attributes, key_buffer_size);
#endif
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
		return tfm_builtin_key_loader_get_key_buffer_size(psa_get_key_id(attributes),
								  key_buffer_size);
#endif /* PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER */
	default:
		(void)key_type;
		(void)key_bits;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_generate_key(const psa_key_attributes_t *attributes,
					     uint8_t *key_buffer, size_t key_buffer_size,
					     size_t *key_buffer_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
#if defined(PSA_CRYPTO_DRIVER_IRONSIDE)
		status = ironside_psa_generate_key(attributes, key_buffer, key_buffer_size,
						   key_buffer_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */
		/* Transparent drivers are limited to generating asymmetric keys */
		if (PSA_KEY_TYPE_IS_ASYMMETRIC(attributes->type)) {
			/* Cycle through all known transparent accelerators */
#if defined(PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER)
			status = cracen_generate_key(attributes, key_buffer, key_buffer_size,
						     key_buffer_length);
			/* Declared with fallback == true */
			if (status != PSA_ERROR_NOT_SUPPORTED) {
				break;
			}
#endif /* PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER*/
#if defined(PSA_NEED_CC3XX_KEY_MANAGEMENT_DRIVER)
			status = cc3xx_generate_key(attributes, key_buffer, key_buffer_size,
						    key_buffer_length);
			/* Declared with fallback == true */
			if (status != PSA_ERROR_NOT_SUPPORTED) {
				break;
			}
#endif /* PSA_NEED_CC3XX_KEY_MANAGEMENT_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER)
			status = oberon_generate_key(attributes, key_buffer, key_buffer_size,
						     key_buffer_length);
			/* Declared with fallback == true */
			if (status != PSA_ERROR_NOT_SUPPORTED) {
				break;
			}
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER */
		}

		/* Software fallback */
		status = psa_generate_key_internal(attributes, key_buffer, key_buffer_size,
						   key_buffer_length);
		break;

#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
		return cracen_generate_key(attributes, key_buffer, key_buffer_size,
					     key_buffer_length);
#endif

	default:
		/* Key is declared with a lifetime not known to us */
		status = PSA_ERROR_INVALID_ARGUMENT;
		break;
	}

	return status;
}

psa_status_t psa_driver_wrapper_import_key(const psa_key_attributes_t *attributes,
					   const uint8_t *data, size_t data_length,
					   uint8_t *key_buffer, size_t key_buffer_size,
					   size_t *key_buffer_length, size_t *bits)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_CRYPTO_DRIVER_IRONSIDE)
		status = ironside_psa_import_key(attributes, data, data_length, key_buffer,
						 key_buffer_size, key_buffer_length, bits);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif
#if defined(PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER)
		status = cracen_import_key(attributes, data, data_length, key_buffer,
					   key_buffer_size, key_buffer_length, bits);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif
#if defined(PSA_NEED_CC3XX_KEY_MANAGEMENT_DRIVER)
		status = cc3xx_import_key(attributes, data, data_length, key_buffer,
					  key_buffer_size, key_buffer_length, bits);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_KEY_MANAGEMENT_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER)
		status = oberon_import_key(attributes, data, data_length, key_buffer,
					   key_buffer_size, key_buffer_length, bits);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER */
		/*
		 * Fall through, meaning no accelerator supports this operation.
		 * Oberon doesn't support importing symmetric keys at the moment
		 * so this is necessary for Oberon to work.
		 */
		return psa_import_key_into_slot(attributes, data, data_length, key_buffer,
						key_buffer_size, key_buffer_length, bits);

#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN:
	case PSA_KEY_LOCATION_CRACEN_KMU:
		return cracen_import_key(attributes, data, data_length, key_buffer,
					   key_buffer_size, key_buffer_length, bits);
#endif

	default:
		(void)status;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_export_key(const psa_key_attributes_t *attributes,
					   const uint8_t *key_buffer, size_t key_buffer_size,
					   uint8_t *data, size_t data_size, size_t *data_length)

{
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		return psa_export_key_internal(attributes, key_buffer, key_buffer_size, data,
					       data_size, data_length);
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
		return cracen_export_key(attributes, key_buffer, key_buffer_size, data, data_size,
					 data_length);
#endif
	default:
		/* Key is declared with a lifetime not known to us */
		return status;
	}
}

psa_status_t psa_driver_wrapper_export_public_key(const psa_key_attributes_t *attributes,
						  const uint8_t *key_buffer, size_t key_buffer_size,
						  uint8_t *data, size_t data_size,
						  size_t *data_length)

{
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif /* defined(PSA_NEED_CRACEN_KMU_DRIVER) */
#if defined(PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER)
		status = cracen_export_public_key(attributes, key_buffer, key_buffer_size, data,
						  data_size, data_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER*/
#if defined(PSA_NEED_CC3XX_KEY_MANAGEMENT_DRIVER)
		status = cc3xx_export_public_key(attributes, key_buffer, key_buffer_size, data,
						 data_size, data_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_KEY_MANAGEMENT_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER)
		status = oberon_export_public_key(attributes, key_buffer, key_buffer_size, data,
						  data_size, data_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER */
#if defined(PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER)
	case PSA_KEY_LOCATION_CRACEN:
		return cracen_export_public_key(attributes, key_buffer, key_buffer_size, data,
						data_size, data_length);
#endif /* PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER*/
		/* Fell through, meaning no accelerator supports this operation.
		 * The CryptoCell driver doesn't support export public keys when
		 * the key is a public key itself, so this is necessary.
		 */
		return psa_export_public_key_internal(attributes, key_buffer, key_buffer_size, data,
						      data_size, data_length);
	default:
		/* Key is declared with a lifetime not known to us */
		return status;
	}
}

psa_status_t psa_driver_wrapper_get_builtin_key(psa_drv_slot_number_t slot_number,
						psa_key_attributes_t *attributes,
						uint8_t *key_buffer, size_t key_buffer_size,
						size_t *key_buffer_length)
{
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
#if defined(PSA_CRYPTO_DRIVER_IRONSIDE)
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
		return ironside_psa_get_builtin_key(slot_number, attributes, key_buffer,
						    key_buffer_size, key_buffer_length);
#endif
#if defined(PSA_CRYPTO_DRIVER_CRACEN)
	case PSA_KEY_LOCATION_CRACEN:
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		return (cracen_get_builtin_key(slot_number, attributes, key_buffer, key_buffer_size,
					       key_buffer_length));
#endif /* PSA_CRYPTO_DRIVER_CRACEN */
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
		return tfm_builtin_key_loader_get_builtin_key(slot_number, attributes, key_buffer,
							      key_buffer_size, key_buffer_length);
#endif /* PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER */
	default:
		(void)slot_number;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)key_buffer_length;
		return PSA_ERROR_DOES_NOT_EXIST;
	}
}

psa_status_t psa_driver_wrapper_copy_key(psa_key_attributes_t *attributes,
					 const uint8_t *source_key, size_t source_key_length,
					 uint8_t *target_key_buffer, size_t target_key_buffer_size,
					 size_t *target_key_buffer_length)
{
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
#if defined(PSA_CRYPTO_DRIVER_IRONSIDE)
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
		return ironside_psa_copy_key(attributes, source_key, source_key_length,
					     target_key_buffer, target_key_buffer_size,
					     target_key_buffer_length);
#endif
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
		return cracen_copy_key(attributes, source_key, source_key_length, target_key_buffer,
				       target_key_buffer_size, target_key_buffer_length);
#endif
	default:
		(void)source_key;
		(void)source_key_length;
		(void)target_key_buffer;
		(void)target_key_buffer_size;
		(void)target_key_buffer_length;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_derive_key(const psa_key_attributes_t *attributes,
					   const uint8_t *input, size_t input_length,
					   uint8_t *key_buffer, size_t key_buffer_size,
					   size_t *key_buffer_length)
{
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;

	switch (PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime)) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
		/* Add cases for transparent drivers here */
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
		status = ironside_psa_derive_key(attributes, input, input_length, key_buffer,
						 key_buffer_size, key_buffer_length);

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */

#ifdef PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER
		status = cracen_derive_key(attributes, input, input_length, key_buffer,
					   key_buffer_size, key_buffer_length);

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER */

#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER
		return oberon_derive_key(attributes, input, input_length, key_buffer,
					   key_buffer_size, key_buffer_length);
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER */
		break;

		/* Add cases for opaque drivers here */

	default:
		/* Key is declared with a lifetime not known to us */
		(void)input;
		(void)input_length;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)key_buffer_length;
	}

	return status;
}

/*
 * Cipher functions
 */
psa_status_t psa_driver_wrapper_cipher_encrypt(const psa_key_attributes_t *attributes,
					       const uint8_t *key_buffer, size_t key_buffer_size,
					       psa_algorithm_t alg, const uint8_t *iv,
					       size_t iv_length, const uint8_t *input,
					       size_t input_length, uint8_t *output,
					       size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
#if defined(PSA_NEED_CRACEN_CIPHER_DRIVER)
		status = cracen_cipher_encrypt(attributes, key_buffer, key_buffer_size, alg, iv,
					       iv_length, input, input_length, output, output_size,
					       output_length);

		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_CIPHER_DRIVER */
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER)
		status = cc3xx_cipher_encrypt(attributes, key_buffer, key_buffer_size, alg, iv,
					      iv_length, input, input_length, output, output_size,
					      output_length);

		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_CIPHER_DRIVER */
#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
		return oberon_cipher_encrypt(attributes, key_buffer, key_buffer_size, alg, iv,
					       iv_length, input, input_length, output, output_size,
					       output_length);
#endif /* PSA_NEED_OBERON_CIPHER_DRIVER */
		(void)attributes;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)iv;
		(void)iv_length;
		(void)input;
		(void)input_length;
		(void)output;
		(void)output_size;
		(void)output_length;
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)iv;
		(void)iv_length;
		(void)input;
		(void)input_length;
		(void)output;
		(void)output_size;
		(void)output_length;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_cipher_decrypt(const psa_key_attributes_t *attributes,
					       const uint8_t *key_buffer, size_t key_buffer_size,
					       psa_algorithm_t alg, const uint8_t *input,
					       size_t input_length, uint8_t *output,
					       size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
#if defined(PSA_NEED_CRACEN_CIPHER_DRIVER)
	case PSA_KEY_LOCATION_CRACEN:
#endif
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif
	case PSA_KEY_LOCATION_LOCAL_STORAGE:

		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_CIPHER_DRIVER)
		status = cracen_cipher_decrypt(attributes, key_buffer, key_buffer_size, alg, input,
					       input_length, output, output_size, output_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_CIPHER_DRIVER */
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER)
		status = cc3xx_cipher_decrypt(attributes, key_buffer, key_buffer_size, alg, input,
					      input_length, output, output_size, output_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_CIPHER_DRIVER */
#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
		return oberon_cipher_decrypt(attributes, key_buffer, key_buffer_size, alg, input,
					       input_length, output, output_size, output_length);
#endif /* PSA_NEED_OBERON_CIPHER_DRIVER */
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)input;
		(void)input_length;
		(void)output;
		(void)output_size;
		(void)output_length;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_cipher_encrypt_setup(psa_cipher_operation_t *operation,
						     const psa_key_attributes_t *attributes,
						     const uint8_t *key_buffer,
						     size_t key_buffer_size, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_CIPHER_DRIVER)
		status = cracen_cipher_encrypt_setup(&operation->ctx.cracen_driver_ctx, attributes,
						     key_buffer, key_buffer_size, alg);
		/* Declared with fallback == true */
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
		}

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_CIPHER_DRIVER */
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER)
		status = cc3xx_cipher_encrypt_setup(&operation->ctx.cc3xx_driver_ctx, attributes,
						    key_buffer, key_buffer_size, alg);
		/* Declared with fallback == true */
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CC3XX_DRIVER_ID;
		}

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_CIPHER_DRIVER */
#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
		status = oberon_cipher_encrypt_setup(&operation->ctx.oberon_driver_ctx, attributes,
						     key_buffer, key_buffer_size, alg);
		/* Declared with fallback == true */
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
		}
		return status;
#endif /* PSA_NEED_OBERON_CIPHER_DRIVER */
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_cipher_decrypt_setup(psa_cipher_operation_t *operation,
						     const psa_key_attributes_t *attributes,
						     const uint8_t *key_buffer,
						     size_t key_buffer_size, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_CIPHER_DRIVER)
		status = cracen_cipher_decrypt_setup(&operation->ctx.cracen_driver_ctx, attributes,
						     key_buffer, key_buffer_size, alg);
		/* Declared with fallback == true */
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
		}

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER)
		status = cc3xx_cipher_decrypt_setup(&operation->ctx.cc3xx_driver_ctx, attributes,
						    key_buffer, key_buffer_size, alg);
		/* Declared with fallback == true */
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CC3XX_DRIVER_ID;
		}

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_CIPHER_DRIVER */
#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
		status = oberon_cipher_decrypt_setup(&operation->ctx.oberon_driver_ctx, attributes,
						     key_buffer, key_buffer_size, alg);
		/* Declared with fallback == true */
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
		}
		return status;
#endif /* PSA_NEED_OBERON_CIPHER_DRIVER */
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv,
					      size_t iv_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_CIPHER_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_cipher_set_iv(&operation->ctx.cracen_driver_ctx, iv, iv_length));
#endif /* PSA_NEED_CRACEN_CIPHER_DRIVER */
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_cipher_set_iv(&operation->ctx.cc3xx_driver_ctx, iv, iv_length);
#endif /* PSA_NEED_CC3XX_CIPHER_DRIVER */
#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_cipher_set_iv(&operation->ctx.oberon_driver_ctx, iv, iv_length);
#endif /* PSA_NEED_OBERON_CIPHER_DRIVER */
	}

	(void)iv;
	(void)iv_length;

	return PSA_ERROR_INVALID_ARGUMENT;
}

psa_status_t psa_driver_wrapper_cipher_update(psa_cipher_operation_t *operation,
					      const uint8_t *input, size_t input_length,
					      uint8_t *output, size_t output_size,
					      size_t *output_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_CIPHER_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_cipher_update(&operation->ctx.cracen_driver_ctx, input, input_length,
					     output, output_size, output_length));
#endif /* PSA_NEED_CRACEN_CIPHER_DRIVER */
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_cipher_update(&operation->ctx.cc3xx_driver_ctx, input, input_length,
					   output, output_size, output_length);
#endif /* PSA_NEED_CC3XX_CIPHER_DRIVER */
#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_cipher_update(&operation->ctx.oberon_driver_ctx, input, input_length,
					    output, output_size, output_length);
#endif /* PSA_NEED_OBERON_CIPHER_DRIVER */
	}

	(void)input;
	(void)input_length;
	(void)output;
	(void)output_size;
	(void)output_length;

	return PSA_ERROR_INVALID_ARGUMENT;
}

psa_status_t psa_driver_wrapper_cipher_finish(psa_cipher_operation_t *operation, uint8_t *output,
					      size_t output_size, size_t *output_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_CIPHER_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_cipher_finish(&operation->ctx.cracen_driver_ctx, output, output_size,
					     output_length));
#endif /* PSA_NEED_CRACEN_CIPHER_DRIVER */
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_cipher_finish(&operation->ctx.cc3xx_driver_ctx, output, output_size,
					   output_length);
#endif /* PSA_NEED_CC3XX_CIPHER_DRIVER*/
#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_cipher_finish(&operation->ctx.oberon_driver_ctx, output, output_size,
					    output_length);
#endif /* PSA_NEED_OBERON_CIPHER_DRIVER */
	}

	(void)output;
	(void)output_size;
	(void)output_length;

	return PSA_ERROR_INVALID_ARGUMENT;
}

psa_status_t psa_driver_wrapper_cipher_abort(psa_cipher_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;
	(void) status;

	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_CIPHER_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		status = cracen_cipher_abort(&operation->ctx.cracen_driver_ctx);
		mbedtls_platform_zeroize(&operation->ctx.cracen_driver_ctx,
					 sizeof(operation->ctx.cracen_driver_ctx));
		return status;
#endif /* PSA_NEED_CRACEN_CIPHER_DRIVER */
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		status = cc3xx_cipher_abort(&operation->ctx.cc3xx_driver_ctx);
		mbedtls_platform_zeroize(&operation->ctx.cc3xx_driver_ctx,
					 sizeof(operation->ctx.cc3xx_driver_ctx));
		return status;
#endif /* PSA_NEED_CC3XX_CIPHER_DRIVER */
#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		status = oberon_cipher_abort(&operation->ctx.oberon_driver_ctx);
		mbedtls_platform_zeroize(&operation->ctx.oberon_driver_ctx,
					 sizeof(operation->ctx.oberon_driver_ctx));
		return status;
#endif /* PSA_NEED_OBERON_CIPHER_DRIVER */
	default:
		return PSA_SUCCESS;
	}
}

/*
 * Hashing functions
 */
psa_status_t psa_driver_wrapper_hash_compute(psa_algorithm_t alg, const uint8_t *input,
					     size_t input_length, uint8_t *hash, size_t hash_size,
					     size_t *hash_length)
{
#if !defined(PSA_WANT_ALG_SHA_1)
	if (alg == PSA_ALG_SHA_1) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
#endif

	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

	/* Try accelerators first */
#if defined(PSA_NEED_CRACEN_HASH_DRIVER)
	status = cracen_hash_compute(alg, input, input_length, hash, hash_size, hash_length);
	if (status != PSA_ERROR_NOT_SUPPORTED) {
		return status;
	}
#endif /* PSA_NEED_CRACEN_HASH_DRIVER */
#if defined(PSA_NEED_CC3XX_HASH_DRIVER)
	status = cc3xx_hash_compute(alg, input, input_length, hash, hash_size, hash_length);
	if (status != PSA_ERROR_NOT_SUPPORTED) {
		return status;
	}
#endif /* PSA_NEED_CC3XX_HASH_DRIVER */
#if defined(PSA_NEED_OBERON_HASH_DRIVER)
	return oberon_hash_compute(alg, input, input_length, hash, hash_size, hash_length);
#endif /* PSA_NEED_OBERON_HASH_DRIVER */

	(void)status;
	(void)alg;
	(void)input;
	(void)input_length;
	(void)hash;
	(void)hash_size;
	(void)hash_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_driver_wrapper_hash_setup(psa_hash_operation_t *operation, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

#if !defined(PSA_WANT_ALG_SHA_1)
	if (alg == PSA_ALG_SHA_1) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
#endif

	/* Try setup on accelerators first */
#if defined(PSA_NEED_CRACEN_HASH_DRIVER)
	status = cracen_hash_setup(&operation->ctx.cracen_driver_ctx, alg);
	if (status == PSA_SUCCESS) {
		operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
	}

	if (status != PSA_ERROR_NOT_SUPPORTED) {
		return status;
	}
#endif /* PSA_NEED_CRACEN_HASH_DRIVER */
#if defined(PSA_NEED_CC3XX_HASH_DRIVER)
	status = cc3xx_hash_setup(&operation->ctx.cc3xx_driver_ctx, alg);
	if (status == PSA_SUCCESS) {
		operation->id = PSA_CRYPTO_CC3XX_DRIVER_ID;
	}

	if (status != PSA_ERROR_NOT_SUPPORTED) {
		return status;
	}
#endif /* PSA_NEED_CC3XX_HASH_DRIVER */
#if defined(PSA_NEED_OBERON_HASH_DRIVER)
	status = oberon_hash_setup(&operation->ctx.oberon_driver_ctx, alg);
	if (status == PSA_SUCCESS) {
		operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
	}
	return status;
#endif /* PSA_NEED_OBERON_HASH_DRIVER */

	/* Nothing left to try if we fall through here */
	(void)status;
	(void)operation;
	(void)alg;
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_driver_wrapper_hash_clone(const psa_hash_operation_t *source_operation,
					   psa_hash_operation_t *target_operation)
{
	switch (source_operation->id) {
#if defined(PSA_NEED_CRACEN_HASH_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		target_operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
		return (cracen_hash_clone(&source_operation->ctx.cracen_driver_ctx,
					  &target_operation->ctx.cracen_driver_ctx));
#endif /* PSA_NEED_CRACEN_HASH_DRIVER */
#if defined(PSA_NEED_CC3XX_HASH_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		target_operation->id = PSA_CRYPTO_CC3XX_DRIVER_ID;
		return cc3xx_hash_clone(&source_operation->ctx.cc3xx_driver_ctx,
					&target_operation->ctx.cc3xx_driver_ctx);
#endif /* PSA_NEED_CC3XX_HASH_DRIVER */
#if defined(PSA_NEED_OBERON_HASH_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		target_operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
		return oberon_hash_clone(&source_operation->ctx.oberon_driver_ctx,
					 &target_operation->ctx.oberon_driver_ctx);
#endif /* PSA_NEED_OBERON_HASH_DRIVER */
	default:
		(void)target_operation;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_hash_update(psa_hash_operation_t *operation, const uint8_t *input,
					    size_t input_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_HASH_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_hash_update(&operation->ctx.cracen_driver_ctx, input, input_length));
#endif /* PSA_NEED_CRACEN_HASH_DRIVER */
#if defined(PSA_NEED_CC3XX_HASH_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_hash_update(&operation->ctx.cc3xx_driver_ctx, input, input_length);
#endif /* PSA_NEED_CC3XX_HASH_DRIVER */
#if defined(PSA_NEED_OBERON_HASH_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_hash_update(&operation->ctx.oberon_driver_ctx, input, input_length);
#endif /* PSA_NEED_OBERON_HASH_DRIVER */
	default:
		(void)input;
		(void)input_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_hash_finish(psa_hash_operation_t *operation, uint8_t *hash,
					    size_t hash_size, size_t *hash_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_HASH_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_hash_finish(&operation->ctx.cracen_driver_ctx, hash, hash_size,
					   hash_length));
#endif /* PSA_NEED_CRACEN_HASH_DRIVER */
#if defined(PSA_NEED_CC3XX_HASH_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_hash_finish(&operation->ctx.cc3xx_driver_ctx, hash, hash_size,
					 hash_length);
#endif /* PSA_NEED_CC3XX_HASH_DRIVER */
#if defined(PSA_NEED_OBERON_HASH_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_hash_finish(&operation->ctx.oberon_driver_ctx, hash, hash_size,
					  hash_length);
#endif /* PSA_NEED_OBERON_HASH_DRIVER */
	default:
		(void)hash;
		(void)hash_size;
		(void)hash_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_hash_abort(psa_hash_operation_t *operation)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_HASH_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_hash_abort(&operation->ctx.cracen_driver_ctx));
#endif /* PSA_NEED_CRACEN_HASH_DRIVER */
#if defined(PSA_NEED_CC3XX_HASH_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_hash_abort(&operation->ctx.cc3xx_driver_ctx);
#endif /* PSA_NEED_CC3XX_HASH_DRIVER */
#if defined(PSA_NEED_OBERON_HASH_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_hash_abort(&operation->ctx.oberon_driver_ctx);
#endif /* PSA_NEED_OBERON_HASH_DRIVER */
	default:
		return PSA_SUCCESS;
	}
}

/*
 * XOF functions
 */
psa_status_t psa_driver_wrapper_xof_setup(psa_xof_operation_t *operation, psa_algorithm_t alg)
{
	psa_status_t status;

#if defined(PSA_NEED_OBERON_XOF_DRIVER)
	status = oberon_xof_setup(&operation->ctx.oberon_xof_ctx, alg);
	if (status == PSA_SUCCESS) {
		operation->id = OBERON_DRIVER_ID;
	}
	return status;
#endif /* PSA_NEED_OBERON_XOF_DRIVER */

	(void)status;
	(void)operation;
	(void)alg;
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_driver_wrapper_xof_set_context(psa_xof_operation_t *operation,
						const uint8_t *context, size_t context_length)
{
	switch (operation->id) {

#if defined(PSA_NEED_OBERON_XOF_DRIVER)
	case OBERON_DRIVER_ID:
		return oberon_xof_set_context(&operation->ctx.oberon_xof_ctx, context,
					      context_length);
#endif /* PSA_NEED_OBERON_XOF_DRIVER */

	default:
		(void)context;
		(void)context_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_xof_update(psa_xof_operation_t *operation, const uint8_t *input,
					   size_t input_length)
{
	switch (operation->id) {

#if defined(PSA_NEED_OBERON_XOF_DRIVER)
	case OBERON_DRIVER_ID:
		return oberon_xof_update(&operation->ctx.oberon_xof_ctx, input, input_length);
#endif /* PSA_NEED_OBERON_XOF_DRIVER */

	default:
		(void)input;
		(void)input_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_xof_output(psa_xof_operation_t *operation, uint8_t *output,
					   size_t output_length)
{
	switch (operation->id) {

#if defined(PSA_NEED_OBERON_XOF_DRIVER)
	case OBERON_DRIVER_ID:
		return oberon_xof_output(&operation->ctx.oberon_xof_ctx, output, output_length);
#endif /* PSA_NEED_OBERON_XOF_DRIVER */

	default:
		(void)output;
		(void)output_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_xof_abort(psa_xof_operation_t *operation)
{
	switch (operation->id) {

#if defined(PSA_NEED_OBERON_XOF_DRIVER)
	case OBERON_DRIVER_ID:
		return oberon_xof_abort(&operation->ctx.oberon_xof_ctx);
#endif /* PSA_NEED_OBERON_XOF_DRIVER */

	default:
		return PSA_SUCCESS;
	}
}

psa_status_t psa_driver_wrapper_aead_encrypt(const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg, const uint8_t *nonce,
					     size_t nonce_length, const uint8_t *additional_data,
					     size_t additional_data_length,
					     const uint8_t *plaintext, size_t plaintext_length,
					     uint8_t *ciphertext, size_t ciphertext_size,
					     size_t *ciphertext_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */

#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
		status = cracen_aead_encrypt(attributes, key_buffer, key_buffer_size, alg, nonce,
					     nonce_length, additional_data, additional_data_length,
					     plaintext, plaintext_length, ciphertext,
					     ciphertext_size, ciphertext_length);

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
		status = cc3xx_aead_encrypt(attributes, key_buffer, key_buffer_size, alg, nonce,
					    nonce_length, additional_data, additional_data_length,
					    plaintext, plaintext_length, ciphertext,
					    ciphertext_size, ciphertext_length);

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
		return oberon_aead_encrypt(attributes, key_buffer, key_buffer_size, alg, nonce,
					     nonce_length, additional_data, additional_data_length,
					     plaintext, plaintext_length, ciphertext,
					     ciphertext_size, ciphertext_length);
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */
		(void)attributes;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)nonce;
		(void)nonce_length;
		(void)additional_data;
		(void)additional_data_length;
		(void)plaintext;
		(void)plaintext_length;
		(void)ciphertext;
		(void)ciphertext_size;
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_aead_decrypt(const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg, const uint8_t *nonce,
					     size_t nonce_length, const uint8_t *additional_data,
					     size_t additional_data_length,
					     const uint8_t *ciphertext, size_t ciphertext_length,
					     uint8_t *plaintext, size_t plaintext_size,
					     size_t *plaintext_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
	case PSA_KEY_LOCATION_CRACEN:
#endif
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
	case PSA_KEY_LOCATION_LOCAL_STORAGE:

		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
		status = cracen_aead_decrypt(attributes, key_buffer, key_buffer_size, alg, nonce,
					     nonce_length, additional_data, additional_data_length,
					     ciphertext, ciphertext_length, plaintext,
					     plaintext_size, plaintext_length);

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
		status = cc3xx_aead_decrypt(attributes, key_buffer, key_buffer_size, alg, nonce,
					    nonce_length, additional_data, additional_data_length,
					    ciphertext, ciphertext_length, plaintext,
					    plaintext_size, plaintext_length);

		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
		return oberon_aead_decrypt(attributes, key_buffer, key_buffer_size, alg, nonce,
					     nonce_length, additional_data, additional_data_length,
					     ciphertext, ciphertext_length, plaintext,
					     plaintext_size, plaintext_length);
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */

		(void)attributes;
		(void)attributes;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)nonce;
		(void)nonce_length;
		(void)additional_data;
		(void)additional_data_length;
		(void)ciphertext;
		(void)ciphertext_length;
		(void)plaintext;
		(void)plaintext_size;
		(void)plaintext_length;
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_aead_encrypt_setup(psa_aead_operation_t *operation,
						   const psa_key_attributes_t *attributes,
						   const uint8_t *key_buffer,
						   size_t key_buffer_size, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
		operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
		status = cracen_aead_encrypt_setup(&operation->ctx.cracen_driver_ctx, attributes,
						   key_buffer, key_buffer_size, alg);

		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER*/
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
		status = cc3xx_aead_encrypt_setup(&operation->ctx.cc3xx_driver_ctx, attributes,
						  key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CC3XX_DRIVER_ID;
		}

		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
		status = oberon_aead_encrypt_setup(&operation->ctx.oberon_driver_ctx, attributes,
						   key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
		}

		return status;
#endif /* PSA_NEED_OBERON_AEAD_DRIVER*/

		(void)operation;
		(void)attributes;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_aead_decrypt_setup(psa_aead_operation_t *operation,
						   const psa_key_attributes_t *attributes,
						   const uint8_t *key_buffer,
						   size_t key_buffer_size, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
		operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
		status = cracen_aead_decrypt_setup(&operation->ctx.cracen_driver_ctx, attributes,
						   key_buffer, key_buffer_size, alg);

		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
		status = cc3xx_aead_decrypt_setup(&operation->ctx.cc3xx_driver_ctx, attributes,
						  key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CC3XX_DRIVER_ID;
		}
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER  */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
		status = oberon_aead_decrypt_setup(&operation->ctx.oberon_driver_ctx, attributes,
						   key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
		}

	return status;
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */

		(void)operation;
		(void)attributes;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_aead_set_nonce(psa_aead_operation_t *operation,
					       const uint8_t *nonce, size_t nonce_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_aead_set_nonce(&operation->ctx.cracen_driver_ctx, nonce,
					      nonce_length));
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_aead_set_nonce(&operation->ctx.cc3xx_driver_ctx, nonce, nonce_length);
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_aead_set_nonce(&operation->ctx.oberon_driver_ctx, nonce,
					     nonce_length);
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */
	}

	(void)nonce;
	(void)nonce_length;

	return PSA_ERROR_INVALID_ARGUMENT;
}

psa_status_t psa_driver_wrapper_aead_set_lengths(psa_aead_operation_t *operation, size_t ad_length,
						 size_t plaintext_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_aead_set_lengths(&operation->ctx.cracen_driver_ctx, ad_length,
						plaintext_length));
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_aead_set_lengths(&operation->ctx.cc3xx_driver_ctx, ad_length,
					      plaintext_length);
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_aead_set_lengths(&operation->ctx.oberon_driver_ctx, ad_length,
					       plaintext_length);
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */
	}

	(void)ad_length;
	(void)plaintext_length;

	return PSA_ERROR_INVALID_ARGUMENT;
}

psa_status_t psa_driver_wrapper_aead_update_ad(psa_aead_operation_t *operation,
					       const uint8_t *input, size_t input_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_aead_update_ad(&operation->ctx.cracen_driver_ctx, input,
					      input_length));
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_aead_update_ad(&operation->ctx.cc3xx_driver_ctx, input, input_length);
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_aead_update_ad(&operation->ctx.oberon_driver_ctx, input,
					     input_length);
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */
	default:
		(void)input;
		(void)input_length;

		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_aead_update(psa_aead_operation_t *operation, const uint8_t *input,
					    size_t input_length, uint8_t *output,
					    size_t output_size, size_t *output_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_aead_update(&operation->ctx.cracen_driver_ctx, input, input_length,
					   output, output_size, output_length));
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_aead_update(&operation->ctx.cc3xx_driver_ctx, input, input_length,
					 output, output_size, output_length);
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_aead_update(&operation->ctx.oberon_driver_ctx, input, input_length,
					  output, output_size, output_length);
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */

	default:
		(void)input;
		(void)input_length;
		(void)output;
		(void)output_size;
		(void)output_length;

		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_aead_finish(psa_aead_operation_t *operation, uint8_t *ciphertext,
					    size_t ciphertext_size, size_t *ciphertext_length,
					    uint8_t *tag, size_t tag_size, size_t *tag_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_aead_finish(&operation->ctx.cracen_driver_ctx, ciphertext,
					   ciphertext_size, ciphertext_length, tag, tag_size,
					   tag_length));
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_aead_finish(&operation->ctx.cc3xx_driver_ctx, ciphertext,
					 ciphertext_size, ciphertext_length, tag, tag_size,
					 tag_length);
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_aead_finish(&operation->ctx.oberon_driver_ctx, ciphertext,
					  ciphertext_size, ciphertext_length, tag, tag_size,
					  tag_length);
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */

	default:
		(void)ciphertext;
		(void)ciphertext_size;
		(void)ciphertext_length;
		(void)tag;
		(void)tag_size;
		(void)tag_length;

		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_aead_verify(psa_aead_operation_t *operation, uint8_t *plaintext,
					    size_t plaintext_size, size_t *plaintext_length,
					    const uint8_t *tag, size_t tag_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_aead_verify(&operation->ctx.cracen_driver_ctx, plaintext,
					   plaintext_size, plaintext_length, tag, tag_length));
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_aead_verify(&operation->ctx.cc3xx_driver_ctx, plaintext,
					 plaintext_size, plaintext_length, tag, tag_length);
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_aead_verify(&operation->ctx.oberon_driver_ctx, plaintext,
					  plaintext_size, plaintext_length, tag, tag_length);
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */

	default:
		(void)plaintext;
		(void)plaintext_size;
		(void)plaintext_length;
		(void)tag;
		(void)tag_length;

		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_aead_abort(psa_aead_operation_t *operation)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_AEAD_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_aead_abort(&operation->ctx.cracen_driver_ctx));
#endif /* PSA_NEED_CRACEN_AEAD_DRIVER */
#if defined(PSA_NEED_CC3XX_AEAD_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_aead_abort(&operation->ctx.cc3xx_driver_ctx);
#endif /* PSA_NEED_CC3XX_AEAD_DRIVER */
#if defined(PSA_NEED_OBERON_AEAD_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_aead_abort(&operation->ctx.oberon_driver_ctx);
#endif /* PSA_NEED_OBERON_AEAD_DRIVER */
	default:
		return PSA_SUCCESS;
	}
}

/*
 * MAC functions
 */
psa_status_t psa_driver_wrapper_mac_compute(const psa_key_attributes_t *attributes,
					    const uint8_t *key_buffer, size_t key_buffer_size,
					    psa_algorithm_t alg, const uint8_t *input,
					    size_t input_length, uint8_t *mac, size_t mac_size,
					    size_t *mac_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

#if !defined(PSA_WANT_ALG_SHA_1)
	if (PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_1) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
#endif

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_MAC_DRIVER)
		status = cracen_mac_compute(attributes, key_buffer, key_buffer_size, alg, input,
					    input_length, mac, mac_size, mac_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_MAC_DRIVER */
#if defined(PSA_NEED_CC3XX_MAC_DRIVER)
		status = cc3xx_mac_compute(attributes, key_buffer, key_buffer_size, alg, input,
					   input_length, mac, mac_size, mac_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_MAC_DRIVER */
#if defined(PSA_NEED_OBERON_MAC_DRIVER)
		return oberon_mac_compute(attributes, key_buffer, key_buffer_size, alg, input,
					    input_length, mac, mac_size, mac_length);
#endif /* PSA_NEED_OBERON_MAC_DRIVER */
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)input;
		(void)input_length;
		(void)mac;
		(void)mac_size;
		(void)mac_length;
		(void)status;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_mac_sign_setup(psa_mac_operation_t *operation,
					       const psa_key_attributes_t *attributes,
					       const uint8_t *key_buffer, size_t key_buffer_size,
					       psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

#if !defined(PSA_WANT_ALG_SHA_1)
	if (PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_1) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
#endif

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_MAC_DRIVER)
		status = cracen_mac_sign_setup(&operation->ctx.cracen_driver_ctx, attributes,
					       key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
		}
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_MAC_DRIVER */
#if defined(PSA_NEED_CC3XX_MAC_DRIVER)
		status = cc3xx_mac_sign_setup(&operation->ctx.cc3xx_driver_ctx, attributes,
					      key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CC3XX_DRIVER_ID;
		}
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_MAC_DRIVER */
#if defined(PSA_NEED_OBERON_MAC_DRIVER)
		status = oberon_mac_sign_setup(&operation->ctx.oberon_driver_ctx, attributes,
					       key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
		}
		return status;
#endif /* PSA_NEED_OBERON_MAC_DRIVER */

		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_mac_verify_setup(psa_mac_operation_t *operation,
						 const psa_key_attributes_t *attributes,
						 const uint8_t *key_buffer, size_t key_buffer_size,
						 psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

#if !defined(PSA_WANT_ALG_SHA_1)
	if (PSA_ALG_HMAC_GET_HASH(alg) == PSA_ALG_SHA_1) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
#endif

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif /* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_MAC_DRIVER)
		status = cracen_mac_verify_setup(&operation->ctx.cracen_driver_ctx, attributes,
						 key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
		}
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_MAC_DRIVER */
#if defined(PSA_NEED_CC3XX_MAC_DRIVER)
		status = cc3xx_mac_verify_setup(&operation->ctx.cc3xx_driver_ctx, attributes,
						key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CC3XX_DRIVER_ID;
		}
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_MAC_DRIVER */
#if defined(PSA_NEED_OBERON_MAC_DRIVER)
		status = oberon_mac_verify_setup(&operation->ctx.oberon_driver_ctx, attributes,
						 key_buffer, key_buffer_size, alg);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
		}
		return status;
#endif /* PSA_NEED_OBERON_MAC_DRIVER */
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)status;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_mac_update(psa_mac_operation_t *operation, const uint8_t *input,
					   size_t input_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_MAC_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_mac_update(&operation->ctx.cracen_driver_ctx, input, input_length));
#endif /* PSA_NEED_CRACEN_MAC_DRIVER */
#if defined(PSA_NEED_CC3XX_MAC_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_mac_update(&operation->ctx.cc3xx_driver_ctx, input, input_length);
#endif /* PSA_NEED_CC3XX_MAC_DRIVER */
#if defined(PSA_NEED_OBERON_MAC_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_mac_update(&operation->ctx.oberon_driver_ctx, input, input_length);
#endif /* PSA_NEED_OBERON_MAC_DRIVER */
	default:
		(void)input;
		(void)input_length;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_mac_sign_finish(psa_mac_operation_t *operation, uint8_t *mac,
						size_t mac_size, size_t *mac_length)
{
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;

	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_MAC_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_mac_sign_finish(&operation->ctx.cracen_driver_ctx, mac, mac_size,
					       mac_length));
#endif /* PSA_NEED_CRACEN_MAC_DRIVER */
#if defined(PSA_NEED_CC3XX_MAC_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		status = cc3xx_mac_sign_finish(&operation->ctx.cc3xx_driver_ctx, mac, mac_size,
					       mac_length);
		/* NCSDK-21377: Clean up operation context on success. */
		if (status == PSA_SUCCESS) {
			cc3xx_mac_abort(&operation->ctx.cc3xx_driver_ctx);
		}

		return status;
#endif /* PSA_NEED_CC3XX_MAC_DRIVER */
#if defined(PSA_NEED_OBERON_MAC_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_mac_sign_finish(&operation->ctx.oberon_driver_ctx, mac, mac_size,
					      mac_length);
#endif /* PSA_NEED_OBERON_MAC_DRIVER */
	default:
		(void)mac;
		(void)mac_size;
		(void)mac_length;
		return status;
	}
}

psa_status_t psa_driver_wrapper_mac_verify_finish(psa_mac_operation_t *operation,
						  const uint8_t *mac, size_t mac_length)
{
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;

	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_MAC_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_mac_verify_finish(&operation->ctx.cracen_driver_ctx, mac,
						 mac_length));
#endif /* PSA_NEED_CRACEN_MAC_DRIVER */
#if defined(PSA_NEED_CC3XX_MAC_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		status = cc3xx_mac_verify_finish(&operation->ctx.cc3xx_driver_ctx, mac, mac_length);
		/* NCSDK-21377: Clean up operation context on success. */
		if (status == PSA_SUCCESS) {
			cc3xx_mac_abort(&operation->ctx.cc3xx_driver_ctx);
		}

		return status;
#endif /* PSA_NEED_CC3XX_MAC_DRIVER */
#if defined(PSA_NEED_OBERON_MAC_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_mac_verify_finish(&operation->ctx.oberon_driver_ctx, mac, mac_length);
#endif /* PSA_NEED_OBERON_MAC_DRIVER */
	default:
		(void)mac;
		(void)mac_length;
		return status;
	}
}

psa_status_t psa_driver_wrapper_mac_abort(psa_mac_operation_t *operation)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_MAC_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return (cracen_mac_abort(&operation->ctx.cracen_driver_ctx));
#endif /* PSA_NEED_CRACEN_MAC_DRIVER */
#if defined(PSA_NEED_CC3XX_MAC_DRIVER)
	case PSA_CRYPTO_CC3XX_DRIVER_ID:
		return cc3xx_mac_abort(&operation->ctx.cc3xx_driver_ctx);
#endif /* PSA_NEED_CC3XX_MAC_DRIVER */
#if defined(PSA_NEED_OBERON_MAC_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_mac_abort(&operation->ctx.oberon_driver_ctx);
#endif /* PSA_NEED_OBERON_MAC_DRIVER */
	default:
		return PSA_SUCCESS;
	}
}

/*
 * Key derivation functions
 */
psa_status_t psa_driver_wrapper_key_derivation_setup(psa_key_derivation_operation_t *operation,
						     psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

#if defined(PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)
	status = cracen_key_derivation_setup(&operation->ctx.cracen_kdf_ctx, alg);
	if (status == PSA_SUCCESS) {
		operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
	}

	if (status != PSA_ERROR_NOT_SUPPORTED) {
		return status;
	}
#endif /* PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_DERIVATION_DRIVER)
	status = oberon_key_derivation_setup(&operation->ctx.oberon_kdf_ctx, alg);
	if (status == PSA_SUCCESS) {
		operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
	}
	return status;
#endif /* PSA_NEED_OBERON_KEY_DERIVATION_DRIVER */

	(void)status;
	(void)operation;
	(void)alg;
	return status;
}

psa_status_t
psa_driver_wrapper_key_derivation_set_capacity(psa_key_derivation_operation_t *operation,
					       size_t capacity)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_key_derivation_set_capacity(&operation->ctx.cracen_kdf_ctx, capacity);
#endif /* PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_key_derivation_set_capacity(&operation->ctx.oberon_kdf_ctx, capacity);
#endif /* PSA_NEED_OBERON_KEY_DERIVATION_DRIVER */

	default:
		(void)capacity;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t
psa_driver_wrapper_key_derivation_input_bytes(psa_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step, const uint8_t *data,
					      size_t data_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_key_derivation_input_bytes(&operation->ctx.cracen_kdf_ctx, step, data,
							 data_length);
#endif /* PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_key_derivation_input_bytes(&operation->ctx.oberon_kdf_ctx, step, data,
							 data_length);
#endif /* PSA_NEED_OBERON_KEY_DERIVATION_DRIVER */

	default:
		(void)step;
		(void)data;
		(void)data_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_key_derivation_input_key(psa_key_derivation_operation_t *operation,
							 psa_key_derivation_step_t step,
							 psa_key_attributes_t *attributes,
							 const uint8_t *data, size_t data_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_key_derivation_input_key(&operation->ctx.cracen_kdf_ctx, step,
						       attributes, data, data_length);
#endif /* PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_key_derivation_input_bytes(&operation->ctx.oberon_kdf_ctx, step, data,
							 data_length);
#endif /* PSA_NEED_OBERON_KEY_DERIVATION_DRIVER */

	default:
		(void)step;
		(void)data;
		(void)data_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t
psa_driver_wrapper_key_derivation_input_integer(psa_key_derivation_operation_t *operation,
						psa_key_derivation_step_t step, uint64_t value)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_key_derivation_input_integer(&operation->ctx.cracen_kdf_ctx, step,
							   value);
#endif /* PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_key_derivation_input_integer(&operation->ctx.oberon_kdf_ctx, step,
							   value);
#endif /* PSA_NEED_OBERON_KEY_DERIVATION_DRIVER */

	default:
		(void)step;
		(void)value;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t
psa_driver_wrapper_key_derivation_output_bytes(psa_key_derivation_operation_t *operation,
					       uint8_t *output, size_t output_length)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_key_derivation_output_bytes(&operation->ctx.cracen_kdf_ctx, output,
							  output_length);
#endif /* PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_key_derivation_output_bytes(&operation->ctx.oberon_kdf_ctx, output,
							  output_length);
#endif /* PSA_NEED_OBERON_KEY_DERIVATION_DRIVER */

	default:
		(void)output;
		(void)output_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_key_derivation_abort(psa_key_derivation_operation_t *operation)
{
	switch (operation->id) {
#if defined(PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_key_derivation_abort(&operation->ctx.cracen_kdf_ctx);
#endif /* PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_DERIVATION_DRIVER)
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_key_derivation_abort(&operation->ctx.oberon_kdf_ctx);
#endif /* PSA_NEED_OBERON_KEY_DERIVATION_DRIVER */

	default:
		return PSA_SUCCESS;
	}
}

/*
 * Key agreement functions
 */
psa_status_t psa_driver_wrapper_key_agreement(const psa_key_attributes_t *attributes,
					      const uint8_t *priv_key, size_t priv_key_size,
					      psa_algorithm_t alg, const uint8_t *publ_key,
					      size_t publ_key_size, uint8_t *output,
					      size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_KEY_AGREEMENT_DRIVER) && defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
#endif
#if defined(PSA_CRYPTO_DRIVER_IRONSIDE)
		status = ironside_psa_key_agreement(attributes, priv_key, priv_key_size, alg,
						    publ_key, publ_key_size, output, output_size,
						    output_length);
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */
#if defined(PSA_NEED_CRACEN_KEY_AGREEMENT_DRIVER)
		status = cracen_key_agreement(attributes, priv_key, priv_key_size, publ_key,
					      publ_key_size, output, output_size, output_length,
					      alg);
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_KEY_AGREEMENT_DRIVER */
#if defined(PSA_NEED_CC3XX_KEY_AGREEMENT_DRIVER)
		status =
			cc3xx_key_agreement(attributes, priv_key, priv_key_size, publ_key,
					    publ_key_size, output, output_size, output_length, alg);
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_KEY_AGREEMENT_DRIVER */
#if defined(PSA_NEED_OBERON_KEY_AGREEMENT_DRIVER)
		return oberon_key_agreement(attributes, priv_key, priv_key_size, alg, publ_key,
					      publ_key_size, output, output_size, output_length);
#endif /* PSA_NEED_OBERON_KEY_AGREEMENT_DRIVER */
		(void)status;
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)priv_key;
		(void)priv_key_size;
		(void)publ_key;
		(void)publ_key_size;
		(void)output;
		(void)output_size;
		(void)output_length;
		(void)alg;

		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

#if defined(CONFIG_PSA_CORE_OBERON)

/*
 * Key encapsulation functions.
 */
psa_status_t psa_driver_wrapper_encapsulate(const psa_key_attributes_t *attributes,
					    const uint8_t *key, size_t key_length,
					    psa_algorithm_t alg,
					    const psa_key_attributes_t *output_attributes,
					    uint8_t *output_key, size_t output_key_size,
					    size_t *output_key_length, uint8_t *ciphertext,
					    size_t ciphertext_size, size_t *ciphertext_length)
{
	psa_status_t status;
	(void)status;

	switch (PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime)) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
		/* Add cases for transparent drivers here */
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
		status = ironside_psa_key_encapsulate(
			attributes, key, key_length, alg, output_attributes, output_key,
			output_key_size, output_key_length, ciphertext, ciphertext_size,
			ciphertext_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */
#ifdef PSA_NEED_OBERON_KEY_ENCAPSULATION_DRIVER
		return oberon_key_encapsulate(attributes, key, key_length, alg, output_attributes,
					      output_key, output_key_size, output_key_length,
					      ciphertext, ciphertext_size, ciphertext_length);
#endif /* PSA_NEED_OBERON_KEY_ENCAPSULATION_DRIVER */
		return PSA_ERROR_NOT_SUPPORTED;

	default:
		/* Key is declared with a lifetime not known to us */
		(void)key;
		(void)key_length;
		(void)alg;
		(void)output_attributes;
		(void)output_key;
		(void)output_key_size;
		(void)output_key_length;
		(void)ciphertext;
		(void)ciphertext_size;
		(void)ciphertext_length;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_decapsulate(const psa_key_attributes_t *attributes,
					    const uint8_t *key, size_t key_length,
					    psa_algorithm_t alg, const uint8_t *ciphertext,
					    size_t ciphertext_length,
					    const psa_key_attributes_t *output_attributes,
					    uint8_t *output_key, size_t output_key_size,
					    size_t *output_key_length)
{
	psa_status_t status;
	(void)status;

	switch (PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime)) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
		/* Add cases for transparent drivers here */
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
		status = ironside_psa_key_decapsulate(
			attributes, key, key_length, alg, ciphertext, ciphertext_length,
			output_attributes, output_key, output_key_size, output_key_length);
		/* Declared with fallback == true */
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */
#ifdef PSA_NEED_OBERON_KEY_ENCAPSULATION_DRIVER
		return oberon_key_decapsulate(attributes, key, key_length, alg, ciphertext,
					      ciphertext_length, output_attributes, output_key,
					      output_key_size, output_key_length);
#endif /* PSA_NEED_OBERON_KEY_ENCAPSULATION_DRIVER */
		return PSA_ERROR_NOT_SUPPORTED;

	default:
		/* Key is declared with a lifetime not known to us */
		(void)key;
		(void)key_length;
		(void)alg;
		(void)ciphertext;
		(void)ciphertext_length;
		(void)output_attributes;
		(void)output_key;
		(void)output_key_size;
		(void)output_key_length;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

/*
 * PAKE functions.
 *
 * These APIs are not standardized and should be considered experimental.
 */
psa_status_t psa_driver_wrapper_pake_setup(psa_pake_operation_t *operation,
					   const psa_key_attributes_t *attributes,
					   const uint8_t *password, size_t password_length,
					   const psa_pake_cipher_suite_t *cipher_suite)
{
	psa_status_t status;
	switch (PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime)) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
		/* Add cases for transparent drivers here */
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
		status = ironside_psa_pake_setup(&operation->ctx.ironside_pake_ctx, attributes,
						 password, password_length, cipher_suite);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_IRONSIDE_DRIVER_ID;
		}
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */

#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
		status = cracen_pake_setup(&operation->ctx.cracen_pake_ctx, attributes, password,
					   password_length, cipher_suite);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_CRACEN_DRIVER_ID;
		}
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_PAKE_DRIVER */

#ifdef PSA_NEED_OBERON_PAKE_DRIVER
		status = oberon_pake_setup(&operation->ctx.oberon_pake_ctx, attributes, password,
					   password_length, cipher_suite);
		if (status == PSA_SUCCESS) {
			operation->id = PSA_CRYPTO_OBERON_DRIVER_ID;
		}
		return status;
#endif /* PSA_NEED_OBERON_PAKE_DRIVER */

		return PSA_ERROR_NOT_SUPPORTED;

		/* Add cases for opaque driver here */

	default:
		(void)status;
		(void)operation;
		(void)password;
		(void)password_length;
		(void)cipher_suite;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_pake_set_role(psa_pake_operation_t *operation, psa_pake_role_t role)
{
	switch (operation->id) {
#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_pake_set_role(&operation->ctx.cracen_pake_ctx, role);
#endif /* PSA_NEED_CRACEN_PAKE_DRIVER */
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
	case PSA_CRYPTO_IRONSIDE_DRIVER_ID:
		return ironside_psa_pake_set_role(&operation->ctx.ironside_pake_ctx, role);
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_pake_set_role(&operation->ctx.oberon_pake_ctx, role);
#endif /* PSA_NEED_OBERON_PAKE_DRIVER */

	default:
		(void)role;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_pake_set_user(psa_pake_operation_t *operation,
					      const uint8_t *user_id, size_t user_id_length)
{
	switch (operation->id) {
#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_pake_set_user(&operation->ctx.cracen_pake_ctx, user_id,
					    user_id_length);
#endif /* PSA_NEED_CRACEN_PAKE_DRIVER */
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
	case PSA_CRYPTO_IRONSIDE_DRIVER_ID:
		return ironside_psa_pake_set_user(&operation->ctx.ironside_pake_ctx, user_id,
						  user_id_length);
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_pake_set_user(&operation->ctx.oberon_pake_ctx, user_id,
					    user_id_length);
#endif /* PSA_NEED_OBERON_PAKE_DRIVER */

	default:
		(void)user_id;
		(void)user_id_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_pake_set_peer(psa_pake_operation_t *operation,
					      const uint8_t *peer_id, size_t peer_id_length)
{
	switch (operation->id) {
#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_pake_set_peer(&operation->ctx.cracen_pake_ctx, peer_id,
					    peer_id_length);
#endif
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
	case PSA_CRYPTO_IRONSIDE_DRIVER_ID:
		return ironside_psa_pake_set_peer(&operation->ctx.ironside_pake_ctx, peer_id,
						  peer_id_length);
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_pake_set_peer(&operation->ctx.oberon_pake_ctx, peer_id,
					    peer_id_length);
#endif /* PSA_NEED_OBERON_PAKE_DRIVER */

	default:
		(void)peer_id;
		(void)peer_id_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_pake_set_context(psa_pake_operation_t *operation,
						 const uint8_t *context, size_t context_length)
{
	switch (operation->id) {
#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_pake_set_context(&operation->ctx.cracen_pake_ctx, context,
					       context_length);
#endif
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
	case PSA_CRYPTO_IRONSIDE_DRIVER_ID:
		return ironside_psa_pake_set_context(&operation->ctx.ironside_pake_ctx, context,
						     context_length);
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_pake_set_context(&operation->ctx.oberon_pake_ctx, context,
					       context_length);
#endif /* PSA_NEED_OBERON_PAKE_DRIVER */

	default:
		(void)context;
		(void)context_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_pake_output(psa_pake_operation_t *operation, psa_pake_step_t step,
					    uint8_t *output, size_t output_size,
					    size_t *output_length)
{
	switch (operation->id) {
#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_pake_output(&operation->ctx.cracen_pake_ctx, step, output,
					  output_size, output_length);
#endif
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
	case PSA_CRYPTO_IRONSIDE_DRIVER_ID:
		return ironside_psa_pake_output(&operation->ctx.ironside_pake_ctx, step, output,
						output_size, output_length);
#endif
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_pake_output(&operation->ctx.oberon_pake_ctx, step, output,
					  output_size, output_length);
#endif

	default:
		(void)step;
		(void)output;
		(void)output_size;
		(void)output_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_pake_input(psa_pake_operation_t *operation, psa_pake_step_t step,
					   const uint8_t *input, size_t input_length)
{
	switch (operation->id) {
#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_pake_input(&operation->ctx.cracen_pake_ctx, step, input,
					 input_length);
#endif
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
	case PSA_CRYPTO_IRONSIDE_DRIVER_ID:
		return ironside_psa_pake_input(&operation->ctx.ironside_pake_ctx, step, input,
					       input_length);
#endif
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_pake_input(&operation->ctx.oberon_pake_ctx, step, input,
					 input_length);
#endif

	default:
		(void)step;
		(void)input;
		(void)input_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_pake_get_shared_key(psa_pake_operation_t *operation,
						    const psa_key_attributes_t *attributes,
						    uint8_t *key_buffer, size_t key_buffer_size,
						    size_t *key_buffer_length)
{
	switch (operation->id) {
#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_pake_get_shared_key(&operation->ctx.cracen_pake_ctx, attributes,
						  key_buffer, key_buffer_size, key_buffer_length);
#endif
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
	case PSA_CRYPTO_IRONSIDE_DRIVER_ID:
		return ironside_psa_pake_get_shared_key(&operation->ctx.ironside_pake_ctx,
							attributes, key_buffer, key_buffer_size,
							key_buffer_length);
#endif
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_pake_get_shared_key(&operation->ctx.oberon_pake_ctx, attributes,
						  key_buffer, key_buffer_size, key_buffer_length);
#endif

	default:
		(void)attributes;
		(void)key_buffer;
		(void)key_buffer_size;
		(void)key_buffer_length;
		return PSA_ERROR_BAD_STATE;
	}
}

psa_status_t psa_driver_wrapper_pake_abort(psa_pake_operation_t *operation)
{
	switch (operation->id) {
#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
	case PSA_CRYPTO_CRACEN_DRIVER_ID:
		return cracen_pake_abort(&operation->ctx.cracen_pake_ctx);
#endif
#ifdef PSA_CRYPTO_DRIVER_IRONSIDE
	case PSA_CRYPTO_IRONSIDE_DRIVER_ID:
		return ironside_psa_pake_abort(&operation->ctx.ironside_pake_ctx);
#endif /* PSA_CRYPTO_DRIVER_IRONSIDE */
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
	case PSA_CRYPTO_OBERON_DRIVER_ID:
		return oberon_pake_abort(&operation->ctx.oberon_pake_ctx);
#endif /* PSA_NEED_OBERON_PAKE_DRIVER */

	default:
		return PSA_SUCCESS;
	}
}

#endif /* defined(CONFIG_PSA_CORE_OBERON) */

/*
 * Asymmetric operations
 */
psa_status_t psa_driver_wrapper_asymmetric_encrypt(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length, const uint8_t *salt,
	size_t salt_length, uint8_t *output, size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER)
		status = cracen_asymmetric_encrypt(attributes, key_buffer, key_buffer_size, alg,
						   input, input_length, salt, salt_length, output,
						   output_size, output_length);
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER */
#if defined(PSA_NEED_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER)
		status = cc3xx_asymmetric_encrypt(attributes, key_buffer, key_buffer_size, alg,
						  input, input_length, salt, salt_length, output,
						  output_size, output_length);
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER */
#if defined(PSA_NEED_OBERON_ASYMMETRIC_ENCRYPTION_DRIVER)
		return oberon_asymmetric_encrypt(attributes, key_buffer, key_buffer_size, alg,
						   input, input_length, salt, salt_length, output,
						   output_size, output_length);
#endif /* PSA_NEED_OBERON_ASYMMETRIC_ENCRYPTION_DRIVER */
		(void)status;
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)input;
		(void)input_length;
		(void)salt;
		(void)salt_length;
		(void)output;
		(void)output_size;
		(void)output_length;

		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_asymmetric_decrypt(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length, const uint8_t *salt,
	size_t salt_length, uint8_t *output, size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#if defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER)
	case TFM_BUILTIN_KEY_LOADER_KEY_LOCATION:
#endif		/* defined(PSA_CRYPTO_DRIVER_TFM_BUILTIN_KEY_LOADER) */
		/* Key is stored in the slot in export representation, so
		 * cycle through all known transparent accelerators
		 */
#if defined(PSA_NEED_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER)
		status = cracen_asymmetric_decrypt(attributes, key_buffer, key_buffer_size, alg,
						   input, input_length, salt, salt_length, output,
						   output_size, output_length);
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER */
#if defined(PSA_NEED_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER)
		status = cc3xx_asymmetric_decrypt(attributes, key_buffer, key_buffer_size, alg,
						  input, input_length, salt, salt_length, output,
						  output_size, output_length);
		if (status != PSA_ERROR_NOT_SUPPORTED) {
			return status;
		}
#endif /* PSA_NEED_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER */
#if defined(PSA_NEED_OBERON_ASYMMETRIC_ENCRYPTION_DRIVER)
		return oberon_asymmetric_decrypt(attributes, key_buffer, key_buffer_size, alg,
						   input, input_length, salt, salt_length, output,
						   output_size, output_length);
#endif /* PSA_NEED_OBERON_ASYMMETRIC_ENCRYPTION_DRIVER */
		(void)status;
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		/* Key is declared with a lifetime not known to us */
		(void)key_buffer;
		(void)key_buffer_size;
		(void)alg;
		(void)input;
		(void)input_length;
		(void)salt;
		(void)salt_length;
		(void)output;
		(void)output_size;
		(void)output_length;

		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_init_random(psa_driver_random_context_t *context)
{
#if defined(PSA_NEED_CRACEN_CTR_DRBG_DRIVER)
	return cracen_init_random(NULL);
#elif defined(PSA_NEED_OBERON_CTR_DRBG_DRIVER)
	return oberon_ctr_drbg_init(&context->oberon_ctr_drbg_ctx);
#elif defined(PSA_NEED_OBERON_HMAC_DRBG_DRIVER)
	return oberon_hmac_drbg_init(&context->oberon_hmac_drbg_ctx);
#else
	/* When the chosen driver does not require to initialize the context
	 * or the get_random call is not supported we can return success.
	 */
	(void)context;
	return PSA_SUCCESS;
#endif
}

/*
 * Key wrapping functions.
 */
psa_status_t psa_driver_wrapper_wrap_key(const psa_key_attributes_t *wrapping_key_attributes,
					 const uint8_t *wrapping_key_data, size_t wrapping_key_size,
					 psa_algorithm_t alg,
					 const psa_key_attributes_t *key_attributes,
					 const uint8_t *key_data, size_t key_size, uint8_t *data,
					 size_t data_size, size_t *data_length)
{
	switch (PSA_KEY_LIFETIME_GET_LOCATION(wrapping_key_attributes->lifetime)) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#ifdef PSA_NEED_CRACEN_KEY_WRAP_DRIVER
		return cracen_wrap_key(wrapping_key_attributes, wrapping_key_data,
				       wrapping_key_size, alg, key_attributes, key_data, key_size,
				       data, data_size, data_length);
#endif /* PSA_NEED_CRACEN_KEY_WRAP_DRIVER */

#ifdef PSA_NEED_OBERON_KEY_WRAP_DRIVER
		return oberon_wrap_key(wrapping_key_attributes, wrapping_key_data,
				       wrapping_key_size, alg, key_attributes, key_data, key_size,
				       data, data_size, data_length);
#endif /* PSA_NEED_OBERON_KEY_WRAP_DRIVER */
		return PSA_ERROR_NOT_SUPPORTED;

		/* Add cases for opaque drivers here */

	default:
		/* Key is declared with a lifetime not known to us */
		(void)key_attributes;
		(void)key_data;
		(void)key_size;
		(void)wrapping_key_data;
		(void)wrapping_key_size;
		(void)alg;
		(void)data;
		(void)data_size;
		(void)data_length;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_unwrap_key(const psa_key_attributes_t *attributes,
					   const psa_key_attributes_t *wrapping_key_attributes,
					   const uint8_t *wrapping_key_data,
					   size_t wrapping_key_size, psa_algorithm_t alg,
					   const uint8_t *data, size_t data_length, uint8_t *key,
					   size_t key_size, size_t *key_length)
{
	switch (PSA_KEY_LIFETIME_GET_LOCATION(wrapping_key_attributes->lifetime)) {
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
#ifdef PSA_NEED_CRACEN_KEY_WRAP_DRIVER
		return cracen_unwrap_key(attributes, wrapping_key_attributes, wrapping_key_data,
					 wrapping_key_size, alg, data, data_length, key, key_size,
					 key_length);
#endif /* PSA_NEED_CRACEN_KEY_WRAP_DRIVER */

#ifdef PSA_NEED_OBERON_KEY_WRAP_DRIVER
		return oberon_unwrap_key(attributes, wrapping_key_attributes, wrapping_key_data,
					 wrapping_key_size, alg, data, data_length, key, key_size,
					 key_length);
#endif /* PSA_NEED_OBERON_KEY_WRAP_DRIVER */
		return PSA_ERROR_NOT_SUPPORTED;

		/* Add cases for opaque drivers here */

	default:
		/* Key is declared with a lifetime not known to us */
		(void)attributes;
		(void)key_size;
		(void)wrapping_key_data;
		(void)wrapping_key_size;
		(void)alg;
		(void)data;
		(void)data_length;
		(void)key;
		(void)key_size;
		(void)key_length;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t psa_driver_wrapper_get_random(psa_driver_random_context_t *context, uint8_t *output,
					   size_t output_size)
{
#if defined(PSA_CRYPTO_DRIVER_ALG_PRNG_TEST)
	psa_status_t status;
	(void)context;

	status = prng_test_generate_random(output, output_size);
	if (status != PSA_ERROR_NOT_SUPPORTED) {
		return status;
	}
#endif

#if defined(PSA_NEED_CRACEN_CTR_DRBG_DRIVER)
	return cracen_get_random(NULL, output, output_size);
#elif defined(PSA_NEED_OBERON_CTR_DRBG_DRIVER)
	return oberon_ctr_drbg_get_random(&context->oberon_ctr_drbg_ctx, output, output_size);
#elif defined(PSA_NEED_OBERON_HMAC_DRBG_DRIVER)
	return oberon_hmac_drbg_get_random(&context->oberon_hmac_drbg_ctx, output, output_size);
#elif defined(PSA_NEED_CC3XX_CTR_DRBG_DRIVER) || defined(PSA_NEED_CC3XX_HMAC_DRBG_DRIVER)
	size_t output_length;
	int err;

	/* Using internal context. */
	(void)context;

#if defined(PSA_NEED_CC3XX_CTR_DRBG_DRIVER)
	err = nrf_cc3xx_platform_ctr_drbg_get(NULL, output, output_size, &output_length);
#elif defined(PSA_NEED_CC3XX_HMAC_DRBG_DRIVER)
	err = nrf_cc3xx_platform_hmac_drbg_get(NULL, output, output_size, &output_length);
#endif
	if (err != 0) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	if (output_size != output_length) {
		return PSA_ERROR_INSUFFICIENT_ENTROPY;
	}

	return PSA_SUCCESS;
#endif

	(void)context;
	(void)output;
	(void)output_size;
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_driver_wrapper_free_random(psa_driver_random_context_t *context)
{
#if defined(PSA_NEED_CRACEN_CTR_DRBG_DRIVER)
	return cracen_free_random(NULL);
#elif defined(PSA_NEED_OBERON_CTR_DRBG_DRIVER)
	return oberon_ctr_drbg_free(&context->oberon_ctr_drbg_ctx);
#elif defined(PSA_NEED_OBERON_HMAC_DRBG_DRIVER)
	return oberon_hmac_drbg_free(&context->oberon_hmac_drbg_ctx);
#endif
	/* When the chosen driver does not require to initialize the context
	 * or the get_random call is not supported we can return success.
	 */
	(void)context;
	return PSA_SUCCESS;
}

psa_status_t psa_driver_wrapper_get_entropy(uint32_t flags, size_t *estimate_bits, uint8_t *output,
					    size_t output_size)
{
#if defined(PSA_NEED_CRACEN_TRNG_DRIVER)
	return cracen_trng_get_entropy(flags, estimate_bits, output, output_size);
#elif defined(PSA_NEED_NRF_RNG_ENTROPY_DRIVER)
	return nrf_rng_get_entropy(flags, estimate_bits, output, output_size);
#endif

	(void)flags;
	(void)output;
	(void)output_size;
	*estimate_bits = 0;
	return PSA_ERROR_INSUFFICIENT_ENTROPY;
}

psa_status_t psa_driver_wrapper_destroy_builtin_key(const psa_key_attributes_t *attributes)
{
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(attributes->lifetime);

	switch (location) {
#if defined(PSA_CRYPTO_DRIVER_IRONSIDE)
	case PSA_KEY_LOCATION_LOCAL_STORAGE:
		return ironside_psa_destroy_builtin_key(attributes);
#endif
#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
	case PSA_KEY_LOCATION_CRACEN_KMU:
		return cracen_destroy_key(attributes);
#endif
	}

	return PSA_ERROR_NOT_SUPPORTED;
}
#endif /* MBEDTLS_PSA_CRYPTO_C */
