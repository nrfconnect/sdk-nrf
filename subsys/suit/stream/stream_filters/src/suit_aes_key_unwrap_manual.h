/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_AES_KEY_UNWRAP_MANUAL_H__
#define SUIT_AES_KEY_UNWRAP_MANUAL_H__

/**
 * This file is TEMPORARY.
 * It contains the implementation of the AES key unwrap algorithm,
 * based on the RFC3394 specification.
 * It should be deleted once the PSA Crypto API supports AES key unwrap.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <psa/crypto.h>

/**
 * @brief Unwrap content encryption key (CEK) using AES key wrap algorithm.
 *
 * @param kek_key_id Key ID of the key encryption key.
 * @param wrapped_cek Pointer to the wrapped (encrypted) content encryption key.
 * @param cek_bits Size of the content encryption key in bits.
 * @param cek_key_type Type of the content encryption key.
 * @param cek_key_alg Algorithm of the content encryption key.
 * @param unwrapped_cek_key_id Pointer to the key ID of the unwrapped content encryption key.
 *
 * @return PSA_SUCCESS if success otherwise error code
 */
psa_status_t suit_aes_key_unwrap_manual(psa_key_id_t kek_key_id, const uint8_t *wrapped_cek,
					size_t cek_bits, psa_key_type_t cek_key_type,
					psa_algorithm_t cek_key_alg,
					mbedtls_svc_key_id_t *unwrapped_cek_key_id);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_AES_KEY_UNWRAP_MANUAL_H__ */
