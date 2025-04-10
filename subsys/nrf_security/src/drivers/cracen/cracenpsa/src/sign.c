/**
 * @file
 *
 * @copyright Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <sicrypto/drbghash.h>
#include <sicrypto/ik.h>
#include <sicrypto/internal.h>
#include <sicrypto/rsapss.h>
#include <sicrypto/rsassa_pkcs1v15.h>
#include <sicrypto/sicrypto.h>
#include <silexpk/blinding.h>
#include <silexpk/ec_curves.h>
#include <silexpk/ed25519.h>
#include <silexpk/ik.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <string.h>
#include <sxsymcrypt/hashdefs.h>
#include <sxsymcrypt/trng.h>

#include "common.h"
#include "cracen_psa.h"
#include "cracen_psa_ecdsa.h"
#include "cracen_psa_eddsa.h"
#include "ecc.h"
#define SI_IS_MESSAGE	   (1)
#define SI_IS_HASH	   (0)
#define SI_EXTRACT_PUBKEY  (1)
#define SI_EXTRACT_PRIVKEY (0)

#define SI_PSA_IS_KEY_FLAG(flag, attr) ((flag) == (psa_get_key_usage_flags((attr)) & (flag)))
#define SI_PSA_IS_KEY_TYPE(flag, attr) ((flag) == (psa_get_key_type((attr)) & (flag)))

#if PSA_MAX_RSA_KEY_BITS > 0
static int cracen_signature_set_hashalgo(const struct sxhashalg **hashalg, psa_algorithm_t alg)
{
	return hash_get_algo(PSA_ALG_SIGN_GET_HASH(alg), hashalg);
}

static int cracen_signature_set_hashalgo_from_digestsz(const struct sxhashalg **hashalg,
						       psa_algorithm_t alg, size_t digestsz)
{
	int status = SX_OK;

	status = cracen_signature_set_hashalgo(hashalg, alg);
	if (status == SX_ERR_INVALID_ARG) {
		switch (digestsz) {
		case 16: /* DES not supported yet */
			return SX_ERR_INCOMPATIBLE_HW;
		case 20:
			*hashalg = &sxhashalg_sha1;
			break;
		case 28:
			*hashalg = &sxhashalg_sha2_224;
			break;
		case 32:
			*hashalg = &sxhashalg_sha2_256;
			break;
		case 48:
			*hashalg = &sxhashalg_sha2_384;
			break;
		case 64:
			*hashalg = &sxhashalg_sha2_512;
			break;
		default:
			return SX_ERR_INVALID_ARG;
		}
	} else if (status != SX_OK) {
		return status;
	}
	/* if status == SX_OK */
	if (sx_hash_get_alg_digestsz(*hashalg) != digestsz) {
		return SX_ERR_INVALID_ARG;
	}

	return SX_OK;
}
#endif /* PSA_MAX_RSA_KEY_BITS > 0 */

static int cracen_signature_prepare_ec_prvkey(struct si_sig_privkey *privkey, char *key_buffer,
					      size_t key_buffer_size,
					      const struct sx_pk_ecurve **sicurve,
					      psa_algorithm_t alg,
					      const psa_key_attributes_t *attributes,
					      bool is_message, size_t digestsz)
{
	/* This code can be removed once IKSIG is rewritten */
	int status;

	status = cracen_ecc_get_ecurve_from_psa(
		PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes)),
		psa_get_key_bits(attributes), sicurve);

	if (status) {
		return status;
	}

	/* IKG supports one SECP256_R1 key */
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {
		if (key_buffer_size != sizeof(ikg_opaque_key)) {
			return SX_ERR_INVALID_ARG;
		}

		if (IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_R1_256)) {
			*privkey = si_sig_fetch_ikprivkey(*sicurve, *key_buffer);
			return status;
		} else {
			return SX_ERR_INCOMPATIBLE_HW;
		}
	}

	if (key_buffer_size != PSA_BITS_TO_BYTES(psa_get_key_bits(attributes))) {
		return SX_ERR_INVALID_ARG;
	}

	if (alg == PSA_ALG_PURE_EDDSA || alg == PSA_ALG_ED25519PH) {
		return SX_OK;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_R1) ||
	    IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_K1) ||
	    IS_ENABLED(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1)) {
		if (PSA_ALG_IS_ECDSA(alg)) {
			return SX_OK;
		}
	}

	return SX_ERR_INCOMPATIBLE_HW;
}

