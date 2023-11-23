/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_battery_srv Generic Battery Server model
 * @ingroup bt_mesh_battery
 * @{
 * @brief API for the Generic Battery Server model.
 */

#ifndef BT_MESH_GEN_BATTERY_SRV_H__
#define BT_MESH_GEN_BATTERY_SRV_H__

#include <bluetooth/mesh/gen_battery.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_battery_srv;

/** @def BT_MESH_BATTERY_SRV_INIT
 *
 * @brief Init parameters for a @ref bt_mesh_battery_srv instance.
 *
 * @param[in] _get_handler Get handler function for the Battery state.
 */
#define BT_MESH_BATTERY_SRV_INIT(_get_handler)                                 \
	{                                                                      \
		.get = _get_handler,                                           \
	}

/** @def BT_MESH_MODEL_BATTERY_SRV
 *
 * @brief Generic Battery Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_battery_srv instance.
 */
#define BT_MESH_MODEL_BATTERY_SRV(_srv)                                        \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_BATTERY_SRV,                     \
			 _bt_mesh_battery_srv_op, &(_srv)->pub,                \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_battery_srv,   \
						 _srv),                        \
			 &_bt_mesh_battery_srv_cb)

/**
 * Generic Battery Server instance. Should primarily be initialized with the
 * @ref BT_MESH_BATTERY_SRV_INIT macro.
 */
struct bt_mesh_battery_srv {
	/** Pointer to the model entry in the composition data. */
	const struct bt_mesh_model *model;
	/** Publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(BT_MESH_BATTERY_OP_STATUS,
					       BT_MESH_BATTERY_MSG_LEN_STATUS)];

	/** @brief Get the Battery state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server instance to get the state of.
	 * @param[in] ctx Message context for the message that triggered the
	 * change, or NULL if the change is not coming from a message.
	 * @param[out] rsp Response structure to be filled. All fields are
	 * initialized to special @em unknown values before calling, so only
	 * states that have known values need to be filled.
	 */
	void (*const get)(struct bt_mesh_battery_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_battery_status *rsp);
};

/** @brief Publish the Generic Battery Server model status.
 *
 * Asynchronously publishes a Generic Battery status message with the
 * configured publish parameters.
 *
 * @note This API is only used for publishing unprompted status messages.
 * Response messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 * @param[in] status Current status.
 *
 * @retval 0 Successfully published a Generic Battery Status message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_battery_srv_pub(struct bt_mesh_battery_srv *srv,
			    struct bt_mesh_msg_ctx *ctx,
			    const struct bt_mesh_battery_status *status);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_battery_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_battery_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_BATTERY_SRV_H__ */

/** @} */
