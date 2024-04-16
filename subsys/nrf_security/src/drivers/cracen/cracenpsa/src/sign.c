/**
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <mbedtls/asn1.h>
#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <sicrypto/drbghash.h>
#include <sicrypto/ecdsa.h>
#include <sicrypto/ed25519.h>
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
#include <sxsymcrypt/sha1.h>
#include <sxsymcrypt/sha2.h>
#include <sxsymcrypt/trng.h>

#include "common.h"
#include "cracen_psa.h"

#define SI_IS_MESSAGE	   (1)
#define SI_IS_HASH	   (0)
#define SI_EXTRACT_PUBKEY  (1)
#define SI_EXTRACT_PRIVKEY (0)

#define SI_PSA_IS_KEY_FLAG(flag, attr) ((flag) == (psa_get_key_usage_flags((attr)) & (flag)))
#define SI_PSA_IS_KEY_TYPE(flag, attr) ((flag) == (psa_get_key_type((attr)) & (flag)))

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

static int cracen_signature_prepare_ec_prvkey(struct si_sig_privkey *privkey, char *key_buffer,
					      size_t key_buffer_size,
					      const struct sx_pk_ecurve **sicurve,
					      psa_algorithm_t alg,
					      const psa_key_attributes_t *attributes, int message,
					      size_t digestsz)
{
	int status;

	status = cracen_ecc_get_ecurve_from_psa(
		PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes)),
		psa_get_key_bits(attributes), sicurve);

	if (status) {
		return status;
	}

	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {
		status = sx_pk_ik_derive_keys(NULL);
		if (status) {
			return status;
		}
		*privkey = si_sig_fetch_ikprivkey(*sicurve, *key_buffer);

		return status;
	}

	if (key_buffer_size != PSA_BITS_TO_BYTES(psa_get_key_bits(attributes))) {
		return SX_ERR_INVALID_ARG;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS)) {
		if (alg == PSA_ALG_PURE_EDDSA) {
			privkey->def = si_sig_def_ed25519;
			privkey->key.ed25519 = (struct sx_ed25519_v *)key_buffer;
			return SX_OK;
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_R1) ||
	    IS_ENABLED(PSA_NEED_CRACEN_ECDSA_SECP_K1) ||
	    IS_ENABLED(PSA_NEED_CRACEN_ECDSA_BRAINPOOL_P_R1)) {
		if (PSA_ALG_IS_ECDSA(alg)) {
			privkey->def = PSA_ALG_ECDSA_IS_DETERMINISTIC(alg)
					       ? si_sig_def_ecdsa_deterministic
					       : si_sig_def_ecdsa;
			privkey->key.eckey.curve = *sicurve;
			privkey->key.eckey.d = key_buffer;
			if (message) {
				return cracen_signature_set_hashalgo(&privkey->hashalg, alg);
			} else {
				return cracen_signature_set_hashalgo_from_digestsz(
					&privkey->hashalg, alg, digestsz);
			}
		}
	}

	return SX_ERR_INCOMPATIBLE_HW;
}

static int cracen_prepare_ecdsa_ec_pubkey(struct si_sig_pubkey *pubkey,
					  const struct sx_pk_ecurve *sicurve, int message,
					  psa_algorithm_t alg, size_t digestsz, size_t curvesz,
					  char *key_buffer)
{

	int status = SX_ERR_INCOMPATIBLE_HW;

	pubkey->def = si_sig_def_ecdsa;
	pubkey->key.eckey.curve = sicurve;

	if (message) {
		status = cracen_signature_set_hashalgo(&pubkey->hashalg, alg);
	} else {
		status = cracen_signature_set_hashalgo_from_digestsz(&pubkey->hashalg, alg,
								     digestsz);
	}

	if (status != SX_OK) {
		return status;
	}

	pubkey->key.eckey.qx = key_buffer;
	pubkey->key.eckey.qy = key_buffer + curvesz;

	return SX_OK;
}

static int cracen_signature_prepare_ec_pubkey(struct sitask *t, struct si_sig_pubkey *pubkey,
					      char *key_buffer, size_t key_buffer_size,
					      const struct sx_pk_ecurve **sicurve,
					      psa_algorithm_t alg,
					      const psa_key_attributes_t *attributes, int message,
					      size_t digestsz, char *pubkey_buffer)
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
		if (alg == PSA_ALG_PURE_EDDSA) {
			pubkey->def = si_sig_def_ed25519;

			if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(psa_get_key_type(attributes))) {
				pubkey->key.ed25519 = (struct sx_ed25519_pt *)key_buffer;
				return SX_OK;
			}
			if (curvesz != key_buffer_size) {
				return SX_ERR_INVALID_KEY_SZ;
			}
			pubkey->key.ed25519 = (struct sx_ed25519_pt *)pubkey_buffer;
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
				status = cracen_prepare_ecdsa_ec_pubkey(pubkey, *sicurve, message,
									alg, digestsz, curvesz,
									key_buffer + 1);
			} else {
				status = cracen_prepare_ecdsa_ec_pubkey(pubkey, *sicurve, message,
									alg, digestsz, curvesz,
									pubkey_buffer);
			}

			if (status != SX_OK) {
				return status;
			}
		}
	}

	if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(attributes))) {
		struct si_sig_privkey m_privkey;

		status = cracen_signature_prepare_ec_prvkey(&m_privkey, key_buffer, key_buffer_size,
							    sicurve, alg, attributes, message,
							    digestsz);
		if (status != SX_OK) {
			return status;
		}

		si_sig_create_pubkey(t, &m_privkey, pubkey);
		si_task_run(t);
		return si_task_wait(t);
	}

	return status;
}

static psa_status_t cracen_signature_ecc_sign(int message, const psa_key_attributes_t *attributes,
					      const uint8_t *key_buffer, size_t key_buffer_size,
					      psa_algorithm_t alg, const uint8_t *input,
					      size_t input_length, uint8_t *signature,
					      size_t signature_size, size_t *signature_length)
{
	int si_status;
	const struct sx_pk_ecurve *curve;
	struct si_sig_privkey privkey = {0};
	struct si_sig_signature sign = {0};
	struct sitask t;
	/* Workmem for ecc sign task is 4 * digestsz + hmac block size + curve size */
	char workmem[4 * PSA_HASH_MAX_SIZE + PSA_HMAC_MAX_HASH_BLOCK_SIZE +
		     PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)];

	si_task_init(&t, workmem, sizeof(workmem));

	if (!PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(attributes))) {
		return silex_statuscodes_to_psa(SX_ERR_INCOMPATIBLE_HW);
	}

	if (!PSA_ALG_IS_ECDSA(alg) && alg != PSA_ALG_PURE_EDDSA) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (((message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_MESSAGE, attributes))) ||
	    ((!message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_HASH, attributes)))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	si_status =
		cracen_signature_prepare_ec_prvkey(&privkey, (char *)key_buffer, key_buffer_size,
						   &curve, alg, attributes, message, input_length);

	if (si_status) {
		return silex_statuscodes_to_psa(si_status);
	}

	if ((int)signature_size < 2 * curve->sz) {
		return silex_statuscodes_to_psa(SX_ERR_OUTPUT_BUFFER_TOO_SMALL);
	}

	*signature_length = 2 * curve->sz;
	sign.sz = *signature_length;
	sign.r = (char *)signature;
	sign.s = (char *)signature + *signature_length / 2;

	if (message) {
		si_sig_create_sign(&t, &privkey, &sign);
		si_task_consume(&t, (char *)input, input_length);
	} else {
		si_sig_create_sign_digest(&t, &privkey, &sign);
		si_task_consume(&t, (char *)input, sx_hash_get_alg_digestsz(privkey.hashalg));
	}

	si_task_run(&t);
	si_status = si_task_wait(&t);
	safe_memzero(workmem, sizeof(workmem));

	return silex_statuscodes_to_psa(si_status);
}