static int cracen_signature_prepare_ec_pubkey(const char *key_buffer, size_t key_buffer_size,
					      const struct sx_pk_ecurve **sicurve,
					      psa_algorithm_t alg,
					      const psa_key_attributes_t *attributes,
					      char *pubkey_buffer)
{
	size_t curvesz = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
	int status;

	status = cracen_ecc_get_ecurve_from_psa(
		PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes)),
		psa_get_key_bits(attributes), sicurve);
	if (status) {
		return status;
	}

	status = SX_ERR_INCOMPATIBLE_HW;

	if (IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS)) {
		if (alg == PSA_ALG_PURE_EDDSA || alg == PSA_ALG_ED25519PH) {
			if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(psa_get_key_type(attributes))) {
				memcpy(pubkey_buffer, key_buffer, key_buffer_size);
				return SX_OK;
			}
			status = cracen_ed25519_create_pubkey(key_buffer, pubkey_buffer);
			return status;
		}
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
				if ((key_buffer[0] != SI_ECC_PUBKEY_UNCOMPRESSED) ||
				    ((2 * curvesz + 1) != key_buffer_size)) {
					return SX_ERR_INVALID_KEY_SZ;
				}

				/* key_buffer + 1 to skip the 0x4 flag in the first byte */
				memcpy(pubkey_buffer, key_buffer + 1, key_buffer_size - 1);
				return SX_OK;

			} else {
				status = ecc_create_genpubkey(key_buffer, pubkey_buffer, *sicurve);
				return status;
			}
		}
	}
	return status;
}

static psa_status_t validate_key_attributes(const psa_key_attributes_t *attributes,
					    size_t key_buffer_size,
					    const struct sx_pk_ecurve **ecurve)
{
	psa_status_t status = cracen_ecc_get_ecurve_from_psa(
		PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes)),
		psa_get_key_bits(attributes), ecurve);
	if (status != PSA_SUCCESS) {
		return status;
	}
	if (key_buffer_size != PSA_BITS_TO_BYTES(psa_get_key_bits(attributes))) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	if (!PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(attributes))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return PSA_SUCCESS;
}

static psa_status_t validate_signing_conditions(bool is_message, psa_algorithm_t alg,
						const psa_key_attributes_t *attributes)
{
	if (!PSA_ALG_IS_ECDSA(alg) && alg != PSA_ALG_PURE_EDDSA && alg != PSA_ALG_ED25519PH) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (is_message && !SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_MESSAGE, attributes)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!is_message && !SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_HASH, attributes)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static psa_status_t handle_eddsa_sign(bool is_message, const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, psa_algorithm_t alg,
				      uint8_t *signature, const uint8_t *input, size_t input_length,
				      const struct sx_pk_ecurve *ecurve, size_t *signature_length)
{
	int status;

	if (alg == PSA_ALG_ED25519PH && IS_ENABLED(CONFIG_PSA_WANT_ALG_ED25519PH)) {
		status = cracen_ed25519ph_sign(key_buffer, signature, input, input_length,
					       is_message);
		if (status == SX_OK) {
			*signature_length = 2 * ecurve->sz;
		}
		return silex_statuscodes_to_psa(status);
	}
	if (alg == PSA_ALG_PURE_EDDSA && psa_get_key_bits(attributes) == 255 &&
	    IS_ENABLED(CONFIG_PSA_WANT_ALG_PURE_EDDSA)) {
		status = cracen_ed25519_sign(key_buffer, signature, input, input_length);
		if (status == SX_OK) {
			*signature_length = 2 * ecurve->sz;
		}
		return silex_statuscodes_to_psa(status);
	}
	return PSA_ERROR_NOT_SUPPORTED;
}

static int si_ikg_sign(bool is_message, const psa_key_attributes_t *attributes,
		       const uint8_t *key_buffer, size_t key_buffer_size, psa_algorithm_t alg,
		       const uint8_t *input, size_t input_length, uint8_t *signature,
		       size_t *signature_length)
{
	const struct sx_pk_ecurve *curve;
	struct si_sig_privkey privkey = {0};
	struct si_sig_signature sign = {0};
	struct sitask t;
	int status;

	/* Workmem for ecc sign task is 4 * digestsz + hmac block size + curve size */
	char workmem[4 * PSA_HASH_MAX_SIZE + PSA_HMAC_MAX_HASH_BLOCK_SIZE +
		     PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)];

	si_task_init(&t, workmem, sizeof(workmem));
	status = cracen_signature_prepare_ec_prvkey(&privkey, (char *)key_buffer, key_buffer_size,
						    &curve, alg, attributes, is_message,
						    input_length);
	if (status) {
		return silex_statuscodes_to_psa(status);
	}

	*signature_length = 2 * curve->sz;
	sign.sz = *signature_length;
	sign.r = (char *)signature;
	sign.s = (char *)signature + *signature_length / 2;

	if (is_message) {
		si_sig_create_sign(&t, &privkey, &sign);
	} else {
		si_sig_create_sign_digest(&t, &privkey, &sign);
	}

	si_task_consume(&t, (char *)input,
			is_message ? input_length : sx_hash_get_alg_digestsz(privkey.hashalg));

	si_task_run(&t);
	status = si_task_wait(&t);
	safe_memzero(workmem, sizeof(workmem));
	return silex_statuscodes_to_psa(status);
}

