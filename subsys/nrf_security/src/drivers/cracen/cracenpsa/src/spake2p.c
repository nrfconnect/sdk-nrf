/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen_psa_primitives.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <psa/crypto_sizes.h>
#include <psa/crypto_types.h>
#include <psa/crypto_values.h>
#include <psa_crypto_driver_wrappers.h>
#include <silexpk/cmddefs/ecc.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <silexpk/iomem.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/eccweierstrass.h>
#include <silexpk/sxops/rsa.h>
#include <stdint.h>
#include <stdlib.h>

#define MAKE_SX_POINT(name, ptr, point_size)                                                       \
	sx_pk_affine_point name = {{.bytes = (ptr), .sz = (point_size) / 2},                       \
				   {.bytes = (ptr) + (point_size) / 2, .sz = (point_size) / 2}};

/*
 * This driver supports uncompressed points only. Uncompressed types
 * are prefixed with the byte 0x04. See 2.3.3.3.3 from "SEC 1:
 * Elliptic Curve Cryptography" for details.
 */
#define UNCOMPRESSED_POINT_TYPE 0x04

/* Point generation seeds specified for P-256 by RFC 9382. */
static const uint8_t SPAKE2P_POINT_M[] = {
	0x88, 0x6e, 0x2f, 0x97, 0xac, 0xe4, 0x6e, 0x55, 0xba, 0x9d, 0xd7, 0x24, 0x25,
	0x79, 0xf2, 0x99, 0x3b, 0x64, 0xe1, 0x6e, 0xf3, 0xdc, 0xab, 0x95, 0xaf, 0xd4,
	0x97, 0x33, 0x3d, 0x8f, 0xa1, 0x2f, 0x5f, 0xf3, 0x55, 0x16, 0x3e, 0x43, 0xce,
	0x22, 0x4e, 0x0b, 0x0e, 0x65, 0xff, 0x02, 0xac, 0x8e, 0x5c, 0x7b, 0xe0, 0x94,
	0x19, 0xc7, 0x85, 0xe0, 0xca, 0x54, 0x7d, 0x55, 0xa1, 0x2e, 0x2d, 0x20};
static const uint8_t SPAKE2P_POINT_N[] = {
	0xd8, 0xbb, 0xd6, 0xc6, 0x39, 0xc6, 0x29, 0x37, 0xb0, 0x4d, 0x99, 0x7f, 0x38,
	0xc3, 0x77, 0x07, 0x19, 0xc6, 0x29, 0xd7, 0x01, 0x4d, 0x49, 0xa2, 0x4b, 0x4f,
	0x98, 0xba, 0xa1, 0x29, 0x2b, 0x49, 0x07, 0xd6, 0x0a, 0xa6, 0xbf, 0xad, 0xe4,
	0x50, 0x08, 0xa6, 0x36, 0x33, 0x7f, 0x51, 0x68, 0xc6, 0x4d, 0x9b, 0xd3, 0x60,
	0x34, 0x80, 0x8c, 0xd5, 0x64, 0x49, 0x0b, 0x1e, 0x65, 0x6e, 0xdb, 0xe7};

