/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/ecc/cracen_ecc_signature.h>
#include <internal/ecc/cracen_ecc_helpers.h>
#include <internal/ecc/cracen_ecc_keygen.h>
#include <internal/ecc/cracen_ecdsa.h>
#include <internal/ecc/cracen_eddsa.h>
#include <internal/ikg/cracen_ikg_operations.h>

#include <cracen/common.h>
#include <silexpk/ec_curves.h>
#include <silexpk/ed25519.h>
#include <silexpk/ik.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <sxsymcrypt/hashdefs.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/trng.h>

static psa_status_t handle_eddsa_sign(bool is_message, const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, psa_algorithm_t alg,
				      uint8_t *signature, const uint8_t *input, size_t input_length,
				      const struct sx_pk_ecurve *ecurve, size_t *signature_length)
{
	int status;

	if (alg == PSA_ALG_ED25519PH && IS_ENABLED(PSA_NEED_CRACEN_ED25519PH)) {
		status = cracen_ed25519ph_sign(key_buffer, signature, input, input_length,
					       is_message);
		if (status == SX_OK) {
			*signature_length = 2 * ecurve->sz;
		}
		return silex_statuscodes_to_psa(status);
	}
	if (alg == PSA_ALG_ED448PH && IS_ENABLED(PSA_NEED_CRACEN_ED448PH)) {
		status = cracen_ed448ph_sign(key_buffer, signature, input, input_length,
						is_message);
		if (status == SX_OK) {
			*signature_length = 2 * ecurve->sz;
		}
		return silex_statuscodes_to_psa(status);
	}
	if (alg == PSA_ALG_PURE_EDDSA && psa_get_key_bits(attributes) == 255 &&
	    IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_255)) {
		status = cracen_ed25519_sign(key_buffer, signature, input, input_length);
		if (status == SX_OK) {
			*signature_length = 2 * ecurve->sz;
		}
		return silex_statuscodes_to_psa(status);
	}
	if (alg == PSA_ALG_PURE_EDDSA && psa_get_key_bits(attributes) == 448 &&
	    IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_448)) {
		status = cracen_ed448_sign(key_buffer, signature, input, input_length);
		if (status == SX_OK) {
			*signature_length = 2 * ecurve->sz;
		}
		return silex_statuscodes_to_psa(status);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

static psa_status_t handle_ikg_sign(bool is_message, const uint8_t *key_buffer,
				    size_t key_buffer_size, psa_algorithm_t alg,
				    const uint8_t *input, size_t input_length,
				    const struct sx_pk_ecurve *ecurve, uint8_t *signature,
				    size_t *signature_length)
{
	if (key_buffer_size != sizeof(ikg_opaque_key)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	int status;
	struct sxhashalg hashalg = {0};
	const struct sxhashalg *hashalgpointer = &hashalg;

	status = hash_get_algo(alg, &hashalgpointer);
	*signature_length = 2 * ecurve->sz;
	if (is_message) {
		status = cracen_ikg_sign_message(((const ikg_opaque_key *)key_buffer)->owner_id,
						 hashalgpointer, ecurve, input, input_length,
						 signature);
	} else {
		status = cracen_ikg_sign_digest(((const ikg_opaque_key *)key_buffer)->owner_id,
						hashalgpointer, ecurve, input, input_length,
						signature);
	}
	return silex_statuscodes_to_psa(status);
}

static psa_status_t handle_ecdsa_sign(bool is_message, const uint8_t *key_buffer,
				      psa_algorithm_t alg, const uint8_t *input,
				      size_t input_length, const struct sx_pk_ecurve *ecurve,
				      uint8_t *signature, size_t *signature_length)
{
	int status;
	struct cracen_ecc_priv_key privkey;
	struct sxhashalg hashalg = {0};
	const struct sxhashalg *hashalgpointer = &hashalg;

	privkey.d = key_buffer;

	*signature_length = 2 * ecurve->sz;
	status = SX_ERR_INCOMPATIBLE_HW;

	if (PSA_ALG_IS_DETERMINISTIC_ECDSA(alg) &&
	    IS_ENABLED(PSA_NEED_CRACEN_DETERMINISTIC_ECDSA)) {
		status = hash_get_algo(alg, &hashalgpointer);
		if (status != PSA_SUCCESS) {
			return status;
		}
		if (is_message) {
			status = cracen_ecdsa_sign_message_deterministic(
				&privkey, hashalgpointer, ecurve, input, input_length, signature);
		} else {
			status = cracen_ecdsa_sign_digest_deterministic(
				&privkey, hashalgpointer, ecurve, input, input_length, signature);
		}
	} else if ((PSA_ALG_IS_ECDSA(alg) && IS_ENABLED(PSA_NEED_CRACEN_ECDSA)) &&
		   !PSA_ALG_IS_DETERMINISTIC_ECDSA(alg)) {
		if (is_message) {
			status = hash_get_algo(alg, &hashalgpointer);
			if (status != PSA_SUCCESS) {
				return status;
			}
			status = cracen_ecdsa_sign_message(&privkey, hashalgpointer, ecurve, input,
							   input_length, signature);
		} else {
			status = cracen_ecdsa_sign_digest(&privkey, hashalgpointer, ecurve, input,
							  input_length, signature);
		}
	}

	return silex_statuscodes_to_psa(status);
}

static psa_status_t validate_signing_conditions(bool is_message, psa_algorithm_t alg,
						const psa_key_attributes_t *attributes,
						size_t ecurve_sz, size_t signature_size)
{
	if (!PSA_ALG_IS_ECDSA(alg) && alg != PSA_ALG_PURE_EDDSA && alg != PSA_ALG_ED25519PH
		&& alg != PSA_ALG_ED448PH) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (is_message && !CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_MESSAGE, attributes)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!is_message && !CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_HASH, attributes)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if ((int)signature_size < 2 * ecurve_sz) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	return PSA_SUCCESS;
}

static psa_status_t validate_key_attributes(const psa_key_attributes_t *attributes,
					    size_t key_buffer_size,
					    const struct sx_pk_ecurve **ecurve)
{
	if (key_buffer_size != (*ecurve)->sz) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	if (!PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(attributes))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_signature_ecc_sign(bool is_message,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, uint8_t *signature,
				       size_t signature_size, size_t *signature_length)
{
	psa_status_t status;
	const struct sx_pk_ecurve *ecurve;

	status = cracen_ecc_get_ecurve_from_psa(
		PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes)),
		psa_get_key_bits(attributes), &ecurve);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = validate_signing_conditions(is_message, alg, attributes, ecurve->sz,
					     signature_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_R1_256) &&
	    PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
		    PSA_KEY_LOCATION_CRACEN) {
		if (IS_ENABLED(CONFIG_CRACEN_IKG)) {
			return handle_ikg_sign(is_message, key_buffer, key_buffer_size, alg, input,
					       input_length, ecurve, signature, signature_length);
		} else {
			return PSA_ERROR_NOT_SUPPORTED;
		}
	}

	status = validate_key_attributes(attributes, key_buffer_size, &ecurve);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if ((int)signature_size < 2 * ecurve->sz) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if ((alg == PSA_ALG_PURE_EDDSA &&
	    (IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_255) ||
		IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_448))) ||
	    (alg == PSA_ALG_ED25519PH && IS_ENABLED(PSA_NEED_CRACEN_ED25519PH)) ||
		(alg == PSA_ALG_ED448PH && IS_ENABLED(PSA_NEED_CRACEN_ED448PH))) {
		return handle_eddsa_sign(is_message, attributes, key_buffer, alg, signature, input,
					 input_length, ecurve, signature_length);
	} else if (PSA_ALG_IS_ECDSA(alg) && (IS_ENABLED(PSA_NEED_CRACEN_ECDSA) ||
					     IS_ENABLED(PSA_NEED_CRACEN_DETERMINISTIC_ECDSA))) {
		return handle_ecdsa_sign(is_message, key_buffer, alg, input, input_length, ecurve,
					 signature, signature_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

static psa_status_t validate_ec_signature_inputs(bool is_message,
						 const psa_key_attributes_t *attributes,
						 psa_key_type_t key_type, psa_algorithm_t alg)
{
	if (!(PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(key_type) || PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (!(PSA_ALG_IS_ECDSA(alg) || PSA_ALG_IS_DETERMINISTIC_ECDSA(alg) ||
	      alg == PSA_ALG_PURE_EDDSA || alg == PSA_ALG_ED25519PH ||
	      alg == PSA_ALG_ED448PH || PSA_ALG_IS_HASH_EDDSA(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if ((is_message && !CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_MESSAGE, attributes)) ||
	    (!is_message && !CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_HASH, attributes))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static psa_status_t
cracen_signature_prepare_ec_pubkey(const uint8_t *key_buffer, size_t key_buffer_size,
				   const struct sx_pk_ecurve **sicurve, psa_algorithm_t alg,
				   const psa_key_attributes_t *attributes, uint8_t *pubkey_buffer)
{
	size_t curvesz = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
	psa_status_t psa_status;
	int sx_status;

	psa_status = cracen_ecc_get_ecurve_from_psa(
		PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes)),
		psa_get_key_bits(attributes), sicurve);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	sx_status = SX_ERR_INCOMPATIBLE_HW;
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {
		if (IS_ENABLED(CONFIG_CRACEN_IKG)) {
			if (key_buffer_size != sizeof(ikg_opaque_key)) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			sx_status = cracen_ikg_create_pub_key(
				((const ikg_opaque_key *)key_buffer)->owner_id, pubkey_buffer);
		}
		return silex_statuscodes_to_psa(sx_status);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_255) &&
	    (alg == PSA_ALG_PURE_EDDSA || alg == PSA_ALG_ED25519PH)) {
		if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(psa_get_key_type(attributes))) {
			memcpy(pubkey_buffer, key_buffer, key_buffer_size);
			return PSA_SUCCESS;
		}
		sx_status = cracen_ed25519_create_pubkey(key_buffer, pubkey_buffer);
		return silex_statuscodes_to_psa(sx_status);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_448) &&
	    (alg == PSA_ALG_PURE_EDDSA || alg == PSA_ALG_ED448PH)) {
		if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(psa_get_key_type(attributes))) {
			memcpy(pubkey_buffer, key_buffer, key_buffer_size);
			return PSA_SUCCESS;
		}
		sx_status = cracen_ed448_create_pubkey(key_buffer, pubkey_buffer);
		return silex_statuscodes_to_psa(sx_status);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_R1) ||
	    IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_K1) ||
	    IS_ENABLED(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1)) {
		if (PSA_ALG_IS_ECDSA(alg)) {
			if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(psa_get_key_type(attributes))) {
				/* public keys must start with 0x04(uncompressed header)
				 * and must have double the size of the EC curve plus 1
				 * (from 0x04)
				 */
				if ((key_buffer[0] != CRACEN_ECC_PUBKEY_UNCOMPRESSED) ||
				    ((2 * curvesz + 1) != key_buffer_size)) {
					return PSA_ERROR_INVALID_ARGUMENT;
				}

				/* key_buffer + 1 to skip the 0x4 flag in the first byte */
				memcpy(pubkey_buffer, key_buffer + 1, key_buffer_size - 1);
				return PSA_SUCCESS;

			} else {
				sx_status = ecc_genpubkey(key_buffer, pubkey_buffer, *sicurve);
			}
		}
	}
	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_signature_ecc_verify(bool is_message,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg, const uint8_t *input,
					 size_t input_length, const uint8_t *signature,
					 size_t signature_length)
{
	int sx_status;
	psa_status_t psa_status;
	psa_key_type_t key_type = psa_get_key_type(attributes);

	psa_status = validate_ec_signature_inputs(is_message, attributes, key_type, alg);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	const size_t public_key_size =
		PSA_EXPORT_PUBLIC_KEY_OUTPUT_SIZE(key_type, psa_get_key_bits(attributes));
	if (public_key_size == 0) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	uint8_t pubkey_buffer[public_key_size];
	const struct sx_pk_ecurve *curve = NULL;

	psa_status = cracen_signature_prepare_ec_pubkey(key_buffer, key_buffer_size, &curve, alg,
							   attributes, pubkey_buffer);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}
	if (signature_length != 2 * curve->sz) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ED25519PH) && alg == PSA_ALG_ED25519PH) {
		sx_status = cracen_ed25519ph_verify(pubkey_buffer, input, input_length,
						    signature, is_message);

	} else if (IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_255) &&
		   alg == PSA_ALG_PURE_EDDSA && signature_length == SX_ED25519_DGST_SZ) {
		sx_status = cracen_ed25519_verify(pubkey_buffer, input, input_length,
						  signature);

	} else if (IS_ENABLED(PSA_NEED_CRACEN_ED448PH) && alg == PSA_ALG_ED448PH) {
		sx_status = cracen_ed448ph_verify(pubkey_buffer, input, input_length,
						    signature, is_message);

	} else if (IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS_448) &&
		   alg == PSA_ALG_PURE_EDDSA) {
		sx_status = cracen_ed448_verify(pubkey_buffer, input, input_length,
						  signature);

	} else if ((PSA_ALG_IS_ECDSA(alg) && IS_ENABLED(PSA_NEED_CRACEN_ECDSA)) ||
		   (IS_ENABLED(PSA_NEED_CRACEN_DETERMINISTIC_ECDSA) &&
		    PSA_ALG_IS_DETERMINISTIC_ECDSA(alg))) {
		struct sxhashalg hashalg = {0};
		const struct sxhashalg *hash_algorithm_ptr = &hashalg;

		psa_status = cracen_ecc_get_ecurve_from_psa(PSA_KEY_TYPE_ECC_GET_FAMILY(key_type),
							    psa_get_key_bits(attributes), &curve);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}
		if (is_message) {
			psa_status = hash_get_algo(alg, &hash_algorithm_ptr);
			if (psa_status != PSA_SUCCESS) {
				return psa_status;
			}
			sx_status = cracen_ecdsa_verify_message(pubkey_buffer, hash_algorithm_ptr,
								input, input_length, curve,
								signature);
		} else {
			sx_status = cracen_ecdsa_verify_digest(pubkey_buffer, input, input_length,
							       curve, signature);
		}
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return silex_statuscodes_to_psa(sx_status);
}
