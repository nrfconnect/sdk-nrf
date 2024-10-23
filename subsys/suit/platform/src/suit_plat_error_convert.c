/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "suit_plat_error_convert.h"

int suit_plat_err_to_processor_err_convert(suit_plat_err_t plat_err)
{
	int proc_err = SUIT_ERR_CRASH;

	switch (plat_err) {
	case SUIT_PLAT_SUCCESS:
		proc_err = SUIT_SUCCESS;
		break;
	case SUIT_PLAT_ERR_NOMEM:
	case SUIT_PLAT_ERR_NO_RESOURCES:
		proc_err = SUIT_ERR_OVERFLOW;
		break;
	case SUIT_PLAT_ERR_AUTHENTICATION:
		proc_err = SUIT_ERR_AUTHENTICATION;
		break;
	case SUIT_PLAT_ERR_CBOR_DECODING:
		proc_err = SUIT_ERR_DECODING;
		break;
	/* To be extended */
	default:
		/* Return SUIT_ERR_CRASH */
		break;
	}

	return proc_err;
}
