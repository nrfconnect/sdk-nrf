/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_AK_H_
#define _FP_STORAGE_AK_H_

#include <sys/types.h>
#include <stdbool.h>

#include "fp_common.h"

/**
 * @defgroup fp_storage_ak Fast Pair storage of Account Keys module
 * @brief Internal API for Fast Pair storage of Account Keys
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef fp_storage_ak_check_cb
 * @brief Callback used to check if a given Account Key satisfies user-defined
 *        conditions.
 *
 * @param[in] account_key Account Key to be checked.
 * @param[in] context     Pointer used to pass operation context.
 *
 * @return True if Account Key satisfies user-defined conditions.
 *         False otherwise.
 */
typedef bool (*fp_storage_ak_check_cb)(const struct fp_account_key *account_key, void *context);

/** Save Account Key.
 *
 * @param[in] account_key Account Key to be saved.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_ak_save(const struct fp_account_key *account_key);

/** Get number of stored Account Keys.
 *
 * @return Number of stored Account Keys, if the operation was successful.
 *	   Otherwise, a (negative) error code is returned.
 */
int fp_storage_ak_count(void);

/** Get stored Account Key List.
 *
 * @param[out] buf Pointer to buffer used to store array of Account Keys. Array length has to be
 *		   greater than or equal to number of stored Account Keys.
 * @param[in,out] key_count Pointer to number of Account Keys. Negative error code is returned when
 *			    this value is less than number of stored Account Keys. If the operation
 *			    was successful, number of stored Account Keys is written to this
 *			    pointer.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_ak_get(struct fp_account_key *buf, size_t *key_count);

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
int fp_storage_ak_find(struct fp_account_key *account_key,
		       fp_storage_ak_check_cb account_key_check_cb, void *context);

/** Clear storage data loaded to RAM.
 *
 * The function is used only by fp_storage unit test.
 */
void fp_storage_ak_ram_clear(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_AK_H_ */
