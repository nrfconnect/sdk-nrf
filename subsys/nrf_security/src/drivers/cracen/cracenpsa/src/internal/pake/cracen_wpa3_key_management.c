/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/pake/cracen_wpa3_key_management.h>
#include <internal/ecc/cracen_ecc_helpers.h>

#include <string.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/rsa.h>
#include <silexpk/sxops/eccweierstrass.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>

static psa_status_t cracen_hkdf_sha256_hmac(const uint8_t *seed,
					    const uint8_t *label,
					    size_t label_len,
					    uint8_t index,
					    const uint8_t *opt_input,
					    size_t opt_input_sz,
					    uint8_t *out)
{
	cracen_mac_operation_t mac_op = {};
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t length;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(CRACEN_P256_KEY_SIZE));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);

	status = cracen_mac_sign_setup(&mac_op, &attr, seed, CRACEN_P256_KEY_SIZE,
				       PSA_ALG_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	if (opt_input != NULL) {
		status = cracen_mac_update(&mac_op, opt_input, opt_input_sz);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	status = cracen_mac_update(&mac_op, label, label_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_mac_update(&mac_op, &index, 1);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_mac_sign_finish(&mac_op, out, CRACEN_P256_KEY_SIZE, &length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	return status;
exit:
	cracen_mac_abort(&mac_op);
	return status;
}

static psa_status_t cracen_hkdf_sha256_expand(const uint8_t *seed,
					      const uint8_t *label,
					      size_t label_len,
					      uint8_t *out)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	status = cracen_hkdf_sha256_hmac(seed, label, label_len, 1, NULL, 0, out);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_hkdf_sha256_hmac(seed, label, label_len, 2, out, CRACEN_P256_KEY_SIZE,
					 out + CRACEN_P256_KEY_SIZE);

	return status;
}

static psa_status_t cracen_wpa3_sae_reduce_p256(const uint8_t *input,
						size_t input_size,
						uint8_t *output,
						size_t output_size)
{
	const uint8_t *prime = sx_pk_curve_prime(&sx_curve_nistp256);

	sx_const_op modulo = {.sz = sx_pk_curve_opsize(&sx_curve_nistp256),
			      .bytes = prime};
	sx_const_op operand = {.sz = input_size, .bytes = input};
	sx_op result = {.sz = output_size, .bytes = output};

	const struct sx_pk_cmd_def *cmd = SX_PK_CMD_ODD_MOD_REDUCE;
	int sx_status = sx_mod_single_op_cmd(cmd, &modulo, &operand, &result);

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t import_wpa3_sae_pt_key(const psa_key_attributes_t *attributes,
				    const uint8_t *data, size_t data_length,
				    uint8_t *key_buffer, size_t key_buffer_size,
				    size_t *key_buffer_length, size_t *key_bits)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	int sx_status;
	/** Note: input for this function is expected to be a pwd-seed value obtained
	 *	  as a result of HKDF extraction
	 */
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_key_type_t type = psa_get_key_type(attributes);
	psa_ecc_family_t psa_curve = PSA_KEY_TYPE_WPA3_SAE_ECC_GET_FAMILY(type);

	switch (type) {
	case PSA_KEY_TYPE_WPA3_SAE_ECC(PSA_ECC_FAMILY_SECP_R1):

		if (data_length != CRACEN_P256_POINT_SIZE) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		if (key_bits_attr != 0u && key_bits_attr != 256u) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		/* check point on curve */
		const struct sx_pk_ecurve *sx_curve;

		status = cracen_ecc_get_ecurve_from_psa(psa_curve, key_bits_attr,
							&sx_curve);
		if (status != PSA_SUCCESS) {
			return status;
		}

		MAKE_SX_CONST_POINT(key_pt, data, CRACEN_P256_POINT_SIZE);
		sx_status = sx_ec_ptoncurve(sx_curve, &key_pt);
		if (sx_status != SX_OK) {
			return silex_statuscodes_to_psa(sx_status);
		}

		if (!memcpy_check_non_zero(key_buffer, key_buffer_size,
						data, data_length)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		*key_buffer_length = data_length;
		*key_bits = key_bits_attr;

		return PSA_SUCCESS;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t cracen_derive_wpa3_sae_pt_key(const psa_key_attributes_t *attributes,
					   const uint8_t *input, size_t input_length,
					   uint8_t *key, size_t key_size, size_t *key_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	int sx_status;
	/** Note: input for this function is expected to be a pwd-seed value obtained
	 *	  as a result of HKDF extraction
	 */
	size_t key_bits_attr = psa_get_key_bits(attributes);
	psa_key_type_t type = psa_get_key_type(attributes);
	psa_ecc_family_t psa_curve = PSA_KEY_TYPE_WPA3_SAE_ECC_GET_FAMILY(type);
	uint8_t pwd_value[CRACEN_P256_POINT_SIZE];
	/* IEEE802.11-24 12.4.4.2.3 */
	const size_t req_pwd_value_len = CRACEN_P256_KEY_SIZE + CRACEN_P256_KEY_SIZE / 2;
	uint8_t u1_buf[CRACEN_P256_KEY_SIZE];
	uint8_t u2_buf[CRACEN_P256_KEY_SIZE];
	uint8_t p1_buf[CRACEN_P256_POINT_SIZE];
	uint8_t p2_buf[CRACEN_P256_POINT_SIZE];

	sx_op u1 = {.sz = CRACEN_P256_KEY_SIZE, .bytes = u1_buf};
	sx_op u2 = {.sz = CRACEN_P256_KEY_SIZE, .bytes = u2_buf};
	sx_op p1 = {.sz = CRACEN_P256_POINT_SIZE, .bytes = p1_buf};
	sx_op p2 = {.sz = CRACEN_P256_POINT_SIZE, .bytes = p2_buf};

	switch (type) {
	case PSA_KEY_TYPE_WPA3_SAE_ECC(PSA_ECC_FAMILY_SECP_R1):

		if (key_bits_attr != 256) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		if (input_length != CRACEN_P256_KEY_SIZE) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		if (key_size < CRACEN_P256_POINT_SIZE) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		/**
		 *  Expand u1:
		 *  pwd-value = HKDF-Expand(pwd-seed, “SAE Hash to Element u1 P1”, len)
		 */
		const char *label_u1 = "SAE Hash to Element u1 P1";

		status = cracen_hkdf_sha256_expand(input, (const uint8_t *)label_u1,
							strlen(label_u1), pwd_value);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* u1 = pwd-value modulo p */
		status = cracen_wpa3_sae_reduce_p256(pwd_value,
							req_pwd_value_len,
							u1.bytes, u1.sz);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* P1 = SSWU(u1) */
		sx_const_op u1_const;

		sx_get_const_op(&u1, &u1_const);
		status = cracen_ecc_h2e_sswu(psa_curve, key_bits_attr, &u1_const, &p1);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/**
		 *  Expand u2:
		 *  pwd-value = HKDF-Expand(pwd-seed, “SAE Hash to Element u2 P2”, len)
		 */
		const char *label_u2 = "SAE Hash to Element u2 P2";

		status = cracen_hkdf_sha256_expand(input, (const uint8_t *)label_u2,
							strlen(label_u2), pwd_value);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* u2 = pwd-value modulo p */
		status = cracen_wpa3_sae_reduce_p256(pwd_value,
							req_pwd_value_len,
							u2.bytes, u2.sz);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* P2 = SSWU(u2) */
		sx_const_op u2_const;

		sx_get_const_op(&u2, &u2_const);
		status = cracen_ecc_h2e_sswu(psa_curve, key_bits_attr, &u2_const, &p2);
		if (status != PSA_SUCCESS) {
			return status;
		}

		const struct sx_pk_ecurve *sx_curve;

		status = cracen_ecc_get_ecurve_from_psa(psa_curve,
							key_bits_attr,
							&sx_curve);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* PT = elem-op(P1, P2) = P1 + P2 operation here */
		MAKE_SX_CONST_POINT(p1_pt, p1.bytes, CRACEN_P256_POINT_SIZE);
		MAKE_SX_CONST_POINT(p2_pt, p2.bytes, CRACEN_P256_POINT_SIZE);
		MAKE_SX_POINT(p_pt, key, key_size);
		sx_status = sx_ecp_ptadd(sx_curve, &p1_pt, &p2_pt, &p_pt);
		if (sx_status != SX_OK) {
			return silex_statuscodes_to_psa(sx_status);
		}

		*key_length = CRACEN_P256_POINT_SIZE;
		return PSA_SUCCESS;

		return PSA_SUCCESS;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}