static psa_status_t handle_ecdsa_sign(bool is_message, const uint8_t *key_buffer,
				      psa_algorithm_t alg, const uint8_t *input,
				      size_t input_length, const struct sx_pk_ecurve *ecurve,
				      uint8_t *signature, size_t *signature_length)
{
	int status;
	struct ecc_priv_key privkey;
	struct sxhashalg hashalg = {0};
	const struct sxhashalg *hashalgpointer = &hashalg;

	privkey.d = (const char *)key_buffer;
	status = hash_get_algo(alg, &hashalgpointer);

	if (status != PSA_SUCCESS) {
		return status;
	}

	*signature_length = 2 * ecurve->sz;
	status = SX_ERR_INCOMPATIBLE_HW;

	if (PSA_ALG_IS_DETERMINISTIC_ECDSA(alg) &&
	    IS_ENABLED(CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA)) {
		if (is_message) {
			status = cracen_ecdsa_sign_message_deterministic(
				&privkey, hashalgpointer, ecurve, input, input_length, signature);
		} else {
			status = cracen_ecdsa_sign_digest_deterministic(
				&privkey, hashalgpointer, ecurve, input, input_length, signature);
		}
	} else if (IS_ENABLED(CONFIG_PSA_WANT_ALG_ECDSA)) {
		if (is_message) {
			status = cracen_ecdsa_sign_message(&privkey, hashalgpointer, ecurve, input,
							   input_length, signature);
		} else {
			status = cracen_ecdsa_sign_digest(&privkey, hashalgpointer, ecurve, input,
							  input_length, signature);
		}
	}

	return silex_statuscodes_to_psa(status);
}

