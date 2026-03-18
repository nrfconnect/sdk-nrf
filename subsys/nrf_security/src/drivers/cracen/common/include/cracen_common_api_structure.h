/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @defgroup cracen_common_apis CRACEN Common APIs
 * @brief These CRACEN common utilities are shared across the PSA driver,
 * software implementations, and hardware abstraction layers.
 *
 * @{
 */

/**
 * @defgroup cracen_hardware CRACEN Hardware
 * @brief CRACEN hardware and peripheral lifecycle management
 *
 * Provides hardware base address macros, and the peripheral lifecycle API
 * (@c cracen_init, @c cracen_acquire, @c cracen_release ).
 */

/**
 * @defgroup cracen_status_codes CRACEN SX Status Codes
 * @brief SX status code definitions for the CRACEN driver
 */

/**
 * @defgroup cracen_kmu_hal CRACEN KMU HAL
 * @brief Low-level KMU HAL library
 *
 * Provides functions to provision, push, block, and revoke KMU slots,
 * read slot metadata, and check whether a slot is empty.
 */

/**
 * @defgroup cracen_common_utils CRACEN Common Utilities
 * @brief Shared internal utilities for the PSA layer
 *
 * Provides big-endian arithmetic helpers, XOR and hashing helpers,
 * key reference loading, and random-in-range generation.
 */

/**
 * @defgroup cracen_hmac CRACEN HMAC Primitives
 * @brief Low-level HMAC primitives for the CRACEN driver
 */

/**
 * @defgroup cracen_kmu CRACEN KMU Integration
 * @brief CRACEN Key Management Unit (KMU) integration for the PSA layer
 *
 * Defines the opaque KMU key buffer and operations to prepare and clean keys,
 * retrieve built-in keys, provision and destroy KMU slots, and manage
 * protected RAM invalidation slots.
 */

/**
 * @defgroup cracen_ec_helpers CRACEN EC Helpers
 * @brief Elliptic curve coordinate and scalar decoding helpers
 *
 * Provides RFC 7748 coordinate and scalar decoding for X25519 and X448.
 */

/**
 * @defgroup cracen_interrupts CRACEN Interrupt Management
 * @brief Interrupt initialization, ISR handler, and blocking wait functions
 *
 * Provides @c cracen_interrupts_init, @c cracen_isr_handler, and blocking
 * waits for CryptoMaster and PK engine interrupts.
 */

/**
 * @defgroup cracen_mem_helpers CRACEN Memory Helpers
 * @brief Constant-time memory utilities
 *
 * Provides @c constant_memcmp, @c constant_memcmp_is_zero,
 * @c constant_select_bin, @c safe_memset, @c safe_memzero, and
 * @c memcpy_check_non_zero.
 */

/**
 * @defgroup cracen_membarriers CRACEN Memory Barriers
 * @brief Compiler and CPU memory barrier utilities
 *
 * Provides compiler memory barrier (@c cmb), write memory barrier (@c wmb),
 * and read memory barrier (@c rmb) with architecture-specific implementations.
 */

/**
 * @defgroup cracen_prng_pool CRACEN PRNG Pool
 * @brief Pre-computed secure random number pool
 *
 * @note Intended for AES counter-measure use only. Not suitable for key
 *       generation or other security-sensitive random number consumption.
 */

/**
 * @defgroup cracen_sx_error CRACEN SX Error Handling
 * @brief SX error code conversion utilities
 *
 * Provides @c sx_err2errno to convert @c SX_* codes to POSIX errno values,
 * and @c sx_err2str to convert them to human-readable string literals.
 */

/** @} */ /* cracen_common_apis */
