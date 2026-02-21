/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/rsa/cracen_rsa_signature.h>
#include <internal/rsa/cracen_rsa_common.h>
#include <internal/rsa/cracen_rsa_signature_pss.h>
#include <internal/rsa/cracen_rsa_signature_pkcs1v15.h>

#include <cracen/common.h>

#include <sxsymcrypt/hashdefs.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/trng.h>

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

psa_status_t cracen_signature_rsa_sign(bool is_message,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, uint8_t *signature,
				       size_t signature_size, size_t *signature_length)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	int sx_status;
	size_t key_bits_attr = psa_get_key_bits(attributes);
	struct cracen_rsa_key privkey = {0};
	struct cracen_signature sign = {0};
	struct sxhashalg hashalg = {0};
	const struct sxhashalg *hashalgpointer = &hashalg;
	struct sx_buf modulus;
	struct sx_buf exponent;

	if (alg == PSA_ALG_RSA_PKCS1V15_SIGN_RAW || key_bits_attr < 2048) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if ((is_message && (!CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_MESSAGE, attributes))) ||
	    ((!is_message) && (!CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_HASH, attributes)))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (is_message) {
		sx_status = cracen_signature_set_hashalgo(&hashalgpointer, alg);
	} else {
		sx_status = cracen_signature_set_hashalgo_from_digestsz(&hashalgpointer, alg,
									input_length);
	}
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = cracen_signature_get_rsa_key(&privkey, false, true, key_buffer, key_buffer_size,
						 &modulus, &exponent);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sign.sz = PSA_SIGN_OUTPUT_SIZE(PSA_KEY_TYPE_RSA_KEY_PAIR, key_bits_attr, alg);
	sign.r = signature;

	if ((size_t)signature_size < sign.sz) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	if (PSA_ALG_IS_RSA_PSS(alg) && IS_ENABLED(PSA_NEED_CRACEN_RSA_PSS)) {
		size_t saltsz = sx_hash_get_alg_digestsz(hashalgpointer);

		if (is_message) {
			sx_status = cracen_rsa_pss_sign_message(&privkey, &sign, hashalgpointer,
								input, input_length, saltsz);
		} else {
			sx_status = cracen_rsa_pss_sign_digest(&privkey, &sign, hashalgpointer,
							       input, input_length, saltsz);
		}
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) &&
		   IS_ENABLED(PSA_NEED_CRACEN_RSA_PKCS1V15_SIGN)) {
		if (is_message) {
			sx_status = cracen_rsa_pkcs1v15_sign_message(
				&privkey, &sign, hashalgpointer, input, input_length);
		} else {
			sx_status = cracen_rsa_pkcs1v15_sign_digest(&privkey, &sign, hashalgpointer,
								    input, input_length);
		}
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*signature_length = sign.sz;
	return silex_statuscodes_to_psa(sx_status);
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_MAX_RSA_KEY_BITS > 0 */
}

psa_status_t cracen_signature_rsa_verify(bool is_message,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg, const uint8_t *input,
					 size_t input_length, const uint8_t *signature,
					 size_t signature_length)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	int sx_status;
	size_t key_bits_attr = psa_get_key_bits(attributes);
	struct cracen_rsa_key privkey = {0};
	struct cracen_const_signature sign = {.sz = signature_length, .r = signature};
	struct sxhashalg hashalg = {0};
	const struct sxhashalg *hashalgpointer = &hashalg;
	struct sx_buf modulus;
	struct sx_buf exponent;
	size_t saltsz;

	if (alg == PSA_ALG_RSA_PKCS1V15_SIGN_RAW || key_bits_attr < 2048) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if ((is_message && (!CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_MESSAGE, attributes))) ||
	    ((!is_message) && (!CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_HASH, attributes)))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (is_message) {
		sx_status = cracen_signature_set_hashalgo(&hashalgpointer, alg);
	} else {
		sx_status = cracen_signature_set_hashalgo_from_digestsz(&hashalgpointer, alg,
									input_length);
	}
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = cracen_signature_get_rsa_key(
		&privkey, true, CRACEN_PSA_IS_KEY_TYPE(PSA_KEY_TYPE_RSA_KEY_PAIR, attributes),
		key_buffer, key_buffer_size, &modulus, &exponent);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}
	if (signature_length !=
	    PSA_SIGN_OUTPUT_SIZE(PSA_KEY_TYPE_RSA_KEY_PAIR, key_bits_attr, alg)) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}
	if (PSA_ALG_IS_RSA_PSS(alg)) {
		saltsz = sx_hash_get_alg_digestsz(hashalgpointer);
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg)) {
		saltsz = 0;
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (PSA_ALG_IS_RSA_PSS(alg) && IS_ENABLED(PSA_NEED_CRACEN_RSA_PSS)) {
		if (is_message) {
			sx_status = cracen_rsa_pss_verify_message(&privkey, &sign, hashalgpointer,
								  input, input_length, saltsz);
		} else {
			sx_status = cracen_rsa_pss_verify_digest(&privkey, &sign, hashalgpointer,
								 input, input_length, saltsz);
		}
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) &&
		   IS_ENABLED(PSA_NEED_CRACEN_RSA_PKCS1V15_SIGN)) {
		if (is_message) {
			sx_status = cracen_rsa_pkcs1v15_verify_message(
				&privkey, &sign, hashalgpointer, input, input_length);
		} else {
			sx_status = cracen_rsa_pkcs1v15_verify_digest(
				&privkey, &sign, hashalgpointer, input, input_length);
		}
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}
	return silex_statuscodes_to_psa(sx_status);
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_MAX_RSA_KEY_BITS > 0 */
}
