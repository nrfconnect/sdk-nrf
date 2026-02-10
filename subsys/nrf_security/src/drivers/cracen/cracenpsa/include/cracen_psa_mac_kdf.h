/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_mac_kdf
 * @{
 * @brief MAC and KDF definitions for CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_PSA_MAC_KDF_H
#define CRACEN_PSA_MAC_KDF_H

/** @brief SP800-108 Counter MAC initial capacity.
 *
 * The defines will eventually become part of the PSA core once the specification is
 * finalized and the core is updated.
 *
 * Value: 2^29 - 1
 */
#define PSA_ALG_SP800_108_COUNTER_MAC_INIT_CAPACITY (0x1fffffff)

/** @} */

#endif /* CRACEN_PSA_MAC_KDF_H */
