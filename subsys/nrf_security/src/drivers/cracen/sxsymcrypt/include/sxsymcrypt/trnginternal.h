/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef TRNG_INTERNAL_H
#define TRNG_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** Internal state of the TRNG hardware
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sx_trng {
	bool initialized;
	int conditioning_key_set;
};

#ifdef __cplusplus
}
#endif

#endif
