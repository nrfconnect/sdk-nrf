/* Nordic PSA extension: secp160r1 curve operations on the CRACEN PK engine.
 *
 * secp160r1 is not expressible in the PSA Crypto API, so this file talks to
 * silexpk directly instead of going through the PSA core. See
 * <psa/psa_ext_ecc.h> for the rationale.
 *
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/sys/util.h>

#include <psa/psa_ext_ecc.h>

#include <silexpk/core.h>
#include <silexpk/iomem.h>
#include <silexpk/ec_curves.h>
#include <silexpk/cmddefs/ecc.h>
#include <silexpk/cmddefs/modmath.h>
/* sxops/rsa.h uses sx_op / sx_const_op without declaring them. */
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/rsa.h>
#include <cracen/statuscodes.h>
#include <cracen/common.h>
#include <nrf_security_mem_helpers.h>

/* Operand size for the curve, in bytes.
 *
 * 21, not 20, even though the field prime is 160 bits. secp160r1 is the
 * unusual case where the group order is wider than the field:
 *
 *   p = FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF 7FFFFFFF        160 bits
 *   n = 01 00000000 00000000 0001F4C8 F927AED3 CA752257     161 bits
 *
 * sx_pk_list_ecc_inslots() derives a single op_size from the curve for every
 * slot, including the scalar. A 20-byte op_size would silently truncate any
 * scalar >= 2^160, which happens for roughly 1 in 2^80 reductions -- and is
 * exercised deliberately by the secp160r1 known-answer vectors in
 * tests/subsys/bluetooth/fast_pair/crypto. So the whole curve is widened by
 * one zero byte instead, which the engine supports: op_size is byte-granular
 * (sx_pk_write_command()), and sx_curve_ed448 already uses .sz = 57 for a
 * 448-bit field for the same reason.
 */
#define SECP160R1_OPSZ 21u

/* Curve parameters, big endian, in the order silexpk expects for a Weierstrass
 * curve: q, n, gx, gy, a, b. From SEC 2 v2, section 2.4.2, each left-padded
 * with one zero byte to SECP160R1_OPSZ.
 *
 * Kept file-local rather than added to silexpk's ec_curves.c: that file is
 * compiled unconditionally, so a curve added there would cost rodata on every
 * CRACEN build. Nothing requires a curve to live in that table -- the silexpk
 * entry points take a plain 'const struct sx_pk_ecurve *'.
 */
static const uint8_t params_secp160r1[] = {
	/* q (field prime) */
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x7f, 0xff, 0xff, 0xff,
	/* n (group order) */
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0xf4, 0xc8, 0xf9, 0x27, 0xae,
	0xd3, 0xca, 0x75, 0x22, 0x57,
	/* gx */
	0x00, 0x4a, 0x96, 0xb5, 0x68, 0x8e, 0xf5, 0x73,
	0x28, 0x46, 0x64, 0x69, 0x89, 0x68, 0xc3, 0x8b,
	0xb9, 0x13, 0xcb, 0xfc, 0x82,
	/* gy */
	0x00, 0x23, 0xa6, 0x28, 0x55, 0x31, 0x68, 0x94,
	0x7d, 0x59, 0xdc, 0xc9, 0x12, 0x04, 0x23, 0x51,
	0x37, 0x7a, 0xc5, 0xfb, 0x32,
	/* a */
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x7f, 0xff, 0xff, 0xfc,
	/* b */
	0x00, 0x1c, 0x97, 0xbe, 0xfc, 0x54, 0xbd, 0x7a,
	0x8b, 0x65, 0xac, 0xf8, 0x9f, 0x81, 0xd4, 0xd4,
	0xad, 0xc5, 0x65, 0xfa, 0x45,
};

BUILD_ASSERT(sizeof(params_secp160r1) == 6 * SECP160R1_OPSZ,
	     "sx_pk_count_curve_params() derives the parameter count by dividing "
	     "params_total_sz by sz, so the array must hold exactly 6 operands");

/* .curveflags = 0 is PK_OP_FLAGS_PRIME, a generic Weierstrass curve over
 * GF(p) whose parameters are uploaded from RAM. The macro itself lives in
 * silexpk/src/regs_curves.h, which is not an exported include directory.
 */
static const struct sx_pk_ecurve curve_secp160r1 = {
	.curveflags = 0,
	.sz = SECP160R1_OPSZ,
	.params = params_secp160r1,
	.params_total_sz = sizeof(params_secp160r1),
};

/* Retry budget for blinded point multiplication, which can fail with
 * SX_ERR_NOT_INVERTIBLE when CONFIG_CRACEN_ECC_COUNTERMEASURES is set.
 * Matches MAX_ECC_ATTEMPTS in cracen_ecc_keygen.c.
 */
