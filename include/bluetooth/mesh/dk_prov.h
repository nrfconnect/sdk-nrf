/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Bluetooth Mesh provisioning handler for Nordic DKs.
 * @defgroup bt_mesh_dk_prov Bluetooth Mesh provisioning handler for Nordic DKs
 * @{
 */

#ifndef BT_MESH_DK_PROV_H__
#define BT_MESH_DK_PROV_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Initialize the provisioning handler.
 *
 * @return The provisioning properties to pass to @em bt_mesh_init(), or NULL
 *         if the provisioning handler has already been initialized.
 */
const struct bt_mesh_prov *bt_mesh_dk_prov_init(void);

/** @brief Initialize the provisioning handler with a static OOB value.
 *
 * @param oob_val Static OOB value, or NULL to disable static OOB. The pointer
 *                must remain valid for as long as the device may be
 *                provisioned.
 * @param oob_len Length of the static OOB value, in bytes. Must be zero if
 *                @p oob_val is NULL, otherwise non-zero.
 *
 * @return The provisioning properties to pass to @em bt_mesh_init(), or NULL
 *         on invalid parameters or if the provisioning handler has already
 *         been initialized.
 */
const struct bt_mesh_prov *bt_mesh_dk_prov_init_with_static_oob(const uint8_t *oob_val,
								uint8_t oob_len);

/** @brief Register a handler for node reset.
 *
 * @param handler The handler to register.
 */
void bt_mesh_dk_prov_node_reset_cb_set(void (*handler)(void));

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_DK_PROV_H__ */

/** @} */
