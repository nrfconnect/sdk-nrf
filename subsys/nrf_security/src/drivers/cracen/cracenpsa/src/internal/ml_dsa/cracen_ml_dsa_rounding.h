/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-DSA rounding and hints (FIPS 204).
 *
 */

#ifndef CRACEN_ML_DSA_RONDING_H
#define CRACEN_ML_DSA_RONDING_H

#include <psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Adjust the coefficient according to the provided hint
 *	  (FIPS 204, Algorithm 40 - UseHint).
 *
 * @param[in] hint_bit Hint bit (0 or 1) for the coefficient.
 * @param[in] r        Coefficient to which the hint is applied.
 * @param[in] gamma2   Low-order rounding range of the active parameter set.
 *
 * @return The high-order bits w1 of the coefficient after applying the hint.
 */
int32_t cracen_ml_dsa_use_hint(int32_t hint_bit, int32_t r, uint32_t gamma2);

#endif /* CRACEN_ML_DSA_RONDING_H */
