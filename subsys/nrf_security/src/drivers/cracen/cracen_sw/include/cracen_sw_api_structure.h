/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @defgroup cracen_sw_apis CRACEN Software Implementation APIs
 * @brief These APIs are pure-software implementations used as a
 * workaround for hardware limitations, particularly for multipart AEAD and
 * cipher operations not supported by the CRACEN hardware engine.
 *
 * @{
 */

/**
 * @defgroup cracen_sw_aead CRACEN Software AEAD Dispatcher
 * @brief Dispatcher layer for software AEAD implementations
 *
 * Dispatches multipart AEAD operations to the appropriate per-algorithm
 * software implementation based on the active algorithm.
 */

/**
 * @defgroup cracen_sw_aes_cbc CRACEN Software AES-CBC
 * @brief Software implementation of AES-CBC
 *
 * Supports @c PSA_ALG_CBC_NO_PADDING and @c PSA_ALG_CBC_PKCS7.
 */

/**
 * @defgroup cracen_sw_aes_ccm CRACEN Software AES-CCM
 * @brief Software implementation of AES-CCM (Counter with CBC-MAC)
 */

/**
 * @defgroup cracen_sw_aes_ctr CRACEN Software AES-CTR
 * @brief Software implementation of AES-CTR mode
 */

/**
 * @defgroup cracen_sw_aes_gcm CRACEN Software AES-GCM
 * @brief Software implementation of AES-GCM (Galois/Counter Mode)
 */

/**
 * @defgroup cracen_sw_chacha_poly CRACEN Software ChaCha20-Poly1305
 * @brief Software implementation of ChaCha20-Poly1305 AEAD
 */

/**
 * @defgroup cracen_sw_mac_cmac CRACEN Software CMAC
 * @brief Software CMAC implementation using hardware AES-ECB as a primitive
 */

/**
 * @defgroup cracen_sw_common CRACEN Software Common Primitives
 * @brief Shared primitives for CRACEN software implementation workarounds.
 *
 * Provides AES-ECB block encrypt/decrypt, big-endian counter increment,
 * and big-endian value encoding used across the software implementations.
 */

/** @} */ /* cracen_sw_apis */
