/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_wpa3_sae.h>

#include <cracen/common.h>
#include <internal/ecc/cracen_ecc_helpers.h>
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <cracen_psa_primitives.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <psa/crypto_sizes.h>
#include <psa/crypto_types.h>
#include <psa/crypto_values.h>
#include <silexpk/cmddefs/ecc.h>
#include <silexpk/ec_curves.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/eccweierstrass.h>
#include <silexpk/sxops/rsa.h>
#include <stdint.h>
#include <stdlib.h>

#define CRACEN_WPA3_SAE_HNP_LOOP_LIMIT		(40u) /* Taken from Oberon */
#define CRACEN_WPA3_SAE_MAX_SEED_SIZE		(PSA_HASH_MAX_SIZE)
#define CRACEN_WPA3_SAE_MAX_CMT_GEN_ATTEMPTS	(16u) /* this is currently a random value */

static uint8_t *cracen_wpa3_sae_get_cmt_scalar(uint8_t *commit_msg)
{
	return commit_msg;
}

static const uint8_t *cracen_wpa3_sae_get_cmt_scalar_const(const uint8_t *commit_msg)
{
	return commit_msg;
}

static uint8_t *cracen_wpa3_sae_get_cmt_element(uint8_t *commit_msg)
{
	return commit_msg + CRACEN_P256_KEY_SIZE;
}

static const uint8_t *cracen_wpa3_sae_get_cmt_element_const(const uint8_t *commit_msg)
{
	return commit_msg + CRACEN_P256_KEY_SIZE;
}

static psa_status_t cracen_wpa3_sae_setup_hmac(cracen_mac_operation_t *mac_op,
					       const uint8_t *key, size_t key_len)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(key_len));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
	safe_memzero(mac_op, sizeof(*mac_op));
	return cracen_mac_sign_setup(mac_op, &attr, key, key_len, PSA_ALG_HMAC(PSA_ALG_SHA_256));
}

