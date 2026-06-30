/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa.h"
#include "cracen_ml_dsa_internal.h"

uint32_t cracen_ml_dsa_bit_length(uint32_t x)
{
	uint32_t n = 0;

	while (x > 0) {
		n++;
		x >>= 1;
	}

	return n;
}
