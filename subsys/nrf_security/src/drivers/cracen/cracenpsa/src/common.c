/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#ifdef CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
#include "platform_keys/platform_keys.h"
#endif

#include <cracen/lib_kmu.h>
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <hal/nrf_cracen.h>
#include <nrfx.h>
#include <sicrypto/rsa_keys.h>
#include <sicrypto/sicrypto.h>
#include <sicrypto/util.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <silexpk/ik.h>
#include <silexpk/sxops/eccweierstrass.h>
#include <silexpk/sxops/rsa.h>
#include <stddef.h>
#include <sxsymcrypt/hashdefs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <psa/nrf_platform_key_ids.h>

LOG_MODULE_DECLARE(cracen, CONFIG_CRACEN_LOG_LEVEL);
#if CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
#include "platform_keys/platform_keys.h"
#endif

#define NOT_ENABLED_CURVE    (0)
#define NOT_ENABLED_HASH_ALG (0)

#ifdef CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
/* Address from the IPS. May come from the MDK in the future. */
#define DEVICE_SECRET_LENGTH  4
#define DEVICE_SECRET_ADDRESS ((uint32_t *)0x0E001620)
#endif

static const uint8_t RSA_ALGORITHM_IDENTIFIER[] = {0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
						   0x0d, 0x01, 0x01, 0x01, 0x05, 0x00};

psa_status_t silex_statuscodes_to_psa(int ret)
{
	if (ret != SX_OK) {
		LOG_DBG("SX_ERR %d", ret);
	}

	switch (ret) {
	case SX_OK:
	case SX_ERR_READY:
		return PSA_SUCCESS;

	case SX_ERR_INCOMPATIBLE_HW:
		return PSA_ERROR_NOT_SUPPORTED;

	case SX_ERR_FEED_AFTER_DATA:
	case SX_ERR_TOO_BIG:
		return PSA_ERROR_NOT_PERMITTED;

	case SX_ERR_OUTPUT_BUFFER_TOO_SMALL:
		return PSA_ERROR_BUFFER_TOO_SMALL;

	case SX_ERR_DMA_FAILED:
	case SX_ERR_RETRY:
		return PSA_ERROR_HARDWARE_FAILURE;

	case SX_ERR_UNINITIALIZED_OBJ:
		return PSA_ERROR_BAD_STATE;

	case SX_ERR_INVALID_TAG:
	case SX_ERR_INVALID_SIGNATURE:
	case SX_ERR_OUT_OF_RANGE:
		return PSA_ERROR_INVALID_SIGNATURE;

	case SX_ERR_INVALID_REQ_SZ:
		return PSA_ERROR_DATA_INVALID;

	case SX_ERR_UNSUPPORTED_HASH_ALG:
		return PSA_ERROR_NOT_SUPPORTED;

	case SX_ERR_WORKMEM_BUFFER_TOO_SMALL:
		return PSA_ERROR_INSUFFICIENT_MEMORY;

	case SX_ERR_INSUFFICIENT_ENTROPY:
	case SX_ERR_TOO_MANY_ATTEMPTS:
		return PSA_ERROR_INSUFFICIENT_ENTROPY;

	case SX_ERR_INVALID_CIPHERTEXT:
		/* This can happen in psa_asymmetric_decrypt. PSA Crypto specification
		 * does not list an appropriate error code for this.
		 */
		return PSA_ERROR_INVALID_ARGUMENT;

	case SX_ERR_INVALID_ARG:
	case SX_ERR_INPUT_BUFFER_TOO_SMALL:
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

__attribute__((warn_unused_result)) psa_status_t
hash_get_algo(psa_algorithm_t alg, const struct sxhashalg **sx_hash_algo)
{
	*sx_hash_algo = NOT_ENABLED_HASH_ALG;

	switch (PSA_ALG_GET_HASH(alg)) {
	case PSA_ALG_SHA_1:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_1, (*sx_hash_algo = &sxhashalg_sha1));
		break;
	case PSA_ALG_SHA_224:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_224, (*sx_hash_algo = &sxhashalg_sha2_224));
		break;
	case PSA_ALG_SHA_256:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_256, (*sx_hash_algo = &sxhashalg_sha2_256));
		break;
	case PSA_ALG_SHA_384:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_384, (*sx_hash_algo = &sxhashalg_sha2_384));
		break;
	case PSA_ALG_SHA_512:
		IF_ENABLED(PSA_NEED_CRACEN_SHA_512, (*sx_hash_algo = &sxhashalg_sha2_512));
		break;
	case PSA_ALG_SHA3_224:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_224, (*sx_hash_algo = &sxhashalg_sha3_224));
		break;
	case PSA_ALG_SHA3_256:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_256, (*sx_hash_algo = &sxhashalg_sha3_256));
		break;
	case PSA_ALG_SHA3_384:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_384, (*sx_hash_algo = &sxhashalg_sha3_384));
		break;
	case PSA_ALG_SHA3_512:
		IF_ENABLED(PSA_NEED_CRACEN_SHA3_512, (*sx_hash_algo = &sxhashalg_sha3_512));
		break;
	default:
		return PSA_ALG_IS_HASH(alg) ? PSA_ERROR_NOT_SUPPORTED : PSA_ERROR_INVALID_ARGUMENT;
	}

	return (*sx_hash_algo == NOT_ENABLED_HASH_ALG) ? PSA_ERROR_NOT_SUPPORTED : PSA_SUCCESS;
}

