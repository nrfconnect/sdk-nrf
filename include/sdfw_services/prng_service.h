/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_PRNG_SERVICE_H__
#define SSF_PRNG_SERVICE_H__

#include <stddef.h>
#include <stdint.h>

/**
 * @brief       Function to generate random numbers.
 *
 * @param[in]   buffer  Pointer to a buffer for the generated data.
 * @param[in]   length  Length of the buffer for generated data.
 *
 * @return 0 on success, otherwise a non-zero return code.
 */
int ssf_prng_get_random(uint8_t *buffer, size_t length);

#endif /* SSF_PRNG_SERVICE_H__ */
