/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_AK_BOND_H_
#define _FP_STORAGE_AK_BOND_H_

#include <sys/types.h>
#include <stdbool.h>
#include <zephyr/bluetooth/addr.h>

#include "fp_common.h"

/**
 * @defgroup fp_storage_ak_bond Fast Pair storage of associations between Account Keys and BLE bonds
 * @brief Internal API for Fast Pair storage of associations between Account Keys and BLE bonds
 *
 * Used only when the CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND Kconfig option is enabled.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Create and save the Account Key association with the potential Bluetooth bond that is being
 *  established from the active connection.
 *
 *  The module links the Account Key information with the Bluetooth bond and stores
 *  it in the NVM. The association and the Bluetooth bond will be deleted if the bond is not
 *  confirmed using the @ref fp_storage_ak_bond_conn_confirm API before the bonding process is
 *  finalized using the @ref fp_storage_ak_bond_conn_finalize API and also, in case of the Fast Pair
 *  initial pairing procedure, if the Account Key is not written by the conn before the
 *  the @ref fp_storage_ak_bond_conn_finalize API is called.
 *
 * @param[in] conn_ctx Connection context associated with the potential the Bluetooth bond.
 * @param[in] addr The Bluetooth address of the potential bond.
 * @param[in] account_key Account Key that the bond is associated with. It can be set to NULL in
 *			  case of the initial pairing Procedure, when the Account Key is not known
 *			  yet.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_ak_bond_conn_create(const void *conn_ctx, const bt_addr_le_t *addr,
				   const struct fp_account_key *account_key);

/** Indicate that the potential Bluetooth bond has been confirmed.
 *
 *  It confirms the potential Bluetooth bond associated with an Account Key. The association should
 *  be earlier created using the @ref fp_storage_ak_bond_conn_create API. In case of the Fast Pair
 *  initial pairing procedure, the Account Key write is required to happen after calling this
 *  function to fully confirm the association between Account Key and BLE bond.
 *
 * @param[in] conn_ctx Connection context associated with the Bluetooth bond.
 * @param[in] addr The Bluetooth address of the bond.
 */
void fp_storage_ak_bond_conn_confirm(const void *conn_ctx, const bt_addr_le_t *addr);

/** Indicate that the bonding process has been finalized.
 *
 *  It relates to the Account Key association with the Bluetooth bond that was created
 *  earlier using the @ref fp_storage_ak_bond_conn_create API. This function should be called when
 *  the BLE conn disconnects to let the storage subsystem clean up incomplete bonds.
 *
 *  If the @ref fp_storage_ak_bond_conn_create API has not been called for this conn earlier,
 *  the call is ignored.
 *
 * @param[in] conn_ctx Connection context associated with the Bluetooth bond.
 */
void fp_storage_ak_bond_conn_finalize(const void *conn_ctx);

/** Update the address of the Bluetooth bond associated with the Account Key.
 *
 *  The update has to be done when the address is resolved.
 *  If the @ref fp_storage_ak_bond_conn_create API has not been called for this conn earlier,
 *  the call is ignored.
 *
 * @param[in] conn_ctx Connection context associated with the Bluetooth bond.
 * @param[in] addr The Bluetooth address of the bond.
 */
void fp_storage_ak_bond_conn_addr_update(const void *conn_ctx, const bt_addr_le_t *new_addr);

/** Delete the Account Key association with the Bluetooth bond.
 *
 *  The function needs to be called when a Bluetooth bond is removed to let the storage module
 *  clean up the association. If the @ref fp_storage_ak_bond_conn_create API has not been called for
 *  this address earlier, the call is ignored.
 *
 * @param[in] addr The Bluetooth address of the bond.
 */
void fp_storage_ak_bond_delete(const bt_addr_le_t *addr);

/** Bluetooth bond request callback structure. */
struct fp_storage_ak_bond_bt_request_cb {
	/** @brief Remove the Bluetooth bond.
	 *
	 *  This callback is called to request the removal of the Bluetooth bond from the Bluetooth
	 *  subsystem.
	 *
	 * @param[in] addr The Bluetooth address of the bond to be removed.
	 *
	 * @return 0 If the operation was successful. Otherwise, a (negative) error code is
	 *	   returned.
	 */
	int (*bond_remove)(const bt_addr_le_t *addr);

	/** @brief Check if the Bluetooth address is bonded.
	 *
	 *  This callback is called to check if there is a Bluetooth bond saved in the Bluetooth
	 *  subsystem with the given Bluetooth address.
	 *
	 * @param[in] addr The Bluetooth address.
	 *
	 * @return true when the Bluetooth address is bonded, false otherwise.
	 */
	bool (*is_addr_bonded)(const bt_addr_le_t *addr);
};

/** Register Bluetooth bond request callbacks.
 *
 *  This function registers an instance of request callbacks. The registered instance needs to
 *  persist in the memory after this function exits, as it is used directly without the copy
 *  operation. It is possible to register only one instance of callbacks with this API.
 *
 *  This function must be called before initialiazing the Fast Pair storage with the
 *  @ref fp_storage_init API and also before performing the Fast Pair factory reset to ensure that
 *  the callbacks are available during those operations.
 *
 *  @param cb Callback struct.
 */
void fp_storage_ak_bond_bt_request_cb_register(const struct fp_storage_ak_bond_bt_request_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_AK_BOND_H_ */