static psa_status_t get_sx_brainpool_curve(size_t curve_bits, const struct sx_pk_ecurve **sicurve)
{
	const struct sx_pk_ecurve *selected_curve = NOT_ENABLED_CURVE;

	switch (curve_bits) {
	case 192:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_192,
			   (selected_curve = &sx_curve_brainpoolP192r1));
		break;
	case 224:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_224,
			   (selected_curve = &sx_curve_brainpoolP224r1));
		break;
	case 256:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_256,
			   (selected_curve = &sx_curve_brainpoolP256r1));
		break;
	case 320:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_320,
			   (selected_curve = &sx_curve_brainpoolP320r1));
		break;
	case 384:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_384,
			   (selected_curve = &sx_curve_brainpoolP384r1));
		break;
	case 512:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_BRAINPOOL_P_R1_512,
			   (selected_curve = &sx_curve_brainpoolP512r1));
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (selected_curve == NOT_ENABLED_CURVE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*sicurve = selected_curve;
	return PSA_SUCCESS;
}

static psa_status_t get_sx_secp_r1_curve(size_t curve_bits, const struct sx_pk_ecurve **sicurve)
{
	const struct sx_pk_ecurve *selected_curve = NOT_ENABLED_CURVE;

	switch (curve_bits) {
	case 192:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_192,
			   (selected_curve = &sx_curve_nistp192));
		break;
	case 224:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_224,
			   (selected_curve = &sx_curve_nistp224));
		break;
	case 256:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_256,
			   (selected_curve = &sx_curve_nistp256));
		break;
	case 384:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_384,
			   (selected_curve = &sx_curve_nistp384));
		break;
	case 521:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_R1_521,
			   (selected_curve = &sx_curve_nistp521));
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (selected_curve == NOT_ENABLED_CURVE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*sicurve = selected_curve;
	return PSA_SUCCESS;
}

static psa_status_t get_sx_secp_k1_curve(size_t curve_bits, const struct sx_pk_ecurve **sicurve)
{
	const struct sx_pk_ecurve *selected_curve = NOT_ENABLED_CURVE;

	switch (curve_bits) {
	case 192:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1_192,
			   (selected_curve = &sx_curve_secp192k1));
		break;
	case 225:
		return PSA_ERROR_NOT_SUPPORTED;
	case 256:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1_256,
			   (selected_curve = &sx_curve_secp256k1));
		break;
	}

	if (selected_curve == NOT_ENABLED_CURVE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*sicurve = selected_curve;
	return PSA_SUCCESS;
}

