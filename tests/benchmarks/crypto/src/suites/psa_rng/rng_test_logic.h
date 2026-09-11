/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RNG_TEST_LOGIC_H__
#define RNG_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

extern const struct op rng_operations[];

#define RNG_OPERATION_COUNT 1

#define RNG_SUITE_ENTRY \
	{ \
		.alg = "rng", \
		.keydesc = NULL, \
		.singlepart = {rng_operations, RNG_OPERATION_COUNT}, \
		.check = rng_check, \
	}

int rng_check(void);

#endif /* RNG_TEST_LOGIC_H__ */

