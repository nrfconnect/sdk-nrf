/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file image_validation.h
 *
 * Step 3: Validate digests of all sub-images in the initial image.
 */

#ifndef IMAGE_VALIDATION_H_
#define IMAGE_VALIDATION_H_

/**
 * @brief Step 3: Validate all sub-image digests.
 *
 * Checks SHA-256 digests of:
 *   - BL1
 *   - BL2, slot A
 *   - BL2, slot B
 *   - Manufacturing application
 *   - Application candidate
 *
 * Suspends execution if any verification fails.
 *
 * @note BL1/BL2 validation is currently stubbed
 *       pending partition address integration.
 */
void step3_image_validate_all(void);

#endif /* IMAGE_VALIDATION_H_ */
