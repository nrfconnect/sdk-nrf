/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/ecc/cracen_ecc_key_management.h>
#include <internal/ecc/cracen_ecc_keygen.h>
#include <internal/ecc/cracen_ecc_helpers.h>
#include <internal/ecc/cracen_montgomery.h>
#include <internal/ecc/cracen_eddsa.h>
#include <internal/ikg/cracen_ikg_operations.h>

#include <string.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen/ec_helpers.h>

#define DEFAULT_KEY_SIZE(bits) (bits), PSA_BITS_TO_BYTES(bits), (1 + 2 * PSA_BITS_TO_BYTES(bits))
static struct {
	psa_ecc_family_t family;
	size_t bits;
	size_t private_key_size_bytes;
	size_t public_key_size_bytes;
	bool supported;
} valid_keys[] = {
	/* Brainpool */
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(192),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(224),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(256),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(320),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(384),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},
	{PSA_ECC_FAMILY_BRAINPOOL_P_R1, DEFAULT_KEY_SIZE(512),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)},

	/* SECP k1 */
	{PSA_ECC_FAMILY_SECP_K1, DEFAULT_KEY_SIZE(192),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1)},
	{PSA_ECC_FAMILY_SECP_K1, DEFAULT_KEY_SIZE(225), false}, /* NCSDK-21311 */
	{PSA_ECC_FAMILY_SECP_K1, DEFAULT_KEY_SIZE(256),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1)},

	/* SECP r1 */
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(192),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(224),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(256),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(320),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(384),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},
	{PSA_ECC_FAMILY_SECP_R1, DEFAULT_KEY_SIZE(521),
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1)},

	/* Montgomery */
	{PSA_ECC_FAMILY_MONTGOMERY, 255, 32, 32,
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY)},
	{PSA_ECC_FAMILY_MONTGOMERY, 448, 56, 56,
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY)},

	/* Twisted Edwards */
	{PSA_ECC_FAMILY_TWISTED_EDWARDS, 255, 32, 32,
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_TWISTED_EDWARDS)},
	{PSA_ECC_FAMILY_TWISTED_EDWARDS, 448, 57, 57,
	 IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_TWISTED_EDWARDS)},
};