static psa_status_t get_sx_montgomery_curve(size_t curve_bits, const struct sx_pk_ecurve **sicurve)
{
	const struct sx_pk_ecurve *selected_curve = NOT_ENABLED_CURVE;

	switch (curve_bits) {
	case 255:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY_255,
			   (selected_curve = &sx_curve_x25519));
		break;
	case 448:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_MONTGOMERY_448,
			   (selected_curve = &sx_curve_x448));
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (selected_curve == NOT_ENABLED_CURVE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*sicurve = selected_curve;
	return PSA_SUCCESS;
}

static psa_status_t get_sx_edwards_curve(size_t curve_bits, const struct sx_pk_ecurve **sicurve)
{
	const struct sx_pk_ecurve *selected_curve = NOT_ENABLED_CURVE;

	switch (curve_bits) {
	case 255:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_TWISTED_EDWARDS_255,
			   (selected_curve = &sx_curve_ed25519));
		break;
	case 448:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_TWISTED_EDWARDS_448,
			   (selected_curve = &sx_curve_ed448));
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (selected_curve == NOT_ENABLED_CURVE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*sicurve = selected_curve;
	return PSA_SUCCESS;
}

psa_status_t cracen_ecc_get_ecurve_from_psa(psa_ecc_family_t curve_family, size_t curve_bits,
					    const struct sx_pk_ecurve **sicurve)

