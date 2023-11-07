/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_ponoff_srv Generic Power OnOff Server model
 * @{
 * @brief API for the Generic Power OnOff Server.
 */

#ifndef BT_MESH_GEN_PONOFF_SRV_H__
#define BT_MESH_GEN_PONOFF_SRV_H__

#include <zephyr/bluetooth/mesh.h>

#include <bluetooth/mesh/gen_ponoff.h>
#include <bluetooth/mesh/gen_onoff_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_ponoff_srv;

/** @def BT_MESH_PONOFF_SRV_INIT
 *
 * @brief Initialization parameters for @ref bt_mesh_ponoff_srv.
 *
 * @param[in] _onoff_handlers Handlers for the underlying Generic OnOff Server.
 * @param[in] _dtt_change_handler Handler function for changes to the Default
 * Transition Time Server state.
 * @param[in] _on_power_up_change_handler Handler function for changes to the
 * OnPowerUp state.
 */
#define BT_MESH_PONOFF_SRV_INIT(_onoff_handlers, _dtt_change_handler,          \
				_on_power_up_change_handler)                   \
	{                                                                      \
		.onoff = BT_MESH_ONOFF_SRV_INIT(                               \
			&_bt_mesh_ponoff_onoff_intercept),                     \
		.dtt = BT_MESH_DTT_SRV_INIT(_dtt_change_handler),              \
		.onoff_handlers = _onoff_handlers,                             \
		.update = _on_power_up_change_handler,                         \
	}

/** @def BT_MESH_MODEL_PONOFF_SRV
 *
 * @brief Generic Power OnOff model entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_ponoff_srv instance.
 */
#define BT_MESH_MODEL_PONOFF_SRV(_srv)                                         \
	BT_MESH_MODEL_ONOFF_SRV(&(_srv)->onoff),                               \
		BT_MESH_MODEL_DTT_SRV(&(_srv)->dtt),                           \
		BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SRV,         \
				 _bt_mesh_ponoff_srv_op, &(_srv)->pub,         \
				 BT_MESH_MODEL_USER_DATA(                      \
					 struct bt_mesh_ponoff_srv, _srv),     \
				 &_bt_mesh_ponoff_srv_cb),                     \
		BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SETUP_SRV,   \
				 _bt_mesh_ponoff_setup_srv_op, NULL,           \
				 BT_MESH_MODEL_USER_DATA(                      \
					 struct bt_mesh_ponoff_srv, _srv),     \
				 &_bt_mesh_ponoff_setup_srv_cb)

/**
 * Generic Power OnOff Server instance.
 *
 * Should be initialized with @ref BT_MESH_PONOFF_SRV_INIT.
 */
struct bt_mesh_ponoff_srv {
	/** Generic OnOff Server instance. */
	struct bt_mesh_onoff_srv onoff;
	/** Generic Default Transition Time server instance. */
	struct bt_mesh_dtt_srv dtt;
	/** Pointer to the model entry in the composition data. */
	struct bt_mesh_model *ponoff_model;
	/** Pointer to the model entry of the Setup Server. */
	struct bt_mesh_model *ponoff_setup_model;
	/** Model publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(BT_MESH_PONOFF_OP_STATUS,
					       BT_MESH_PONOFF_MSG_LEN_STATUS)];
	/** Handlers for the Generic OnOff Server. */
	const struct bt_mesh_onoff_srv_handlers *const onoff_handlers;

	/** @brief Update handler.
	 *
	 * Called every time there's a change to the OnPowerUp state.
	 *
	 * @param[in] srv Server instance that was updated
	 * @param[in] ctx Context of the set message that triggered the update,
	 * or NULL if it was not triggered by a message.
	 * @param[in] old_on_power_up The previous OnPowerUp value.
	 * @param[in] new_on_power_up The new OnPowerUp value.
	 */
	void (*const update)(struct bt_mesh_ponoff_srv *srv,
			     struct bt_mesh_msg_ctx *ctx,
			     enum bt_mesh_on_power_up old_on_power_up,
			     enum bt_mesh_on_power_up new_on_power_up);

	/** Current OnPowerUp state. */
	enum bt_mesh_on_power_up on_power_up;
};

/** @brief Set the OnPowerUp state of a Power OnOff server.
 *
 * If an update handler is set, it'll be called with the updated OnPowerUp
 * value. If publication is configured, the change will cause the server to
 * publish.
 *
 * @param[in] srv Server to set the OnPowerUp state of.
 * @param[in] on_power_up New OnPowerUp value.
 */
void bt_mesh_ponoff_srv_set(struct bt_mesh_ponoff_srv *srv,
			    enum bt_mesh_on_power_up on_power_up);

/** @brief Publish the current OnPowerUp state.
 *
 * Publishes a Generic Power OnOff status message with the configured publish
 * parameters, or using the given message context.
 *
 * @note This API is only used for publishing unprompted status messages.
 * Response messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish with.
 * @param[in] ctx Message context, or NULL to publish with the configured
 * parameters.
 *
 * @return 0 Successfully published the current OnPowerUp state.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_ponoff_srv_pub(struct bt_mesh_ponoff_srv *srv,
			   struct bt_mesh_msg_ctx *ctx);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_cb _bt_mesh_ponoff_srv_cb;
extern const struct bt_mesh_model_cb _bt_mesh_ponoff_setup_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_ponoff_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_ponoff_setup_srv_op[];
extern const struct bt_mesh_onoff_srv_handlers _bt_mesh_ponoff_onoff_intercept;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_PONOFF_SRV_H__ */

/** @} */
