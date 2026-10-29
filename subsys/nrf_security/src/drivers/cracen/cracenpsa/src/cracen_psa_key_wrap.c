/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_key_wrap.h>
#include <cracen_psa_primitives.h>

#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#include <internal/key_wrap/cracen_key_wrap_kw.h>
#include <internal/key_wrap/cracen_key_wrap_kwp.h>

psa_status_t cracen_wrap_key(const psa_key_attributes_t *wrapping_key_attributes,
			     const uint8_t *wrapping_key_data, size_t wrapping_key_size,
			     psa_algorithm_t alg,
			     const psa_key_attributes_t *key_attributes,
			     const uint8_t *key_data, size_t key_size,
			     uint8_t *data, size_t data_size, size_t *data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (IS_ENABLED(PSA_NEED_CRACEN_AES_KW) && alg == PSA_ALG_KW) {
		status = cracen_key_wrap_kw_wrap(wrapping_key_attributes, wrapping_key_data,
						 wrapping_key_size, key_data, key_size, data,
						 data_size, data_length);

	} else if (IS_ENABLED(CONFIG_PSA_NEED_CRACEN_AES_KWP) && alg == PSA_ALG_KWP) {
		status = cracen_key_wrap_kwp_wrap(wrapping_key_attributes, wrapping_key_data,
						  wrapping_key_size, key_data, key_size, data,
						  data_size, data_length);
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
	}

	return status;
}

psa_status_t cracen_unwrap_key(const psa_key_attributes_t *attributes,
			       const psa_key_attributes_t *wrapping_key_attributes,
			       const uint8_t *wrapping_key_data, size_t wrapping_key_size,
			       psa_algorithm_t alg, const uint8_t *data, size_t data_length,
			       uint8_t *key, size_t key_size, size_t *key_length, size_t *bits)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (IS_ENABLED(PSA_NEED_CRACEN_AES_KW) && alg == PSA_ALG_KW) {
		status = cracen_key_wrap_kw_unwrap(wrapping_key_attributes, wrapping_key_data,
						   wrapping_key_size, data, data_length, key,
						   key_size, key_length, bits);

	} else if (IS_ENABLED(CONFIG_PSA_NEED_CRACEN_AES_KWP) && alg == PSA_ALG_KWP) {
		status = cracen_key_wrap_kwp_unwrap(wrapping_key_attributes, wrapping_key_data,
						    wrapping_key_size, data, data_length, key,
						    key_size, key_length, bits);
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
	}

	if (status == PSA_SUCCESS && PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
		psa_key_type_t key_type = psa_get_key_type(attributes);
		psa_ecc_family_t family = PSA_KEY_TYPE_ECC_GET_FAMILY(key_type);
		size_t key_bytes = *key_length;

		/* Weierstrass public keys are 0x04 || x || y. */
		if (PSA_KEY_TYPE_IS_PUBLIC_KEY(key_type) && PSA_ECC_FAMILY_IS_WEIERSTRASS(family)) {
			if (key_bytes % 2 == 0 || key[0] != 0x04) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			key_bytes /= 2;
		}

		*bits = PSA_BYTES_TO_BITS(key_bytes);
		if (family == PSA_ECC_FAMILY_SECP_R1 && key_bytes == 66) {
			*bits = 521;
		} else if ((family == PSA_ECC_FAMILY_MONTGOMERY ||
			    family == PSA_ECC_FAMILY_TWISTED_EDWARDS) &&
			   key_bytes == 32) {
			*bits = 255;
		} else if (family == PSA_ECC_FAMILY_TWISTED_EDWARDS && key_bytes == 57) {
			*bits = 448;
		} else {
			/* For compliance */
		}
	}

	return status;
}
