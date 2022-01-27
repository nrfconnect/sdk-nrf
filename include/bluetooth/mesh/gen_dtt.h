/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * The RST file for this library can be found in
 * doc/nrf/libraries/bluetooth_services/mesh/gen_dtt.rst.
 * Rendered documentation is available at
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/bluetooth_services/mesh/gen_dtt.html.
 */

/**
 * @file
 * @defgroup bt_mesh_dtt Generic Default Transition Time models
 * @{
 * @brief API for the Generic Default Transition Time models.
 */

#ifndef BT_MESH_GEN_DTT_H__
#define BT_MESH_GEN_DTT_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_DTT_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x0D)
#define BT_MESH_DTT_OP_SET BT_MESH_MODEL_OP_2(0x82, 0x0E)
#define BT_MESH_DTT_OP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x0F)
#define BT_MESH_DTT_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x10)

#define BT_MESH_DTT_MSG_LEN_GET 0
#define BT_MESH_DTT_MSG_LEN_SET 1
#define BT_MESH_DTT_MSG_LEN_STATUS 1
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_DTT_H__ */

/** @} */
