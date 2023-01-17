/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_scene Scene models
 * @{
 * @brief Common API for the Scene models.
 */

#ifndef BT_MESH_SCENE_H__
#define BT_MESH_SCENE_H__

#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** No scene is active */
#define BT_MESH_SCENE_NONE 0x0000

/** Scene status codes */
enum bt_mesh_scene_status {
	/** Operation successful */
	BT_MESH_SCENE_SUCCESS,
	/** Not enough room in the Server's scene register. */
	BT_MESH_SCENE_REGISTER_FULL,
	/** Couldn't find the requested scene */
	BT_MESH_SCENE_NOT_FOUND,
};

/** Scene state */
struct bt_mesh_scene_state {
	/** Status of the previous operation. */
	enum bt_mesh_scene_status status;
	/** Current scene, or @ref BT_MESH_SCENE_NONE if no scene is active. */
	uint16_t current;
	/** Target scene, or @ref BT_MESH_SCENE_NONE if no transition is in
	 *  progress.
	 */
	uint16_t target;
	/** Remaining time of the scene transition in milliseconds. */
	uint32_t remaining_time;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_SCENE_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x41)
#define BT_MESH_SCENE_OP_RECALL BT_MESH_MODEL_OP_2(0x82, 0x42)
#define BT_MESH_SCENE_OP_RECALL_UNACK BT_MESH_MODEL_OP_2(0x82, 0x43)
#define BT_MESH_SCENE_OP_STATUS BT_MESH_MODEL_OP_1(0x5E)
#define BT_MESH_SCENE_OP_REGISTER_GET BT_MESH_MODEL_OP_2(0x82, 0x44)
#define BT_MESH_SCENE_OP_REGISTER_STATUS BT_MESH_MODEL_OP_2(0x82, 0x45)
#define BT_MESH_SCENE_OP_STORE BT_MESH_MODEL_OP_2(0x82, 0x46)
#define BT_MESH_SCENE_OP_STORE_UNACK BT_MESH_MODEL_OP_2(0x82, 0x47)
#define BT_MESH_SCENE_OP_DELETE BT_MESH_MODEL_OP_2(0x82, 0x9E)
#define BT_MESH_SCENE_OP_DELETE_UNACK BT_MESH_MODEL_OP_2(0x82, 0x9F)

#define BT_MESH_SCENE_MSG_LEN_GET 0
#define BT_MESH_SCENE_MSG_MINLEN_RECALL 3
#define BT_MESH_SCENE_MSG_MAXLEN_RECALL 5
#define BT_MESH_SCENE_MSG_MINLEN_STATUS 3
#define BT_MESH_SCENE_MSG_MAXLEN_STATUS 6
#define BT_MESH_SCENE_MSG_LEN_REGISTER_GET 0
#define BT_MESH_SCENE_MSG_MINLEN_REGISTER_STATUS 3
#define BT_MESH_SCENE_MSG_LEN_STORE 2
#define BT_MESH_SCENE_MSG_LEN_DELETE 2
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SCENE_H__ */

/** @} */
