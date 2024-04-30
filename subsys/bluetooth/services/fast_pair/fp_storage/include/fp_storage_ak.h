/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_AK_H_
#define _FP_STORAGE_AK_H_

#include <sys/types.h>
#include <stdbool.h>
#include <zephyr/bluetooth/conn.h>

#include "fp_common.h"

/**
 * @defgroup fp_storage_ak Fast Pair storage of Account Keys module
 * @brief Internal API for Fast Pair storage of Account Keys
 *
 * The module must be initialized before using API functions.
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
 * @param[in] conn Connection object associated with the Account Key. It is used to associate
 *		   Bluetooth bond with the Account Key. It can be set to NULL if this information is
 *		   irrelevant.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_ak_save(const struct fp_account_key *account_key, const struct bt_conn *conn);

/** Get number of stored Account Keys.
 *
 * @return Number of stored Account Keys, if the operation was successful.
 *	   Otherwise, a (negative) error code is returned.
 */
int fp_storage_ak_count(void);

/** Verify that any Account Key is stored.
 *
 * @return True when at least one Account Key is stored, false otherwise.
 */
bool fp_storage_ak_has_account_key(void);

/** Get stored Account Key List.
 *  This function should not be used if only one key is about to be used, because it doesn't trigger
 *  key usage order update. Use @ref fp_storage_ak_find instead.
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
 *  Found key is marked as recently used by storage module.
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

/** Check if a given Account Key belongs to the Owner.
 *
 *  The current implementation assumes that the Owner Account Key is the first Account Key
 *  that was introduced to the device.
 *
 * @param[in] account_key Account Key to be checked.
 *
 * @return 1 If the Account Key belongs to the owner.
 *         0 If the Account Key does not belong to the owner.
 *         Otherwise, a negative value is returned which indicates an error.
 */
int fp_storage_ak_is_owner(const struct fp_account_key *account_key);

/** Save potential bond.
 *  The bond will be deleted if the pairing fails or the Fast Pair Procedure is incomplete.
 *
 * @param[in] conn Potential bond's connection object.
 * @param[in] account_key Account Key that the bond is associated with. It can be set to NULL in
 *			  case of the initial pairing Procedure, when the Account Key is not known.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_ak_bond_save(const struct bt_conn *conn, const struct fp_account_key *account_key);

/** Indicate that the bonding process has been completed.
 *  It confirms the potential bond that was saved earlier with the @ref fp_storage_ak_bond_save.
 *
 * @param[in] conn Bond's connection object.
 */
void fp_storage_ak_bonding_complete(const struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_AK_H_ */
