/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/ecc/cracen_ecc_helpers.h>

#include <cracen/common.h>
#include <hal/nrf_cracen.h>
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <nrfx.h>
#include <silexpk/core.h>
#include <silexpk/ec_curves.h>
#include <silexpk/ik.h>
#include <silexpk/sxops/eccweierstrass.h>
#include <silexpk/sxops/rsa.h>
#include <silexpk/cmddefs/modexp.h>
#include <silexpk/cmddefs/rsa.h>
#include <stddef.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/hashdefs.h>
#include <zephyr/sys/util.h>
#include <psa/nrf_platform_key_ids.h>

#define NOT_ENABLED_CURVE	(0)

static psa_status_t get_sx_brainpool_curve(size_t curve_bits, const struct sx_pk_ecurve **sicurve)
{
	const struct sx_pk_ecurve *selected_curve = NOT_ENABLED_CURVE;

	switch (curve_bits) {
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
	case 256:
		IF_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_SECP_K1_256,
			   (selected_curve = &sx_curve_secp256k1));
		break;
	default: /* For compliance */
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

static psa_status_t check_wstr_publ_key_for_ecdh(psa_ecc_family_t curve_family, size_t curve_bits,
						 const uint8_t *data, size_t data_length)
{
	size_t priv_key_size = PSA_BITS_TO_BYTES(curve_bits);
	psa_status_t psa_status;
	const struct sx_pk_ecurve *curve = NULL;

	sx_pk_const_affine_point publ_key_pnt = {};

	publ_key_pnt.x.bytes = &data[1];
	publ_key_pnt.x.sz = priv_key_size;

	publ_key_pnt.y.bytes = &data[1 + priv_key_size];
	publ_key_pnt.y.sz = priv_key_size;

	psa_status = cracen_ecc_get_ecurve_from_psa(curve_family, curve_bits, &curve);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	return cracen_ecc_check_public_key(curve, &publ_key_pnt);
}

psa_status_t check_wstr_pub_key_data(psa_algorithm_t key_alg, psa_ecc_family_t curve,
				     size_t key_bits, const uint8_t *data,
				     size_t data_length)
{
	size_t expected_pub_key_size =
		cracen_ecc_wstr_expected_pub_key_bytes(PSA_BITS_TO_BYTES(key_bits));

	if (data[0] != CRACEN_ECC_PUBKEY_UNCOMPRESSED) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_length != expected_pub_key_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (PSA_ALG_IS_ECDH(key_alg)) {
		return check_wstr_publ_key_for_ecdh(curve, key_bits, data, data_length);
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_ecc_check_public_key(const struct sx_pk_ecurve *curve,
					 const sx_pk_const_affine_point *in_pnt)
{
	int sx_status;
	uint8_t char_x[CRACEN_MAC_ECC_PRIVKEY_BYTES];
	uint8_t char_y[CRACEN_MAC_ECC_PRIVKEY_BYTES];

	/* Get the order of the curve from the parameters */
	struct sx_const_buf n = {.sz = sx_pk_curve_opsize(curve),
				 .bytes = sx_pk_curve_order(curve)};

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

psa_status_t cracen_ecc_reduce_p256(const uint8_t *input, size_t input_size, uint8_t *output,
				    size_t output_size)
{
	const uint8_t *order = sx_pk_curve_order(&sx_curve_nistp256);

	sx_const_op modulo = {.sz = CRACEN_P256_KEY_SIZE, .bytes = order};
	sx_const_op operand = {.sz = input_size, .bytes = input};
	sx_op result = {.sz = output_size, .bytes = output};

	/* The nistp256 curve order (n) is prime so we use the ODD variant of the reduce command. */
	const struct sx_pk_cmd_def *cmd = SX_PK_CMD_ODD_MOD_REDUCE;
	int sx_status = sx_mod_single_op_cmd(cmd, &modulo, &operand, &result);

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_ecc_is_quadratic_residue(const sx_const_op *curve_prime,
					     const sx_const_op *value,
					     bool *is_qr)
{
#if PSA_VENDOR_ECC_MAX_CURVE_BITS > 0
	int sx_status;
	const size_t op_size = curve_prime->sz;
	/** Multipurpose buffers: these can be reused so their
	 *  names consist of the values they can contain, ending with "_buf"
	 */
	uint8_t one_prsh_buf[PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)] = {};
	uint8_t pm1_tmp_buf[PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)] = {};

	/**
	 * value is a quadratic residue mod p if
	 * value^((p-1)/2) mod p equals zero or one
	 */

	/* p_m_1 = (p - 1) */
	cracen_be_add(one_prsh_buf, op_size, 1); /* 1 */
	if (cracen_be_sub(curve_prime->bytes, one_prsh_buf, pm1_tmp_buf, op_size) != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	sx_op p_m_1 = {.sz = op_size, .bytes = pm1_tmp_buf};

	/* (p - 1) / 2 */
	cracen_be_rshift(p_m_1.bytes, 1, one_prsh_buf, op_size);
	sx_const_op pm1_div_2 = {.sz = op_size, .bytes = one_prsh_buf};

	/* tmp = value^((p-1)/2) mod p */
	sx_op tmp = {.sz = op_size, .bytes = pm1_tmp_buf};

	sx_status = sx_mod_exp(NULL, value, &pm1_div_2, curve_prime, &tmp);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	safe_memzero(one_prsh_buf, op_size);
	cracen_be_add(one_prsh_buf, op_size, 1);

	*is_qr = constant_memcmp_is_zero(tmp.bytes, tmp.sz);
	*is_qr = *is_qr || (constant_memcmp(tmp.bytes, one_prsh_buf, tmp.sz) == 0);

	return PSA_SUCCESS;
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_VENDOR_ECC_MAX_CURVE_BITS > 0 */
}

#if PSA_VENDOR_ECC_MAX_CURVE_BITS > 0
static psa_status_t cracen_get_sswu_z_brainpool_curve(size_t curve_bits, int *z)
{
	/**
	 *  Unique curve parameter Z is taken from table 12.2 of
	 *  the IEEE802.11 12.4.4.2.3
	 */
	switch (curve_bits) {
	case 256:
		/* IANA value (group): 28 */
		*z = -2;
		break;
	case 384:
		/* IANA value (group): 29 */
		*z = -5;
		break;
	case 512:
		/* IANA value (group): 30 */
		*z = 7;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static psa_status_t cracen_get_sswu_z_r1_curve(size_t curve_bits, int *z)
{
	/**
	 *  Unique curve parameter Z is taken from table 12.2 of
	 *  the IEEE802.11 12.4.4.2.3
	 */
	switch (curve_bits) {
	case 192:
		/* IANA value (group): 25 */
		*z = -5;
		break;
	case 224:
		/* IANA value (group): 26 */
		*z = 31;
		break;
	case 256:
		/* IANA value (group): 19 */
		*z = -10;
		break;
	case 384:
		/* IANA value (group): 20 */
		*z = -12;
		break;
	case 521:
		/* IANA value (group): 21 */
		*z = -4;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

static psa_status_t cracen_get_sswu_z(psa_ecc_family_t curve_family,
				      size_t curve_bits,
				      int *z)
{
	switch (curve_family) {
	case PSA_ECC_FAMILY_BRAINPOOL_P_R1:
		return cracen_get_sswu_z_brainpool_curve(curve_bits, z);
	case PSA_ECC_FAMILY_SECP_R1:
		return cracen_get_sswu_z_r1_curve(curve_bits, z);
	case PSA_ECC_FAMILY_MONTGOMERY:
	case PSA_ECC_FAMILY_TWISTED_EDWARDS:
	case PSA_ECC_FAMILY_SECP_K1:
	case PSA_ECC_FAMILY_SECP_R2:
	case PSA_ECC_FAMILY_SECT_K1:
	case PSA_ECC_FAMILY_SECT_R1:
	case PSA_ECC_FAMILY_SECT_R2:
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

static int cracen_ecc_h2e_sswu_calc_gx(const sx_const_op *curve_a, const sx_const_op *curve_b,
				       const sx_const_op *modulo, const sx_const_op *x, sx_op *gx)
{
	int sx_status = SX_ERR_CORRUPTION_DETECTED;
	size_t op_size = curve_a->sz;
	/** Multipurpose buffers: these can be reused so their
	 *  names consist of the values they can contain, ending with "_buf"
	 */
	uint8_t three_ax_x3ax_buf[PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)] = {};
	uint8_t x3_buf[PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)] = {};
	uint8_t ax_buf[PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)] = {};

	const struct sx_pk_cmd_def *cmd_mul = SX_PK_CMD_ODD_MOD_MULT;
	const struct sx_pk_cmd_def *cmd_add = SX_PK_CMD_MOD_ADD;

	/* gx = x^3 + a*x + b */
	/* x_3 = x^3 */
	cracen_be_add(three_ax_x3ax_buf, op_size, 3); /* 3 */
	sx_const_op exp = {.sz = op_size, .bytes = three_ax_x3ax_buf};
	sx_op x_3 = {.sz = op_size, .bytes = x3_buf};

	sx_status = sx_mod_exp(NULL, x, &exp, modulo, &x_3);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* ax = a*x */
	sx_op ax = {.sz = op_size, .bytes = ax_buf};

	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, modulo, curve_a, x, &ax);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* x_3_ax = x^3 + a*x */
	sx_const_op x_3_const;
	sx_const_op ax_const;
	sx_op x_3_ax = {.sz = op_size, .bytes = three_ax_x3ax_buf};

	sx_get_const_op(&x_3, &x_3_const);
	sx_get_const_op(&ax, &ax_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_add, modulo, &x_3_const, &ax_const, &x_3_ax);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* gx = x_3_ax + b */
	sx_const_op x_3_ax_const;

	sx_get_const_op(&x_3_ax, &x_3_ax_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_add, modulo, &x_3_ax_const, curve_b, gx);

	return sx_status;
}

static int cracen_ecc_h2e_sswu_calc_m(size_t curve_opsize,
				      const sx_const_op *z_param,
				      const sx_const_op *u,
				      const sx_const_op *modulo,
				      sx_op *zu_sqr, sx_op *m)
{
	int sx_status = SX_ERR_CORRUPTION_DETECTED;
	const size_t max_key_size = PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS);
	/** Multipurpose buffer: it can be reused so its
	 *  name consists of the values they can contain, ending with "_buf"
	 */
	uint8_t usqr_zufourth_buf[max_key_size];

	const struct sx_pk_cmd_def *cmd_mul = SX_PK_CMD_ODD_MOD_MULT;
	const struct sx_pk_cmd_def *cmd_add = SX_PK_CMD_MOD_ADD;

	/** m = (z^2 * u^4 + z * u^2) modulo p
	 *  m = z*u^2(z*u^2 + 1);
	 *  zu_sqr = z*u^2 => m = zu_sqr(zu_sqr + 1) => m = (zu_sqr^2 + zu_sqr) modulo p
	 */

	/* u_sqr = u^2 */
	sx_op u_sqr = {.sz = curve_opsize, .bytes = usqr_zufourth_buf};

	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, modulo, u, u, &u_sqr);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* zu_sqr = z * u_sqr */
	sx_const_op u_sqr_const;

	sx_get_const_op(&u_sqr, &u_sqr_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, modulo, z_param, &u_sqr_const, zu_sqr);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* zu_fourth = zu_sqr^2 */
	sx_const_op zu_sqr_const;
	sx_op zu_fourth = {.sz = curve_opsize, .bytes = usqr_zufourth_buf};

	sx_get_const_op(zu_sqr, &zu_sqr_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, modulo, &zu_sqr_const,
					 &zu_sqr_const, &zu_fourth);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* m = zu_fourth + zu_sqr */
	sx_const_op zu_fourth_const;

	sx_get_const_op(&zu_fourth, &zu_fourth_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_add, modulo, &zu_fourth_const,
					 &zu_sqr_const, m);

	return sx_status;
}

static int cracen_ecc_h2e_sswu_calc_x1(const sx_const_op *a,
				       const sx_const_op *b,
				       const sx_const_op *modulo,
				       const sx_const_op *z_param,
				       const sx_const_op *t_const,
				       bool is_m_zero,
				       sx_op *x1)
{
	int sx_status = SX_ERR_CORRUPTION_DETECTED;
	const size_t max_key_size = PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS);
	size_t curve_op_size = a->sz;
	/** Multipurpose buffers: these can be reused so their
	 *  names consist of the values they can contain, ending with "_buf"
	 */
	uint8_t x1b_buf[max_key_size];
	uint8_t za_pmb_one_buf[max_key_size];
	uint8_t x1a_buf[max_key_size];
	uint8_t inva_onept_buf[max_key_size];
	uint8_t mbinva_buf[max_key_size];

	const struct sx_pk_cmd_def *cmd_mul  = SX_PK_CMD_ODD_MOD_MULT;
	const struct sx_pk_cmd_def *cmd_add  = SX_PK_CMD_MOD_ADD;
	const struct sx_pk_cmd_def *cmd_div  = SX_PK_CMD_ODD_MOD_DIV;
	const struct sx_pk_cmd_def *cmd_inv  = SX_PK_CMD_ODD_MOD_INV;

	/**
	 *  x1 = CSEL(l, (b / (z * a) modulo p), ((– b/a) * (1 + t)) modulo p)
	 *
	 *  x1a = b / (z * a) modulo p;
	 *  x1b = ((– b/a) * (1 + t)) modulo p;
	 *  x1 = CSEL(l, x1a, x1b);
	 */

	/* (z * a) */
	sx_op za = {.sz = curve_op_size, .bytes = za_pmb_one_buf};

	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, modulo, z_param, a, &za);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* x1a = b / (z * a) modulo p */
	sx_const_op za_const;
	sx_op x1a = {.sz = curve_op_size, .bytes = x1a_buf};

	sx_get_const_op(&za, &za_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_div, modulo, b, &za_const, &x1a);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* x1b calculation */
	/* p_m_b = p - b */
	if (cracen_be_sub(modulo->bytes, b->bytes, za_pmb_one_buf, modulo->sz) != 0) {
		return SX_ERR_INVALID_ARG;
	}
	sx_const_op p_m_b = {.sz = modulo->sz, .bytes = za_pmb_one_buf};

	/* inv_a = 1/a mod p */
	sx_op inv_a = {.sz = curve_op_size, .bytes = inva_onept_buf};

	sx_status = sx_mod_single_op_cmd(cmd_inv, modulo, a, &inv_a);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* m_b_inv_a = (p - b) mod p * 1/a mod p = (– b/a) mod p */
	sx_const_op inv_a_const;
	sx_op m_b_inv_a = {.sz = curve_op_size, .bytes = mbinva_buf};

	sx_get_const_op(&inv_a, &inv_a_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, modulo, &p_m_b, &inv_a_const, &m_b_inv_a);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* one_p_t = (1 + t) mod p */
	safe_memzero(za_pmb_one_buf, curve_op_size);
	cracen_be_add(za_pmb_one_buf, curve_op_size, 1);

	sx_const_op one = {.sz = curve_op_size, .bytes = za_pmb_one_buf};
	sx_op one_p_t = {.sz = curve_op_size, .bytes = inva_onept_buf};

	sx_status = sx_mod_primitive_cmd(NULL, cmd_add, modulo, &one, t_const, &one_p_t);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* x1b = (m_b_inv_a * one_p_t) mod p = ((– b/a) * (1 + t)) mod p */
	sx_const_op m_b_inv_a_const;
	sx_const_op one_p_t_const;
	sx_op x1b = {.sz = curve_op_size, .bytes = x1b_buf};

	sx_get_const_op(&m_b_inv_a, &m_b_inv_a_const);
	sx_get_const_op(&one_p_t, &one_p_t_const);
	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, modulo, &m_b_inv_a_const,
					 &one_p_t_const, &x1b);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* x1 = CSEL(l, (b / (z * a) modulo p), ((– b/a) * (1 + t)) modulo p) */
	constant_select_bin(is_m_zero, x1a.bytes, x1b.bytes, x1->bytes, x1->sz);
	return sx_status;
}
#endif /* PSA_VENDOR_ECC_MAX_CURVE_BITS > 0 */

psa_status_t cracen_ecc_h2e_sswu(psa_ecc_family_t curve_family, size_t curve_bits,
				 const sx_const_op *u, const sx_op *result)
{
#if PSA_VENDOR_ECC_MAX_CURVE_BITS > 0
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	int sx_status;
	const size_t max_key_size = PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS);

	int z_int;
	uint8_t z_buf[max_key_size];
	uint8_t x2_buf[max_key_size];
	uint8_t v_buf[max_key_size];
	/** Multipurpose buffers: these can be reused so their
	 *  names consist of the values they can contain, ending with "_buf"
	 */
	uint8_t two_pdec_buf[max_key_size];
	uint8_t pmy_buf[max_key_size];
	uint8_t zero_t_gx1_x_buf[max_key_size];
	uint8_t zusqr_gx2_y_buf[max_key_size];
	uint8_t m_x1_buf[max_key_size];

	const struct sx_pk_ecurve *sx_curve = NULL;
	const struct sx_pk_cmd_def *cmd_mul  = SX_PK_CMD_ODD_MOD_MULT;
	const struct sx_pk_cmd_def *cmd_sqrt = SX_PK_CMD_MOD_SQRT;

	status = cracen_ecc_get_ecurve_from_psa(curve_family, curve_bits, &sx_curve);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Output of this algorithm is ECC curve point (x; y) */
	if (result->sz < (u->sz * 2u)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	sx_const_op z_param = {
		.sz = sx_pk_curve_opsize(sx_curve),
		.bytes = z_buf
	};

	status = cracen_get_sswu_z(curve_family, curve_bits, &z_int);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (z_int < 0) {
		safe_memzero(zero_t_gx1_x_buf, sx_pk_curve_opsize(sx_curve));
		cracen_be_add(zero_t_gx1_x_buf, sx_pk_curve_opsize(sx_curve), abs(z_int));
		cracen_be_sub(sx_pk_curve_prime(sx_curve), zero_t_gx1_x_buf, z_buf,
			      sx_pk_curve_opsize(sx_curve));
	} else {
		cracen_be_add(z_buf, sx_pk_curve_opsize(sx_curve), abs(z_int));
	}

	sx_const_op modulo = {.sz = sx_pk_curve_opsize(sx_curve),
			      .bytes = sx_pk_curve_prime(sx_curve)
	};

	/**
	 * CEQ(x, y) - operates in constant time and returns true if x equals y and false otherwise;
	 * CSEL(x, y, z) - operates in constant time and returns y if x is true and z otherwise
	 * inv0(x) - is calculated as x^(p-2) modulo p.
	 *
	 * Note: SSWU algorithm shall always be executed in constant time.
	 */

	sx_const_op zu_sqr_const;
	sx_op zu_sqr = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = zusqr_gx2_y_buf};
	sx_op m = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = m_x1_buf};

	/* m = (z^2 * u^4 + z * u^2) modulo p */
	sx_status = cracen_ecc_h2e_sswu_calc_m(sx_pk_curve_opsize(sx_curve),
					       &z_param, u, &modulo, &zu_sqr, &m);
	if (sx_status != SX_OK) {
		goto error;
	}
	sx_get_const_op(&zu_sqr, &zu_sqr_const);

	/**
	 * l = CEQ(m, 0) =>
	 * is_m_zero = (m == 0);
	 */
	bool is_m_zero = constant_memcmp_is_zero(m.bytes, m.sz);

	/**
	 * t = inv0(m) =>
	 * t = m^(p-2) modulo p;
	 */
	safe_memzero(two_pdec_buf, sx_pk_curve_opsize(sx_curve));
	cracen_be_add(two_pdec_buf, sx_pk_curve_opsize(sx_curve), 2);
	cracen_be_sub(sx_pk_curve_prime(sx_curve), two_pdec_buf, two_pdec_buf,
		      sx_pk_curve_opsize(sx_curve));

	sx_const_op m_const;
	sx_const_op p_decreased = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = two_pdec_buf};
	sx_op t = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = zero_t_gx1_x_buf};

	sx_get_const_op(&m, &m_const);
	sx_status = sx_mod_exp(NULL, &m_const, &p_decreased, &modulo, &t);
	if (sx_status != SX_OK) {
		goto error;
	}

	/* x1 = CSEL(l, (b / (z * a) modulo p), ((– b/a) * (1 + t)) modulo p) */
	sx_const_op a = {.sz = sx_pk_curve_opsize(sx_curve),
			 .bytes = sx_pk_curve_param_a(sx_curve)};
	sx_const_op b = {.sz = sx_pk_curve_opsize(sx_curve),
			 .bytes = sx_pk_curve_param_b(sx_curve)};
	sx_op x1 = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = m_x1_buf};
	sx_const_op t_const;

	sx_get_const_op(&t, &t_const);
	sx_status = cracen_ecc_h2e_sswu_calc_x1(&a, &b, &modulo, &z_param,
						&t_const, is_m_zero, &x1);
	if (sx_status != SX_OK) {
		goto error;
	}

	/* gx1 = (x1^3 + a * x1 + b) modulo p */
	sx_const_op x1_const;
	sx_op gx1 = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = zero_t_gx1_x_buf};

	sx_get_const_op(&x1, &x1_const);
	sx_status = cracen_ecc_h2e_sswu_calc_gx(&a, &b, &modulo, &x1_const, &gx1);
	if (sx_status != SX_OK) {
		goto error;
	}

	/* x2 = (z * u^2 * x1) modulo p */
	sx_op x2 = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = x2_buf};

	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, &modulo, &zu_sqr_const, &x1_const, &x2);
	if (sx_status != SX_OK) {
		goto error;
	}

	/* gx2 = (x2^3 + a * x2 + b) modulo p */
	sx_const_op x2_const;
	sx_op gx2 = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = zusqr_gx2_y_buf};

	sx_get_const_op(&x2, &x2_const);
	sx_status = cracen_ecc_h2e_sswu_calc_gx(&a, &b, &modulo, &x2_const, &gx2);
	if (sx_status != SX_OK) {
		goto error;
	}

	/* l = gx1 is a quadratic residue modulo p */
	sx_const_op gx1_const;
	bool is_gx1_qr = false;

	sx_get_const_op(&gx1, &gx1_const);
	status = cracen_ecc_is_quadratic_residue(&modulo, &gx1_const, &is_gx1_qr);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* v = CSEL(l, gx1, gx2) */
	sx_op v = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = v_buf};

	constant_select_bin(is_gx1_qr, gx1.bytes, gx2.bytes, v.bytes, v.sz);

	/* x = CSEL(l, x1, x2 ) */
	sx_op x = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = zero_t_gx1_x_buf};

	constant_select_bin(is_gx1_qr, x1.bytes, x2.bytes, x.bytes, x.sz);

	/* y = sqrt(v) */
	sx_const_op v_const;
	sx_op y = {.sz = sx_pk_curve_opsize(sx_curve), .bytes = zusqr_gx2_y_buf};

	sx_get_const_op(&v, &v_const);
	sx_status = sx_mod_single_op_cmd(cmd_sqrt, &modulo, &v_const, &y);
	if (sx_status != SX_OK) {
		goto error;
	}

	/* l = CEQ(LSB(u), LSB(y)) */
	uint8_t lsb_u = u->bytes[u->sz - 1] & 0x01;
	uint8_t lsb_y = y.bytes[y.sz - 1] & 0x01;
	bool lsb_eq = (constant_memcmp(&lsb_u, &lsb_y, 1) == 0);

	/* P = CSEL(l, (x,y), (x, p – y)) */
	memcpy(result->bytes, x.bytes, sx_pk_curve_opsize(sx_curve));
	/* pmy_buf = p - y */
	cracen_be_sub(sx_pk_curve_prime(sx_curve), y.bytes, pmy_buf,
		      sx_pk_curve_opsize(sx_curve));
	constant_select_bin(lsb_eq, y.bytes, pmy_buf,
			    result->bytes + sx_pk_curve_opsize(sx_curve),
			    sx_pk_curve_opsize(sx_curve));

	return status;
error:
	return silex_statuscodes_to_psa(sx_status);
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_VENDOR_ECC_MAX_CURVE_BITS > 0 */
}
