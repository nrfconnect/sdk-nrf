
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
#include <sicrypto/internal.h>
#include <sicrypto/rsapss.h>
#include <sicrypto/rsassa_pkcs1v15.h>
#include <sicrypto/sicrypto.h>
#include <silexpk/blinding.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <string.h>
#include <sxsymcrypt/sha1.h>
#include <sxsymcrypt/sha2.h>
#include <sxsymcrypt/trng.h>
#include "hashdefs.h"

#include "common.h"
#include "cracen_psa.h"

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

static int set_internal_defs(psa_algorithm_t alg, bool is_message, size_t input_length,
			     const struct si_sig_def **def, const struct sxhashalg **hashalg,
			     size_t *saltsz)
{
	int sx_status;

	if (is_message) {
		sx_status = cracen_signature_set_hashalgo(hashalg, alg);
	} else {
		sx_status = cracen_signature_set_hashalgo_from_digestsz(hashalg, alg, input_length);
	}
	if (sx_status) {
		return sx_status;
	}

	if (PSA_ALG_IS_RSA_PSS(alg)) {
		*def = si_sig_def_rsa_pss;
		*saltsz = sx_hash_get_alg_digestsz(*hashalg);
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg)) {
		*def = si_sig_def_rsa_pkcs1v15;
	} else {
		return SX_ERR_INCOMPATIBLE_HW;
	}

	return SX_OK;
}

psa_status_t cracen_signature_rsa_sign(bool is_message, const psa_key_attributes_t *attributes,
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

	sx_status = set_internal_defs(alg, is_message, input_length,
		  &privkey.def, &privkey.hashalg, &privkey.saltsz);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = cracen_signature_get_rsa_key(&privkey.key.rsa, 0, true, key_buffer,
						 key_buffer_size, &modulus, &exponent);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
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


psa_status_t cracen_signature_rsa_verify(bool is_message, const psa_key_attributes_t *attributes,
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


	sx_status = set_internal_defs(alg, is_message, input_length,
		  &pubkey.def, &pubkey.hashalg, &pubkey.saltsz);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = cracen_signature_get_rsa_key(
		&pubkey.key.rsa, true, SI_PSA_IS_KEY_TYPE(PSA_KEY_TYPE_RSA_KEY_PAIR, attributes),
		key_buffer, key_buffer_size, &modulus, &exponent);

	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
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
