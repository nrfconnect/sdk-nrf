/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_le_pair_resp LE Pairing Responder model
 * @{
 * @brief API for the LE Pairing Responder model.
 */

#ifndef BT_MESH_LE_PAIR_RESP_H__
#define BT_MESH_LE_PAIR_RESP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/bluetooth/mesh/access.h>

#define BT_MESH_VENDOR_COMPANY_ID_LE_PAIR_RESP 0x0059
#define BT_MESH_MODEL_ID_LE_PAIR_RESP   0x000E

/** @def BT_MESH_MODEL_LE_PAIR_RESP
 *
 * @brief LE Pairing Responder model entry.
 */
#define BT_MESH_MODEL_LE_PAIR_RESP                                                                 \
	BT_MESH_MODEL_VND_CB(BT_MESH_VENDOR_COMPANY_ID_LE_PAIR_RESP, BT_MESH_MODEL_ID_LE_PAIR_RESP,\
			     _bt_mesh_le_pair_resp_op, NULL, NULL,                                 \
			     &_bt_mesh_le_pair_resp_cb)

/* @brief Invalidate previously used passkey.
 *
 * A user must call this function when a pairing request completes regardless of the status.
 */
void bt_mesh_le_pair_resp_passkey_invalidate(void);

/* @brief Set own passkey instead of using randomly generating passkeys.
 *
 * By default, passkeys will be randomly generated on every new request. This function allows to use
 * pre-defined passkey instead.
 *
 * @params passkey Passkey to use for the pairing, or @ref BT_PASSKEY_INVALID to use randomly
 *		   generated passkey again.
 */
void bt_mesh_le_pair_resp_passkey_set(uint32_t passkey);

/** @cond INTERNAL_HIDDEN */

extern const struct bt_mesh_model_op _bt_mesh_le_pair_resp_op[];
extern const struct bt_mesh_model_cb _bt_mesh_le_pair_resp_cb;

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LE_PAIR_RESP_H__ */

/** @} */
