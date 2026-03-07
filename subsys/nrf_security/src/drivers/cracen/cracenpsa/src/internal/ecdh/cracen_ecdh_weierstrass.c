/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/ecdh/cracen_ecdh_weierstrass.h>
#include <internal/ecc/cracen_ecc_helpers.h>

#include <cracen/common.h>
#include <cracen/mem_helpers.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/eccweierstrass.h>

psa_status_t cracen_ecdh_wrstr_calc_secret(const struct sx_pk_ecurve *curve,
					   const uint8_t *priv_key, size_t priv_key_size,
					   const uint8_t *publ_key, size_t publ_key_size,
					   uint8_t *output, size_t output_size,
					   size_t *output_length)
{
	int sx_status;
	psa_status_t psa_status;

	uint8_t scratch_char_x[CRACEN_MAC_ECC_PRIVKEY_BYTES];
	uint8_t scratch_char_y[CRACEN_MAC_ECC_PRIVKEY_BYTES];

	sx_const_ecop priv_key_buff = {.sz = priv_key_size, .bytes = priv_key};

	sx_pk_affine_point scratch_pnt = {{.bytes = scratch_char_x, .sz = priv_key_size},
					  {.bytes = scratch_char_y, .sz = priv_key_size}};
	sx_pk_const_affine_point publ_key_pnt = {};

	sx_pk_req req;

	if (publ_key_size != cracen_ecc_wstr_expected_pub_key_bytes(priv_key_size)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (publ_key[0] != CRACEN_ECC_PUBKEY_UNCOMPRESSED) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* For Weierstrass curves in PSA we only use the X coordinate of the
	 * point multiplication result.
	 */
	if (output_size < priv_key_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	publ_key_pnt.x.bytes = publ_key + 1;
	publ_key_pnt.x.sz = priv_key_size;

	publ_key_pnt.y.bytes = publ_key + priv_key_size + 1;
	publ_key_pnt.y.sz = priv_key_size;

	psa_status = cracen_ecc_check_public_key(curve, &publ_key_pnt);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	sx_pk_acquire_hw(&req);

	sx_status = sx_ecp_ptmult(&req, curve, &priv_key_buff, &publ_key_pnt, &scratch_pnt);
	if (sx_status == SX_OK) {
		sx_pk_const_affine_point c_scratch_pnt;

		sx_get_const_affine_point(&scratch_pnt, &c_scratch_pnt);
		sx_pk_ecop2mem(&c_scratch_pnt.x, output, c_scratch_pnt.x.sz);
		*output_length = c_scratch_pnt.x.sz;
	}

	safe_memzero(scratch_pnt.x.bytes, scratch_pnt.x.sz);
	safe_memzero(scratch_pnt.y.bytes, scratch_pnt.y.sz);

	psa_status = silex_statuscodes_to_psa(sx_status);

	sx_pk_release_req(&req);
	return psa_status;
}
