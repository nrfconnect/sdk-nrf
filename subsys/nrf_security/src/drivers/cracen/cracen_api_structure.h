/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @defgroup cracen_driver_apis CRACEN Driver APIs
 * @brief Complete API structure for the CRACEN driver
 *
 * The CRACEN driver APIs are organized into the following sub-modules:
 *
 * - **PSA driver APIs** — the public and internal PSA Crypto driver interface.
 *   See cracenpsa/include/cracen_psa_api_structure.h for the full hierarchy.
 *
 * - **Software implementation APIs** — pure-software fallback implementations
 *   for operations not supported by the CRACEN hardware engine.
 *   See cracen_sw/include/cracen_sw_api_structure.h for the full hierarchy.
 *
 * - **Common APIs** — shared utilities, hardware abstraction, KMU integration,
 *   status codes, and memory helpers used across all sub-modules.
 *   See common/include/cracen_common_api_structure.h for the full hierarchy.
 *
 * @{
 */

/**
 * @defgroup cracen_psa_driver_apis CRACEN PSA Driver APIs
 * @brief Public and internal PSA Crypto driver interface for CRACEN
 */

/**
 * @defgroup cracen_sw_apis CRACEN Software Implementation APIs
 * @brief Pure-software fallback implementations for CRACEN hardware limitations
 */

/**
 * @defgroup cracen_common_apis CRACEN Common APIs
 * @brief Shared utilities, hardware abstraction, and support libraries
 */

/** @} */ /* cracen_driver_apis */