static psa_status_t cracen_signature_ecc_sign(bool is_message,
					      const psa_key_attributes_t *attributes,
					      const uint8_t *key_buffer, size_t key_buffer_size,
					      psa_algorithm_t alg, const uint8_t *input,
					      size_t input_length, uint8_t *signature,
					      size_t signature_size, size_t *signature_length)
{
	psa_status_t status;
	const struct sx_pk_ecurve *ecurve;

	status = validate_signing_conditions(is_message, alg, attributes);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_R1_256) &&
	    PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
		    PSA_KEY_LOCATION_CRACEN) {
		return si_ikg_sign(is_message, attributes, key_buffer, key_buffer_size, alg, input,
				   input_length, signature, signature_length);
	}

	status = validate_key_attributes(attributes, key_buffer_size, &ecurve);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if ((int)signature_size < 2 * ecurve->sz) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if ((alg == PSA_ALG_PURE_EDDSA && IS_ENABLED(CONFIG_PSA_WANT_ALG_PURE_EDDSA)) ||
	    (alg == PSA_ALG_ED25519PH && IS_ENABLED(CONFIG_PSA_WANT_ALG_ED25519PH))) {
		return handle_eddsa_sign(is_message, attributes, key_buffer, alg, signature, input,
					 input_length, ecurve, signature_length);
	} else if (PSA_ALG_IS_ECDSA(alg) && (IS_ENABLED(CONFIG_PSA_WANT_ALG_ECDSA) ||
					     IS_ENABLED(CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA))) {
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
	      PSA_ALG_IS_HASH_EDDSA(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if ((is_message && !SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_MESSAGE, attributes)) ||
	    (!is_message && !SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_HASH, attributes))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static psa_status_t cracen_signature_ecc_verify(bool is_message,
						const psa_key_attributes_t *attributes,
						const uint8_t *key_buffer, size_t key_buffer_size,
						psa_algorithm_t alg, const uint8_t *input,
						size_t input_length, const uint8_t *signature,
						size_t signature_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);

	psa_status_t status = validate_ec_signature_inputs(is_message, attributes, key_type, alg);

	if (status != PSA_SUCCESS) {
		return status;
	}

	size_t public_key_size =
		PSA_EXPORT_PUBLIC_KEY_OUTPUT_SIZE(key_type, psa_get_key_bits(attributes));
	if (public_key_size == 0) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	char pubkey_buffer[public_key_size];
	const struct sx_pk_ecurve *curve = NULL;

	int sx_status = cracen_signature_prepare_ec_pubkey(
		(const char *)key_buffer, key_buffer_size, &curve, alg, attributes, pubkey_buffer);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	if (signature_length != 2 * curve->sz) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	if (alg == PSA_ALG_ED25519PH) {
		sx_status = cracen_ed25519ph_verify(pubkey_buffer, (char *)input, input_length,
						    signature, is_message);

	} else if (alg == PSA_ALG_PURE_EDDSA) {
		sx_status = cracen_ed25519_verify(pubkey_buffer, (char *)input, input_length,
						  signature);

	} else if (PSA_ALG_IS_ECDSA(alg) || PSA_ALG_IS_DETERMINISTIC_ECDSA(alg)) {
		struct sxhashalg hashalg = {0};
		const struct sxhashalg *hash_algorithm_ptr = &hashalg;

		status = cracen_ecc_get_ecurve_from_psa(PSA_KEY_TYPE_ECC_GET_FAMILY(key_type),
							psa_get_key_bits(attributes), &curve);
		if (status != PSA_SUCCESS) {
			return status;
		}

		status = hash_get_algo(alg, &hash_algorithm_ptr);
		if (status != PSA_SUCCESS) {
			return status;
		}

		sx_status = is_message ? cracen_ecdsa_verify_message(pubkey_buffer,
								     hash_algorithm_ptr, input,
								     input_length, curve, signature)
				       : cracen_ecdsa_verify_digest(pubkey_buffer, input,
								    input_length, curve, signature);

	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return silex_statuscodes_to_psa(sx_status);
}

static psa_status_t cracen_signature_rsa_sign(bool is_message,
					      const psa_key_attributes_t *attributes,
					      const uint8_t *key_buffer, size_t key_buffer_size,
					      psa_algorithm_t alg, const uint8_t *input,
					      size_t input_length, uint8_t *signature,
					      size_t signature_size, size_t *signature_length)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	int sx_status;
	size_t key_bits_attr = psa_get_key_bits(attributes);
	struct si_sig_privkey privkey = {0};
	struct si_sig_signature sign = {0};
	struct sx_buf modulus;
	struct sx_buf exponent;
	struct sitask t;

	/* Workmem for RSA signature task is rsa_modulus_size + 2*hash_digest_size + 4 */
	char workmem[PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + 2 * PSA_HASH_MAX_SIZE + 4];

	si_task_init(&t, workmem, sizeof(workmem));

	if (alg == PSA_ALG_RSA_PKCS1V15_SIGN_RAW || key_bits_attr < 2048) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (((is_message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_MESSAGE, attributes))) ||
	    ((!is_message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_HASH, attributes)))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (is_message) {
		sx_status = cracen_signature_set_hashalgo(&privkey.hashalg, alg);
	} else {
		sx_status = cracen_signature_set_hashalgo_from_digestsz(&privkey.hashalg, alg,
									input_length);
	}
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = cracen_signature_get_rsa_key(&privkey.key.rsa, 0, true, key_buffer,
						 key_buffer_size, &modulus, &exponent);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	if (PSA_ALG_IS_RSA_PSS(alg)) {
		privkey.def = si_sig_def_rsa_pss;
		privkey.saltsz = sx_hash_get_alg_digestsz(privkey.hashalg);
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg)) {
		privkey.def = si_sig_def_rsa_pkcs1v15;
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	sign.sz = PSA_SIGN_OUTPUT_SIZE(PSA_KEY_TYPE_RSA_KEY_PAIR, key_bits_attr, alg);
	sign.r = (char *)signature;

	if ((size_t)signature_size < sign.sz) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (is_message) {
		si_sig_create_sign(&t, &privkey, &sign);
		si_task_consume(&t, (char *)input, input_length);
	} else {
		si_sig_create_sign_digest(&t, &privkey, &sign);
		si_task_consume(&t, (char *)input, sx_hash_get_alg_digestsz(privkey.hashalg));
	}
	si_task_run(&t);
	sx_status = si_task_wait(&t);

	safe_memzero(workmem, sizeof(workmem));
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	*signature_length = sign.sz;
	return PSA_SUCCESS;
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_MAX_RSA_KEY_BITS > 0 */
}

