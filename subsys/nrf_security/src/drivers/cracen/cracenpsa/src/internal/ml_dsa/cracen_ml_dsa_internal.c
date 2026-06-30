/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"

uint32_t cracen_ml_dsa_bit_length(uint32_t x)
{
	if (x == 0) {
		return 0;
	}

	return 32u - __builtin_clz(x);
}