static psa_status_t check_ecc_key_attributes(const psa_key_attributes_t *attributes,
					     size_t key_buffer_size, size_t *key_bits)
{
	psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	psa_algorithm_t key_alg = psa_get_key_algorithm(attributes);
	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	bool is_public_key = PSA_KEY_TYPE_IS_PUBLIC_KEY(psa_get_key_type(attributes));

	for (size_t i = 0; i < ARRAY_SIZE(valid_keys); i++) {
		if (valid_keys[i].family == curve) {
			size_t valid_key_size = is_public_key
							? valid_keys[i].public_key_size_bytes
							: valid_keys[i].private_key_size_bytes;

			if (*key_bits == 0 && valid_key_size == key_buffer_size) {
				*key_bits = valid_keys[i].bits;
			}
			if (*key_bits == valid_keys[i].bits) {
				if (valid_key_size != key_buffer_size) {
					return PSA_ERROR_INVALID_ARGUMENT;
				}
				status = valid_keys[i].supported ? PSA_SUCCESS
								 : PSA_ERROR_NOT_SUPPORTED;
				break;
			}
		}
	}

	if (status == PSA_SUCCESS &&
	    (curve == PSA_ECC_FAMILY_TWISTED_EDWARDS) && (key_alg != PSA_ALG_PURE_EDDSA &&
	    key_alg != PSA_ALG_ED25519PH && key_alg != PSA_ALG_ED448PH)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return status;
}

psa_status_t generate_ecc_private_key(const psa_key_attributes_t *attributes,
				      uint8_t *key_buffer, size_t key_buffer_size,
				      size_t *key_buffer_length)
{
#if PSA_VENDOR_ECC_MAX_CURVE_BITS > 0
	size_t key_bits_attr = psa_get_key_bits(attributes);
	size_t key_size_bytes = PSA_BITS_TO_BYTES(key_bits_attr);
	psa_ecc_family_t psa_curve = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	psa_status_t psa_status;
	int sx_status;
	const struct sx_pk_ecurve *sx_curve;
	uint8_t workmem[PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(PSA_VENDOR_ECC_MAX_CURVE_BITS)] = {};

	*key_buffer_length = 0;

	if (key_size_bytes > key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (key_size_bytes > sizeof(workmem)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	psa_status = check_ecc_key_attributes(attributes, key_buffer_size, &key_bits_attr);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	psa_status = cracen_ecc_get_ecurve_from_psa(psa_curve, key_bits_attr, &sx_curve);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	if (psa_curve == PSA_ECC_FAMILY_MONTGOMERY || psa_curve == PSA_ECC_FAMILY_TWISTED_EDWARDS) {
		psa_status = cracen_get_random(NULL, workmem, key_size_bytes);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}

		if (constant_memcmp_is_zero(workmem, key_size_bytes)) {
			return PSA_ERROR_INSUFFICIENT_ENTROPY;
		}

		if (psa_curve == PSA_ECC_FAMILY_MONTGOMERY) {
			/* For X448 and X25519 we must clamp the private keys.
			 */
			if (key_size_bytes == 32) {
				/* X25519 */
				decode_scalar_25519(&workmem[0]);
			} else if (key_size_bytes == 56) {
				/* X448 */
				decode_scalar_448(&workmem[0]);
			}
		}

		memcpy(key_buffer, workmem, key_size_bytes);
	} else {

		sx_status = ecc_genprivkey(sx_curve, key_buffer, key_buffer_size);
		if (sx_status != SX_OK) {
			return silex_statuscodes_to_psa(sx_status);
		}
	}

	safe_memzero(workmem, sizeof(workmem));
	*key_buffer_length = key_size_bytes;

	return PSA_SUCCESS;
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_VENDOR_ECC_MAX_CURVE_BITS > 0 */
}

static size_t get_expected_pub_key_size(psa_ecc_family_t psa_curve, size_t key_bits_attr)
{
	switch (psa_curve) {
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
	case PSA_ECC_FAMILY_SECP_R1:
	case PSA_ECC_FAMILY_SECP_K1:
		return cracen_ecc_wstr_expected_pub_key_bytes(PSA_BITS_TO_BYTES(key_bits_attr));
	case PSA_ECC_FAMILY_MONTGOMERY:
	case PSA_ECC_FAMILY_TWISTED_EDWARDS:
		return PSA_BITS_TO_BYTES(key_bits_attr);
	default:
		return 0;
	}
}

static psa_status_t handle_identity_key(const uint8_t *key_buffer, size_t key_buffer_size,
					       const struct sx_pk_ecurve *sx_curve, uint8_t *data)
{
	if (key_buffer_size != sizeof(ikg_opaque_key)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_R1_256)) {
		data[0] = CRACEN_ECC_PUBKEY_UNCOMPRESSED;
		return silex_statuscodes_to_psa(cracen_ikg_create_pub_key(
			((const ikg_opaque_key *)key_buffer)->owner_id, data + 1));
	}
	return PSA_ERROR_NOT_SUPPORTED;
}

static psa_status_t handle_curve_family(psa_ecc_family_t psa_curve, size_t key_bits_attr,
					const uint8_t *key_buffer, uint8_t *data,
					const struct sx_pk_ecurve *sx_curve)
{

	switch (psa_curve) {
	case PSA_ECC_FAMILY_SECP_R1:
	case PSA_ECC_FAMILY_SECP_K1:
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
		if (IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1) ||
		    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1) ||
		    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1)) {
			data[0] = CRACEN_ECC_PUBKEY_UNCOMPRESSED;
			return silex_statuscodes_to_psa(
				ecc_genpubkey(key_buffer, data + 1, sx_curve));
		} else {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;

	case PSA_ECC_FAMILY_MONTGOMERY:
		if (key_bits_attr == 255 &&
		    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY_255)) {
			return silex_statuscodes_to_psa(cracen_x25519_genpubkey(key_buffer, data));
		} else if (key_bits_attr == 448 &&
			   IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY_448)) {
			return silex_statuscodes_to_psa(cracen_x448_genpubkey(key_buffer, data));
		} else {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;

	case PSA_ECC_FAMILY_TWISTED_EDWARDS:
		if (key_bits_attr == 255 &&
		    IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_255)) {
			return silex_statuscodes_to_psa(
				cracen_ed25519_create_pubkey(key_buffer, data));
		} else if (key_bits_attr == 448 &&
			   IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_448)) {
			return silex_statuscodes_to_psa(
				cracen_ed448_create_pubkey(key_buffer, data));
			return PSA_ERROR_NOT_SUPPORTED;
		} else {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;

	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return PSA_SUCCESS;
}

