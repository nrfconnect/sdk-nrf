/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_TRNG_H
#define CRACEN_TRNG_H

#include "psa/crypto.h"

/**
 * @brief Get entropy from CRACEN TRNG
 *
 * PSA crypto entropy driver interface implementation for CRACEN TRNG.
 *
 * @param flags         Entropy generation flags (ignored)
 * @param estimate_bits Pointer to store estimated entropy bits
 * @param output        Buffer to store entropy data
 * @param output_size   Size of output buffer
 *
 * @return psa_status_t statuscode
 */
psa_status_t cracen_trng_get_entropy(uint32_t flags, size_t *estimate_bits,
				      uint8_t *output, size_t output_size);

#endif /* CRACEN_TRNG_H */