{
	switch (curve_family) {
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
		return get_sx_brainpool_curve(curve_bits, sicurve);
	case PSA_ECC_FAMILY_SECP_R1:
		return get_sx_secp_r1_curve(curve_bits, sicurve);
	case PSA_ECC_FAMILY_MONTGOMERY:
		return get_sx_montgomery_curve(curve_bits, sicurve);
	case PSA_ECC_FAMILY_TWISTED_EDWARDS:
		return get_sx_edwards_curve(curve_bits, sicurve);
	case PSA_ECC_FAMILY_SECP_K1:
		return get_sx_secp_k1_curve(curve_bits, sicurve);
	case PSA_ECC_FAMILY_SECP_R2:
	case PSA_ECC_FAMILY_SECT_K1:
	case PSA_ECC_FAMILY_SECT_R1:
	case PSA_ECC_FAMILY_SECT_R2:
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

bool cracen_ecc_curve_is_weierstrass(psa_ecc_family_t curve)
{
	switch (curve) {
	case PSA_ECC_FAMILY_SECP_R1:
	case PSA_ECC_FAMILY_SECP_R2:
	case PSA_ECC_FAMILY_SECP_K1:
	case PSA_ECC_FAMILY_SECT_R1:
	case PSA_ECC_FAMILY_SECT_R2:
	case PSA_ECC_FAMILY_SECT_K1:
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
		return true;
	default:
		return false;
	}
}

psa_status_t cracen_ecc_reduce_p256(const uint8_t *input, size_t input_size, uint8_t *output,
				    size_t output_size)
{
	const uint8_t *order = sx_pk_curve_order(&sx_curve_nistp256);

	sx_op modulo = {.sz = CRACEN_P256_KEY_SIZE, .bytes = (char *)order};
	sx_op operand = {.sz = input_size, .bytes = (char *)input};
	sx_op result = {.sz = output_size, .bytes = output};

	/* The nistp256 curve order (n) is prime so we use the ODD variant of the reduce command. */
	const struct sx_pk_cmd_def *cmd = SX_PK_CMD_ODD_MOD_REDUCE;
	int sx_status = sx_mod_single_op_cmd(cmd, &modulo, &operand, &result);

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_ecc_check_public_key(const struct sx_pk_ecurve *curve,
					 const sx_pk_affine_point *in_pnt)
{
	int sx_status;
	char char_x[CRACEN_MAC_ECC_PRIVKEY_BYTES];
	char char_y[CRACEN_MAC_ECC_PRIVKEY_BYTES];

	/* Get the order of the curve from the parameters */
	struct sx_buf n = {.sz = sx_pk_curve_opsize(curve),
			   .bytes = (char *)sx_pk_curve_order(curve)};

	sx_pk_affine_point scratch_pnt = {.x = {.sz = n.sz, .bytes = char_x},
					  .y = {.sz = n.sz, .bytes = char_y}};

	/* This function checks if the point is on the curve, it also checks
	 * that both x and y are <= p - 1. So it gives us coverage for steps 1, 2 and 3.
	 */
	sx_status = sx_ec_ptoncurve(curve, in_pnt);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* Skip step 4.
	 * Only do partial key validation as we only support NIST curves and X25519.
	 * See DLT-3834 for more information.
	 */

	safe_memzero(scratch_pnt.x.bytes, scratch_pnt.x.sz);
	safe_memzero(scratch_pnt.x.bytes, scratch_pnt.x.sz);

	return PSA_SUCCESS;
}

psa_status_t rnd_in_range(uint8_t *n, size_t sz, const uint8_t *upperlimit, size_t retry_limit)
{
	size_t retries = 0;

	/* Fill leading zeroes. */
	while (upperlimit[0] == 0) {
		*n = 0;
		n++;
		upperlimit++;
		sz--;
	}

	uint8_t msb_mask;

	for (msb_mask = 0xFF; upperlimit[0] & msb_mask; msb_mask <<= 1) {
		;
	}
	msb_mask = ~msb_mask;

	while (retries++ < retry_limit) {
		psa_status_t status = psa_generate_random(n, sz);

		if (status) {
			return status;
		}
		n[0] &= msb_mask;

		int ge = cracen_be_cmp(n, upperlimit, sz, 0);

		if (ge == -1) {

			bool is_zero = constant_memcmp_is_zero(n, sz);

			if (is_zero == false) {
				return PSA_SUCCESS;
			}
		}
	}

	return PSA_ERROR_INSUFFICIENT_ENTROPY;
}

void cracen_xorbytes(char *a, const char *b, size_t sz)
{
	for (size_t i = 0; i < sz; i++, a++, b++) {
		*a = *a ^ *b;
	}
}

static int cracen_asn1_get_len(unsigned char **p, const unsigned char *end, size_t *len)
{
	if ((end - *p) < 1) {
		return SX_ERR_INVALID_PARAM;
	}

	if ((**p & 0x80) == 0) {
		*len = *(*p)++;
	} else {
		int n = (**p) & 0x7F;

		if (n == 0 || n > 4) {
			return SX_ERR_INVALID_PARAM;
		}
		if ((end - *p) <= n) {
			return SX_ERR_INVALID_PARAM;
		}
		*len = 0;
		(*p)++;
		while (n--) {
			*len = (*len << 8) | **p;
			(*p)++;
		}
	}

	if (*len > (size_t)(end - *p)) {
		return SX_ERR_INVALID_PARAM;
	}

	return 0;
}

static int cracen_asn1_get_tag(unsigned char **p, const unsigned char *end, size_t *len, int tag)
{
	if ((end - *p) < 1) {
		return SX_ERR_INVALID_PARAM;
	}

	if (**p != tag) {
		return SX_ERR_INVALID_PARAM;
	}

	(*p)++;

	return cracen_asn1_get_len(p, end, len);
}

static int cracen_asn1_get_int(unsigned char **p, const unsigned char *end, int *val)
{
	int ret = SX_ERR_INVALID_PARAM;
	size_t len;

	ret = cracen_asn1_get_tag(p, end, &len, ASN1_INTEGER);
	if (ret != 0) {
		return ret;
	}

	if (len == 0) {
		return SX_ERR_INVALID_PARAM;
	}
	/* Reject negative integers. */
	if ((**p & 0x80) != 0) {
		return SX_ERR_INVALID_PARAM;
	}

	/* Skip leading zeros. */
	while (len > 0 && **p == 0) {
		++(*p);
		--len;
	}

	/* Reject integers that don't fit in an int. This code assumes that
	 * the int type has no padding bit.
	 */
	if (len > sizeof(int)) {
		return SX_ERR_INVALID_PARAM;
	}
	if (len == sizeof(int) && (**p & 0x80) != 0) {
		return SX_ERR_INVALID_PARAM;
	}

	*val = 0;
	while (len-- > 0) {
		*val = (*val << 8) | **p;
		(*p)++;
	}

	return 0;
}

int cracen_signature_asn1_get_operand(unsigned char **p, const unsigned char *end,
				      struct sx_buf *op)
{
	int ret;
	size_t len = 0;
	size_t i = 0;

	ret = cracen_asn1_get_tag(p, end, &len, ASN1_INTEGER);
	if (ret) {
		return SX_ERR_INVALID_PARAM;
	}

	if (*p + len > end) {
		return SX_ERR_INVALID_PARAM;
	}

	/* Drop starting zeros, if any */
	for (i = 0; i < len; i++) {
		if ((*p)[i] != 0) {
			break;
		}
	}
	op->bytes = (char *)(*p + i);
	op->sz = len - i;

	*p += len;

	return SX_OK;
}

/*
 * This function extracts a RSA key from a key pair.
 * Depending on extract_pubkey, we extract the public key, or
 * the private key.
 */
int cracen_signature_get_rsa_key(struct si_rsa_key *rsa, bool extract_pubkey, bool is_key_pair,
				 const unsigned char *key, size_t keylen, struct sx_buf *modulus,
				 struct sx_buf *exponent)
{
	int ret, version;
	size_t len;
	unsigned char *p, *end;

	p = (unsigned char *)key;
	end = p + keylen;

	if (!extract_pubkey && !is_key_pair) {
		return SX_ERR_INVALID_KEYREF;
	}

	/*
	 * This function parses the RSA keys (PKCS#1)
	 *
	 *  RSAPrivateKey ::= SEQUENCE {
	 *      version           Version,
	 *      modulus           INTEGER,  -- n
	 *      publicExponent    INTEGER,  -- e
	 *      privateExponent   INTEGER,  -- d
	 *      prime1            INTEGER,  -- p
	 *      prime2            INTEGER,  -- q
	 *      exponent1         INTEGER,  -- d mod (p-1)
	 *      exponent2         INTEGER,  -- d mod (q-1)
	 *      coefficient       INTEGER,  -- (inverse of q) mod p
	 *      otherPrimeInfos   OtherPrimeInfos OPTIONAL
	 *  }
	 *
	 *  RSAPublicKey ::= SEQUENCE {
	 *      version           Version,
	 *      modulus           INTEGER,  -- n
	 *      publicExponent    INTEGER,  -- e
	 *  }
	 *
	 *  OpenSSL wraps public keys with an RSA algorithm identifier that we skip
	 *  if it is present.
	 */
	ret = cracen_asn1_get_tag(&p, end, &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE);
	if (ret) {
		return SX_ERR_INVALID_KEYREF;
	}

	end = p + len;

	if (is_key_pair) {
		ret = cracen_asn1_get_int(&p, end, &version);
		if (ret) {
			return SX_ERR_INVALID_KEYREF;
		}
		if (version != 0) {
			return SX_ERR_INVALID_KEYREF;
		}
	} else {
		/* Skip algorithm identifier prefix. */
		unsigned char *id_seq = p;

		ret = cracen_asn1_get_tag(&id_seq, end, &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE);
		if (ret == 0) {
			if (len != sizeof(RSA_ALGORITHM_IDENTIFIER)) {
				return SX_ERR_INVALID_KEYREF;
			}

			if (memcmp(id_seq, RSA_ALGORITHM_IDENTIFIER, len) != 0) {
				return SX_ERR_INVALID_KEYREF;
			}

			id_seq += len;

			ret = cracen_asn1_get_tag(&id_seq, end, &len, ASN1_BIT_STRING);
			if (ret != 0 || *id_seq != 0) {
				return SX_ERR_INVALID_KEYREF;
			}

			p = id_seq + 1;

			ret = cracen_asn1_get_tag(&p, end, &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE);
			if (ret) {
				return SX_ERR_INVALID_KEYREF;
			}
		}
	}

	*rsa = SI_KEY_INIT_RSA(modulus, exponent);

	/* Import N */
	ret = cracen_signature_asn1_get_operand(&p, end, modulus);
	if (ret) {
		return ret;
	}

	if (PSA_BYTES_TO_BITS(modulus->sz) > PSA_MAX_KEY_BITS) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	/* Import E */
	ret = cracen_signature_asn1_get_operand(&p, end, exponent);
	if (ret) {
		return ret;
	}

	if (extract_pubkey) {
		return SX_OK;
	}

	/* Import D */
	ret = cracen_signature_asn1_get_operand(&p, end, exponent);
	if (ret) {
		return ret;
	}

	return SX_OK;
}

int cracen_prepare_ik_key(const uint8_t *user_data)
{
#ifdef CONFIG_CRACEN_IKG_SEED_LOAD
	if (!nrf_cracen_seedram_lock_check(NRF_CRACEN)) {
		if (lib_kmu_push_slot(CONFIG_CRACEN_IKG_SEED_KMU_SLOT + 0) ||
		    lib_kmu_push_slot(CONFIG_CRACEN_IKG_SEED_KMU_SLOT + 1) ||
		    lib_kmu_push_slot(CONFIG_CRACEN_IKG_SEED_KMU_SLOT + 2)) {
			return SX_ERR_INVALID_KEYREF;
		}
		nrf_cracen_seedram_lock_enable_set(NRF_CRACEN, true);
	}
#endif

	__attribute__((unused)) struct sx_pk_config_ik cfg = {};

#ifdef CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
	cfg.device_secret = DEVICE_SECRET_ADDRESS;
	cfg.device_secret_sz = DEVICE_SECRET_LENGTH;

	switch (((uint32_t *)user_data)[0]) {
		/* Helper macro to set up an array containing the personalization string.
		 * The array is a multiple of 4, since the IKG takes a number of uint32_t
		 * as personalization string.
		 */
#define SET_STR(x)                                                                                 \
	{                                                                                          \
		static const char lstr_##x[((sizeof(#x) + 3) / 4) * 4] = #x;                       \
		cfg.key_bundle = (uint32_t *)lstr_##x;                                             \
		cfg.key_bundle_sz = sizeof(lstr_##x) / sizeof(uint32_t);                           \
	}
	case DOMAIN_NONE:
		SET_STR(NONE0);
		break;
	case DOMAIN_SECURE:
		SET_STR(SECURE0);
		break;
	case DOMAIN_APPLICATION:
		SET_STR(APPLICATION0);
		break;
	case DOMAIN_RADIO:
		SET_STR(RADIOCORE0);
		break;
	case DOMAIN_CELL:
		SET_STR(CELL0);
		break;
	case DOMAIN_ISIM:
		SET_STR(ISIM0);
		break;
	case DOMAIN_WIFI:
		SET_STR(WIFI0);
		break;
	case DOMAIN_SYSCTRL:
		SET_STR(SYSCTRL0);
		break;

	default:
		return SX_ERR_INVALID_KEYREF;
	}
#endif


#if defined(CONFIG_CRACEN_IKG)
	return sx_pk_ik_derive_keys(&cfg);
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif
}

static int cracen_clean_ik_key(const uint8_t *user_data)
{
	return SX_OK;
}

static bool cracen_is_ikg_key(const psa_key_attributes_t *attributes)
{
#if CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
	return cracen_platform_keys_is_ikg_key(attributes);
#else
	switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes))) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
	case CRACEN_BUILTIN_MKEK_ID:
	case CRACEN_BUILTIN_MEXT_ID:
		return true;
	default:
		return false;
	}
#endif
};

