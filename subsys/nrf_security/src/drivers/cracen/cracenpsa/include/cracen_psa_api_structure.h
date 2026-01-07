/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @defgroup cracen_psa_driver_apis CRACEN PSA Driver APIs
 * @brief Complete API structure for CRACEN PSA driver
 *
 * This file defines the hierarchical organization of all CRACEN PSA driver APIs.
 * The APIs are organized into logical groups based on their purpose and visibility.
 *
 * @{
 */

/**
 * @defgroup cracen_psa_driver_api CRACEN PSA Driver API
 * @brief Public driver API for CRACEN PSA driver
 */

/**
 * @defgroup cracen_psa_driver_internal CRACEN PSA Driver Internal API
 * @brief Internal implementation APIs for CRACEN PSA driver
 *
 * @note These APIs are for internal use only. Applications must use the
 *       PSA Crypto API (psa_* functions) instead of calling these functions
 *       directly.
 * @{
 */

/**
 * @defgroup cracen_psa_ecdsa ECDSA Operations
 * @brief ECDSA signature and verification operations (internal use only)
 */

/**
 * @defgroup cracen_psa_rsa_keygen RSA Key Generation
 * @brief RSA key generation operations (internal use only)
 */

/**
 * @defgroup cracen_psa_rsa_encryption RSA Encryption
 * @brief RSA encryption and decryption operations (internal use only)
 */

/**
 * @defgroup cracen_psa_rsa_signatures RSA Signatures
 * @brief RSA signature operations (internal use only)
 *
 * This group includes both PKCS#1 v1.5 and PSS signature schemes.
 */

/**
 * @defgroup cracen_psa_eddsa EDDSA Operations
 * @brief EDDSA signature and verification operations (internal use only)
 */

/**
 * @defgroup cracen_psa_mac_kdf MAC and KDF Operations
 * @brief MAC and Key Derivation Function operations (internal use only)
 */

/**
 * @defgroup cracen_psa_montgomery Montgomery Operations
 * @brief Montgomery arithmetic operations (internal use only)
 */

/**
 * @defgroup cracen_psa_ikg Internal Key Generation
 * @brief Internal Key Generation (IKG) operations (internal use only)
 */

/**
 * @defgroup cracen_psa_wpa3_sae WPA3 SAE Operations
 * @brief WPA3 Simultaneous Authentication of Equals (SAE) operations (internal use only)
 */

/** @} */ /* cracen_psa_driver_internal */

/**
 * @defgroup cracen_psa_primitives CRACEN PSA Primitives
 */

/**
 * @defgroup cracen_psa_key_ids CRACEN Key IDs
 */

/**
 * @defgroup cracen_psa_builtin_key_policy CRACEN Built-in Key Policy
 */

/**
 * @defgroup cracen_psa_kmu CRACEN Key Management Unit (KMU)
 * @brief Key Management Unit (KMU) support for CRACEN PSA driver
 */

/** @} */ /* cracen_psa_driver_apis */
