/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "common.h"
#include "cracen_psa_cmac_kdf.h"
#include "cracen_psa_primitives.h"
#include <cracen/mem_helpers.h>
#include <cracen_psa.h>
#include <psa/crypto.h>
#include <psa/crypto_types.h>
#include <psa/crypto_values.h>
#include <psa_crypto_driver_wrappers.h>
#include <sicrypto/hash.h>
#include <sicrypto/hmac.h>
#include <silexpk/ik.h>
#include <silexpk/montgomery.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/eccweierstrass.h>
#include <stdint.h>
#include <string.h>

#define uint32_to_be(i)                                                                            \
	((((i)&0xFF) << 24) | ((((i) >> 8) & 0xFF) << 16) | ((((i) >> 16) & 0xFF) << 8) |          \
	 (((i) >> 24) & 0xFF))

static psa_status_t ecc_key_agreement_check_alg(psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_NOT_SUPPORTED;

	if (PSA_ALG_IS_RAW_KEY_AGREEMENT(alg) && PSA_ALG_IS_ECDH(alg)) {
		IF_ENABLED(PSA_NEED_CRACEN_ECDH_MONTGOMERY, (status = PSA_SUCCESS));
		IF_ENABLED(PSA_NEED_CRACEN_ECDH_SECP_R1, (status = PSA_SUCCESS));
	} else if (PSA_ALG_IS_ECDH(alg) || PSA_ALG_IS_FFDH(alg)) {
		status = PSA_ERROR_NOT_SUPPORTED;
	} else {
		status = PSA_ERROR_INVALID_ARGUMENT;
	}

	return status;
}