static psa_status_t cracen_load_ikg_keyref(const psa_key_attributes_t *attributes,
					   const uint8_t *key_buffer, size_t key_buffer_size,
					   struct sxkeyref *k)
{
	k->prepare_key = cracen_prepare_ik_key;
	k->clean_key = cracen_clean_ik_key;

#if CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
	if (key_buffer_size != sizeof(ikg_opaque_key)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	k->cfg = ((ikg_opaque_key *)key_buffer)->slot_number;
	k->owner_id = ((ikg_opaque_key *)key_buffer)->owner_id;
#else
	/* IKG keys are identified from the ID */
	(void)key_buffer;
	(void)key_buffer_size;

	switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes))) {
	case CRACEN_BUILTIN_MKEK_ID:
		k->cfg = CRACEN_INTERNAL_HW_KEY1_ID;
		break;
	case CRACEN_BUILTIN_MEXT_ID:
		k->cfg = CRACEN_INTERNAL_HW_KEY2_ID;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	};

	k->owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes));
#endif
	k->user_data = (uint8_t *)&k->owner_id;
	return PSA_SUCCESS;
}

psa_status_t cracen_load_keyref(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, struct sxkeyref *k)
{
	safe_memzero(k, sizeof(*k));

#if CONFIG_PSA_NEED_CRACEN_KMU_DRIVER
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN_KMU) {
		kmu_opaque_key_buffer *key = (kmu_opaque_key_buffer *)key_buffer;
		enum cracen_kmu_metadata_key_usage_scheme key_usage_scheme = key->key_usage_scheme;

		k->clean_key = cracen_kmu_clean_key;
		k->prepare_key = cracen_kmu_prepare_key;
		k->user_data = key_buffer;

		switch (key_usage_scheme) {
		case CRACEN_KMU_KEY_USAGE_SCHEME_RAW:
		case CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED:
			k->sz = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
			k->key = kmu_push_area;

			return PSA_SUCCESS;
		case CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED:
			k->sz = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
			k->key = (uint8_t *)CRACEN_PROTECTED_RAM_AES_KEY0;

			return PSA_SUCCESS;

		default:
			return PSA_ERROR_NOT_PERMITTED;
		}
	}