static psa_status_t cracen_signature_rsa_verify(bool is_message,
						const psa_key_attributes_t *attributes,
						const uint8_t *key_buffer, size_t key_buffer_size,
						psa_algorithm_t alg, const uint8_t *input,
						size_t input_length, const uint8_t *signature,
						size_t signature_length)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	int sx_status = 0;
	size_t key_bits_attr = psa_get_key_bits(attributes);
	struct si_sig_pubkey pubkey = {0};
	struct si_sig_signature sign = {0};
	struct sx_buf modulus;
	struct sx_buf exponent;
	struct sitask t;

	/* Workmem for RSA verify task is  rsa_modulus_size + 2 * hash_digest_size + 4 */
	char workmem[PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + 2 * PSA_HASH_MAX_SIZE + 4];

	si_task_init(&t, workmem, sizeof(workmem));

	if (alg == PSA_ALG_RSA_PKCS1V15_SIGN_RAW || key_bits_attr < 2048) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
	if (((is_message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_MESSAGE, attributes))) ||
	    ((!is_message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_HASH, attributes)))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (is_message) {
		sx_status = cracen_signature_set_hashalgo(&pubkey.hashalg, alg);
	} else {
		sx_status = cracen_signature_set_hashalgo_from_digestsz(&pubkey.hashalg, alg,
									input_length);
	}
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = cracen_signature_get_rsa_key(
		&pubkey.key.rsa, true, SI_PSA_IS_KEY_TYPE(PSA_KEY_TYPE_RSA_KEY_PAIR, attributes),
		key_buffer, key_buffer_size, &modulus, &exponent);

	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	if (PSA_ALG_IS_RSA_PSS(alg)) {
		pubkey.def = si_sig_def_rsa_pss;
		pubkey.saltsz = sx_hash_get_alg_digestsz(pubkey.hashalg);
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg)) {
		pubkey.def = si_sig_def_rsa_pkcs1v15;
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if ((size_t)signature_length !=
	    PSA_SIGN_OUTPUT_SIZE(PSA_KEY_TYPE_RSA_KEY_PAIR, key_bits_attr, alg)) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}
	sign.sz = signature_length;
	sign.r = (char *)signature;

	if (is_message) {
		si_sig_create_verify(&t, &pubkey, &sign);
		si_task_consume(&t, (char *)input, input_length);
	} else {
		if (sx_hash_get_alg_digestsz(pubkey.hashalg) != input_length) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		si_sig_create_verify_digest(&t, &pubkey, &sign);
		si_task_consume(&t, (char *)input, sx_hash_get_alg_digestsz(pubkey.hashalg));
	}

	si_task_run(&t);
	sx_status = si_task_wait(&t);

	safe_memzero(workmem, sizeof(workmem));
	return silex_statuscodes_to_psa(sx_status);
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_MAX_RSA_KEY_BITS > 0 */
}

psa_status_t cracen_sign_message(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				 size_t input_length, uint8_t *signature, size_t signature_size,
				 size_t *signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC)) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			return cracen_signature_ecc_sign(
				SI_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg, input,
				input_length, signature, signature_size, signature_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)) {
		if (PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
			return cracen_signature_rsa_sign(
				SI_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg, input,
				input_length, signature, signature_size, signature_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_sign_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			      size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
			      size_t hash_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC)) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			return cracen_signature_ecc_sign(
				SI_IS_HASH, attributes, key_buffer, key_buffer_size, alg, hash,
				hash_length, signature, signature_size, signature_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)) {
		if (PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
			return cracen_signature_rsa_sign(
				SI_IS_HASH, attributes, key_buffer, key_buffer_size, alg, hash,
				hash_length, signature, signature_size, signature_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_verify_message(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   const uint8_t *signature, size_t signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC)) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			return cracen_signature_ecc_verify(
				SI_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg, input,
				input_length, signature, signature_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)) {
		if (PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
			return cracen_signature_rsa_verify(
				SI_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg, input,
				input_length, signature, signature_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_verify_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
				size_t hash_length, const uint8_t *signature,
				size_t signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC)) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			return cracen_signature_ecc_verify(SI_IS_HASH, attributes, key_buffer,
							   key_buffer_size, alg, hash, hash_length,
							   signature, signature_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)) {
		if (PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
			return cracen_signature_rsa_verify(SI_IS_HASH, attributes, key_buffer,
							   key_buffer_size, alg, hash, hash_length,
							   signature, signature_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}