static psa_status_t cracen_ecdh_wrstr_calc_secret(const struct sx_pk_ecurve *curve,
						  const uint8_t *priv_key, size_t priv_key_size,
						  const uint8_t *publ_key, size_t publ_key_size,
						  uint8_t *output, size_t output_size,
						  size_t *output_length)
{
	int sx_status;
	psa_status_t psa_status;

	char scratch_char_x[CRACEN_MAC_ECC_PRIVKEY_BYTES];
	char scratch_char_y[CRACEN_MAC_ECC_PRIVKEY_BYTES];

	struct sx_buf priv_key_buff = {.sz = priv_key_size, .bytes = (char *)priv_key};

	sx_pk_affine_point scratch_pnt = {{.bytes = scratch_char_x, .sz = priv_key_size},
					  {.bytes = scratch_char_y, .sz = priv_key_size}};
	sx_pk_affine_point publ_key_pnt = {};

	if (publ_key_size != cracen_ecc_wstr_expected_pub_key_bytes(priv_key_size)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (publ_key[0] != SI_ECC_PUBKEY_UNCOMPRESSED) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* For Weierstrass curves in PSA we only use the X coordinate of the
	 * point multiplication result.
	 */
	if (output_size < priv_key_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	publ_key_pnt.x.bytes = (char *)publ_key + 1;
	publ_key_pnt.x.sz = priv_key_size;

	publ_key_pnt.y.bytes = (char *)publ_key + priv_key_size + 1;
	publ_key_pnt.y.sz = priv_key_size;

	psa_status = cracen_ecc_check_public_key(curve, &publ_key_pnt);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	sx_status = sx_ecp_ptmult(curve, &priv_key_buff, &publ_key_pnt, &scratch_pnt);
	if (sx_status == SX_OK) {
		sx_pk_ecop2mem(&scratch_pnt.x, output, scratch_pnt.x.sz);
		*output_length = scratch_pnt.x.sz;
	}

	safe_memzero(scratch_pnt.x.bytes, scratch_pnt.x.sz);
	safe_memzero(scratch_pnt.y.bytes, scratch_pnt.y.sz);

	return silex_statuscodes_to_psa(sx_status);
}

/**
 * @brief Convert a byte array containing a scalar to sx_x25519_op.
 *
 * Loads a byte array (of size 32) and decodes it. This corresponds to the
 * decodeScalar25519 function in RFC 7748.
 */
static struct sx_x25519_op decode_scalar_25519(const uint8_t *k)
{
	struct sx_x25519_op r;

	memcpy(r.bytes, k, CRACEN_X25519_KEY_SIZE_BYTES);

	r.bytes[0] &= 248;
	r.bytes[31] &= 127;
	r.bytes[31] |= 64;

	return r;
}

/**
 * @brief Convert a byte array containing a coordinate to sx_x25519_op.
 *
 * Loads a byte array (of size 32) and decodes it. This corresponds to the
 * decodeUCoordinate (for 255 bits) function in RFC 7748.
 */
static struct sx_x25519_op decode_coordinate_25519(const uint8_t *k)
{
	struct sx_x25519_op r;

	memcpy(r.bytes, k, CRACEN_X25519_KEY_SIZE_BYTES);

	r.bytes[31] &= (1 << 7) - 1;

	return r;
}

/**
 * @brief Convert a byte array containing a scalar to sx_x448_op.
 *
 * Loads a byte array (of size 56) and decodes it. This corresponds to the
 * decodeScalar448 function in RFC 7748.
 */
static struct sx_x448_op decode_scalar_448(const uint8_t *k)
{
	struct sx_x448_op r;

	memcpy(r.bytes, k, CRACEN_X448_KEY_SIZE_BYTES);

	r.bytes[0] &= 252;
	r.bytes[55] |= 128;

	return r;
}

static psa_status_t cracen_ecdh_montgmr_calc_secret(const struct sx_pk_ecurve *curve,
						    const uint8_t *priv_key, size_t priv_key_size,
						    const uint8_t *publ_key, size_t publ_key_size,
						    uint8_t *output, size_t output_size,
						    size_t *output_length)
{
	int sx_status;
	const size_t curve_op_sz = sx_pk_curve_opsize(curve);

	if (publ_key_size != curve_op_sz) {
		return PSA_ERROR_INVALID_ARGUMENT;
	} else if (output_size < curve_op_sz) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	sx_status = SX_ERR_INVALID_CURVE_PARAM;

	if (curve_op_sz == CRACEN_X25519_KEY_SIZE_BYTES) {
		struct sx_x25519_op k = decode_scalar_25519(priv_key);
		struct sx_x25519_op pt = decode_coordinate_25519(publ_key);

		sx_status = sx_x25519_ptmult(&k, &pt, (struct sx_x25519_op *)output);

	} else if (curve_op_sz == CRACEN_X448_KEY_SIZE_BYTES) {
		struct sx_x448_op k = decode_scalar_448(priv_key);
		/* 448 % 8 = 0, so there is no need to decode pt coordinate. */
		sx_status = sx_x448_ptmult(&k, (struct sx_x448_op *)publ_key,
					   (struct sx_x448_op *)output);
	}

	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	*output_length = curve_op_sz;

	return PSA_SUCCESS;
}

/**
 * \brief Initialize and set up the MAC task that will be used to generate pseudo-random
 *        bytes for HDKF and PBKDF2.
 *
 * \param[in, out] operation        Cracen key derivation operation object.
 * \param[in]      key_buffer       Key buffer or HKDF salt.
 * \param[in]      key_buffer_size  Size of key buffer in bytes.
 *
 * \return psa_status_t PSA status code.
 */
static psa_status_t start_mac_operation(cracen_key_derivation_operation_t *operation,
					const uint8_t *key_buffer, size_t key_buffer_size)
{
	psa_key_attributes_t attributes = {0};

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(key_buffer_size));

	psa_algorithm_t mac_alg;

	if (operation->alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128) {
		psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
		mac_alg = PSA_ALG_CMAC;
	} else {
		psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
		mac_alg = PSA_ALG_HMAC(PSA_ALG_GET_HASH(operation->alg));
	}
	return cracen_mac_sign_setup(&operation->mac_op, &attributes, key_buffer, key_buffer_size,
				     mac_alg);
}

static size_t pbkdf2_prf_output_length(psa_algorithm_t alg)
{
	if (alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128) {
		return SX_BLKCIPHER_AES_BLK_SZ;
	} else {
		return PSA_HASH_LENGTH(PSA_ALG_GET_HASH(alg));
	}
}

psa_status_t cracen_key_derivation_setup(cracen_key_derivation_operation_t *operation,
					 psa_algorithm_t alg)
{
	operation->alg = alg;

	if (IS_ENABLED(PSA_NEED_CRACEN_HKDF) && PSA_ALG_IS_HKDF(operation->alg)) {
		size_t hash_size = PSA_HASH_LENGTH(PSA_ALG_HKDF_GET_HASH(alg));

		if (hash_size == 0) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		operation->capacity =
			UINT8_MAX * hash_size; /* Max value of counter (1 byte) size of hash. */
		operation->state = CRACEN_KD_STATE_HKDF_INIT;

		return PSA_SUCCESS;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_PBKDF2_HMAC) && PSA_ALG_IS_PBKDF2(operation->alg)) {
		size_t output_length = pbkdf2_prf_output_length(operation->alg);

		if (output_length == 0) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		operation->capacity = (uint64_t)(UINT32_MAX)*output_length;
		operation->state = CRACEN_KD_STATE_PBKDF2_INIT;
		return PSA_SUCCESS;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS)) {
		if (operation->alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
			operation->capacity = PSA_TLS12_ECJPAKE_TO_PMS_DATA_SIZE;
			operation->state = CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_INIT;
			return PSA_SUCCESS;
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_TLS12_PRF) && PSA_ALG_IS_TLS12_PRF(operation->alg)) {
		operation->state = CRACEN_KD_STATE_TLS12_PRF_INIT;
		operation->capacity = UINT64_MAX;
		return PSA_SUCCESS;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_TLS12_PSK_TO_MS) &&
	    PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
		operation->state = CRACEN_KD_STATE_TLS12_PSK_TO_MS_INIT;
		operation->capacity = UINT64_MAX;
		return PSA_SUCCESS;
	}

	if (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC) {
		operation->capacity = PSA_ALG_SP800_108_COUNTER_CMAC_INIT_CAPACITY;
		operation->state = CRACEN_KD_STATE_CMAC_CTR_INIT;
		/* CMAC CTR key derivation starts the counter with 1, see NIST.SP.800-108r1 */
		operation->cmac_ctr.counter = 1;

		return PSA_SUCCESS;
	}

	return PSA_ERROR_NOT_SUPPORTED;
}
psa_status_t cracen_key_derivation_set_capacity(cracen_key_derivation_operation_t *operation,
						size_t capacity)
{
	if (operation->state == CRACEN_KD_STATE_INVALID) {
		return PSA_ERROR_BAD_STATE;
	}

	if (capacity > operation->capacity) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	operation->capacity = capacity;
	/* The capacity changes when the output bytes are derived, but L must not change, therefore
	 * saving it separately
	 */
	if (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC) {
		operation->cmac_ctr.L = uint32_to_be(PSA_BYTES_TO_BITS(operation->capacity));
	}
	return PSA_SUCCESS;
}