static psa_status_t cracen_update_hash_with_length(cracen_hash_operation_t *hash_op,
						   const uint8_t *data, size_t data_len,
						   uint8_t prefix)
{
	psa_status_t status;
	uint8_t len[8] = {0};

	if (prefix) {
		data_len++;
	}
	if (data_len > 0xffff) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	len[0] = (uint8_t)data_len;
	len[1] = (uint8_t)(data_len >> 8);

	/* The length is a 8-byte little endian number. */
	status = cracen_hash_update(hash_op, len, sizeof(len));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	if (prefix) {
		status = cracen_hash_update(hash_op, &prefix, 1);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	status = cracen_hash_update(hash_op, data, data_len - !!prefix);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

exit:
	return status;
}

static psa_status_t cracen_update_transcript(cracen_spake2p_operation_t *operation)
{
	psa_status_t status;

	/* add prover, verifier, M and N to protocol transcript (TT) */
	status = cracen_update_hash_with_length(&operation->hash_op, operation->prover,
						operation->prover_len, 0);
	if (status) {
		return status;
	}
	status = cracen_update_hash_with_length(&operation->hash_op, operation->verifier,
						operation->verifier_len, 0);
	if (status) {
		return status;
	}

	status = cracen_update_hash_with_length(&operation->hash_op, SPAKE2P_POINT_M,
						sizeof(SPAKE2P_POINT_M), UNCOMPRESSED_POINT_TYPE);
	if (status) {
		return status;
	}
	return cracen_update_hash_with_length(&operation->hash_op, SPAKE2P_POINT_N,
					      sizeof(SPAKE2P_POINT_N), UNCOMPRESSED_POINT_TYPE);
}
/**
 * @brief Will calculate Z and V.
 *
 * Client:
 *     Z = h * x * (Y - w0 * N)
 *     V = h * w1 * (Y - w0 * N)
 *
 * Server:
 *     Z = h * y * (X - w0 * M)
 *     V = h * y * L
 */
static psa_status_t cracen_get_ZV(cracen_spake2p_operation_t *operation, uint8_t *Z, uint8_t *V)
{
	psa_status_t status;

	uint8_t w0N[CRACEN_P256_POINT_SIZE];

	MAKE_SX_POINT(pt_n_w0N, w0N, CRACEN_P256_POINT_SIZE);
	MAKE_SX_POINT(pt_NM, (char *)operation->NM, CRACEN_P256_POINT_SIZE);
	sx_ecop w0 = {.bytes = operation->w0, .sz = sizeof(operation->w0)};

	/* w0 * N. */
	status = sx_ecp_ptmult(operation->curve, &w0, &pt_NM, &pt_n_w0N);
	if (status) {
		return silex_statuscodes_to_psa(status);
	}

	/* Calculate -(previous result) */
	/* To negate a point we have -(x, y) = (x, -y) */
	const struct sx_pk_cmd_def *cmd = SX_PK_CMD_MOD_SUB;
	const uint8_t *field_size = sx_pk_field_size(operation->curve);

	sx_op modulo = {.sz = CRACEN_P256_KEY_SIZE, .bytes = (char *)field_size};
	uint8_t zeroes[CRACEN_P256_KEY_SIZE] = {0};
	sx_op a = {.sz = CRACEN_P256_KEY_SIZE, .bytes = (char *)zeroes};
	sx_op b = {.sz = CRACEN_P256_KEY_SIZE, .bytes = &w0N[CRACEN_P256_KEY_SIZE]};

	status = sx_mod_primitive_cmd(NULL, cmd, &modulo, &a, &b, &b);
	if (status) {
		return silex_statuscodes_to_psa(status);
	}

	/* Calculate Y + (- w0 * N) */
	MAKE_SX_POINT(pt_Y, operation->YX + 1, CRACEN_P256_POINT_SIZE);
	status = sx_ecp_ptadd(operation->curve, &pt_Y, &pt_n_w0N, &pt_n_w0N);
	if (status) {
		return silex_statuscodes_to_psa(status);
	}

	/* Calculate Z = x * (previous result) */
	sx_ecop x = {.sz = CRACEN_P256_KEY_SIZE, .bytes = operation->xy};

	MAKE_SX_POINT(pt_Z, Z, CRACEN_P256_POINT_SIZE);
	status = sx_ecp_ptmult(operation->curve, &x, &pt_n_w0N, &pt_Z);
	if (status) {
		return silex_statuscodes_to_psa(status);
	}

	/* Calculate V */
	MAKE_SX_POINT(pt_V, V, CRACEN_P256_POINT_SIZE);
	MAKE_SX_POINT(pt_L, (char *)operation->w1_or_L, CRACEN_P256_POINT_SIZE);
	sx_ecop k = {.bytes = operation->role == PSA_PAKE_ROLE_CLIENT ? operation->w1_or_L
								      : operation->xy,
		     .sz = CRACEN_P256_KEY_SIZE};

	status = sx_ecp_ptmult(operation->curve, &k,
			       operation->role == PSA_PAKE_ROLE_SERVER ? &pt_L : &pt_n_w0N, &pt_V);
	if (status) {
		return silex_statuscodes_to_psa(status);
	}

	return status;
}

static psa_status_t cracen_get_confirmation_keys(cracen_spake2p_operation_t *operation,
						 uint8_t *KconfP, uint8_t *KconfV)
{
	psa_status_t status;
	psa_key_derivation_operation_t kdf_op = {};
	uint8_t Z[CRACEN_P256_POINT_SIZE];
	uint8_t V[CRACEN_P256_POINT_SIZE];
	size_t hash_len;

	cracen_get_ZV(operation, Z, V);

	status = cracen_update_hash_with_length(&operation->hash_op, Z, CRACEN_P256_POINT_SIZE,
						UNCOMPRESSED_POINT_TYPE);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_update_hash_with_length(&operation->hash_op, V, CRACEN_P256_POINT_SIZE,
						UNCOMPRESSED_POINT_TYPE);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_update_hash_with_length(&operation->hash_op, operation->w0,
						CRACEN_P256_KEY_SIZE, 0);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_hash_finish(&operation->hash_op, V, sizeof(V), &hash_len);
	if (status != PSA_SUCCESS) {
		cracen_hash_abort(&operation->hash_op);
		return status;
	}

	if (operation->alg == PSA_ALG_SPAKE2P_MATTER) {
		operation->shared_len = hash_len / 2;
		memcpy(operation->shared, V + operation->shared_len, operation->shared_len);
	} else {
		operation->shared_len = hash_len;
		status = psa_driver_wrapper_key_derivation_setup(&kdf_op,
								 PSA_ALG_HKDF(PSA_ALG_SHA_256));
		if (status) {
			goto exit;
		}
		status = psa_driver_wrapper_key_derivation_input_bytes(
			&kdf_op, PSA_KEY_DERIVATION_INPUT_INFO, (uint8_t *)"SharedKey", 9);
		if (status) {
			goto exit;
		}
		status = psa_driver_wrapper_key_derivation_input_bytes(
			&kdf_op, PSA_KEY_DERIVATION_INPUT_SECRET, V, hash_len);
		if (status) {
			goto exit;
		}
		status = psa_driver_wrapper_key_derivation_output_bytes(&kdf_op, operation->shared,
									operation->shared_len);
		if (status) {
			goto exit;
		}

		/* KDF operations must be aborted, since they don't have a natural finish like other
		 * PSA operations.
		 */
		(void)psa_driver_wrapper_key_derivation_abort(&kdf_op);
	}

	status = psa_driver_wrapper_key_derivation_setup(&kdf_op, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	if (status) {
		goto exit;
	}
	status = psa_driver_wrapper_key_derivation_input_bytes(
		&kdf_op, PSA_KEY_DERIVATION_INPUT_INFO, (uint8_t *)"ConfirmationKeys", 16);
	if (status) {
		goto exit;
	}

	status = psa_driver_wrapper_key_derivation_input_bytes(
		&kdf_op, PSA_KEY_DERIVATION_INPUT_SECRET, V, operation->shared_len);
	if (status) {
		goto exit;
	}
	status = psa_driver_wrapper_key_derivation_output_bytes(&kdf_op, KconfP,
								operation->shared_len);
	if (status) {
		goto exit;
	}
	status = psa_driver_wrapper_key_derivation_output_bytes(&kdf_op, KconfV,
								operation->shared_len);

exit:
	safe_memzero(V, sizeof(V));
	safe_memzero(Z, sizeof(Z));

	/* KDF operations must be aborted, since they don't have a natural finish like other
	 * PSA operations.
	 */
	(void)psa_driver_wrapper_key_derivation_abort(&kdf_op);

	return status;
}

static psa_status_t cracen_get_confirmation(cracen_spake2p_operation_t *operation,
					    const uint8_t *kconf, const uint8_t *share,
					    uint8_t *confirmation)
{
	psa_key_attributes_t attributes = {};

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);

	size_t length;

	return psa_driver_wrapper_mac_compute(
		&attributes, kconf, operation->shared_len, PSA_ALG_HMAC(PSA_ALG_SHA_256), share,
		CRACEN_P256_POINT_SIZE + 1, confirmation, CRACEN_SPAKE2P_HASH_LEN, &length);
}

static psa_status_t cracen_p256_reduce(cracen_spake2p_operation_t *operation,
				       uint8_t *result_buffer, uint8_t *input, size_t input_length)
{
	if (!input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	const uint8_t *order = sx_pk_curve_order(operation->curve);

	sx_op modulo = {.sz = CRACEN_P256_KEY_SIZE, .bytes = (char *)order};
	sx_op b = {.sz = input_length, .bytes = (char *)input};
	sx_op result = {.sz = CRACEN_P256_KEY_SIZE, .bytes = result_buffer};

	/* The nistp256 curve order (n) is prime so we use the ODD variant of the reduce
	 * command.
	 */
	const struct sx_pk_cmd_def *cmd = SX_PK_CMD_ODD_MOD_REDUCE;
	int sx_status = sx_mod_single_op_cmd(cmd, &modulo, &b, &result);

	return silex_statuscodes_to_psa(sx_status);
}

static psa_status_t cracen_read_key_share(cracen_spake2p_operation_t *operation,
					  const uint8_t *input, size_t input_length)
{
	if (input_length != (CRACEN_P256_POINT_SIZE + 1) || input[0] != UNCOMPRESSED_POINT_TYPE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	memcpy(operation->YX, input, CRACEN_P256_POINT_SIZE + 1);
	if (operation->role == PSA_PAKE_ROLE_SERVER) {
		cracen_update_transcript(operation);
	}
	return cracen_update_hash_with_length(&operation->hash_op, operation->YX,
					      CRACEN_P256_POINT_SIZE + 1, 0);
}

static psa_status_t cracen_read_confirm(cracen_spake2p_operation_t *operation, const uint8_t *input,
					size_t input_length)
{
	psa_status_t status;
	uint8_t confirmation[CRACEN_SPAKE2P_HASH_LEN];

	if (input_length != CRACEN_SPAKE2P_HASH_LEN) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	if (operation->role == PSA_PAKE_ROLE_CLIENT) {
		status = cracen_get_confirmation_keys(operation, operation->KconfPV,
						      operation->KconfVP);
		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	status =
		cracen_get_confirmation(operation, operation->KconfVP, operation->XY, confirmation);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (constant_memcmp(input, confirmation, CRACEN_SPAKE2P_HASH_LEN) != 0) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	return status;
}

/**
 * @brief Will calculate key share step for SPAKE2+
 *
 * Client:
 *     pA = X = x * P + w0 * M (client)
 * Server:
 *     pB = Y = y * P * w0 * N (server)
 */
static psa_status_t cracen_get_key_share(cracen_spake2p_operation_t *operation)
{
	int status;

	uint8_t xP[CRACEN_P256_POINT_SIZE];
	uint8_t w0M[CRACEN_P256_POINT_SIZE];

	MAKE_SX_POINT(pt_xP, xP, CRACEN_P256_POINT_SIZE);

	/* Calculate x * P */
	sx_ecop x = {.bytes = operation->xy, .sz = CRACEN_P256_KEY_SIZE};

	status = sx_ecp_ptmult(operation->curve, &x, SX_PTMULT_CURVE_GENERATOR, &pt_xP);
	if (status != SX_OK) {
		return silex_statuscodes_to_psa(status);
	}

	/* Calculate w0 * M */
	sx_ecop w0 = {.bytes = operation->w0, .sz = sizeof(operation->w0)};

	MAKE_SX_POINT(pt_w0M, w0M, CRACEN_P256_POINT_SIZE);
	MAKE_SX_POINT(pt_M, (char *)operation->MN, CRACEN_P256_POINT_SIZE);
	status = sx_ecp_ptmult(operation->curve, &w0, &pt_M, &pt_w0M);
	if (status != SX_OK) {
		return silex_statuscodes_to_psa(status);
	}

	/* Add the two previous results. */
	MAKE_SX_POINT(pt_XY, operation->XY + 1, CRACEN_P256_POINT_SIZE);
	status = sx_ecp_ptadd(operation->curve, &pt_w0M, &pt_xP, &pt_XY);
	if (status != SX_OK) {
		return silex_statuscodes_to_psa(status);
	}

	operation->XY[0] = UNCOMPRESSED_POINT_TYPE;

	return PSA_SUCCESS;
}

static psa_status_t cracen_write_key_share(cracen_spake2p_operation_t *operation, uint8_t *output,
					   size_t output_size, size_t *output_length)
{
	psa_status_t status;
	uint8_t xs[40];

	if (output_size < CRACEN_P256_POINT_SIZE + 1) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (operation->role == PSA_PAKE_ROLE_CLIENT) {
		cracen_update_transcript(operation);
	}

	status = psa_generate_random(xs, sizeof(xs));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_p256_reduce(operation, operation->xy, xs, sizeof(xs));

	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_get_key_share(operation);
	if (status != PSA_SUCCESS) {
		return status;
	}
	memcpy(output, operation->XY, CRACEN_P256_POINT_SIZE + 1);
	*output_length = CRACEN_P256_POINT_SIZE + 1;

	return cracen_update_hash_with_length(&operation->hash_op, operation->XY,
					      CRACEN_P256_POINT_SIZE + 1, 0);
}

static psa_status_t cracen_write_confirm(cracen_spake2p_operation_t *operation, uint8_t *output,
					 size_t output_size, size_t *output_length)
{
	psa_status_t status;

	if (output_size < CRACEN_SPAKE2P_HASH_LEN) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (operation->role == PSA_PAKE_ROLE_SERVER) {
		status = cracen_get_confirmation_keys(operation, operation->KconfVP,
						      operation->KconfPV);

		if (status) {
			return status;
		}
	}

	status = cracen_get_confirmation(operation, operation->KconfPV, operation->YX, output);
	if (status) {
		return status;
	}
	*output_length = CRACEN_SPAKE2P_HASH_LEN;

	return PSA_SUCCESS;
}

static psa_status_t set_password_key(cracen_spake2p_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *password, size_t password_length)
{
	psa_status_t status;

	/* Compute w0 and w1 according to spec. */
	if (password_length == 2 * CRACEN_P256_KEY_SIZE) {
		/* Password contains: w0 || w1 */
		status = cracen_p256_reduce(operation, operation->w0, (uint8_t *)password,
					    password_length / 2);
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_p256_reduce(operation, operation->w1_or_L,
					    (uint8_t *)password + password_length / 2,
					    password_length / 2);
		if (status != PSA_SUCCESS) {
			return status;
		}

	} else if (password_length == CRACEN_P256_POINT_SIZE + CRACEN_P256_KEY_SIZE + 1) {
		/* Password contains: w0 || L */
		status = cracen_p256_reduce(operation, operation->w0, (uint8_t *)password,
					    CRACEN_P256_KEY_SIZE);
		password += password_length - CRACEN_P256_POINT_SIZE;

		/* Verify that the L part of password is an uncompressed point. */
		if (password[-1] != UNCOMPRESSED_POINT_TYPE) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		MAKE_SX_POINT(pt, (char *)password, CRACEN_P256_POINT_SIZE);
		status = cracen_ecc_check_public_key(operation->curve, &pt);
		if (status != PSA_SUCCESS) {
			return status;
		}
		memcpy(operation->w1_or_L, password, CRACEN_P256_POINT_SIZE);
	} else {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_spake2p_setup(cracen_spake2p_operation_t *operation,

				  const psa_key_attributes_t *attributes, const uint8_t *password,
				  size_t password_length,
				  const psa_pake_cipher_suite_t *cipher_suite)
{
	if (psa_pake_cs_get_primitive(cipher_suite) !=
		    PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC, PSA_ECC_FAMILY_SECP_R1, 256) ||
	    psa_pake_cs_get_key_confirmation(cipher_suite) != PSA_PAKE_CONFIRMED_KEY) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	psa_status_t status =
		cracen_ecc_get_ecurve_from_psa(PSA_ECC_FAMILY_SECP_R1, 256, &operation->curve);

	if (status != PSA_SUCCESS) {
		return status;
	}

	status = set_password_key(operation, attributes, password, password_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->alg = psa_pake_cs_get_algorithm(cipher_suite);

	/* Initialize hash for protocol transcript TT. */
	return cracen_hash_setup(&operation->hash_op, PSA_ALG_SHA_256);
}

psa_status_t cracen_spake2p_set_user(cracen_spake2p_operation_t *operation, const uint8_t *user_id,
				     size_t user_id_len)
{
	if (operation->role == PSA_PAKE_ROLE_CLIENT) {
		/* prover = user */
		if (user_id_len > sizeof(operation->prover)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		memcpy(operation->prover, user_id, user_id_len);
		operation->prover_len = (uint8_t)user_id_len;
	} else {
		/* verifier = user */
		if (user_id_len > sizeof(operation->verifier)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		memcpy(operation->verifier, user_id, user_id_len);
		operation->verifier_len = (uint8_t)user_id_len;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_spake2p_set_peer(cracen_spake2p_operation_t *operation, const uint8_t *peer_id,
				     size_t peer_id_len)
{
	if (operation->role == PSA_PAKE_ROLE_CLIENT) {
		if (peer_id_len > sizeof(operation->verifier)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		memcpy(operation->verifier, peer_id, peer_id_len);
		operation->verifier_len = (uint8_t)peer_id_len;
	} else {
		if (peer_id_len > sizeof(operation->prover)) {
			return PSA_ERROR_INSUFFICIENT_MEMORY;
		}
		memcpy(operation->prover, peer_id, peer_id_len);
		operation->prover_len = (uint8_t)peer_id_len;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_spake2p_set_context(cracen_spake2p_operation_t *operation,
					const uint8_t *context, size_t context_len)
{
	if (context_len == 0) {
		return PSA_SUCCESS;
	}

	return cracen_update_hash_with_length(&operation->hash_op, context, context_len, 0);
}

static psa_status_t cracen_spake2p_get_L_from_w1(cracen_spake2p_operation_t *operation,
						 uint8_t *w1_buf, uint8_t *L_buf)
{
	int sx_status;

	struct sx_buf w1 = {.sz = operation->curve->sz, .bytes = w1_buf};

	sx_pk_affine_point L_pnt = {.x = {.sz = w1.sz, .bytes = L_buf},
				    .y = {.sz = w1.sz, .bytes = L_buf + w1.sz}};

	sx_status = sx_ecp_ptmult(operation->curve, &w1, SX_PTMULT_CURVE_GENERATOR, &L_pnt);

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_spake2p_set_role(cracen_spake2p_operation_t *operation, psa_pake_role_t role)
{
	psa_status_t status;
	const size_t L_half_len = sizeof(operation->w1_or_L) / 2;

	switch (role) {
	case PSA_PAKE_ROLE_CLIENT:
		operation->MN = SPAKE2P_POINT_M;
		operation->NM = SPAKE2P_POINT_N;
		break;
	case PSA_PAKE_ROLE_SERVER:
		operation->MN = SPAKE2P_POINT_N;
		operation->NM = SPAKE2P_POINT_M;

		/* When the password contains "w0 || w1" we need to generate L based on w1. */
		if (constant_memcmp_is_zero(&operation->w1_or_L[L_half_len], L_half_len)) {
			/* We can reuse the w1_or_L buffer since the server role doesn't use w1
			 * afterwards.
			 */
			status = cracen_spake2p_get_L_from_w1(operation, operation->w1_or_L,
							      operation->w1_or_L);
			if (status != PSA_SUCCESS) {
				return status;
			}
		}
		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
	operation->role = role;
	return PSA_SUCCESS;
}

psa_status_t cracen_spake2p_output(cracen_spake2p_operation_t *operation, psa_pake_step_t step,
				   uint8_t *output, size_t output_size, size_t *output_length)
{
	switch (step) {
	case PSA_PAKE_STEP_KEY_SHARE:
		return cracen_write_key_share(operation, output, output_size, output_length);
	case PSA_PAKE_STEP_CONFIRM:
		return cracen_write_confirm(operation, output, output_size, output_length);
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t cracen_spake2p_input(cracen_spake2p_operation_t *operation, psa_pake_step_t step,
				  const uint8_t *input, size_t input_length)
{
	switch (step) {
	case PSA_PAKE_STEP_KEY_SHARE:
		return cracen_read_key_share(operation, input, input_length);
	case PSA_PAKE_STEP_CONFIRM:
		return cracen_read_confirm(operation, input, input_length);
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t cracen_spake2p_get_shared_key(cracen_spake2p_operation_t *operation,
					   const psa_key_attributes_t *attributes, uint8_t *output,
					   size_t output_size, size_t *output_length)
{
	if (output_size < operation->shared_len) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(output, operation->shared, operation->shared_len);
	*output_length = operation->shared_len;

	return PSA_SUCCESS;
}

psa_status_t cracen_spake2p_abort(cracen_spake2p_operation_t *operation)
{
	cracen_hash_abort(&operation->hash_op);
	safe_memzero(operation, sizeof(*operation));
	return PSA_SUCCESS;
}
