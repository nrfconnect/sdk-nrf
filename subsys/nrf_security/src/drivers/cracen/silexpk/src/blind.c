/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include <silexpk/blinding.h>
#include <cracen/statuscodes.h>
#include <cracen/prng_pool.h>
#include "internal.h"

int sx_pk_get_blinding_factor(sx_pk_blind_factor *bld_factor)
{
	int status;
	const struct sx_pk_capabilities *caps = sx_pk_fetch_capabilities();
	if (!caps->blinding) {
		return SX_ERR_INCOMPATIBLE_HW;
	};

	/* The blinding factor needs to be a non zero value, make sure that
	 * at least half of it is not zero.
	 */
	do {
		status = cracen_prng_value_from_pool((uint32_t *)bld_factor);
		if (status != SX_OK) {
			return status;
		}
	} while (bld_factor == 0);

	return cracen_prng_value_from_pool((uint32_t *)bld_factor + 1);
}
