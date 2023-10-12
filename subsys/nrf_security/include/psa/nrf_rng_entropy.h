/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __ZEPHYR_ENTROPY_H__
#define __ZEPHYR_ENTROPY_H__

#include <stdint.h>
#include <stddef.h>

#include "psa/crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

psa_status_t nrf_rng_get_entropy(uint32_t flags, size_t *estimate_bits, uint8_t *output,
				size_t output_size);

#ifdef __cplusplus
}
#endif

#endif /* __ZEPHYR_ENTROPY_H__ */
