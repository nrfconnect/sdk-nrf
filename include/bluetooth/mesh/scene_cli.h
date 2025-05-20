/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_scene_cli Scene Client model
 * @{
 * @brief API for the Scene Client model.
 */

#ifndef BT_MESH_SCENE_CLI_H__
#define BT_MESH_SCENE_CLI_H__

#include <bluetooth/mesh/scene.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_scene_cli;

/** @def BT_MESH_MODEL_SCENE_CLI
 *
 *  @brief Scene Client model composition data entry.
 *
 *  @param[in] _cli Pointer to a @ref bt_mesh_scene_cli instance.
 */
#define BT_MESH_MODEL_SCENE_CLI(_cli)                                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SCENE_CLI, _bt_mesh_scene_cli_op,    \
			 &(_cli)->pub,                                         \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_scene_cli,     \
						 _cli),                        \
			 &_bt_mesh_scene_cli_cb)

/** Scene register parameters */
struct bt_mesh_scene_register {
	/** Status of the previous operation. */
	enum bt_mesh_scene_status status;
	/** Current scene. */
	uint16_t current;
	/** Number of entries in the scenes list. */
	size_t count;
	/** Optional list of scenes to populate. */
	__packed uint16_t *scenes;
};

/** Scene client model instance. */
struct bt_mesh_scene_cli {
	/** @brief Status message callback.
	 *
	 *  @param[in] cli   Scene client model.
	 *  @param[in] ctx   Message context parameters.
	 *  @param[in] state Received scene state.
	 */
	void (*status)(struct bt_mesh_scene_cli *cli,
		       struct bt_mesh_msg_ctx *ctx,
		       const struct bt_mesh_scene_state *state);

	/** @brief Scene register message callback.
	 *
	 *  @param[in] cli Scene client model.
	 *  @param[in] ctx Message context parameters.
	 *  @param[in] reg Received scene register.
	 */
	void (*scene_register)(struct bt_mesh_scene_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_scene_register *reg);

	/* Composition data entry pointer. */
	const struct bt_mesh_model *model;
	/* Model publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication message */
	struct net_buf_simple pub_msg;
	/* Publication message buffer. */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(BT_MESH_SCENE_OP_RECALL,
					  BT_MESH_SCENE_MSG_MAXLEN_RECALL)];
	/* Ack context */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/* Transaction ID */
	uint8_t tid;
};