static psa_status_t cracen_signature_ecc_verify(int message, const psa_key_attributes_t *attributes,
						const uint8_t *key_buffer, size_t key_buffer_size,
						psa_algorithm_t alg, const uint8_t *input,
						size_t input_length, const uint8_t *signature,
						size_t signature_length)
{
	int si_status = 0;
	const struct sx_pk_ecurve *curve;
	struct si_sig_pubkey pubkey = {0};
	struct si_sig_signature sign = {0};
	char pubkey_buffer[132] = {0}; /* 521 bits * 2 */

	/* Workmem for sicrypto ecc verify task is digest size. */
	char workmem[PSA_HASH_MAX_SIZE];
	struct sitask t;

	if (!PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(psa_get_key_type(attributes)) &&
	    !PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(attributes))) {
		return silex_statuscodes_to_psa(SX_ERR_INCOMPATIBLE_HW);
	}

	if (!PSA_ALG_IS_ECDSA(alg) && alg != PSA_ALG_PURE_EDDSA) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (((message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_MESSAGE, attributes))) ||
	    ((!message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_HASH, attributes)))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	si_task_init(&t, workmem, sizeof(workmem));

	si_status = cracen_signature_prepare_ec_pubkey(&t, &pubkey, (char *)key_buffer,
						       key_buffer_size, &curve, alg, attributes,
						       message, input_length, pubkey_buffer);
	if (si_status) {
		return silex_statuscodes_to_psa(si_status);
	}

	if ((int)signature_length != 2 * curve->sz) {
		return silex_statuscodes_to_psa(SX_ERR_INVALID_SIGNATURE);
	}
	sign.sz = signature_length;
	sign.r = (char *)signature;
	sign.s = (char *)signature + signature_length / 2;

	if (message) {
		si_sig_create_verify(&t, &pubkey, &sign);
	} else {
		if (sx_hash_get_alg_digestsz(pubkey.hashalg) != input_length) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		si_sig_create_verify_digest(&t, &pubkey, &sign);
	}

	si_task_consume(&t, (char *)input, input_length);
	si_task_run(&t);
	si_status = si_task_wait(&t);

	safe_memzero(workmem, sizeof(workmem));
	return silex_statuscodes_to_psa(si_status);
}

