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
 * @return The provisioning properties to pass to @em bt_mesh_init().
 */
const struct bt_mesh_prov *bt_mesh_dk_prov_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_DK_PROV_H__ */

/** @} */