#endif
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {

		if (cracen_is_ikg_key(attributes)) {
			if (IS_ENABLED(CONFIG_CRACEN_IKG)) {
				return cracen_load_ikg_keyref(attributes, key_buffer,
							      key_buffer_size, k);
			} else {
				return PSA_ERROR_NOT_SUPPORTED;
			}
		}

		k->owner_id = MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes));
		k->user_data = (uint8_t *)&k->owner_id;
		k->prepare_key = NULL;
		k->clean_key = NULL;

		switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes))) {
		case CRACEN_PROTECTED_RAM_AES_KEY0_ID:
			k->sz = 32;
			k->key = (uint8_t *)CRACEN_PROTECTED_RAM_AES_KEY0;
			break;
		default:
			if (key_buffer_size == 0) {
				return PSA_ERROR_CORRUPTION_DETECTED;
			}

			/* Normal transparent key. */
			k->key = key_buffer;
			k->sz = key_buffer_size;
		}
	} else {
		k->key = key_buffer;
		k->sz = key_buffer_size;
	}

	return PSA_SUCCESS;
}

static psa_status_t cracen_get_ikg_opaque_key_size(const psa_key_attributes_t *attributes,
						   size_t *key_size)
{
#ifdef CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS
	return cracen_platform_keys_get_size(attributes, key_size);
#else
	switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes))) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
		if (psa_get_key_type(attributes) ==
		    PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)) {
			*key_size = sizeof(ikg_opaque_key);
			return PSA_SUCCESS;
		}
		break;
	case CRACEN_BUILTIN_MEXT_ID:
	case CRACEN_BUILTIN_MKEK_ID:
		if (psa_get_key_type(attributes) == PSA_KEY_TYPE_AES) {
			*key_size = sizeof(ikg_opaque_key);
			return PSA_SUCCESS;
		}
		break;
	}

	return PSA_ERROR_INVALID_ARGUMENT;