static psa_status_t cracen_calc_pwd_seed_hnp(cracen_wpa3_sae_operation_t *op,
					     const uint8_t *counter, uint8_t *seed,
					     size_t *seed_len)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	/* seed = H(addr1 | addr2, pw | cnt) */
	status = cracen_wpa3_sae_setup_hmac(&op->mac_op, op->max_id_min_id,
					    2 * CRACEN_WPA3_SAE_STA_ID_LEN);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_update(&op->mac_op, op->password, op->pw_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_update(&op->mac_op, counter, 1);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_sign_finish(&op->mac_op, seed, CRACEN_WPA3_SAE_MAX_SEED_SIZE, seed_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

exit:
	cracen_mac_abort(&op->mac_op);
	return status;
}

/* IEEE802.11, 12.7.1.6.2 */
static psa_status_t sha256_prf_block(cracen_wpa3_sae_operation_t *op,
				     const uint8_t *key, size_t key_len,
				     const uint8_t *label, size_t label_len,
				     const uint8_t *context, size_t context_len,
				     uint16_t iteration, uint16_t bit_length,
				     uint8_t *output,
				     size_t output_size)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t cnt[sizeof(uint16_t)]; /* represent encoded 16-bit unsigned integers */
	uint8_t len[sizeof(uint16_t)]; /* represent encoded 16-bit unsigned integers */
	size_t length;

	/**
	 * IEEE802.11, 12.7.1.6.2 assumes there might be multiple iterations
	 * of this KDF. This would be calculated as follows:
	 *	iteration = key_len / op->hash_length
	 */

	/* block number */
	cnt[0] = (uint8_t)iteration;
	cnt[1] = (uint8_t)(iteration >> 8);

	/* bit length of the derived key */
	len[0] = (uint8_t)bit_length;
	len[1] = (uint8_t)(bit_length >> 8);

	status = cracen_wpa3_sae_setup_hmac(&op->mac_op, key, key_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_update(&op->mac_op, cnt, sizeof(cnt));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_update(&op->mac_op, label, label_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_update(&op->mac_op, context, context_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_update(&op->mac_op, len, sizeof(len));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_sign_finish(&op->mac_op, output, output_size, &length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	return status;

exit:
	cracen_mac_abort(&op->mac_op);
	return status;
}

static int cracen_ec_pt_calc_y_sqr(const struct sx_pk_ecurve *curve,
				   const sx_const_op *x,
				   sx_op *sqr_y)
{
	int sx_status = SX_ERR_CORRUPTION_DETECTED;
	uint8_t tmp[CRACEN_P256_KEY_SIZE];
	uint8_t tmp_2[CRACEN_P256_KEY_SIZE];
	const struct sx_pk_cmd_def *cmd_mul = SX_PK_CMD_ODD_MOD_MULT;
	const struct sx_pk_cmd_def *cmd_add = SX_PK_CMD_MOD_ADD;
	sx_const_op modulo = {.sz = sx_pk_curve_opsize(curve), .bytes = sx_pk_curve_prime(curve)};

	/** This function calculates the following:
	 *  y^2 = x^3 + ax + b = (x^2 + a)x + b
	 */

	/* x^2 */
	sx_op x_sqr = {.sz = CRACEN_P256_KEY_SIZE, .bytes = tmp};

	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, &modulo, x, x, &x_sqr);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* x^2 + a */
	sx_const_op x_sqr_const;
	sx_const_op a = {.sz = sx_pk_curve_opsize(curve), .bytes = sx_pk_curve_param_a(curve)};
	sx_op xsqr_a = {.sz = CRACEN_P256_KEY_SIZE, .bytes = tmp_2};

	sx_get_const_op(&x_sqr, &x_sqr_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_add, &modulo, &x_sqr_const, &a, &xsqr_a);
	sx_status = silex_statuscodes_to_psa(sx_status);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* (x^2 + a) * x */
	sx_const_op xsqr_a_const;
	sx_op xsqr_a_x = {.sz = CRACEN_P256_KEY_SIZE, .bytes = tmp};

	sx_get_const_op(&xsqr_a, &xsqr_a_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, &modulo, &xsqr_a_const, x, &xsqr_a_x);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* (x^2 + a) * x + b */
	sx_const_op xsqr_a_x_const;
	sx_const_op b = {.sz = sx_pk_curve_opsize(curve), .bytes = sx_pk_curve_param_b(curve)};

	sx_get_const_op(&xsqr_a_x, &xsqr_a_x_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_add, &modulo, &xsqr_a_x_const, &b, sqr_y);
	sx_status = silex_statuscodes_to_psa(sx_status);

	return sx_status;
}

static psa_status_t cracen_wpa3_sae_calc_pwe_hnp(cracen_wpa3_sae_operation_t *op)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t seed[CRACEN_WPA3_SAE_MAX_SEED_SIZE];

	uint8_t seed_lsb;
	uint8_t x[CRACEN_P256_KEY_SIZE];
	uint8_t x_cand[CRACEN_P256_KEY_SIZE];
	uint8_t y_sqr[CRACEN_P256_KEY_SIZE];
	uint8_t y_cand[CRACEN_P256_KEY_SIZE];
	uint8_t y[CRACEN_P256_KEY_SIZE];
	uint8_t p_y[CRACEN_P256_KEY_SIZE];

	size_t length;
	uint8_t counter = 1;
	int sx_status;
	bool pwd_value_in_range;
	int mask;
	int found = 0;
	const char *label = "SAE Hunting and Pecking";

	/* hunt and peck */
	do {
		/* seed = H(addr1 | addr2, pw | cnt) */
		status = cracen_calc_pwd_seed_hnp(op, &counter, seed, &length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/** pwd-value = KDF-Hash-Length(pwd-seed, “SAE Hunting and Pecking”, p)
		 *
		 *  Total bitlength of the derived key is 256 bits, so using
		 *  pwd-value = KDF-256(pwd-seed, “SAE Hunting and Pecking”, p)
		 */
		status = sha256_prf_block(op, seed, length, (const uint8_t *)label, strlen(label),
					  sx_pk_curve_prime(op->curve),
					  sx_pk_curve_opsize(op->curve), 1, 256,
					  x_cand, CRACEN_P256_KEY_SIZE);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/**
		 *  x-candidate check: pwd-value < p
		 *
		 *  Note: "p" in this case (according to spec) is the curve order (prime number)
		 *	  "r" is the prime order of G (generator point)
		 */
		pwd_value_in_range = (cracen_be_cmp(x_cand, sx_pk_curve_prime(op->curve),
						    CRACEN_P256_KEY_SIZE, 0) == -1);

		sx_const_op x_cand_op = {.sz = CRACEN_P256_KEY_SIZE, .bytes = x_cand};
		sx_op y_sqr_op = {.sz = CRACEN_P256_KEY_SIZE, .bytes = y_sqr};

		sx_status = cracen_ec_pt_calc_y_sqr(op->curve, &x_cand_op, &y_sqr_op);
		if (status != SX_OK) {
			return silex_statuscodes_to_psa(sx_status);
		}

		/** Currently sqrt calculation on CRACEN is used instead of this check
		 *  for quadratic residue.
		 *  It is able to report if the value is not a quadratic residue.
		 */
		sx_const_op y_sqr_cop;
		sx_const_op modulo = {.sz = sx_pk_curve_opsize(op->curve),
				      .bytes = sx_pk_curve_prime(op->curve)};
		sx_op sqrt_y = {.sz = CRACEN_P256_KEY_SIZE, .bytes = y_cand};
		const struct sx_pk_cmd_def *cmd = SX_PK_CMD_MOD_SQRT;

		sx_get_const_op(&y_sqr_op, &y_sqr_cop);
		sx_status = sx_mod_single_op_cmd(cmd, &modulo, &y_sqr_cop, &sqrt_y);

		/** save if !found
		 *
		 *  the mask is used to save "candidate x", "candidate y"
		 *  and seed_lsb values in constant time
		 */
		mask = found - 1;
		for (uint8_t i = 0; i < CRACEN_P256_KEY_SIZE; i++) {
			x[i] = (uint8_t)((x[i] & ~mask) | (x_cand[i] & mask));
			y[i] = (uint8_t)((y[i] & ~mask) | (y_cand[i] & mask));
		}
		seed_lsb = (uint8_t)((seed_lsb & ~mask) |
				     ((int)(seed[CRACEN_P256_KEY_SIZE - 1] & 0x01) & mask));
		found |= (pwd_value_in_range && sx_status == SX_OK);

		counter++;
		if (counter == UINT8_MAX) {
			return PSA_ERROR_INSUFFICIENT_ENTROPY;
		}
	} while (counter <= CRACEN_WPA3_SAE_HNP_LOOP_LIMIT || !found);

	/* y = sqrt(x^3 + ax + b) mod p
	 * if LSB(save) == LSB(y): PWE = (x, y)
	 * else: PWE = (x, p - y)
	 *
	 * Calculate y and the two possible values for PWE and after that,
	 * copy the correct alternative with constant time.
	 */

	/* y is already calculated */

	/* p_y = p - y */
	cracen_be_sub(sx_pk_curve_prime(op->curve), y, p_y, CRACEN_P256_KEY_SIZE);

	uint8_t y_lsb = (uint8_t)(y[CRACEN_P256_KEY_SIZE - 1] & 0x01);
	int lsb_eq = constant_memcmp(&seed_lsb, &y_lsb, 1);

	lsb_eq -= 1; /* 0xFF if equal, 0x00 otherwise */
	memcpy(op->pwe, x, CRACEN_P256_KEY_SIZE);
	for (uint8_t i = 0; i < CRACEN_P256_KEY_SIZE; i++) {
		op->pwe[i + CRACEN_P256_KEY_SIZE] = (uint8_t)((p_y[i] & ~lsb_eq) | (y[i] & lsb_eq));
	}

	return status;
}

static psa_status_t cracen_wpa3_sae_calc_pwe_h2e(cracen_wpa3_sae_operation_t *op)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	int sx_status;
	uint8_t val[PSA_HASH_MAX_SIZE];
	uint8_t val_reduced[PSA_HASH_MAX_SIZE];
	uint8_t sub_curve_order[CRACEN_P256_KEY_SIZE];
	size_t length;

	/* val = H(0, addr1 | addr2) */
	safe_memzero(val, op->hash_length);
	status = cracen_wpa3_sae_setup_hmac(&op->mac_op, val, op->hash_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_update(&op->mac_op, op->max_id, CRACEN_WPA3_SAE_STA_ID_LEN);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_update(&op->mac_op, op->min_id, CRACEN_WPA3_SAE_STA_ID_LEN);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_sign_finish(&op->mac_op, val, sizeof(val), &length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* PWE = pt * (val mod (q-1) + 1) */

	/* val = val mod (r - 1) + 1 */

	/* sub_curve_order = r - 1 */
	memcpy(sub_curve_order, sx_pk_curve_order(op->curve), CRACEN_P256_KEY_SIZE);
	cracen_be_add(sub_curve_order, CRACEN_P256_KEY_SIZE, -1);

	/* val_reduced = val mod (sub_curve_order) */
	sx_const_op modulo = {.sz = CRACEN_P256_KEY_SIZE, .bytes = sub_curve_order};
	sx_const_op b = {.sz = length, .bytes = val};
	sx_op result = {.sz = CRACEN_P256_KEY_SIZE, .bytes = val_reduced};

	/* Note: Modular reduction for even number */
	const struct sx_pk_cmd_def *cmd = SX_PK_CMD_EVEN_MOD_REDUCE;

	sx_status = sx_mod_single_op_cmd(cmd, &modulo, &b, &result);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* val_reduced = val_reduced + 1 */
	cracen_be_add(val_reduced, CRACEN_P256_KEY_SIZE, 1);

	/* PWE = val * PT */
	sx_const_ecop k = {.sz = length, .bytes = val_reduced};

	MAKE_SX_CONST_POINT(pt_point, op->password, op->pw_length);
	MAKE_SX_POINT(pwe_point, op->pwe, CRACEN_P256_POINT_SIZE);
	status = sx_ecp_ptmult(op->curve, &k, &pt_point, &pwe_point);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	return PSA_SUCCESS;

exit:
	cracen_mac_abort(&op->mac_op);
	return status;
}

static psa_status_t cracen_get_rnd_in_ext_range(const uint8_t *lower_limit,
						const uint8_t *upper_limit,
						size_t lim_sz,
						uint8_t *out)
{
	int sx_status;
	int g = 1;

	do {
		sx_status = cracen_get_rnd_in_range(upper_limit, lim_sz, out);
		g = cracen_be_cmp(out, lower_limit, lim_sz, 0);
	} while (g <= 0 && sx_status == SX_OK);

	return silex_statuscodes_to_psa(sx_status);
}

static psa_status_t cracen_wpa3_sae_inverse_key_op(const struct sx_pk_ecurve *curve,
						   uint8_t *key)
{
	int sx_status = SX_ERR_CORRUPTION_DETECTED;
	uint8_t zeroes[CRACEN_P256_KEY_SIZE] = {0};
	const struct sx_pk_cmd_def *cmd = SX_PK_CMD_MOD_SUB;

	sx_const_op modulo = {.sz = CRACEN_P256_KEY_SIZE, .bytes = sx_pk_field_size(curve)};
	sx_const_op a = {.sz = CRACEN_P256_KEY_SIZE, .bytes = zeroes};
	sx_const_op b = {.sz = CRACEN_P256_KEY_SIZE, .bytes = key};
	sx_op res = {.sz = CRACEN_P256_KEY_SIZE, .bytes = key};

	sx_status = sx_mod_primitive_cmd(NULL, cmd, &modulo, &a, &b, &res);
	return silex_statuscodes_to_psa(sx_status);
}

static psa_status_t cracen_construct_commit_msg(cracen_wpa3_sae_operation_t *op)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t rand_gen_loop_cntr = 0;
	int sx_status;
	uint8_t mask[CRACEN_P256_KEY_SIZE]; /* temporary key */
	const uint8_t *order = sx_pk_curve_order(op->curve);
	/* lower_rand_limit = 1 */
	const uint8_t lower_rand_limit[CRACEN_P256_KEY_SIZE] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
	};

	/**
	 * Commit message = scalar | COMMIT_ELEMENT
	 */
	uint8_t *scalar = cracen_wpa3_sae_get_cmt_scalar(op->commit);
	uint8_t *commit_element = cracen_wpa3_sae_get_cmt_element(op->commit);
	int scalar_g;

	do {
		/**
		 * Rand and mask values required to be in range (1; r)
		 */
		status = cracen_get_rnd_in_ext_range(lower_rand_limit, order,
						     sizeof(op->rand), op->rand);
		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_get_rnd_in_ext_range(lower_rand_limit, order,
						     sizeof(mask), mask);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* (rand + mask) mod r > 1 */
		sx_const_op rand_op = {.sz = sizeof(op->rand), .bytes = op->rand};
		sx_const_op mask_op = {.sz = sizeof(mask), .bytes = mask};
		sx_const_op modulo = {.sz = sx_pk_curve_opsize(op->curve), .bytes = order};
		sx_op result = {.sz = CRACEN_P256_KEY_SIZE, .bytes = scalar};
		const struct sx_pk_cmd_def *cmd_add = SX_PK_CMD_MOD_ADD;

		sx_status = sx_mod_primitive_cmd(NULL, cmd_add, &modulo,
						 &rand_op, &mask_op, &result);
		if (sx_status != SX_OK) {
			return silex_statuscodes_to_psa(sx_status);
		}

		/* ((rand + mask) mod r) > 1 */
		scalar_g = cracen_be_cmp(scalar, lower_rand_limit, CRACEN_P256_KEY_SIZE, 0);

		rand_gen_loop_cntr++;
	} while ((scalar_g <= 0) && rand_gen_loop_cntr <= CRACEN_WPA3_SAE_MAX_CMT_GEN_ATTEMPTS);

	if (scalar_g <= 0) {
		return PSA_ERROR_INSUFFICIENT_ENTROPY;
	}

	/* commit_element calculation: COMMIT-ELEMENT = inverse-op(scalar-op(mask, PWE)) */
	/* scalar-op(mask, PWE) */
	sx_const_ecop mask_scalar = {.sz = sizeof(mask), .bytes = mask};

	MAKE_SX_CONST_POINT(pwe_pt, op->pwe, CRACEN_P256_POINT_SIZE);
	MAKE_SX_POINT(cmt_element_pt, commit_element, CRACEN_P256_POINT_SIZE);
	sx_status = sx_ecp_ptmult(op->curve, &mask_scalar, &pwe_pt, &cmt_element_pt);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/** inverse-op(cmt_element_pt)
	 *  The result of the operation is the second point on a curve so that
	 *  (x; y) + (x; -y) = "point at infinity"
	 */
	status = cracen_wpa3_sae_inverse_key_op(op->curve, commit_element + CRACEN_P256_KEY_SIZE);
	return status;
}

static psa_status_t cracen_check_commit_msg(cracen_wpa3_sae_operation_t *op,
					    const uint8_t *commit_msg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	const uint8_t *order = sx_pk_curve_order(op->curve);
	/* lower_scalar_limit = 1 */
	const uint8_t lower_scalar_limit[CRACEN_P256_KEY_SIZE] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
	};
	int scalar_cmp;
	bool scalar_valid;

	/**
	 * Peer commit message = peer-scalar | peer-COMMIT_ELEMENT
	 */
	const uint8_t *peer_scalar = cracen_wpa3_sae_get_cmt_element_const(commit_msg);
	const uint8_t *peer_commit_element = cracen_wpa3_sae_get_cmt_element_const(commit_msg);

	/* Verify both the peer-commit-scalar and PEER-COMMIT-ELEMENT */
	/* 1 < peer-commit-scalar < r */
	scalar_cmp = cracen_be_cmp(peer_scalar, lower_scalar_limit, CRACEN_P256_KEY_SIZE, 0);
	scalar_valid = (scalar_cmp >= 1);
	scalar_cmp = cracen_be_cmp(order, peer_scalar, CRACEN_P256_KEY_SIZE, 0);
	scalar_valid = scalar_valid && (scalar_cmp >= 1);
	if (!scalar_valid) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* PEER-COMMIT-ELEMENT must be a valid point on curve */
	MAKE_SX_CONST_POINT(pt, peer_commit_element, CRACEN_P256_POINT_SIZE);
	status = cracen_ecc_check_public_key(op->curve, &pt);

	return status;
}

static psa_status_t cracen_wpa3_sae_calc_k(cracen_wpa3_sae_operation_t *op, uint8_t *k)
{
	int sx_status;
	/* Buffers might be reused */
	uint8_t pnt_buf_1[CRACEN_P256_POINT_SIZE];
	uint8_t pnt_buf_2[CRACEN_P256_POINT_SIZE];

	/**
	 *  K = scalar-op(rand, (elem-op(scalar-op(peer-commit-scalar, PWE), PEER-COMMIT-ELEMENT)));
	 *  K = rand * ((peer_scalar * PWE) + PEER-COMMIT-ELEMENT);
	 */

	/* ps_pwe = peer_scalar * PWE */
	sx_const_ecop peer_scalar = {.sz = CRACEN_P256_KEY_SIZE,
				     .bytes = cracen_wpa3_sae_get_cmt_scalar(op->peer_commit)};
	MAKE_SX_CONST_POINT(pwe_point, op->pwe, CRACEN_P256_POINT_SIZE);
	MAKE_SX_POINT(ps_pwe, pnt_buf_1, CRACEN_P256_POINT_SIZE);

	sx_status = sx_ecp_ptmult(op->curve, &peer_scalar, &pwe_point, &ps_pwe);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* ps_pwe_lmnt = ps_pwe + PEER-COMMIT-ELEMENT */
	sx_pk_const_affine_point ps_pwe_const;

	MAKE_SX_CONST_POINT(pce, cracen_wpa3_sae_get_cmt_element(op->peer_commit),
			    CRACEN_P256_POINT_SIZE);
	MAKE_SX_POINT(ps_pwe_lmnt, pnt_buf_2, CRACEN_P256_POINT_SIZE);
	sx_get_const_affine_point(&ps_pwe, &ps_pwe_const);
	sx_status = sx_ecp_ptadd(op->curve, &ps_pwe_const, &pce, &ps_pwe_lmnt);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* K = rand * ps_pwe_lmnt */
	sx_pk_const_affine_point ps_pwe_lmnt_const;
	sx_const_ecop rand_scalar = {.sz = CRACEN_P256_KEY_SIZE, .bytes = op->rand};

	MAKE_SX_POINT(K, pnt_buf_1, CRACEN_P256_POINT_SIZE);
	sx_get_const_affine_point(&ps_pwe_lmnt, &ps_pwe_lmnt_const);
	sx_status = sx_ecp_ptmult(op->curve, &rand_scalar, &ps_pwe_lmnt_const, &K);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/**
	 *  k = F(K);
	 *  k = K.x;
	 */
	memcpy(k, pnt_buf_1, CRACEN_P256_KEY_SIZE);
	return PSA_SUCCESS;
}

static psa_status_t cracen_wpa3_sae_calc_keys(cracen_wpa3_sae_operation_t *op)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	uint8_t k[CRACEN_P256_KEY_SIZE];
	uint8_t keyseed[PSA_HASH_MAX_SIZE];
	uint8_t ctx[CRACEN_P256_KEY_SIZE];
	uint8_t salt[CRACEN_P256_KEY_SIZE];
	size_t length;
	int sx_status;
	const char *label = "SAE KCK and PMK";

	/* key already set */
	if (op->keys_set) {
		return PSA_SUCCESS;
	}

	/**
	 * Note: According to PSA spec (https://github.com/ARM-software/psa-api/pull/293/files)
	 *	 k should be calculated at the PSA_PAKE_STEP_COMMIT step (after commit message
	 *	 validation), yet now it is done similar to the Oberon implementation
	 *	 so as to avoid possible issues with failing tests.
	 */
	status = cracen_wpa3_sae_calc_k(op, k);
	if (status != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/**
	 * Note: The following operations (SAE-KCK, PMK calculation) should
	 *	 be executed inside a call to psa_pake_input() for salt
	 *	 (according to PSA spec).
	 *	 Now the implementation is similar to Oberon.
	 *
	 * See: https://github.com/ARM-software/psa-api/pull/293/files
	 */

	/* keyseed = H(0, k) or H(rej_list, k); */
	if (!op->salt_set) {
		safe_memzero(salt, op->hash_length);
		status = cracen_wpa3_sae_setup_hmac(&op->mac_op, salt, op->hash_length);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	} /* else hmac is already set up with salt = rejected group list - Oberon */

	status = cracen_mac_update(&op->mac_op, k, CRACEN_P256_KEY_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_mac_sign_finish(&op->mac_op, keyseed, PSA_HASH_MAX_SIZE, &length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* context = (commit-scalar + peer-commit-scalar) mod r */
	sx_const_op cmt_scalar = {.sz = CRACEN_P256_KEY_SIZE,
				  .bytes = cracen_wpa3_sae_get_cmt_scalar(op->commit)};
	sx_const_op peer_cmt_scalar = {.sz = CRACEN_P256_KEY_SIZE,
				       .bytes = cracen_wpa3_sae_get_cmt_scalar(op->peer_commit)};
	sx_const_op modulo = {.sz = sx_pk_curve_opsize(op->curve),
			      .bytes = sx_pk_curve_order(op->curve)};
	sx_op result = {.sz = CRACEN_P256_KEY_SIZE, .bytes = ctx};
	const struct sx_pk_cmd_def *cmd_add = SX_PK_CMD_MOD_ADD;

	sx_status = sx_mod_primitive_cmd(NULL, cmd_add, &modulo, &cmt_scalar,
					 &peer_cmt_scalar, &result);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/** KCK | PMK = KDF-Hash-Length(keyseed, "SAE KCK and PMK", context)
	 *
	 *  Total bitlength of the derived key is 512 bits, so using
	 *  KCK | PMK = KDF-512(keyseed, "SAE KCK and PMK", context)
	 */
	status = sha256_prf_block(op, keyseed, length, (const uint8_t *)label, strlen(label),
				  ctx, CRACEN_P256_KEY_SIZE, 1, 512,
				  op->kck, CRACEN_P256_KEY_SIZE);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = sha256_prf_block(op, keyseed, length, (const uint8_t *)label, strlen(label),
				  ctx, CRACEN_P256_KEY_SIZE, 2, 512,
				  op->pmk, CRACEN_P256_KEY_SIZE);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* pmkid = first 16 bytes (128 bits) of context */
	memcpy(op->pmkid, ctx, CRACEN_WPA3_SAE_PMKID_SIZE);

	op->keys_set = 1;
	return PSA_SUCCESS;

exit:
	cracen_mac_abort(&op->mac_op);
	return status;
}

static psa_status_t cracen_wpa3_sae_calc_verif_confirm(cracen_wpa3_sae_operation_t *op,
						       const uint8_t *commit,
						       const uint8_t *peer_commit,
						       uint16_t send_confirm,
						       uint8_t *verif_confirm)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t length;

	/* add send_confirm counter value */
	verif_confirm[0] = (uint8_t)send_confirm;
	verif_confirm[1] = (uint8_t)(send_confirm >> 8);

	/* CN(KCK, send_confirm, scalar, element, peer_scalar, peer_element) */
	status = cracen_wpa3_sae_setup_hmac(&op->mac_op,
					    op->kck,
					    CRACEN_P256_KEY_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_mac_update(&op->mac_op,
				   verif_confirm,
				   CRACEN_WPA3_SAE_SEND_CONFIRM_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_mac_update(&op->mac_op,
				   cracen_wpa3_sae_get_cmt_scalar_const(commit),
				   CRACEN_P256_KEY_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_mac_update(&op->mac_op,
				   cracen_wpa3_sae_get_cmt_element_const(commit),
				   CRACEN_P256_POINT_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_mac_update(&op->mac_op,
				   cracen_wpa3_sae_get_cmt_scalar_const(peer_commit),
				   CRACEN_P256_KEY_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_mac_update(&op->mac_op,
				   cracen_wpa3_sae_get_cmt_element_const(peer_commit),
				   CRACEN_P256_POINT_SIZE);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_mac_sign_finish(&op->mac_op,
					&verif_confirm[CRACEN_WPA3_SAE_SEND_CONFIRM_SIZE],
					CRACEN_P256_KEY_SIZE,
					&length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	return status;

exit:
	cracen_mac_abort(&op->mac_op);
	return status;
}

psa_status_t cracen_wpa3_sae_setup(cracen_wpa3_sae_operation_t *operation,
				   const psa_key_attributes_t *attributes,
				   const uint8_t *password, size_t password_length,
				   const psa_pake_cipher_suite_t *cipher_suite)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_type_t type = psa_get_key_type(attributes);
	psa_algorithm_t alg = psa_pake_cs_get_algorithm(cipher_suite);
	psa_algorithm_t hash_alg = PSA_ALG_GET_HASH(alg);
	psa_pake_primitive_t primitive = psa_pake_cs_get_primitive(cipher_suite);
	size_t bits = PSA_PAKE_PRIMITIVE_GET_BITS(primitive);

	if (PSA_PAKE_PRIMITIVE_GET_TYPE(primitive) != PSA_PAKE_PRIMITIVE_TYPE_ECC ||
		PSA_PAKE_PRIMITIVE_GET_FAMILY(primitive) != PSA_ECC_FAMILY_SECP_R1) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	switch (bits) {
	case 256:
		if (hash_alg != PSA_ALG_SHA_256) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	/* store password (either PT or password) */
	if (password_length > sizeof(operation->password)) {
		return PSA_ERROR_INSUFFICIENT_MEMORY;
	}

	status = cracen_ecc_get_ecurve_from_psa(PSA_PAKE_PRIMITIVE_GET_FAMILY(primitive),
						bits,
						&operation->curve);

	if (status != PSA_SUCCESS) {
		return status;
	}

	/**
	 *  The password is saved here for future (since PWE can be calculated
	 *  when we have two MAC addresses - these will be given at the next step)
	 */
	memcpy(operation->password, password, password_length);
	operation->pw_length = (uint16_t)password_length;

	operation->hash_alg = hash_alg;
	operation->hash_length = (uint8_t)PSA_HASH_LENGTH(hash_alg);
	operation->pmk_length = CRACEN_WPA3_SAE_PMK_LEN;
	operation->keys_set = 0;
	operation->salt_set = 0;
	operation->use_h2e = PSA_KEY_TYPE_IS_WPA3_SAE_ECC(type);
	return PSA_SUCCESS;
}

psa_status_t cracen_wpa3_sae_set_user(cracen_wpa3_sae_operation_t *operation,
				      const uint8_t *user_id, size_t user_id_len)
{
	if (user_id_len != CRACEN_WPA3_SAE_STA_ID_LEN) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	memcpy(operation->max_id, user_id, user_id_len);
	return PSA_SUCCESS;
}

psa_status_t cracen_wpa3_sae_set_peer(cracen_wpa3_sae_operation_t *operation,
				      const uint8_t *peer_id, size_t peer_id_len)
{
	if (peer_id_len != CRACEN_WPA3_SAE_STA_ID_LEN) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/** This check can only be done here since peer is required to be set after the user.
	 *  Oberon verifies that prior to call driver wrapper.
	 */
	if (memcmp(peer_id, operation->max_id, peer_id_len) > 0) {
		memcpy(operation->min_id, operation->max_id, peer_id_len);
		memcpy(operation->max_id, peer_id, peer_id_len);
	} else {
		memcpy(operation->min_id, peer_id, peer_id_len);
	}

	/* calculate PWE */
	if (operation->use_h2e) {
		return cracen_wpa3_sae_calc_pwe_h2e(operation);
	} else {
		return cracen_wpa3_sae_calc_pwe_hnp(operation);
	}
}

psa_status_t cracen_wpa3_sae_output(cracen_wpa3_sae_operation_t *operation,
				    psa_pake_step_t step, uint8_t *output,
				    size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	switch (step) {
	case PSA_PAKE_STEP_COMMIT:
		if (output_size < CRACEN_WPA3_SAE_COMMIT_SIZE) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		status = cracen_construct_commit_msg(operation);
		if (status != PSA_SUCCESS) {
			return status;
		}
		memcpy(output, operation->commit, CRACEN_WPA3_SAE_COMMIT_SIZE);
		*output_length = CRACEN_WPA3_SAE_COMMIT_SIZE;
		break;
	case PSA_PAKE_STEP_CONFIRM:
		if (output_size < CRACEN_WPA3_SAE_CONFIRM_SIZE) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		status = cracen_wpa3_sae_calc_keys(operation);
		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_wpa3_sae_calc_verif_confirm(operation,
							    operation->commit,
							    operation->peer_commit,
							    operation->send_confirm,
							    output);
		if (status != PSA_SUCCESS) {
			return status;
		}
		*output_length = CRACEN_WPA3_SAE_CONFIRM_SIZE;
		break;
	case PSA_PAKE_STEP_KEY_ID:
		if (output_size < CRACEN_WPA3_SAE_PMKID_SIZE) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		memcpy(output, operation->pmkid, CRACEN_WPA3_SAE_PMKID_SIZE);
		*output_length = CRACEN_WPA3_SAE_PMKID_SIZE;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_wpa3_sae_input(cracen_wpa3_sae_operation_t *operation,
				   psa_pake_step_t step, const uint8_t *input,
				   size_t input_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t verify[CRACEN_WPA3_SAE_CONFIRM_SIZE];
	uint16_t send_confirm;
	int res;

	switch (step) {
	case PSA_PAKE_STEP_COMMIT:
		if (input_length < CRACEN_WPA3_SAE_COMMIT_SIZE) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		status = cracen_check_commit_msg(operation, input);
		if (status != PSA_SUCCESS) {
			return status;
		}
		memcpy(operation->peer_commit, input, CRACEN_WPA3_SAE_COMMIT_SIZE);
		break;
	case PSA_PAKE_STEP_SALT:
		/* rejected groups list */
		status = cracen_wpa3_sae_setup_hmac(&operation->mac_op, input, input_length);
		if (status != PSA_SUCCESS) {
			return status;
		}
		operation->salt_set = 1;
		break;
	case PSA_PAKE_STEP_CONFIRM:
		if (input_length != CRACEN_WPA3_SAE_CONFIRM_SIZE) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		status = cracen_wpa3_sae_calc_keys(operation);
		if (status != PSA_SUCCESS) {
			return status;
		}

		send_confirm = input[0] | (input[1] << 8);
		status = cracen_wpa3_sae_calc_verif_confirm(operation,
							    operation->peer_commit,
							    operation->commit,
							    send_confirm,
							    verify);
		if (status != PSA_SUCCESS) {
			return status;
		}

		res = constant_memcmp(input, verify, CRACEN_WPA3_SAE_CONFIRM_SIZE);
		if (res) {
			return PSA_ERROR_INVALID_SIGNATURE;
		}
		break;
	case PSA_PAKE_STEP_CONFIRM_COUNT:
		if (input_length != CRACEN_WPA3_SAE_SEND_CONFIRM_SIZE) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		operation->send_confirm = input[0] | (input[1] << 8);
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_wpa3_sae_get_shared_key(cracen_wpa3_sae_operation_t *operation,
					    const psa_key_attributes_t *attributes,
					    uint8_t *output, size_t output_size,
					    size_t *output_length)
{
	(void)attributes;

	if (output_size < operation->pmk_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(output, operation->pmk, operation->pmk_length);
	*output_length = operation->pmk_length;
	return PSA_SUCCESS;
}

psa_status_t cracen_wpa3_sae_abort(cracen_wpa3_sae_operation_t *operation)
{
	cracen_mac_abort(&operation->mac_op);
	safe_memzero(operation, sizeof(*operation));
	return PSA_SUCCESS;
}
