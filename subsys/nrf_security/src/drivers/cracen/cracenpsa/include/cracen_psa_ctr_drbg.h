/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_CTR_DRBG_H
#define CRACEN_PSA_CTR_DRBG_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include "cracen_psa_primitives.h"

/** @brief Initialize a random number generator context.
 *
 * @param[in,out] context PRNG context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_init_random(cracen_prng_context_t *context);

/** @brief Get random bytes from a PRNG context.
 *
 * @param[in,out] context    PRNG context.
 * @param[out] output        Buffer to store the random bytes.
 * @param[in] output_size    Number of random bytes to generate.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_get_random(cracen_prng_context_t *context, uint8_t *output, size_t output_size);

/** @brief Free a random number generator context.
 *
 * @param[in,out] context PRNG context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_free_random(cracen_prng_context_t *context);

/** @brief Get random bytes from the True Random Number Generator.
 *
 * @param[out] output      Buffer to store the random bytes.
 * @param[in] output_size  Number of random bytes to generate.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_get_trng(uint8_t *output, size_t output_size);

#endif /* CRACEN_PSA_CTR_DRBG_H */
