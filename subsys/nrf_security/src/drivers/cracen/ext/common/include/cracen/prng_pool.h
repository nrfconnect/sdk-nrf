/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PRNG_POOL_H
#define PRNG_POOL_H

#include <stdint.h>

/*!
 * \brief Get a secure random number from a pre-computed pool.
 *        Refill the pool before returning if necessary.
 *
 * The secure random numbers are generated using the PSA PRNG driver.
 *
 * Important note: Exposing sensitive data in the memory for long periods of time is
 * not a good security policy. Thus we don't advise using this function for key generation
 * or other security critical operations. We intend to use this function only to provide
 * secure random numbers to the AES counter-measures mechanism. Even though the AES
 * counter-measures are important to reduce the side-channel attack surface we classify them
 * as less security critical than cryptographic keys. Thus we allow this pre-computed pool
 * to be used to improve power consumption and execution time.
 *
 * @param[out] prng_value         A secure random number from the PRNG pool
 *
 * \retval SX_OK on success
 * \retval SX_ERR_UNKNOWN_ERROR or error
 */
int cracen_prng_value_from_pool(uint32_t *prng_value);

#endif /* PRNG_POOL_H */
