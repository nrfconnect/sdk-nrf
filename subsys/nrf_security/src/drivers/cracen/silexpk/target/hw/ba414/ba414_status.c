/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/statuscodes.h>
#include <silexpk/core.h>
#include "ba414_status.h"

int convert_ba414_status(int code)
{
	code &= 0x1FFF0;

	switch (code) {
	case 0:
		return SX_OK;
	case (1 << 4):
		return SX_ERR_POINT_NOT_ON_CURVE;
	case (1 << 5):
		return SX_ERR_POINT_AT_INFINITY;
	case (1 << 6):
		return SX_ERR_OUT_OF_RANGE;
	case (1 << 7):
		return SX_ERR_INVALID_MODULUS;
	case (1 << 8):
		return SX_ERR_NOT_IMPLEMENTED;
	case (1 << 9):
		return SX_ERR_INVALID_SIGNATURE;
	case (1 << 10):
		return SX_ERR_INVALID_CURVE_PARAM;
	case (1 << 11):
		return SX_ERR_NOT_INVERTIBLE;
	case (1 << 12):
		return SX_ERR_COMPOSITE_VALUE;
	case (1 << 13):
		return SX_ERR_NOT_QUADRATIC_RESIDUE;
	case (1 << 15):
		return SX_ERR_EXPIRED;
	case (1 << 16):
		return SX_ERR_BUSY;
	default:
		return SX_ERR_UNKNOWN_ERROR;
	}
}

#define BA414EP_PREDEFCURVES_MASK (0x0F << 20)
#define BA414EP_CM_DISABLE	  (1 << 31)

void convert_ba414_capabilities(uint32_t ba414epfeatures, struct sx_pk_capabilities *caps)
{
	uint32_t maxopsz = ba414epfeatures & 0xFFF;

	caps->max_gfp_opsz = maxopsz;
	caps->max_ecc_opsz = (maxopsz > 72) ? 72 : maxopsz;
	caps->max_gfb_opsz = caps->max_ecc_opsz;
	caps->blinding = 1;
	caps->blinding_sz = 8;
	if (!(ba414epfeatures & (1 << 17))) {
		/* No binary field support */
		caps->max_gfb_opsz = 0;
	}
	if (!(ba414epfeatures & (1 << 16))) {
		/* No prime field support */
		caps->max_gfp_opsz = 0;
		caps->max_ecc_opsz = 0;
	}
	if ((ba414epfeatures & BA414EP_PREDEFCURVES_MASK) != BA414EP_PREDEFCURVES_MASK) {
		/* Standard curve parameters are not predefined. Currently
		 * not supported yet by SilexPK.
		 */
		caps->max_ecc_opsz = 0;
	}
	if (ba414epfeatures & BA414EP_CM_DISABLE) {
		caps->blinding = 0;
		caps->blinding_sz = 0;
	}
}
