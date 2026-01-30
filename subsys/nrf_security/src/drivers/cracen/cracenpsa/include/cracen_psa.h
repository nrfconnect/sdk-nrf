/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_driver_api
 * @{
 * @brief PSA Crypto driver API for CRACEN hardware accelerator.
 *
 * @note This is the public driver API. Applications must use the PSA Crypto API
 *       (psa_* functions) instead of calling these driver functions directly.
 *       This header documents the driver implementation that is called by the
 *       PSA Crypto layer. See Arm's PSA Crypto API for the public interface.
 *
 * This module provides the PSA Crypto driver API implementation for the
 * CRACEN hardware accelerator. It implements cryptographic operations
 * including signing, verification, hashing, encryption, decryption, key
 * management, and more.
 */

#ifndef CRACEN_PSA_H
#define CRACEN_PSA_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"
#include "cracen_psa_kmu.h"
#include "cracen_psa_key_ids.h"
#include "cracen_psa_ctr_drbg.h"
#include "sxsymcrypt/keyref.h"

#include "cracen_psa_sign_verify.h"
#include "cracen_psa_hash.h"
#include "cracen_psa_cipher.h"
#include "cracen_psa_asymmetric.h"
#include "cracen_psa_aead.h"
#include "cracen_psa_mac.h"
#include "cracen_psa_key_derivation.h"
#include "cracen_psa_key_management.h"
#include "cracen_psa_pake.h"
#include "cracen_psa_jpake.h"
#include "cracen_psa_spake2p.h"
#include "cracen_psa_srp.h"
#include "cracen_psa_wpa3_sae.h"
#include "cracen_psa_key_wrap.h"

/** @brief Get the size of an opaque key.
 *
 * @param[in] attributes Key attributes.
 * @param[out] key_size  Size of the key representation in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 */
psa_status_t cracen_get_opaque_size(const psa_key_attributes_t *attributes, size_t *key_size);

/** @} */

#endif /* CRACEN_PSA_H */