/** @brief Get the current state of a Scene Server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. The response will
 *  always be passed to the @ref bt_mesh_scene_cli::status callback.
 *
 *  @param[in]  cli Scene client model.
 *  @param[in]  ctx Message context to send with, or NULL to use the configured
 *                  publication parameters.
 *  @param[out] rsp Response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully got the scene state.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_scene_cli_get(struct bt_mesh_scene_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_scene_state *rsp);

/** @brief Get the full scene register of a Scene Server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. The response will
 *  always be passed to the @ref bt_mesh_scene_cli::scene_register callback.
 *
 *  @param[in]  cli Scene client model.
 *  @param[in]  ctx Message context to send with, or NULL to use the configured
 *                  publication parameters.
 *  @param[out] rsp Response buffer, or NULL to keep from blocking. If the
 *                  @c rsp.scenes parameter points to a valid buffer, it will be
 *                  filled with at most @c rsp.count number of scenes, and
 *                  @c rsp.count will be changed to reflect the number of
 *                  retrieved scenes.
 *
 *  @retval 0 Successfully got the scene register.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_scene_cli_register_get(struct bt_mesh_scene_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_scene_register *rsp);

/** @brief Store the current state as a scene.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. The response will
 *  always be passed to the @ref bt_mesh_scene_cli::scene_register callback.
 *
 *  @param[in]  cli   Scene client model.
 *  @param[in]  ctx   Message context to send with, or NULL to use the
 *                    configured publication parameters.
 *  @param[in]  scene Scene number to store. Cannot be @ref BT_MESH_SCENE_NONE.
 *  @param[out] rsp   Response buffer, or NULL to keep from blocking. If the
 *                    @c rsp.scenes parameter points to a valid buffer, it will
 *                    be filled with at most @c rsp.count number of scenes, and
 *                    @c rsp.count will be changed to reflect the number of
 *                    retrieved scenes.
 *
 *  @retval 0 Successfully sent the store message and processed the response.
 *  @retval -EINVAL Invalid scene number.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_scene_cli_store(struct bt_mesh_scene_cli *cli,
			    struct bt_mesh_msg_ctx *ctx, uint16_t scene,
			    struct bt_mesh_scene_register *rsp);

/** @brief Store the current state as a scene without requesting a response.
 *
 *  @param[in] cli   Scene client model.
 *  @param[in] ctx   Message context to send with, or NULL to use the configured
 *                   publication parameters.
 *  @param[in] scene Scene number to store. Cannot be @ref BT_MESH_SCENE_NONE.
 *
 *  @retval 0 Successfully sent the store message.
 *  @retval -EINVAL Invalid scene number.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_scene_cli_store_unack(struct bt_mesh_scene_cli *cli,
				  struct bt_mesh_msg_ctx *ctx, uint16_t scene);

/** @brief Delete the given scene.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. The response will
 *  always be passed to the @ref bt_mesh_scene_cli::scene_register callback.
 *
 *  @param[in]  cli   Scene client model.
 *  @param[in]  ctx   Message context to send with, or NULL to use the
 *                    configured publication parameters.
 *  @param[in]  scene Scene to delete. Cannot be @ref BT_MESH_SCENE_NONE.
 *  @param[out] rsp   Response buffer, or NULL to keep from blocking. If the
 *                    @c rsp.scenes parameter points to a valid buffer, it will
 *                    be filled with at most @c rsp.count number of scenes, and
 *                    @c rsp.count will be changed to reflect the number of
 *                    retrieved scenes.
 *
 *  @retval 0 Successfully sent the delete message and processed the response.
 *  @retval -EINVAL Invalid scene number.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_scene_cli_delete(struct bt_mesh_scene_cli *cli,
			     struct bt_mesh_msg_ctx *ctx, uint16_t scene,
			     struct bt_mesh_scene_register *rsp);

/** @brief Delete the given scene without requesting a response.
 *
 *  @param[in] cli   Scene client model.
 *  @param[in] ctx   Message context to send with, or NULL to use the configured
 *                   publication parameters.
 *  @param[in] scene Scene to delete. Cannot be @ref BT_MESH_SCENE_NONE.
 *
 *  @retval 0 Successfully sent the delete message.
 *  @retval -EINVAL Invalid scene number.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_scene_cli_delete_unack(struct bt_mesh_scene_cli *cli,
				   struct bt_mesh_msg_ctx *ctx, uint16_t scene);

/** @brief Recall the given scene.
 *
 *  All models that participate in the scene will transition to the stored scene
 *  state with the given transition parameters.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. The response will
 *  always be passed to the @ref bt_mesh_scene_cli::status callback.
 *
 *  @param[in]  cli        Scene client model.
 *  @param[in]  ctx        Message context to send with, or NULL to use the
 *                         configured publication parameters.
 *  @param[in]  scene      Scene to recall. Cannot be @ref BT_MESH_SCENE_NONE.
 *  @param[in]  transition Parameters for the scene transition, or NULL to use
 *                         the target's default parameters.
 *  @param[out] rsp        Response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the recall message and processed the response.
 *  @retval -EINVAL Invalid scene number or transition parameters.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_scene_cli_recall(struct bt_mesh_scene_cli *cli,
			     struct bt_mesh_msg_ctx *ctx, uint16_t scene,
			     const struct bt_mesh_model_transition *transition,
			     struct bt_mesh_scene_state *rsp);

/** @brief Recall the given scene without requesting a response.
 *
 *  All models that participate in the scene will transition to the stored scene
 *  state with the given transition parameters.
 *
 *  @param[in] cli        Scene client model.
 *  @param[in] ctx        Message context to send with, or NULL to use the
 *                        configured publication parameters.
 *  @param[in] scene      Scene to recall. Cannot be @ref BT_MESH_SCENE_NONE.
 *  @param[in] transition Parameters for the scene transition, or NULL to use
 *                        the target's default parameters.
 *
 *  @retval 0 Successfully sent the recall message.
 *  @retval -EINVAL Invalid scene number or transition parameters.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_scene_cli_recall_unack(
	struct bt_mesh_scene_cli *cli, struct bt_mesh_msg_ctx *ctx,
	uint16_t scene, const struct bt_mesh_model_transition *transition);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_scene_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_scene_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SCENE_CLI_H__ */

/** @} */