#define SECP160R1_MAX_ATTEMPTS 10

psa_status_t psa_ext_ecc_secp160r1_scalar_reduce(const uint8_t *input, size_t input_length,
						 uint8_t *output, size_t output_size)
{
	sx_const_op modulo = {.sz = SECP160R1_OPSZ,
			      .bytes = sx_pk_curve_order(&curve_secp160r1)};
	sx_const_op operand = {.sz = input_length, .bytes = input};
	sx_op result = {.sz = PSA_EXT_ECC_SECP160R1_SCALAR_SIZE, .bytes = output};
	sx_pk_req req;
	int sx_status;

	if (input_length == 0u || input_length > PSA_EXT_ECC_SECP160R1_MAX_INPUT_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	if (output_size < PSA_EXT_ECC_SECP160R1_SCALAR_SIZE) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	sx_pk_acquire_hw(&req);

	/* The secp160r1 group order is odd, so the ODD variant applies. It is
	 * also the only reduce command whose operands may differ in size
	 * (sx_pk_gfcmd_opsize() takes the MAX), which is what lets a 32-byte
	 * input be reduced by a 21-byte modulus.
	 */
	sx_status = sx_mod_single_op_cmd(&req, SX_PK_CMD_ODD_MOD_REDUCE, &modulo, &operand,
					&result);
	sx_pk_release_req(&req);

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t psa_ext_ecc_secp160r1_scalar_mult_base(const uint8_t *scalar, size_t scalar_length,
						    uint8_t *x, size_t x_size)
{
	uint8_t scalar_padded[SECP160R1_OPSZ] = {0};
	uint8_t point_x[SECP160R1_OPSZ];
	struct sx_pk_inops_ecp_mult inputs;
	const uint8_t **outputs;
	sx_pk_req req;
	int sx_status;
	int attempts = 0;
	uint8_t acc = 0;

	if (scalar_length == 0u || scalar_length > PSA_EXT_ECC_SECP160R1_SCALAR_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	if (x_size < PSA_EXT_ECC_SECP160R1_COORD_SIZE) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* A zero scalar yields the point at infinity, which has no affine
	 * x-coordinate. Reject it here rather than let a caller advertise an
	 * all-zero result as if it were a valid point.
	 */
	for (size_t i = 0; i < scalar_length; i++) {
		acc |= scalar[i];
	}
	if (acc == 0u) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	memcpy(&scalar_padded[SECP160R1_OPSZ - scalar_length], scalar, scalar_length);

	sx_pk_acquire_hw(&req);

	do {
		sx_pk_set_cmd(&req, SX_PK_CMD_ECC_PTMUL);

		sx_status = sx_pk_list_ecc_inslots(&req, &curve_secp160r1, 0,
						   (struct sx_pk_slot *)&inputs);
		if (sx_status != SX_OK) {
			sx_pk_release_req(&req);
			safe_memzero(scalar_padded, sizeof(scalar_padded));
			return silex_statuscodes_to_psa(sx_status);
		}

		/* sx_pk_list_ecc_inslots() does not clear the input slots, so
		 * every slot the command lists must be written.
		 */
		sx_wrpkmem(inputs.k.addr, scalar_padded, SECP160R1_OPSZ);
		sx_pk_write_curve_gen(&req, &curve_secp160r1, inputs.px, inputs.py);

		sx_pk_run(&req);
		sx_status = sx_pk_wait(&req);

		if (sx_status == SX_ERR_NOT_INVERTIBLE) {
			if (++attempts == SECP160R1_MAX_ATTEMPTS) {
				sx_pk_release_req(&req);
				safe_memzero(scalar_padded, sizeof(scalar_padded));
				return silex_statuscodes_to_psa(SX_ERR_TOO_MANY_ATTEMPTS);
			}
		}
	} while (sx_status == SX_ERR_NOT_INVERTIBLE);

	if (sx_status == SX_OK) {
		outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
		sx_rdpkmem(point_x, outputs[0], SECP160R1_OPSZ);
	}
	sx_pk_release_req(&req);
	safe_memzero(scalar_padded, sizeof(scalar_padded));

	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* Drop the operand padding byte. The x-coordinate is reduced mod p, so
	 * it always fits in PSA_EXT_ECC_SECP160R1_COORD_SIZE bytes.
	 */
	memcpy(x, &point_x[SECP160R1_OPSZ - PSA_EXT_ECC_SECP160R1_COORD_SIZE],
	       PSA_EXT_ECC_SECP160R1_COORD_SIZE);
	return PSA_SUCCESS;
}
