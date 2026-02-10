/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_key_ids
 * @{
 * @brief Key ID definitions and constants for the CRACEN PSA driver.
 *
 * @note These key IDs are shared between the public driver API and internal
 *       implementation.
 *
 * This module provides key ID definitions and constants, shared between the
 * public driver API and internal implementation.
 */

#ifndef CRACEN_PSA_KEY_IDS_H
#define CRACEN_PSA_KEY_IDS_H

/** @brief Built-in identity key ID. */
#define CRACEN_BUILTIN_IDENTITY_KEY_ID ((uint32_t)0x7fffc001)

/** @brief Built-in MKEK (Master Key Encryption Key) ID. */
#define CRACEN_BUILTIN_MKEK_ID	       ((uint32_t)0x7fffc002)

/** @brief Built-in MEXT (Master External Key) ID. */
#define CRACEN_BUILTIN_MEXT_ID	       ((uint32_t)0x7fffc003)

/** @brief Protected RAM AES key 0 ID. */
#define CRACEN_PROTECTED_RAM_AES_KEY0_ID ((uint32_t)0x7fffc004)

/** @brief Key location identifier for CRACEN. */
#define PSA_KEY_LOCATION_CRACEN ((psa_key_location_t)(0x800000 | ('N' << 8)))

/** @brief Key persistence state: revokable.
 *
 * Defines a persistence state where deleted keys are permanently revoked.
 * In this state, once a key is deleted, its corresponding slot cannot be
 * provisioned again.
 */
#define CRACEN_KEY_PERSISTENCE_REVOKABLE 0x02

/** @brief Key persistence state: read-only.
 *
 * Defines a persistence state where the key cannot be erased.
 * In this state, the key will only be erased if ERASEALL is available and run.
 */
#define CRACEN_KEY_PERSISTENCE_READ_ONLY 0x03

/** @} */

#endif
