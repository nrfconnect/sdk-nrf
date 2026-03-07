/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/ecdh/cracen_ecdh_montgomery.h>
#include <internal/ecc/cracen_ecc_helpers.h>

#include <cracen/common.h>
#include <cracen/mem_helpers.h>
#include <cracen/ec_helpers.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/montgomery.h>
#include <silexpk/ec_curves.h>

psa_status_t cracen_ecdh_montgmr_calc_secret(const struct sx_pk_ecurve *curve,
					     const uint8_t *priv_key, size_t priv_key_size,
					     const uint8_t *publ_key, size_t publ_key_size,
					     uint8_t *output, size_t output_size,
					     size_t *output_length)
{
	int sx_status;
	const size_t curve_op_sz = sx_pk_curve_opsize(curve);
	sx_pk_req req;

	if (publ_key_size != curve_op_sz) {
		return PSA_ERROR_INVALID_ARGUMENT;
	} else if (output_size < curve_op_sz) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	} else {
		/* For compliance */
	}

	sx_status = SX_ERR_INVALID_CURVE_PARAM;

	sx_pk_acquire_hw(&req);

	if (curve_op_sz == CRACEN_X25519_KEY_SIZE_BYTES) {
		struct sx_x25519_op k;

		memcpy(k.bytes, priv_key, CRACEN_X25519_KEY_SIZE_BYTES);
		decode_scalar_25519(k.bytes);

		struct sx_x25519_op pt;

		memcpy(pt.bytes, publ_key, CRACEN_X25519_KEY_SIZE_BYTES);
		decode_u_coordinate_25519(pt.bytes);

		sx_status = sx_x25519_ptmult(&req, &k, &pt, (struct sx_x25519_op *)output);
		if (sx_status != SX_OK) {
			goto exit;
		}

	} else if (curve_op_sz == CRACEN_X448_KEY_SIZE_BYTES) {
		struct sx_x448_op k;

		memcpy(k.bytes, priv_key, CRACEN_X448_KEY_SIZE_BYTES);
		decode_scalar_448(k.bytes);

		/* 448 % 8 = 0, so there is no need to decode pt coordinate. */
		sx_status = sx_x448_ptmult(&req, &k, (struct sx_x448_op *)publ_key,
					   (struct sx_x448_op *)output);
		if (sx_status != SX_OK) {
			goto exit;
		}
	} else {
		/* For compliance */
	}

	*output_length = curve_op_sz;
exit:
	sx_pk_release_req(&req);
	return silex_statuscodes_to_psa(sx_status);
}
