/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/sxbuf/sxbufop.h>
#include <psa/crypto.h>
#include "common.h"
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"
#include "psa/crypto_driver_contexts_key_derivation.h"
#include "psa/crypto_sizes.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"
#include "silexpk/core.h"
#include "silexpk/ec_curves.h"
#include <stdint.h>
#include <silexpk/cmddefs/ecc.h>
#include <silexpk/iomem.h>
#include <silexpk/sxops/rsa.h>
#include <silexpk/sxops/ecjpake.h>
#include <psa_crypto_driver_wrappers.h>

#define MAKE_SX_POINT(name, ptr, point_size)                                                       \
	sx_pk_affine_point name = {{.bytes = (ptr), .sz = (point_size) / 2},                       \
				   {.bytes = (ptr) + (point_size) / 2, .sz = (point_size) / 2}};

/*
 * This driver supports uncompressed points only. Uncompressed types
 * are prefixed with the byte 0x04. See 2.3.3.3.3 from "SEC 1:
 * Elliptic Curve Cryptography" for details.
 */
#define UNCOMPRESSED_POINT_TYPE 0x04

#define RANDOM_RETRY_LIMIT 16

psa_status_t cracen_jpake_setup(cracen_jpake_operation_t *operation,
				const psa_key_attributes_t *attributes, const uint8_t *password,
				size_t password_length, const psa_pake_cipher_suite_t *cipher_suite)
{
	if (psa_pake_cs_get_primitive(cipher_suite) !=
	    PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC, PSA_ECC_FAMILY_SECP_R1, 256)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (PSA_ALG_GET_HASH(psa_pake_cs_get_algorithm(cipher_suite)) != PSA_ALG_SHA_256) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	psa_status_t status =
		cracen_ecc_get_ecurve_from_psa(PSA_ECC_FAMILY_SECP_R1, 256, &operation->curve);

	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->alg = PSA_ALG_SHA_256;
	operation->rd_idx = 0;
	operation->wr_idx = 0;

	return cracen_jpake_set_password_key(operation, attributes, password, password_length);
}

