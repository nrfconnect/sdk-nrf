/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_onoff_srv Generic OnOff Server model
 * @ingroup bt_mesh_onoff
 * @{
 * @brief API for the Generic OnOff Server model.
 */

#ifndef BT_MESH_GEN_ONOFF_SRV_H__
#define BT_MESH_GEN_ONOFF_SRV_H__

#include <bluetooth/mesh/gen_onoff.h>
#include <bluetooth/mesh/scene_srv.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_onoff_srv;

/** @def BT_MESH_ONOFF_SRV_INIT
 *
 * @brief Init parameters for a @ref bt_mesh_onoff_srv instance.
 *
 * @param[in] _handlers State access handlers to use in the model instance.
 */
#define BT_MESH_ONOFF_SRV_INIT(_handlers)                                      \
	{                                                                      \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_ONOFF_SRV
 *
 * @brief Generic OnOff Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_onoff_srv instance.
 */
#define BT_MESH_MODEL_ONOFF_SRV(_srv)                                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ONOFF_SRV,                       \
			 _bt_mesh_onoff_srv_op, &(_srv)->pub,                  \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_onoff_srv,     \
						 _srv),                        \
			 &_bt_mesh_onoff_srv_cb)

/** Generic OnOff Server state access handlers. */
struct bt_mesh_onoff_srv_handlers {
	/** @brief Set the OnOff state.
	 *
	 * When a set message is received, the model publishes a status message, with the response
	 * set to @c rsp. When an acknowledged set message is received, the model also sends a
	 * response back to a client. If a state change is non-instantaneous, for example when
	 * @ref bt_mesh_model_transition_time returns a nonzero value, the application is
	 * responsible for publishing a value of the OnOff state at the end of the transition.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server instance to set the state of.
	 * @param[in] ctx Message context for the message that triggered the
	 * change, or NULL if the change is not coming from a message.
	 * @param[in] set Parameters of the state change.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const set)(struct bt_mesh_onoff_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_onoff_set *set,
			  struct bt_mesh_onoff_status *rsp);

	/** @brief Get the OnOff state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server instance to get the state of.
	 * @param[in] ctx Message context for the message that triggered the
	 * change, or NULL if the change is not coming from a message.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const get)(struct bt_mesh_onoff_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_onoff_status *rsp);
};

/**
 * Generic OnOff Server instance. Should primarily be initialized with the
 * @ref BT_MESH_ONOFF_SRV_INIT macro.
 */
struct bt_mesh_onoff_srv {
	/** Transaction ID tracker. */
	struct bt_mesh_tid_ctx prev_transaction;
	/** Handler function structure. */
	const struct bt_mesh_onoff_srv_handlers *handlers;
	/** Access model pointer. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_ONOFF_OP_STATUS, BT_MESH_ONOFF_MSG_MAXLEN_STATUS)];
	/** Internal flag state. */
	atomic_t flags;
};

/** @brief Publish the Generic OnOff Server model status.
 *
 * Asynchronously publishes a Generic OnOff status message with the configured
 * publish parameters.
 *
 * @note This API is only used for publishing unprompted status messages.
 * Response messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 * @param[in] status Current status.
 *
 * @retval 0 Successfully published a Generic OnOff Status message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_onoff_srv_pub(struct bt_mesh_onoff_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_onoff_status *status);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_onoff_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_onoff_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_ONOFF_SRV_H__ */

/** @} */
