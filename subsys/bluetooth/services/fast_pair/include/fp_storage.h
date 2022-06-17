/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_H_
#define _FP_STORAGE_H_

/**
 * @defgroup fp_storage Fast Pair storage module
 * @brief Internal API for Fast Pair storage
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "fp_crypto.h"

/**
 * @typedef fp_storage_account_key_check_cb
 * @brief Callback used to check if a given Account Key satisfies user-defined
 *        conditions.
 *
 * @param[in] account_key 128-bit (16-byte) Account Key to be checked.
 * @param[in] context     Pointer used to pass operation context.
 *
 * @return True if Account Key satisfies user-defined conditions.
 *         False otherwise.
 */
typedef bool (*fp_storage_account_key_check_cb)(const uint8_t *account_key, void *context);

/** Save Account Key.
 *
 * @param[in] account_key 128-bit (16-byte) Account Key to be saved.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_account_key_save(const uint8_t *account_key);

/** Get number of stored Account Keys.
 *
 * @return Number of stored Account Keys, if the operation was successful.
 *	   Otherwise, a (negative) error code is returned.
 */
int fp_storage_account_key_count(void);

/** Get stored Account Key List.
 *
 * @param[out] buf Pointer to 2-dimensional buffer. Buffer shape has to be
 *		   number of stored Account Keys (or greater) x 16 bytes (length of Account Key in
 *		   bytes).
 * @param[in,out] key_count Pointer to number of Account Keys. Negative error code is returned when
 *			    this value is less than number of stored Account Keys. If the operation
 *			    was successful, number of stored Account Keys is written to this
 *			    pointer.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_account_keys_get(uint8_t buf[][FP_CRYPTO_ACCOUNT_KEY_LEN], size_t *key_count);

/** Iterate over stored Account Keys to find a key that matches user-defined conditions.
 *  If such a key is found, the iteration process stops and this function returns.
 *
 * @param[out] account_key Found Account Key. It is possible to pass NULL pointer if the found
 *                         key value is irrelevant.
 * @param[in] account_key_check_cb Callback for determining if a given Account Key satisfies
 *                                 user-defined conditions.
 * @param[in] context Pointer used to pass operation context.
 *
 * @return 0 If the Account Key was found. Otherwise, a (negative) error code is returned.
 */
int fp_storage_account_key_find(uint8_t account_key[FP_CRYPTO_ACCOUNT_KEY_LEN],
				fp_storage_account_key_check_cb account_key_check_cb,
				void *context);

/** Clear storage data loaded to RAM.
 *
 * The function is used only by fp_storage unit test.
 */
void fp_storage_ram_clear(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_H_ */