static psa_status_t
cracen_key_derivation_input_bytes_hkdf(cracen_key_derivation_operation_t *operation,
				       psa_key_derivation_step_t step, const uint8_t *data,
				       size_t data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	/* Operation must be initialized to a HKDF state */
	if (!(operation->state & CRACEN_KD_STATE_HKDF_INIT)) {
		return PSA_ERROR_BAD_STATE;
	}

	/* No more input can be provided after we've started outputting data. */
	if (operation->state == CRACEN_KD_STATE_HKDF_OUTPUT) {
		return PSA_ERROR_BAD_STATE;
	}

	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_SALT:
		/* This must be the first input */
		if (operation->state != CRACEN_KD_STATE_HKDF_INIT) {
			return PSA_ERROR_BAD_STATE;
		}
		status = start_mac_operation(operation, data, data_length);
		if (status != PSA_SUCCESS) {
			return status;
		}
		operation->state = CRACEN_KD_STATE_HKDF_STARTED;
		break;

	case PSA_KEY_DERIVATION_INPUT_SECRET: {
		size_t hmac_length = 0;

		if (operation->state == CRACEN_KD_STATE_HKDF_INIT) {
			/* rfc5869: salt is optional. Set to string of HashLen
			 * zeroes if not provided.
			 * We reuse prk from the operation context since it's
			 * not in use yet.
			 */
			safe_memzero(operation->hkdf.prk, sizeof(operation->hkdf.prk));
			status = start_mac_operation(
				operation, operation->hkdf.prk,
				PSA_HASH_BLOCK_LENGTH(PSA_ALG_GET_HASH(operation->alg)));
			if (status != PSA_SUCCESS) {
				return status;
			}
		} else if (operation->state != CRACEN_KD_STATE_HKDF_STARTED) {
			return PSA_ERROR_BAD_STATE;
		}

		status = cracen_mac_update(&operation->mac_op, data, data_length);
		if (status != PSA_SUCCESS) {
			return status;
		}
		operation->state = CRACEN_KD_STATE_HKDF_KEYED;
		return cracen_mac_sign_finish(
			&operation->mac_op, operation->hkdf.prk,
			PSA_HASH_BLOCK_LENGTH(PSA_ALG_GET_HASH(operation->alg)), &hmac_length);
	}

	case PSA_KEY_DERIVATION_INPUT_INFO:
		if (operation->state == CRACEN_KD_STATE_HKDF_OUTPUT) {
			return PSA_ERROR_BAD_STATE;
		}
		if (operation->hkdf.info_set) {
			return PSA_ERROR_BAD_STATE;
		}

		if (data_length > sizeof(operation->hkdf.info)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}

		memcpy(operation->hkdf.info, data, data_length);
		operation->hkdf.info_length = data_length;
		operation->hkdf.info_set = true;

		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static psa_status_t
cracen_key_derivation_input_bytes_pbkdf2(cracen_key_derivation_operation_t *operation,
					 psa_key_derivation_step_t step, const uint8_t *data,
					 size_t data_length)
{
	/* Operation must be initialized to a PBKDF2 state */
	if (!(operation->state & CRACEN_KD_STATE_PBKDF2_INIT)) {
		return PSA_ERROR_BAD_STATE;
	}

	/* No more input can be provided after we've started outputting data. */
	if (operation->state == CRACEN_KD_STATE_PBKDF2_OUTPUT) {
		return PSA_ERROR_BAD_STATE;
	}

	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_SALT:
		if ((data_length + operation->pbkdf2.salt_length) >
		    sizeof(operation->pbkdf2.salt)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		/* Must provided one or more times. If used multiple times, the
		 * inputs will be concatenated.
		 */
		memcpy(operation->pbkdf2.salt + operation->pbkdf2.salt_length, data, data_length);
		operation->pbkdf2.salt_length += data_length;
		operation->state = CRACEN_KD_STATE_PBKDF2_SALT;
		break;

	case PSA_KEY_DERIVATION_INPUT_PASSWORD:
		/* Salt must have been provided. Password must be provided once.
		 */
		if (operation->state == CRACEN_KD_STATE_PBKDF2_PASSWORD ||
		    operation->state != CRACEN_KD_STATE_PBKDF2_SALT) {
			return PSA_ERROR_BAD_STATE;
		}

		size_t output_length = pbkdf2_prf_output_length(operation->alg);
		bool aes_cmac_prf = operation->alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128;

		if (aes_cmac_prf && data_length != output_length) {
			/* Password must be 128-bit (AES Key size) */
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		if (data_length > output_length) {
			size_t hash_length = 0;
			/* Password needs to be hashed. */
			psa_status_t status = cracen_hash_compute(
				PSA_ALG_HMAC_GET_HASH(operation->alg), data, data_length,
				operation->pbkdf2.password,
				PSA_HASH_LENGTH(PSA_ALG_GET_HASH(operation->alg)), &hash_length);
			operation->pbkdf2.password_length = hash_length;
			if (status != PSA_SUCCESS) {
				return status;
			}
		} else {
			memcpy(operation->pbkdf2.password, data, data_length);
			operation->pbkdf2.password_length = data_length;
		}

		operation->state = CRACEN_KD_STATE_PBKDF2_PASSWORD;
		break;

	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static psa_status_t
cracen_key_derivation_input_bytes_cmac_ctr(cracen_key_derivation_operation_t *operation,
					   psa_key_derivation_step_t step, const uint8_t *data,
					   size_t data_length)
{
	/* Make sure the key is already loaded here and allow multiple calls to the function in
	 * order to fill label/context
	 */
	if (operation->state != CRACEN_KD_STATE_CMAC_CTR_KEY_LOADED &&
	    operation->state != CRACEN_KD_STATE_CMAC_CTR_INPUT_LABEL &&
	    operation->state != CRACEN_KD_STATE_CMAC_CTR_INPUT_CONTEXT) {
		return PSA_ERROR_BAD_STATE;
	}

	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_LABEL: {
		size_t label_remaining_bytes =
			sizeof(operation->cmac_ctr.label) - operation->cmac_ctr.label_length;

		/* Reserve the last byte of the label for setting the byte 0x0 which is required
		 * by the CMAC CTR key derivation.
		 */
		if (label_remaining_bytes > 0) {
			label_remaining_bytes--;
		}

		if (data_length > label_remaining_bytes) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}

		memcpy(&operation->cmac_ctr.label[operation->cmac_ctr.label_length], data,
		       data_length);
		operation->cmac_ctr.label_length += data_length;
		operation->state = CRACEN_KD_STATE_CMAC_CTR_INPUT_LABEL;
		break;
	}
	case PSA_KEY_DERIVATION_INPUT_CONTEXT: {
		size_t context_remaining_bytes =
			sizeof(operation->cmac_ctr.context) - operation->cmac_ctr.context_length;
		if (data_length > context_remaining_bytes) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}

		memcpy(&operation->cmac_ctr.context[operation->cmac_ctr.context_length], data,
		       data_length);
		operation->cmac_ctr.context_length += data_length;
		operation->state = CRACEN_KD_STATE_CMAC_CTR_INPUT_CONTEXT;
		break;
	}

	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static psa_status_t
cracen_key_derivation_input_bytes_tls12(cracen_key_derivation_operation_t *operation,
					psa_key_derivation_step_t step, const uint8_t *data,
					size_t data_length)
{
	/* Operation must be initialized to a TLS12 PRF state */
	if (operation->state != CRACEN_KD_STATE_TLS12_PRF_INIT &&
	    operation->state != CRACEN_KD_STATE_TLS12_PSK_TO_MS_INIT) {
		return PSA_ERROR_BAD_STATE;
	}

	/* Each input can only be provided once. */
	switch (step) {
	case PSA_KEY_DERIVATION_INPUT_SEED:
		if (data_length > sizeof(operation->tls12.seed)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		memcpy(operation->tls12.seed, data, data_length);
		operation->tls12.seed_length = data_length;
		break;
	case PSA_KEY_DERIVATION_INPUT_OTHER_SECRET:
		if (!PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		if (data_length > (sizeof(operation->tls12.secret) - 4)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		/* First two bytes is uint16 that will be encoded in the mandatory
		 * PSA_KEY_DERIVATION_INPUT_SECRET step
		 */
		memcpy(operation->tls12.secret + 2, data, data_length);
		operation->tls12.secret_length = data_length;
		break;
	case PSA_KEY_DERIVATION_INPUT_SECRET:
		if (data_length > sizeof(operation->tls12.secret)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		if (PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
			if (data_length > PSA_TLS12_PSK_TO_MS_PSK_MAX_SIZE) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
			size_t other_secret_length = operation->tls12.secret_length;
			/* Other secret not provided - plain PSK. */
			if (other_secret_length == 0) {
				memset(operation->tls12.secret, 0, data_length);
				other_secret_length = data_length;
			}
			operation->tls12.secret[0] = other_secret_length >> 8;
			operation->tls12.secret[1] = other_secret_length;
			operation->tls12.secret[other_secret_length + 2] = data_length >> 8;
			operation->tls12.secret[other_secret_length + 3] = data_length;
			memcpy(operation->tls12.secret + other_secret_length + 4, data,
			       data_length);
			operation->tls12.secret_length = other_secret_length + data_length + 4;
		} else if (PSA_ALG_IS_TLS12_PRF(operation->alg)) {
			memcpy(operation->tls12.secret, data, data_length);
			operation->tls12.secret_length = data_length;
		} else {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
	case PSA_KEY_DERIVATION_INPUT_LABEL:
		if (data_length > sizeof(operation->tls12.label)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		memcpy(operation->tls12.label, data, data_length);
		operation->tls12.label_length = data_length;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	return PSA_SUCCESS;
}
psa_status_t cracen_key_derivation_input_bytes(cracen_key_derivation_operation_t *operation,
					       psa_key_derivation_step_t step, const uint8_t *data,
					       size_t data_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_HKDF) && PSA_ALG_IS_HKDF(operation->alg)) {
		return cracen_key_derivation_input_bytes_hkdf(operation, step, data, data_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_PBKDF2_HMAC) && PSA_ALG_IS_PBKDF2(operation->alg)) {
		return cracen_key_derivation_input_bytes_pbkdf2(operation, step, data, data_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC) &&
	    (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC)) {
		return cracen_key_derivation_input_bytes_cmac_ctr(operation, step, data,
								  data_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS) &&
	    operation->alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
		if (operation->state != CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_INIT) {
			return PSA_ERROR_BAD_STATE;
		}
		operation->state = CRACEN_KD_STATE_TLS12_ECJPAKE_TO_PMS_OUTPUT;
		if (data_length != 65 || data[0] != 0x04) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		/* Copy the x coordinate of the point. */
		memcpy(operation->ecjpake_to_pms.key, data + 1,
		       sizeof(operation->ecjpake_to_pms.key));
		return PSA_SUCCESS;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_TLS12_PRF) && PSA_ALG_IS_TLS12_PRF(operation->alg)) {
		return cracen_key_derivation_input_bytes_tls12(operation, step, data, data_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_TLS12_PSK_TO_MS) &&
	    PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
		return cracen_key_derivation_input_bytes_tls12(operation, step, data, data_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_key_derivation_input_key(cracen_key_derivation_operation_t *operation,
					     psa_key_derivation_step_t step,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation->alg != PSA_ALG_SP800_108_COUNTER_CMAC) {
		return cracen_key_derivation_input_bytes(operation, step, key_buffer,
							 key_buffer_size);
	}

	if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (operation->state != CRACEN_KD_STATE_CMAC_CTR_INIT ||
	    step != PSA_KEY_DERIVATION_INPUT_SECRET) {
		return PSA_ERROR_BAD_STATE;
	}

	/*
	 * Copy the key into the operation struct as it is not guaranteed
	 * to be valid longer than the function call.
	 */
	if (key_buffer_size > sizeof(operation->cmac_ctr.key_buffer)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	memcpy(operation->cmac_ctr.key_buffer, key_buffer,
	       PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)));

	status = cracen_load_keyref(attributes, operation->cmac_ctr.key_buffer, key_buffer_size,
				    &operation->cmac_ctr.keyref);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->state = CRACEN_KD_STATE_CMAC_CTR_KEY_LOADED;
	return status;
}

psa_status_t cracen_key_derivation_input_integer(cracen_key_derivation_operation_t *operation,
						 psa_key_derivation_step_t step, uint64_t value)
{

	if (IS_ENABLED(PSA_NEED_CRACEN_PBKDF2_HMAC)) {
		if ((PSA_ALG_IS_PBKDF2(operation->alg)) && step == PSA_KEY_DERIVATION_INPUT_COST) {
			if (operation->pbkdf2.input_cost) {
				/* Can only be provided once. */
				return PSA_ERROR_BAD_STATE;
			}
			operation->pbkdf2.input_cost = value;
			return PSA_SUCCESS;
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

static int
cracen_key_derivation_cmac_ctr_add_core_fixed_input(cracen_key_derivation_operation_t *operation,
						    struct sxmac *cmac_ctx)
{
	/* Make sure the byte after the label is set to zero */
	safe_memzero(operation->cmac_ctr.label + operation->cmac_ctr.label_length, 1);

	/* Label + 0x00*/
	int sx_status = sx_mac_feed(cmac_ctx, operation->cmac_ctr.label,
				    operation->cmac_ctr.label_length + 1);
	if (sx_status) {
		return sx_status;
	}

	/* Context */
	sx_status = sx_mac_feed(cmac_ctx, operation->cmac_ctr.context,
				operation->cmac_ctr.context_length);
	if (sx_status) {
		return sx_status;
	}

	/* L_4 */
	return sx_mac_feed(cmac_ctx, (uint8_t *)&operation->cmac_ctr.L,
			   sizeof(operation->cmac_ctr.L));
}

static psa_status_t
cracen_key_derivation_cmac_ctr_generate_K_0(cracen_key_derivation_operation_t *operation)
{
	struct sxmac cmac_ctx;
	int sx_status;

	sx_status = sx_mac_create_aescmac(&cmac_ctx, &operation->cmac_ctr.keyref);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = cracen_key_derivation_cmac_ctr_add_core_fixed_input(operation, &cmac_ctx);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_mac_generate(&cmac_ctx, operation->cmac_ctr.K_0);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_mac_wait(&cmac_ctx);

	return silex_statuscodes_to_psa(sx_status);
}

/**
 * @brief Generate a PRF block based on CMAC in counter mode (NIST.SP.800-108r1)
 *
 * Here are the parameters of this implementation:
 * r = 32 bits (counter is a uint32_t)
 * h = 128 bits (since we use CMAC the output block is 128 bit long)
 *
 * The algorithm specifies L as the bit length of the requested data length but the
 * PSA APIS don't support setting the length of the requested output so here we
 * always set L = 128 bits since we always output a single block.
 *
 */
static psa_status_t
cracen_key_derivation_cmac_ctr_generate_block(cracen_key_derivation_operation_t *operation)
{
	struct sxmac cmac_ctx;
	int sx_status;
	uint32_t counter_be;

	sx_status = sx_mac_create_aescmac(&cmac_ctx, &operation->cmac_ctr.keyref);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	counter_be = uint32_to_be(operation->cmac_ctr.counter);
	sx_status = sx_mac_feed(&cmac_ctx, (uint8_t *)&counter_be, sizeof(counter_be));
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = cracen_key_derivation_cmac_ctr_add_core_fixed_input(operation, &cmac_ctx);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}
	sx_status =
		sx_mac_feed(&cmac_ctx, operation->cmac_ctr.K_0, sizeof(operation->cmac_ctr.K_0));
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_mac_generate(&cmac_ctx, operation->output_block);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_mac_wait(&cmac_ctx);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	operation->output_block_available_bytes = SX_BLKCIPHER_AES_BLK_SZ;
	operation->cmac_ctr.counter++;
	return PSA_SUCCESS;
}

/**
 * \brief Generates the next block for HKDF.
 *
 * \param[in,out] operation  The key derivation operation.
 *
 * \return psa_status_t
 */
static psa_status_t
cracen_key_derivation_hkdf_generate_block(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	unsigned int digestsz = PSA_HASH_LENGTH(PSA_ALG_GET_HASH(operation->alg));
	size_t hmac_length = 0;

	/* Create T(N) = HMAC-Hash(PRK, T(N-1) || info || N) */
	status = start_mac_operation(operation, operation->hkdf.prk, digestsz);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (operation->hkdf.blk_counter) {
		/* T(0) is empty. */
		status = cracen_mac_update(&operation->mac_op, operation->hkdf.t, digestsz);
		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	operation->hkdf.blk_counter++;

	status = cracen_mac_update(&operation->mac_op, operation->hkdf.info,
				   operation->hkdf.info_length);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_update(&operation->mac_op, &operation->hkdf.blk_counter,
				   sizeof(operation->hkdf.blk_counter));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_sign_finish(&operation->mac_op, operation->hkdf.t, digestsz,
					&hmac_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	memcpy(operation->output_block, operation->hkdf.t, digestsz);
	operation->output_block_available_bytes = digestsz;

	return PSA_SUCCESS;
}

/**
 * \brief Generates the next block for PBKDF2.
 *
 * \param[in,out] operation  The key derivation operation.
 *
 * \return psa_status_t
 */
static psa_status_t
cracen_key_derivation_pbkdf2_generate_block(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	unsigned int h_len = pbkdf2_prf_output_length(operation->alg);
	size_t mac_length = 0;

	operation->pbkdf2.blk_counter++;
	uint32_t blk_counter_be = uint32_to_be(operation->pbkdf2.blk_counter);

	/* Generate U1 = MAC(Password, Salt || BigEndian(i)) */
	status = start_mac_operation(operation, operation->pbkdf2.password,
				     operation->pbkdf2.password_length);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_update(&operation->mac_op, operation->pbkdf2.salt,
				   operation->pbkdf2.salt_length);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_update(&operation->mac_op, (uint8_t *)&blk_counter_be,
				   sizeof(blk_counter_be));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_mac_sign_finish(&operation->mac_op, operation->pbkdf2.uj,
					sizeof(operation->pbkdf2.uj), &mac_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	memcpy(operation->pbkdf2.tj, operation->pbkdf2.uj, h_len);

	for (uint64_t i = 1; i < operation->pbkdf2.input_cost; i++) {
		status = start_mac_operation(operation, operation->pbkdf2.password,
					     operation->pbkdf2.password_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_mac_update(&operation->mac_op, operation->pbkdf2.uj, h_len);
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_mac_sign_finish(&operation->mac_op, operation->pbkdf2.uj, h_len,
						&mac_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* compute T_i = T_i ^ U_j */
		for (size_t i = 0; i < h_len; i++) {
			operation->pbkdf2.tj[i] = operation->pbkdf2.tj[i] ^ operation->pbkdf2.uj[i];
		}
	}

	memcpy(operation->output_block, operation->pbkdf2.tj, h_len);
	operation->output_block_available_bytes = h_len;

	return PSA_SUCCESS;
}

static psa_status_t
cracen_key_derivation_tls12_prf_generate_block(cracen_key_derivation_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t length;

	const struct sxhashalg *hash;

	status = silex_statuscodes_to_psa(hash_get_algo(PSA_ALG_GET_HASH(operation->alg), &hash));
	if (status != PSA_SUCCESS) {
		return status;
	}
	size_t digest_size = sx_hash_get_alg_digestsz(hash);

	operation->tls12.counter++;

	/* A(i) = HMAC(A(i - 1)), A(0) = label || seed */
	status = start_mac_operation(operation, operation->tls12.secret,
				     operation->tls12.secret_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (operation->tls12.counter == 1) {
		status = cracen_mac_update(&operation->mac_op, operation->tls12.label,
					   operation->tls12.label_length);
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_mac_update(&operation->mac_op, operation->tls12.seed,
					   operation->tls12.seed_length); /* A(0) */
	} else {
		status = cracen_mac_update(&operation->mac_op, operation->tls12.a,
					   digest_size); /* A(1) */
	}
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_sign_finish(&operation->mac_op, operation->tls12.a, digest_size,
					&length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* P_hash() = HMAC(A(i) || label || seed) */
	status = start_mac_operation(operation, operation->tls12.secret,
				     operation->tls12.secret_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_update(&operation->mac_op, operation->tls12.a, digest_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_update(&operation->mac_op, operation->tls12.label,
				   operation->tls12.label_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_update(&operation->mac_op, operation->tls12.seed,
				   operation->tls12.seed_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_mac_sign_finish(&operation->mac_op, operation->output_block, digest_size,
					&length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->output_block_available_bytes = digest_size;

	return status;
}

psa_status_t cracen_key_agreement(const psa_key_attributes_t *attributes, const uint8_t *priv_key,
				  size_t priv_key_size, const uint8_t *publ_key,
				  size_t publ_key_size, uint8_t *output, size_t output_size,
				  size_t *output_length, psa_algorithm_t alg)
{
	psa_ecc_family_t curve_family = PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(attributes));
	size_t curve_bits = psa_get_key_bits(attributes);
	psa_status_t psa_status;
	const struct sx_pk_ecurve *curve;

	*output_length = 0;

	psa_status = ecc_key_agreement_check_alg(alg);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	if (!PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(attributes))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_status = cracen_ecc_get_ecurve_from_psa(curve_family, curve_bits, &curve);
	if (psa_status != PSA_SUCCESS) {
		return PSA_SUCCESS;
	}

	if (sx_pk_curve_opsize(curve) != priv_key_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECDH_SECP_R1)) {
		if (cracen_ecc_curve_is_weierstrass(curve_family)) {
			return cracen_ecdh_wrstr_calc_secret(curve, priv_key, priv_key_size,
							     publ_key, publ_key_size, output,
							     output_size, output_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECDH_MONTGOMERY)) {
		if (curve_family == PSA_ECC_FAMILY_MONTGOMERY) {
			return cracen_ecdh_montgmr_calc_secret(curve, priv_key, priv_key_size,
							       publ_key, publ_key_size, output,
							       output_size, output_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_key_derivation_output_bytes(cracen_key_derivation_operation_t *operation,
						uint8_t *output, size_t output_length)
{
	psa_status_t (*generator)(cracen_key_derivation_operation_t *) = NULL;

	if (IS_ENABLED(PSA_NEED_CRACEN_HKDF) && PSA_ALG_IS_HKDF(operation->alg)) {
		if (operation->state < CRACEN_KD_STATE_HKDF_KEYED || !operation->hkdf.info_set) {
			return PSA_ERROR_BAD_STATE;
		}

		operation->state = CRACEN_KD_STATE_HKDF_OUTPUT;
		generator = cracen_key_derivation_hkdf_generate_block;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_PBKDF2_HMAC) && PSA_ALG_IS_PBKDF2(operation->alg)) {
		/* Salt, password and input cost must have been provided. */
		if (!operation->pbkdf2.input_cost) {
			return PSA_ERROR_BAD_STATE;
		}

		if (operation->state != CRACEN_KD_STATE_PBKDF2_PASSWORD &&
		    operation->state != CRACEN_KD_STATE_PBKDF2_OUTPUT) {
			return PSA_ERROR_BAD_STATE;
		}

		operation->state = CRACEN_KD_STATE_PBKDF2_OUTPUT;
		generator = cracen_key_derivation_pbkdf2_generate_block;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC) &&
	    (operation->alg == PSA_ALG_SP800_108_COUNTER_CMAC)) {
		if (operation->state == CRACEN_KD_STATE_CMAC_CTR_KEY_LOADED ||
		    operation->state == CRACEN_KD_STATE_CMAC_CTR_INPUT_LABEL ||
		    operation->state == CRACEN_KD_STATE_CMAC_CTR_INPUT_CONTEXT ||
		    operation->state == CRACEN_KD_STATE_CMAC_CTR_OUTPUT) {
			if (operation->state != CRACEN_KD_STATE_CMAC_CTR_OUTPUT) {
				operation->state = CRACEN_KD_STATE_CMAC_CTR_OUTPUT;
				psa_status_t status =
					cracen_key_derivation_cmac_ctr_generate_K_0(operation);
				if (status != PSA_SUCCESS) {
					return status;
				}
			}
			generator = cracen_key_derivation_cmac_ctr_generate_block;

		} else {
			return PSA_ERROR_BAD_STATE;
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS) &&
	    operation->alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
		size_t outlen;
		psa_status_t status = psa_driver_wrapper_hash_compute(
			PSA_ALG_SHA_256, operation->ecjpake_to_pms.key, 32, output, 32, &outlen);
		if (status != PSA_SUCCESS) {
			return status;
		}

		if (outlen != 32) {
			return PSA_ERROR_HARDWARE_FAILURE;
		}
		return PSA_SUCCESS;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_TLS12_PRF) && PSA_ALG_IS_TLS12_PRF(operation->alg)) {
		operation->state = CRACEN_KD_STATE_TLS12_PRF_OUTPUT;
		generator = cracen_key_derivation_tls12_prf_generate_block;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_TLS12_PSK_TO_MS) &&
	    PSA_ALG_IS_TLS12_PSK_TO_MS(operation->alg)) {
		operation->state = CRACEN_KD_STATE_TLS12_PSK_TO_MS_OUTPUT;
		generator = cracen_key_derivation_tls12_prf_generate_block;
	}

	if (generator == NULL) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (output_length > operation->capacity) {
		return PSA_ERROR_INSUFFICIENT_DATA;
	}

	operation->capacity -= output_length;

	/* Fill up the output buffer with generated bytes. */
	while (output_length) {
		if (operation->output_block_available_bytes) {
			/* Copy out buffered output. This may be a partial
			 * block.
			 */
			unsigned int out =
				MIN(output_length, operation->output_block_available_bytes);

			memcpy(output, operation->output_block, out);

			output += out;
			output_length -= out;
			operation->output_block_available_bytes -= out;

			if (operation->output_block_available_bytes) {
				memmove(operation->output_block, operation->output_block + out,
					operation->output_block_available_bytes);
			}
		} else {
			/* No data available, produce new block. */
			psa_status_t status = generator(operation);

			if (status != PSA_SUCCESS) {
				return status;
			}
		}
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_key_derivation_abort(cracen_key_derivation_operation_t *operation)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_HKDF) && PSA_ALG_IS_HKDF(operation->alg)) {
		cracen_mac_abort(&operation->mac_op);
	}

	safe_memzero(operation, sizeof(*operation));
	return PSA_SUCCESS;
}