static psa_status_t cracen_signature_rsa_sign(int message, const psa_key_attributes_t *attributes,
					      const uint8_t *key_buffer, size_t key_buffer_size,
					      psa_algorithm_t alg, const uint8_t *input,
					      size_t input_length, uint8_t *signature,
					      size_t signature_size, size_t *signature_length)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	int si_status;
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

	if (((message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_MESSAGE, attributes))) ||
	    ((!message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_HASH, attributes)))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (message) {
		si_status = cracen_signature_set_hashalgo(&privkey.hashalg, alg);
	} else {
		si_status = cracen_signature_set_hashalgo_from_digestsz(&privkey.hashalg, alg,
									input_length);
	}
	if (si_status) {
		return silex_statuscodes_to_psa(si_status);
	}

	si_status = cracen_signature_get_rsa_key(&privkey.key.rsa, 0, true, key_buffer,
						 key_buffer_size, &modulus, &exponent);
	if (si_status) {
		return silex_statuscodes_to_psa(si_status);
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
		return silex_statuscodes_to_psa(SX_ERR_OUTPUT_BUFFER_TOO_SMALL);
	}

	if (message) {
		si_sig_create_sign(&t, &privkey, &sign);
		si_task_consume(&t, (char *)input, input_length);
	} else {
		si_sig_create_sign_digest(&t, &privkey, &sign);
		si_task_consume(&t, (char *)input, sx_hash_get_alg_digestsz(privkey.hashalg));
	}
	si_task_run(&t);
	si_status = si_task_wait(&t);

	safe_memzero(workmem, sizeof(workmem));
	if (si_status != SX_OK) {
		return silex_statuscodes_to_psa(si_status);
	}

	*signature_length = sign.sz;
	return PSA_SUCCESS;
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_MAX_RSA_KEY_BITS > 0 */
}

static psa_status_t cracen_signature_rsa_verify(int message, const psa_key_attributes_t *attributes,
						const uint8_t *key_buffer, size_t key_buffer_size,
						psa_algorithm_t alg, const uint8_t *input,
						size_t input_length, const uint8_t *signature,
						size_t signature_length)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	int si_status = 0;
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
	if (((message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_MESSAGE, attributes))) ||
	    ((!message) && (!SI_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_HASH, attributes)))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (message) {
		si_status = cracen_signature_set_hashalgo(&pubkey.hashalg, alg);
	} else {
		si_status = cracen_signature_set_hashalgo_from_digestsz(&pubkey.hashalg, alg,
									input_length);
	}
	if (si_status) {
		return silex_statuscodes_to_psa(si_status);
	}

	si_status = cracen_signature_get_rsa_key(
		&pubkey.key.rsa, true, SI_PSA_IS_KEY_TYPE(PSA_KEY_TYPE_RSA_KEY_PAIR, attributes),
		key_buffer, key_buffer_size, &modulus, &exponent);

	if (si_status) {
		return silex_statuscodes_to_psa(si_status);
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
		return silex_statuscodes_to_psa(SX_ERR_INVALID_SIGNATURE);
	}
	sign.sz = signature_length;
	sign.r = (char *)signature;

	if (message) {
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
	si_status = si_task_wait(&t);

	safe_memzero(workmem, sizeof(workmem));
	return silex_statuscodes_to_psa(si_status);
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
