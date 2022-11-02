/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * The RST file for this library can be found in
 * doc/nrf/libraries/bluetooth_services/mesh/dk_prov.rst.
 * Rendered documentation is available at
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/bluetooth_services/mesh/dk_prov.html.
 */

/**
 * @file
 * @brief Bluetooth mesh provisioning handler for Nordic DKs.
 * @defgroup bt_mesh_dk_prov Bluetooth mesh provisioning handler for Nordic DKs
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
