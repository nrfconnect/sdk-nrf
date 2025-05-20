/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_ponoff Generic Power OnOff Models
 * @{
 * @brief API for the Generic Power OnOff models.
 */

#ifndef BT_MESH_GEN_PONOFF_H__
#define BT_MESH_GEN_PONOFF_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Generic Power OnOff On Power Up state values. */
enum bt_mesh_on_power_up {
	/** On power up, set state to off. */
	BT_MESH_ON_POWER_UP_OFF,
	/** On power up, set state to on. */
	BT_MESH_ON_POWER_UP_ON,
	/** On power up, Restore the previous state value. */
	BT_MESH_ON_POWER_UP_RESTORE,
	/** Invalid power up state. */
	BT_MESH_ON_POWER_UP_INVALID,
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_PONOFF_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x11)
#define BT_MESH_PONOFF_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x12)
#define BT_MESH_PONOFF_OP_SET BT_MESH_MODEL_OP_2(0x82, 0x13)
#define BT_MESH_PONOFF_OP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x14)

#define BT_MESH_PONOFF_MSG_LEN_GET 0
#define BT_MESH_PONOFF_MSG_LEN_STATUS 1
#define BT_MESH_PONOFF_MSG_LEN_SET 1
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_PONOFF_H__ */

/** @} */
