/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_dm_cli Distance Measurement Client model
 * @{
 * @brief API for the Distance Measurement Client model.
 */

#ifndef BT_MESH_DM_CLI_H__
#define BT_MESH_DM_CLI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/mesh/models.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/sys/slist.h>
#include <zephyr/bluetooth/mesh/access.h>
#include "dm_common.h"
#include <dm.h>

struct bt_mesh_dm_cli;

/** Parameters for the Distance Measurement Result Status message. */
struct bt_mesh_dm_cli_results {
	/** Message status code. */
	uint8_t status;
	/** Number of entries in the result. */
	uint8_t entry_cnt;
	/** Result memory object. */
	struct bt_mesh_dm_res_entry *res;
};

/** Parameters for the Distance Measurement Config Status message. */
struct bt_mesh_dm_cli_cfg_status {
	/** Message status code. */
	uint8_t status;
	/** Measurement in progress flag. */
	bool is_in_progress;
	/** Maximum number of result entries available. */
	uint8_t result_entry_cnt;
	/** Default parameter values. */
	struct bt_mesh_dm_cfg def;
};

/** Parameters for the Distance Measurement Start message. */
struct bt_mesh_dm_cli_start {
	/** Use previous transaction id. */
	bool reuse_transaction;
	/** Mode used for ranging. */
	enum dm_ranging_mode mode;
	/** Unicast address of the node to measure distance with. */
	uint16_t addr;
	/** Desired configuration parameter values. */
	struct bt_mesh_dm_cfg *cfg;
};

/** Handler functions for the Distance Measurement Client. */
struct bt_mesh_dm_cli_handlers {
	/** @brief Config status message handler.
	 *
	 * @note This handler is optional.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status Config status contained in the message.
	 */
	void (*const cfg_status_handler)(struct bt_mesh_dm_cli *cli,
					 struct bt_mesh_msg_ctx *ctx,
					 const struct bt_mesh_dm_cli_cfg_status *status);

	/** @brief Result status message handler.
	 *
	 * @note This handler is optional.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] results Results contained in the message.
	 */
	void (*const result_handler)(struct bt_mesh_dm_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_dm_cli_results *results);
};

/** Distance Measurement Client instance. */
struct bt_mesh_dm_cli {
	/** Application handler functions. */
	const struct bt_mesh_dm_cli_handlers *const handlers;
	/** Transaction ID. */
	uint8_t tid;
	/** Access model pointer. */
	const struct bt_mesh_model *model;
	/** Response context for tracking acknowledged messages. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Publication buffer */
	struct net_buf_simple pub_buf;
	/** Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(BT_MESH_DM_START_OP,
					       BT_MESH_DM_START_MSG_LEN_MAX)];
	/** Pointer to Distance measurement Result memory object */
	struct bt_mesh_dm_res_entry *res_arr;
	/** Total number of entry spaces in result memory object */
	uint8_t entry_cnt;
};

/** @def BT_MESH_MODEL_DM_CLI_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_dm_cli
 * instance.
 *
 * @param[in] _mem Pointer to a @ref bt_mesh_dm_res_entry memory object.
 * @param[in] _cnt Array size of @ref bt_mesh_dm_res_entry memory object.
 * @param[in] _handlers Pointer to @ref bt_mesh_dm_cli_handlers structure or NULL.
 *
 */
#define BT_MESH_MODEL_DM_CLI_INIT(_mem, _cnt, _handlers)                                           \
	{                                                                                          \
		.res_arr = _mem,                                                                   \
		.entry_cnt = _cnt,                                                                 \
		.handlers = _handlers,								   \
	}

/** @def BT_MESH_MODEL_DM_CLI
 *
 * @brief Distance measurement Client model entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_dm_cli instance.
 */
#define BT_MESH_MODEL_DM_CLI(_cli)                                                       \
	BT_MESH_MODEL_VND_CB(BT_MESH_VENDOR_COMPANY_ID, BT_MESH_MODEL_ID_DM_CLI,         \
			     _bt_mesh_dm_cli_op, &(_cli)->pub,                           \
			     BT_MESH_MODEL_USER_DATA(struct bt_mesh_dm_cli, _cli),       \
			     &_bt_mesh_dm_cli_cb)

/** @brief Set or get the config state in the Distance Measurement server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_dm_cli_handlers::cfg_status_handler callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set New configuration parameters, or NULL to get the current parameters.
 * @param[out] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 * @retval -EBADMSG The supplied configuration parameters are invalid.
 */
int bt_mesh_dm_cli_config(struct bt_mesh_dm_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_dm_cfg *set,
			  struct bt_mesh_dm_cli_cfg_status *rsp);

/** @brief Start a distance measurement on a Distance Measurement server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_dm_cli_handlers::result_handler callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] start Distance measurement Start parameters.
 * @param[out] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 * @retval -EBADMSG The supplied configuration parameters are invalid or
 * the address is not a unicast address.
 */
int bt_mesh_dm_cli_measurement_start(struct bt_mesh_dm_cli *cli, struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_dm_cli_start *start,
				    struct bt_mesh_dm_cli_results *rsp);

/** @brief Get measurement results from a Distance Measurement server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_dm_cli_handlers::result_handler callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] entry_cnt Number of entries the user wish to receive.
 * @param[out] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 * @retval -EBADMSG Invalid entry count provided.
 */
int bt_mesh_dm_cli_results_get(struct bt_mesh_dm_cli *cli, struct bt_mesh_msg_ctx *ctx,
			       uint8_t entry_cnt, struct bt_mesh_dm_cli_results *rsp);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_dm_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_dm_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_DM_CLI_H__ */

/** @} */
