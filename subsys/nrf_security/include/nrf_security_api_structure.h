/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @defgroup nrf_security_api_structures Nordic-specific PSA API structures
 * @brief Platform Security Architecture (PSA) API structures for nRF Security
 *
 * The nRF Security subsystem provides integration between Mbed TLS and
 * hardware-accelerated cryptographic libraries through PSA drivers. These structures
 * define the implementation-specific context types used in the PSA Crypto API.
 *
 * The `crypto_driver_contexts_*` files are Nordic-specific implementations
 * for the corresponding `crypto_driver_contexts_*` files in the Oberon API. This API is defined
 * in sdk-nrf at https://github.com/nrfconnect/sdk-nrf/tree/main/subsys/nrf_security/include/psa and
 * should be consulted together with the three Nordic-specific files.
 *
 * Nordic-specific implementations are designed to extend available cryptographic features
 * with hardware acceleration or alternative implementations. The supported drivers include:
 *   - Arm CryptoCell cc3xx (hardware acceleration for nRF52840, nRF91 Series, and nRF5340)
 *   - nrf_oberon (optimized cryptographic algorithms)
 *   - CRACEN (hardware acceleration for nRF54L and nRF71 Series)
 *
 * For detailed documentation on these drivers, see the nRF Connect SDK documentation:
 * https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/libraries/security/nrf_security/index.html
 *
 * The Nordic-specific implementation also uses files located at:
 * - https://github.com/nrfconnect/sdk-nrf/blob/main/subsys/nrf_security/src/drivers/cracen/cracenpsa/include/cracen_psa_primitives.h
 *
 * @{
 */

/**
 * @defgroup psa_crypto_drivers PSA Crypto Drivers
 * @brief PSA Crypto driver context structures
 *
 * @{
 */

/**
 * @defgroup psa_crypto_driver_primitives Primitive Crypto Drivers
 * @brief Driver context structures for primitive cryptographic operations
 *
 * These structures define the contexts for primitive cryptographic operations,
 * which do not rely on other contexts. They include contexts for hash operations
 * and cipher operations implemented by various drivers such as cc3xx, Oberon,
 * and CRACEN.
 *
 * @file crypto_driver_contexts_primitives.h
 */

/**
 * @defgroup psa_crypto_driver_composites Composite Crypto Drivers
 * @brief Driver context structures for composite cryptographic operations
 *
 * These structures define the contexts for composite operations,
 * which make use of other primitive operations. They include contexts for MAC
 * and AEAD operations implemented by various drivers such as cc3xx, Oberon,
 * and CRACEN.
 *
 * @file crypto_driver_contexts_composites.h
 */

/**
 * @defgroup psa_crypto_driver_key_derivation Key Derivation Crypto Drivers
 * @brief Driver context structures for key derivation operations
 *
 * These structures define the contexts for key derivation, PAKE, and random
 * generation operations implemented by various drivers such as Oberon and CRACEN.
 *
 * @file crypto_driver_contexts_key_derivation.h
 */

/** @} */

/** @} */