psa_status_t cracen_jpake_set_password_key(cracen_jpake_operation_t *operation,
					   const psa_key_attributes_t *attributes,
					   const uint8_t *password, size_t password_length)
{
	/* The password is interpreted as a big endian integer and then reduced modulo the curve
	 * order.
	 */
	const uint8_t *order = sx_pk_curve_order(operation->curve);

	sx_op modulo = {.sz = CRACEN_P256_KEY_SIZE, .bytes = (char *)order};
	sx_op b = {.sz = password_length, .bytes = (char *)password};
	sx_op result = {.sz = sizeof(operation->secret), .bytes = operation->secret};

	/* The nistp256 curve order (n) is prime so we use the ODD variant of the reduce command. */
	const struct sx_pk_cmd_def *cmd = SX_PK_CMD_ODD_MOD_REDUCE;
	int sx_status = sx_mod_single_op_cmd(cmd, &modulo, &b, &result);

	if (sx_status == SX_OK) {
		if (constant_memcmp_is_zero(operation->secret, sizeof(operation->secret))) {
			return PSA_ERROR_INVALID_HANDLE;
		}
	}

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_jpake_set_user(cracen_jpake_operation_t *operation, const uint8_t *user_id,
				   size_t user_id_len)
{
	if (user_id_len > sizeof(operation->user_id)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	memcpy(operation->user_id, user_id, user_id_len);
	operation->user_id_length = (uint8_t)user_id_len;

	return PSA_SUCCESS;
}

psa_status_t cracen_jpake_set_peer(cracen_jpake_operation_t *operation, const uint8_t *peer_id,
				   size_t peer_id_len)
{
	if (peer_id_len > sizeof(operation->peer_id)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	memcpy(operation->peer_id, peer_id, peer_id_len);
	operation->peer_id_length = (uint8_t)peer_id_len;

	return PSA_SUCCESS;
}

psa_status_t cracen_jpake_set_role(cracen_jpake_operation_t *operation, psa_pake_role_t role)
{
	if (role != PSA_PAKE_ROLE_FIRST && role != PSA_PAKE_ROLE_SECOND &&
	    role != PSA_PAKE_ROLE_NONE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	/* J-PAKE does not assign specific roles. */
	return PSA_SUCCESS;
}

/**
 * @brief Point multiplication of secret * G.
 *
 * @param[in,out] operation  Operation context object.
 * @param[out]    public_key Public key, result of operation.
 * @param[in]     G          Generator point. If NULL, base point of curve is used.
 * @param[in]     secret     Private key.
 * @return psa_status_t
 */
static psa_status_t cracen_ecjpake_get_public_key(cracen_jpake_operation_t *operation,
						  uint8_t *public_key, const uint8_t *G,
						  const uint8_t *secret)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecp_mult inputs;
	int opsz;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECC_PTMUL);
	if (pkreq.status) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, operation->curve, 0,
					      (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	opsz = sx_pk_curve_opsize(operation->curve);

	/* Write the private key into device memory */
	sx_wrpkmem(inputs.k.addr, secret, opsz);

	if (G == NULL) {
		sx_pk_write_curve_gen(pkreq.req, operation->curve, inputs.px, inputs.py);
	} else {
		sx_wrpkmem(inputs.px.addr, G, CRACEN_P256_KEY_SIZE);
		sx_wrpkmem(inputs.py.addr, G + CRACEN_P256_KEY_SIZE, CRACEN_P256_KEY_SIZE);
	}

	sx_pk_run(pkreq.req);
	int status = sx_pk_wait(pkreq.req);

	if (status) {
		return silex_statuscodes_to_psa(status);
	}

	const char **outputs = sx_pk_get_output_ops(pkreq.req);

	sx_rdpkmem(&public_key[0], outputs[0], opsz);
	sx_rdpkmem(&public_key[32], outputs[1], opsz);
	sx_pk_release_req(pkreq.req);

	return PSA_SUCCESS;
}

/**
 * @brief Feed field to hashing operation.
 *
 * @param[in,out] hash_op   Active hashing operation to update.
 * @param[in]     data      Data buffer.
 * @param[in]     data_len  Size of data in bytes.
 * @param[in]     type      Optional prefix. 0x04 for ECC point.
 * @return psa_status_t
 */
static psa_status_t cracen_update_hash_with_prefix(psa_hash_operation_t *hash_op,
						   const uint8_t *data, size_t data_len,
						   uint8_t type)
{
	psa_status_t status;
	uint8_t prefix[5];
	size_t hash_len = data_len;
	size_t prefix_len = 4;

	if (type) {
		prefix[4] = type;
		hash_len++;
		prefix_len++;
	}

	/* Recommended field separator in RFC 8235. */
	prefix[0] = 0;
	prefix[1] = 0;
	prefix[2] = (uint8_t)(hash_len >> 8);
	prefix[3] = (uint8_t)hash_len;

	status = psa_driver_wrapper_hash_update(hash_op, prefix, prefix_len);

	if (status != PSA_SUCCESS) {
		return status;
	}

	return psa_driver_wrapper_hash_update(hash_op, data, data_len);
}

/**
 * @brief Creates zkp hash according to RFC 8235.
 *
 * @param[in]  hash_alg    Which hashing algorithm to use.
 * @param[in]  X           Public key.
 * @param[in]  V           Random point.
 * @param[in]  G           Generator point.
 * @param[in]  id          Identity string.
 * @param[in]  id_len      Size of identity in bytes.
 * @param[out] hash        Output buffer.
 * @param[in]  hash_size   Size of output buffer.
 * @param[out] hash_length Number of bytes written to output buffer.
 *
 * @return psa_status_t
 */
static psa_status_t cracen_get_zkp_hash(psa_algorithm_t hash_alg,
					const uint8_t X[CRACEN_P256_POINT_SIZE],
					const uint8_t V[CRACEN_P256_POINT_SIZE],
					const uint8_t G[CRACEN_P256_POINT_SIZE], const uint8_t *id,
					size_t id_len, uint8_t *hash, size_t hash_size,
					size_t *hash_length)
{
	psa_status_t status;
	psa_hash_operation_t hash_op = {0};

	status = psa_driver_wrapper_hash_setup(&hash_op, hash_alg);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_update_hash_with_prefix(&hash_op, G, CRACEN_P256_POINT_SIZE,
						SI_ECC_PUBKEY_UNCOMPRESSED);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_update_hash_with_prefix(&hash_op, V, CRACEN_P256_POINT_SIZE,
						SI_ECC_PUBKEY_UNCOMPRESSED);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_update_hash_with_prefix(&hash_op, X, CRACEN_P256_POINT_SIZE,
						SI_ECC_PUBKEY_UNCOMPRESSED);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_update_hash_with_prefix(&hash_op, id, id_len, 0);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = psa_driver_wrapper_hash_finish(&hash_op, hash, hash_size, hash_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	return PSA_SUCCESS;

exit:
	psa_driver_wrapper_hash_abort(&hash_op);
	return status;
}

static psa_status_t cracen_write_key_share(cracen_jpake_operation_t *operation, uint8_t *output,
					   size_t output_size, size_t *output_length)
{
	int idx = operation->wr_idx;
	psa_status_t status;
	uint8_t v[CRACEN_P256_KEY_SIZE];
	uint8_t h[PSA_HASH_MAX_SIZE];
	uint8_t generator[CRACEN_P256_POINT_SIZE];
	size_t h_len;
	int sx_status = 0;

	if (output_size < sizeof(operation->X[idx]) + 1) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (idx == 0 || idx == 1) {
		/* First round of J-PAKE: Generate x1 and x2 and corresponding public keys. */

		/* The first 32 bits in the curve order of nistp256 are
		 * 1's. Giving us a 1 / 2^32 chance of not generating a random
		 * number that is small enough on the first try. We then try 16
		 * more times giving us a statistically impossible chance of
		 * generating a number that is too small.
		 */
		status = rnd_in_range(operation->x[idx], CRACEN_P256_KEY_SIZE,
				      sx_pk_curve_order(operation->curve), RANDOM_RETRY_LIMIT);
		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_ecjpake_get_public_key(operation, operation->X[idx],
						       SX_PT_CURVE_GENERATOR, operation->x[idx]);
		if (status != PSA_SUCCESS) {
			return status;
		}
	} else if (idx == 2) {
		/* Second round of J-PAKE: Compute G_a, x2s and A. */
		MAKE_SX_POINT(G4, operation->P[0], CRACEN_P256_POINT_SIZE);
		MAKE_SX_POINT(G3, operation->P[1], CRACEN_P256_POINT_SIZE);
		MAKE_SX_POINT(G1, operation->X[0], CRACEN_P256_POINT_SIZE);
		MAKE_SX_POINT(ga, generator, CRACEN_P256_POINT_SIZE);
		MAKE_SX_POINT(a, operation->X[2], CRACEN_P256_POINT_SIZE);

		sx_ecop x2s = {.bytes = operation->x[1], .sz = CRACEN_P256_KEY_SIZE};
		sx_ecop s = {.bytes = operation->secret, .sz = CRACEN_P256_KEY_SIZE};
		sx_ecop rx2s = {.bytes = operation->x[2], .sz = CRACEN_P256_KEY_SIZE};

		sx_status = sx_ecjpake_gen_step_2(operation->curve, &G4, &G3, &G1, &x2s, &s, &a,
						  &rx2s, &ga);
		if (sx_status) {
			return silex_statuscodes_to_psa(sx_status);
		}

	} else {
		return PSA_ERROR_BAD_STATE;
	}

	status =
		rnd_in_range(v, sizeof(v), sx_pk_curve_order(operation->curve), RANDOM_RETRY_LIMIT);

	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_ecjpake_get_public_key(operation, operation->V,
					       idx == 2 ? generator : SX_PT_CURVE_GENERATOR, v);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_get_zkp_hash(
		operation->alg, operation->X[idx], operation->V,
		idx == 2 ? generator : (uint8_t *)sx_pk_generator_point(operation->curve),
		operation->user_id, operation->user_id_length, h, sizeof(h), &h_len);
	if (status != PSA_SUCCESS) {
		return status;
	}

	sx_ecop sx_v = {.bytes = v, .sz = sizeof(v)};
	sx_ecop sx_x = {.bytes = operation->x[idx], .sz = sizeof(operation->x[idx])};
	sx_ecop sx_h = {.bytes = h, .sz = PSA_HASH_LENGTH(PSA_ALG_SHA_256)};
	sx_ecop sx_r = {.bytes = operation->r, .sz = sizeof(operation->r)};

	sx_status = sx_ecjpake_generate_zkp(operation->curve, &sx_v, &sx_x, &sx_h, &sx_r);

	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	output[0] = SI_ECC_PUBKEY_UNCOMPRESSED;
	memcpy(output + 1, operation->X[idx], sizeof(operation->X[idx]));
	*output_length = sizeof(operation->X[idx]) + 1;

	return PSA_SUCCESS;
}
static psa_status_t cracen_write_zk_public(cracen_jpake_operation_t *operation, uint8_t *output,
					   size_t output_size, size_t *output_length)
{
	if (sizeof(operation->V) >= output_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	output[0] = SI_ECC_PUBKEY_UNCOMPRESSED;
	memcpy(&output[1], operation->V, sizeof(operation->V));
	*output_length = sizeof(operation->V) + 1;

	return PSA_SUCCESS;
}

static psa_status_t cracen_write_zk_proof(cracen_jpake_operation_t *operation, uint8_t *output,
					  size_t output_size, size_t *output_length)
{
	if (sizeof(operation->r) > output_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(output, operation->r, sizeof(operation->r));
	*output_length = sizeof(operation->r);
	operation->wr_idx++;

	return PSA_SUCCESS;
}

psa_status_t cracen_jpake_output(cracen_jpake_operation_t *operation, psa_pake_step_t step,
				 uint8_t *output, size_t output_size, size_t *output_length)
{
	switch (step) {
	case PSA_PAKE_STEP_KEY_SHARE:
		return cracen_write_key_share(operation, output, output_size, output_length);
	case PSA_PAKE_STEP_ZK_PUBLIC:
		return cracen_write_zk_public(operation, output, output_size, output_length);
	case PSA_PAKE_STEP_ZK_PROOF:
		return cracen_write_zk_proof(operation, output, output_size, output_length);
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

static psa_status_t cracen_read_key_share(cracen_jpake_operation_t *operation, const uint8_t *input,
					  size_t input_length)
{
	int idx = operation->rd_idx;

	if (input_length != sizeof(operation->P[idx]) + 1 ||
	    input[0] != SI_ECC_PUBKEY_UNCOMPRESSED) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	memcpy(operation->P[idx], &input[1], sizeof(operation->P[idx]));

	return PSA_SUCCESS;
}

static psa_status_t cracen_read_zk_public(cracen_jpake_operation_t *operation, const uint8_t *input,
					  size_t input_length)
{
	if (input_length != sizeof(operation->V) + 1 || input[0] != SI_ECC_PUBKEY_UNCOMPRESSED) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	memcpy(operation->V, &input[1], sizeof(operation->V));

	return PSA_SUCCESS;
}

static psa_status_t cracen_read_zk_proof(cracen_jpake_operation_t *operation, const uint8_t *input,
					 size_t input_length)
{
	psa_status_t status;
	uint8_t generator[CRACEN_P256_POINT_SIZE];
	int idx = operation->rd_idx;
	uint8_t *rp = operation->r;
	uint8_t h[PSA_HASH_MAX_SIZE];
	size_t h_len;

	if (input_length > sizeof(operation->r)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	if (input_length < sizeof(operation->r)) {
		safe_memset(rp, sizeof(operation->r), 0, sizeof(operation->r) - input_length);
		rp += sizeof(operation->r) - input_length;
	}
	memcpy(rp, input, input_length);

	if (idx == 0 || idx == 1) {
		/* Verify that user id != peer id */
		if (operation->peer_id_length == operation->user_id_length) {
			bool equal = true;

			for (size_t i = 0; i < operation->user_id_length; i++) {
				if (operation->peer_id[i] != operation->user_id[i]) {
					equal = false;
					break;
				}
			}
			if (equal) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}
		}

	} else if (idx == 2) {
		/* Second round of J-PAKE: Compute G_B. */
		MAKE_SX_POINT(G1, operation->X[0], CRACEN_P256_POINT_SIZE);
		MAKE_SX_POINT(G2, operation->X[1], CRACEN_P256_POINT_SIZE);
		MAKE_SX_POINT(G3, operation->P[0], CRACEN_P256_POINT_SIZE);
		MAKE_SX_POINT(g, generator, CRACEN_P256_POINT_SIZE);

		int sx_status = sx_ecjpake_3pt_add(operation->curve, &G1, &G2, &G3, &g);

		if (sx_status) {
			return silex_statuscodes_to_psa(sx_status);
		}
	} else {
		return PSA_ERROR_BAD_STATE;
	}

	status = cracen_get_zkp_hash(
		operation->alg, operation->P[idx], operation->V,
		idx == 2 ? generator : (uint8_t *)sx_pk_generator_point(operation->curve),
		operation->peer_id, operation->peer_id_length, h, sizeof(h), &h_len);

	if (status != PSA_SUCCESS) {
		return status;
	}

	MAKE_SX_POINT(sx_v, operation->V, CRACEN_P256_POINT_SIZE);
	MAKE_SX_POINT(sx_x, operation->P[idx], CRACEN_P256_POINT_SIZE);
	MAKE_SX_POINT(sx_g, generator, CRACEN_P256_POINT_SIZE);

	sx_ecop sx_r = {.bytes = operation->r, .sz = sizeof(operation->r)};
	sx_ecop sx_h = {.bytes = h, .sz = h_len};

	int sx_status = sx_ecjpake_verify_zkp(operation->curve, &sx_v, &sx_x, &sx_r, &sx_h,
					      idx == 2 ? &sx_g : SX_PT_CURVE_GENERATOR);

	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	operation->rd_idx++;

	return PSA_SUCCESS;
}

psa_status_t cracen_jpake_input(cracen_jpake_operation_t *operation, psa_pake_step_t step,
				const uint8_t *input, size_t input_length)
{
	switch (step) {
	case PSA_PAKE_STEP_KEY_SHARE:
		return cracen_read_key_share(operation, input, input_length);
	case PSA_PAKE_STEP_ZK_PUBLIC:
		return cracen_read_zk_public(operation, input, input_length);
	case PSA_PAKE_STEP_ZK_PROOF:
		return cracen_read_zk_proof(operation, input, input_length);
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t cracen_jpake_get_shared_key(cracen_jpake_operation_t *operation,
					 const psa_key_attributes_t *attributes, uint8_t *output,
					 size_t output_size, size_t *output_length)
{
	int sx_status;

	if (output_size <= CRACEN_P256_POINT_SIZE) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	output[0] = SI_ECC_PUBKEY_UNCOMPRESSED;

	/* get PMSK */
	MAKE_SX_POINT(x4, operation->P[1], CRACEN_P256_POINT_SIZE);
	MAKE_SX_POINT(b, operation->P[2], CRACEN_P256_POINT_SIZE);
	sx_ecop x2 = {.bytes = operation->x[1], .sz = CRACEN_P256_KEY_SIZE};
	sx_ecop x2s = {.bytes = operation->x[2], .sz = CRACEN_P256_KEY_SIZE};

	MAKE_SX_POINT(t, output + 1, CRACEN_P256_POINT_SIZE);
	sx_status = sx_ecjpake_gen_sess_key(operation->curve, &x4, &b, &x2, &x2s, &t);

	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	*output_length = 65;

	return PSA_SUCCESS;
}

psa_status_t cracen_jpake_abort(cracen_jpake_operation_t *operation)
{
	safe_memzero(operation, sizeof(*operation));

	return PSA_SUCCESS;
}