#endif /* CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS */
}

psa_status_t cracen_get_opaque_size(const psa_key_attributes_t *attributes, size_t *key_size)
{
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN) {
		return cracen_get_ikg_opaque_key_size(attributes, key_size);
	}

	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes)) ==
	    PSA_KEY_LOCATION_CRACEN_KMU) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			if (psa_get_key_type(attributes) ==
			    PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)) {
				*key_size = PSA_EXPORT_PUBLIC_KEY_OUTPUT_SIZE(
					psa_get_key_type(attributes), psa_get_key_bits(attributes));
			} else {
				*key_size = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
			}
		} else if (psa_get_key_type(attributes) == PSA_KEY_TYPE_HMAC) {
			*key_size = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
		} else {
			*key_size = sizeof(kmu_opaque_key_buffer);
		}

		return PSA_SUCCESS;
	}

	return PSA_ERROR_INVALID_ARGUMENT;
}

void cracen_be_add(uint8_t *v, size_t sz, size_t summand)
{
	while (sz > 0) {
		sz--;
		summand += v[sz];
		v[sz] = summand & 0xFF;
		summand >>= 8;
	}
}

int cracen_be_cmp(const uint8_t *a, const uint8_t *b, size_t sz, int carry)
{
	unsigned int neq = 0;
	unsigned int gt = 0;
	unsigned int ucarry;
	unsigned int d;
	unsigned int lt;

	/* transform carry to work with unsigned numbers */
	ucarry = 0x100 + carry;

	for (int i = sz - 1; i >= 0; i--) {
		d = ucarry + a[i] - b[i];
		ucarry = 0xFF + (d >> 8);
		neq |= d & 0xFF;
	}

	neq |= ucarry & 0xFF;
	lt = ucarry < 0x100;
	gt = neq && !lt;

	return (gt ? 1 : 0) - (lt ? 1 : 0);
}

