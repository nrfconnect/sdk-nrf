/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_scheduler_cli Scheduler Client model
 * @{
 * @brief API for the Scheduler Client model.
 */

#ifndef BT_MESH_SCHEDULER_CLI_H__
#define BT_MESH_SCHEDULER_CLI_H__

#include <bluetooth/mesh/scheduler.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_scheduler_cli;

/** @def BT_MESH_MODEL_SCHEDULER_CLI
 *
 *  @brief Scheduler Client model composition data entry.
 *
 *  @param[in] _cli Pointer to a @ref bt_mesh_scheduler_cli instance.
 */
#define BT_MESH_MODEL_SCHEDULER_CLI(_cli)                                     \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SCHEDULER_CLI,                      \
			_bt_mesh_scheduler_cli_op,                            \
			 &(_cli)->pub,                                        \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_scheduler_cli,\
						 _cli),                       \
			 &_bt_mesh_scheduler_cli_cb)

/** Scheduler client model instance. */
struct bt_mesh_scheduler_cli {
	/** @brief Scheduler Status message callback.
	 *
	 *  @param[in] cli       Scheduler client model.
	 *  @param[in] ctx       Message context parameters.
	 *  @param[in] schedules Received Scheduler Status.
	 */
	void (*status_handler)(struct bt_mesh_scheduler_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       uint16_t schedules);

	/** @brief Scheduler Action Status message callback.
	 *
	 *  @param[in] cli    Scheduler client model.
	 *  @param[in] ctx    Message context parameters.
	 *  @param[in] idx    Index of the received action.
	 *  @param[in] action Received Scheduler action or
	 *                    NULL if there is only index in the reply.
	 */
	void (*action_status_handler)(struct bt_mesh_scheduler_cli *cli,
			struct bt_mesh_msg_ctx *ctx,
			uint8_t idx,
			const struct bt_mesh_schedule_entry *action);

	/* Composition data entry pointer. */
	const struct bt_mesh_model *model;
	/* Model publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication message */
	struct net_buf_simple pub_msg;
	/* Publication message buffer. */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(BT_MESH_SCHEDULER_OP_ACTION_STATUS,
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS)];
	/* Ack context */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/* Action index expected in the Ack. */
	uint8_t ack_idx;
};

/** @brief Get the current Schedule Register status.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. The response will
 *  always be passed to the @ref bt_mesh_scheduler_cli::status_handler.
 *
 *  @param[in]  cli Scheduler client model.
 *  @param[in]  ctx Message context to send with, or NULL to use the configured
 *                  publication parameters.
 *  @param[out] rsp Response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully got the Schedule Register status.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_scheduler_cli_get(struct bt_mesh_scheduler_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      uint16_t *rsp);

/** @brief Get the appropriate Scheduler Action status.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. The response will
 *  always be passed to the @ref bt_mesh_scheduler_cli::action_status_handler.
 *
 *  @param[in]  cli   Scheduler client model.
 *  @param[in]  ctx   Message context to send with, or NULL to use
 *                    the configured publication parameters.
 *  @param[in]  idx   Index of the Schedule Register entry to get.
 *  @param[out] rsp   Response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully got the Scheduler Action status.
 *  @retval -EINVAL Invalid index.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_scheduler_cli_action_get(struct bt_mesh_scheduler_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				uint8_t idx,
				struct bt_mesh_schedule_entry *rsp);

/** @brief Set the appropriate Scheduler Action.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. The response will
 *  always be passed to the @ref bt_mesh_scheduler_cli::action_status_handler.
 *
 *  @param[in]  cli   Scheduler client model.
 *  @param[in]  ctx   Message context to send with, or NULL to use the
 *                    configured publication parameters.
 *  @param[in]  idx   Index of the Schedule Register entry to set.
 *  @param[in]  entry The entry of the Scheduler Register.
 *  @param[out] rsp   Response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the set message and processed the response.
 *  @retval -EINVAL Invalid parameters.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_scheduler_cli_action_set(struct bt_mesh_scheduler_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				uint8_t idx,
				const struct bt_mesh_schedule_entry *entry,
				struct bt_mesh_schedule_entry *rsp);

/** @brief Set the appropriate Scheduler Action without requesting a response.
 *
 *  @param[in]  cli   Scheduler client model.
 *  @param[in]  ctx   Message context to send with, or NULL to use the
 *                    configured publication parameters.
 *  @param[in]  idx   Index of the Schedule Register entry to set.
 *  @param[in]  entry The entry of the Scheduler Register.
 *
 *  @retval 0 Successfully sent the set message.
 *  @retval -EINVAL Invalid parameters.
 *  @retval -EADDRNOTAVAIL A message context was not provided and
 *  publishing is not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_scheduler_cli_action_set_unack(struct bt_mesh_scheduler_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				uint8_t idx,
				const struct bt_mesh_schedule_entry *entry);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_scheduler_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_scheduler_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SCHEDULER_CLI_H__ */

/** @} */
