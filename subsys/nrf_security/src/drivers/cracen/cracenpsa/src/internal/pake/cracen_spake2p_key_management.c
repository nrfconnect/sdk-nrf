/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/pake/cracen_spake2p_key_management.h>
#include <internal/ecc/cracen_ecc_helpers.h>
#include <internal/ecc/cracen_ecc_keygen.h>

#include <string.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>

psa_status_t import_spake2p_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				size_t data_length, uint8_t *key_buffer,
				size_t key_buffer_size, size_t *key_buffer_length,
				size_t *key_bits)
{
	size_t bits = psa_get_key_bits(attributes);
	psa_key_type_t type = psa_get_key_type(attributes);

	/* Check for invalid key bits*/
	if (bits != 0 && (bits != 256)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (key_buffer_size < data_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* We only support 256 bit keys and they PSA APIs does not enforce setting the key bits. */
	bits = 256;

	switch (type) {
	case PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
		/* These keys contains w0 and w1. */
		if (data_length != CRACEN_P256_KEY_SIZE * 2) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		/* Do not allow w0 to be 0. */
		if (!memcpy_check_non_zero(key_buffer, key_buffer_size, data,
					   CRACEN_P256_KEY_SIZE)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		/* Do not allow w1 to be 0. */
		if (!memcpy_check_non_zero(key_buffer + CRACEN_P256_KEY_SIZE,
					   key_buffer_size - CRACEN_P256_KEY_SIZE,
					   data + CRACEN_P256_KEY_SIZE, CRACEN_P256_KEY_SIZE)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		/* We don't check if the keys are in range. This will generate an error on usage. */
		break;
	case PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1):
		/* These keys contains w0 and L. L is an uncompressed point. */
		if (data_length != CRACEN_P256_KEY_SIZE + CRACEN_P256_POINT_SIZE + 1) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		/* Do not allow w0 to be 0. */
		if (!memcpy_check_non_zero(key_buffer, key_buffer_size, data,
					   CRACEN_P256_KEY_SIZE)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		/* Validate L */
		uint8_t L[CRACEN_P256_POINT_SIZE + 1];

		memcpy(L, &data[CRACEN_P256_KEY_SIZE], sizeof(L));
		if (check_wstr_pub_key_data(PSA_ALG_ECDH, PSA_ECC_FAMILY_SECP_R1, bits, L,
					    sizeof(L))) {
			safe_memzero(L, sizeof(L));
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		memcpy(key_buffer + CRACEN_P256_KEY_SIZE, L, sizeof(L));

		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*key_buffer_length = data_length;
	*key_bits = bits;

	return PSA_SUCCESS;
}

psa_status_t cracen_derive_spake2p_key(const psa_key_attributes_t *attributes,
				       const uint8_t *input, size_t input_length,
				       uint8_t *key, size_t key_size, size_t *key_length)
{
	size_t bits = psa_get_key_bits(attributes);
	psa_key_type_t type = psa_get_key_type(attributes);
	psa_status_t status;

	switch (type) {
	case PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):

		if (bits != 256) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		if (input_length != 80) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		/* Described in section 3.2 of rfc9383, split the output of the PBKDF2
		 * into two parts, treat each part as integer and perform a modulo operation
		 * on each half.
		 */
		status = cracen_ecc_reduce_p256(input, input_length / 2, key, CRACEN_P256_KEY_SIZE);
		if (status != PSA_SUCCESS) {
			return status;
		}

		if (constant_memcmp_is_zero(key, CRACEN_P256_KEY_SIZE)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		status = cracen_ecc_reduce_p256(input + (input_length / 2), input_length / 2,
						key + CRACEN_P256_KEY_SIZE, CRACEN_P256_KEY_SIZE);
		if (status != PSA_SUCCESS) {
			return status;
		}

		if (constant_memcmp_is_zero(key + CRACEN_P256_KEY_SIZE, CRACEN_P256_KEY_SIZE)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		*key_length = 2 * CRACEN_P256_KEY_SIZE;
		return PSA_SUCCESS;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t export_spake2p_public_key_from_keypair(const psa_key_attributes_t *attributes,
						    const uint8_t *priv_key,
						    size_t priv_key_length, uint8_t *pub_key,
						    size_t pub_key_size,
						    size_t *pub_key_length)
{
	psa_status_t status;
	psa_key_type_t type = psa_get_key_type(attributes);
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_ecc_family_t psa_curve = PSA_KEY_TYPE_SPAKE2P_GET_FAMILY(psa_get_key_type(attributes));
	const struct sx_pk_ecurve *sx_curve;
	const uint8_t *w0;
	const uint8_t *w1;

	switch (type) {
	case PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
		w0 = priv_key;
		w1 = priv_key + CRACEN_P256_KEY_SIZE;
		status = cracen_ecc_get_ecurve_from_psa(psa_curve, key_bits_attr, &sx_curve);
		if (status != PSA_SUCCESS) {
			return status;
		}

		if (key_bits_attr != 256) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		if (pub_key_size < CRACEN_P256_KEY_SIZE + CRACEN_P256_POINT_SIZE + 1) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}
		if (priv_key_length != 2 * CRACEN_P256_KEY_SIZE) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		pub_key[CRACEN_P256_KEY_SIZE] = CRACEN_ECC_PUBKEY_UNCOMPRESSED;
		status = silex_statuscodes_to_psa(
			ecc_genpubkey(w1, &pub_key[CRACEN_P256_KEY_SIZE + 1], sx_curve));
		if (status != PSA_SUCCESS) {
			return status;
		}
		memcpy(pub_key, w0, CRACEN_P256_KEY_SIZE);
		*pub_key_length = CRACEN_P256_KEY_SIZE + CRACEN_P256_POINT_SIZE + 1;
		return PSA_SUCCESS;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}