int cracen_hash_all_inputs_with_context(struct sxhash *hashopctx, const uint8_t *inputs[],
					const size_t input_lengths[], size_t input_count,
					const struct sxhashalg *hashalg, uint8_t *digest)
{
	int status;

	status = sx_hash_create(hashopctx, hashalg, sizeof(*hashopctx));
	if (status != SX_OK) {
		return status;
	}

	for (size_t i = 0; i < input_count; i++) {
		status = sx_hash_feed(hashopctx, inputs[i], input_lengths[i]);
		if (status != SX_OK) {
			return status;
		}
	}
	status = sx_hash_digest(hashopctx, digest);
	if (status != SX_OK) {
		return status;
	}

	status = sx_hash_wait(hashopctx);

	return status;
}

int cracen_hash_all_inputs(const uint8_t *inputs[], const size_t input_lengths[],
			   size_t input_count, const struct sxhashalg *hashalg, uint8_t *digest)
{
	struct sxhash hashopctx;

	return cracen_hash_all_inputs_with_context(&hashopctx, inputs, input_lengths, input_count,
						   hashalg, digest);
}

int cracen_hash_input(const uint8_t *input, const size_t input_length,
		      const struct sxhashalg *hashalg, uint8_t *digest)
{
	return cracen_hash_all_inputs(&input, &input_length, 1, hashalg, digest);
}

int cracen_hash_input_with_context(struct sxhash *hashopctx, const uint8_t *input,
				   const size_t input_length, const struct sxhashalg *hashalg,
				   uint8_t *digest)
{
	return cracen_hash_all_inputs_with_context(hashopctx, &input, &input_length, 1, hashalg,
						   digest);
}