psa_status_t export_ecc_public_key_from_keypair(const psa_key_attributes_t *attributes,
						const uint8_t *key_buffer,
						size_t key_buffer_size, uint8_t *data,
						size_t data_size, size_t *data_length)
{
	psa_status_t status;

	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_ecc_family_t psa_curve = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	size_t expected_pub_key_size = get_expected_pub_key_size(psa_curve, key_bits_attr);

	if (expected_pub_key_size > data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	const struct sx_pk_ecurve *sx_curve;

	status = cracen_ecc_get_ecurve_from_psa(psa_curve, key_bits_attr, &sx_curve);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {
		if (IS_ENABLED(CONFIG_CRACEN_IKG)) {
			status = handle_identity_key(key_buffer, key_buffer_size, sx_curve, data);
		} else {
			status = PSA_ERROR_NOT_SUPPORTED;
		}
	} else {
		status = handle_curve_family(psa_curve, key_bits_attr, key_buffer, data, sx_curve);
	}
	if (status != PSA_SUCCESS) {
		return status;
	}

	*data_length = expected_pub_key_size;
	return PSA_SUCCESS;
}

psa_status_t import_ecc_private_key(const psa_key_attributes_t *attributes,
				    const uint8_t *data, size_t data_length,
				    uint8_t *key_buffer, size_t key_buffer_size,
				    size_t *key_buffer_length, size_t *key_bits)
{
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_status_t psa_status;

	if (data_length > key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	psa_status = check_ecc_key_attributes(attributes, data_length, &key_bits_attr);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	/* TODO: NCSDK-24516: Here we don't check if key < n.
	 * We don't consider this a security issue since it should return an
	 * error when tried to be used. Remove the comment when more testing is
	 * done and we can verify that this is not needed.
	 */
	if (memcpy_check_non_zero(key_buffer, key_buffer_size, data, data_length)) {
		*key_bits = key_bits_attr;
		*key_buffer_length = data_length;
		return PSA_SUCCESS;
	} else {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t import_ecc_public_key(const psa_key_attributes_t *attributes,
				   const uint8_t *data, size_t data_length,
				   uint8_t *key_buffer, size_t key_buffer_size,
				   size_t *key_buffer_length, size_t *key_bits)
{
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	psa_algorithm_t key_alg = psa_get_key_algorithm(attributes);
	psa_status_t psa_status;

	uint8_t local_key_buffer[PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(
		PSA_VENDOR_ECC_MAX_CURVE_BITS)];

	if (!memcpy_check_non_zero(local_key_buffer, sizeof(local_key_buffer), data, data_length)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_length > key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	psa_status = check_ecc_key_attributes(attributes, data_length, &key_bits_attr);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	switch (curve) {
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
	case PSA_ECC_FAMILY_SECP_R1:
		psa_status = check_wstr_pub_key_data(key_alg, curve, key_bits_attr,
						     local_key_buffer, data_length);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}
		break;
	/* prevent -Wswitch warning*/
	default:
		break;
	}

	memcpy(key_buffer, local_key_buffer, data_length);
	*key_bits = key_bits_attr;
	*key_buffer_length = data_length;

	return PSA_SUCCESS;
}

psa_status_t ecc_export_key(const psa_key_attributes_t *attributes,
			    const uint8_t *key_buffer, size_t key_buffer_size, uint8_t *data,
			    size_t data_size, size_t *data_length)
{
	if (data_size < key_buffer_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(data, key_buffer, key_buffer_size);
	*data_length = key_buffer_size;
	return PSA_SUCCESS;
}
