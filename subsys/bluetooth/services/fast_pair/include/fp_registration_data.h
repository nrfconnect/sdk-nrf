/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_REGISTRATION_DATA_H_
#define _FP_REGISTRATION_DATA_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup fp_registration_data Fast Pair registration data
 * @brief Internal API for Fast Pair registration data
 *
 * The module must be initialized with @ref bt_fast_pair_enable before using API functions.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Fast Pair Model ID length (24-bits = 3 bytes). */
#define FP_REG_DATA_MODEL_ID_LEN		3U
/** Fast Pair Anti-Spoofing private key length (256 bits = 32 bytes). */
#define FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN	32U

/** Get Fast Pair Model ID.
 *
 * @param[out] buf  Pointer to the buffer to be filled with the Model ID.
 * @param[in]  size Buffer length in bytes. The buffer must be at least FP_REG_DATA_MODEL_ID_LEN
 *                  bytes long.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_reg_data_get_model_id(uint8_t *buf, size_t size);

/** Get Fast Pair anti-spoofing private key.
 *
 * @param[out] buf  Pointer to the buffer to be filled with the anti-spoofing private key.
 * @param[in]  size Buffer length (in bytes). The buffer must be at least
 *                  FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN bytes long.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_get_anti_spoofing_priv_key(uint8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_REGISTRATION_DATA_H_ */
