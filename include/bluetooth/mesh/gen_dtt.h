/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
