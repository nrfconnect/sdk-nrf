/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_loc_srv Generic Location Server model
 * @ingroup bt_mesh_loc
 * @{
 * @brief API for the Generic Location Server model.
 */

#ifndef BT_MESH_GEN_LOC_SRV_H__
#define BT_MESH_GEN_LOC_SRV_H__

#include <bluetooth/mesh/gen_loc.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_loc_srv;

/** @def BT_MESH_LOC_SRV_INIT
 *
 * @brief Init parameters for a @ref bt_mesh_loc_srv instance.
 *
 * @param[in] _handlers Handler function structure.
 */
#define BT_MESH_LOC_SRV_INIT(_handlers)                                        \
	{                                                                      \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_LOC_SRV
 *
 * @brief Generic Location Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_loc_srv instance.
 */
#define BT_MESH_MODEL_LOC_SRV(_srv)                                            \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_LOCATION_SRV,                    \
			 _bt_mesh_loc_srv_op, &(_srv)->pub,                    \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_loc_srv,       \
						 _srv),                        \
			 &_bt_mesh_loc_srv_cb),                                \
		BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LOCATION_SETUPSRV,          \
			      _bt_mesh_loc_setup_srv_op, NULL,                 \
			      BT_MESH_MODEL_USER_DATA(struct bt_mesh_loc_srv,  \
						      _srv))

/** Location Server handler functions. */
struct bt_mesh_loc_srv_handlers {
	/** @brief Get the Global Location state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to get the Global Location state of.
	 * @param[in] ctx Context of the get message that triggered the query,
	 * or NULL if it was not triggered by a message.
	 * @param[out] rsp Global Location state to fill.
	 */
	void (*const global_get)(struct bt_mesh_loc_srv *srv,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_loc_global *rsp);

	/** @brief Set the Global Location state.
	 *
	 * Any changes to the new @c loc parameter will be taken into account
	 * before a response is sent to the client.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to set the Global Location state of.
	 * @param[in] ctx Context of the set message that triggered the change,
	 * or NULL if it was not triggered by a message.
	 * @param[in,out] loc New Global Location state.
	 */
	void (*const global_set)(struct bt_mesh_loc_srv *srv,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_loc_global *loc);

	/** @brief Get the Local Location state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to get the Local Location state of.
	 * @param[in] ctx Context of the get message that triggered the query,
	 * or NULL if it was not triggered by a message.
	 * @param[out] rsp Local Location state to fill.
	 */
	void (*const local_get)(struct bt_mesh_loc_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_loc_local *rsp);

	/** @brief Set the Local Location state.
	 *
	 * Any changes to the new @c loc parameter will be taken into account
	 * before a response is sent to the client.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to set the Local Location state of.
	 * @param[in] ctx Context of the set message that triggered the change,
	 * or NULL if it was not triggered by a message.
	 * @param[in,out] loc New Local Location state.
	 */
	void (*const local_set)(struct bt_mesh_loc_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_loc_local *loc);
};

/**
 * Generic Location Server instance. Should primarily be initialized with the
 * @ref BT_MESH_LOC_SRV_INIT macro.
 */
struct bt_mesh_loc_srv {
	/** Pointer to the model instance. */
	struct bt_mesh_model *model;
	/** Publish parameters for this model instance. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[MAX(
		BT_MESH_MODEL_BUF_LEN(BT_MESH_LOC_OP_LOCAL_STATUS,
				      BT_MESH_LOC_MSG_LEN_LOCAL_STATUS),
		BT_MESH_MODEL_BUF_LEN(BT_MESH_LOC_OP_GLOBAL_STATUS,
				      BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS))];

	/** Location publishing state. */
	struct {
		/** Global location is available for publishing. */
		uint8_t is_global_available: 1;
		/** Local location is available for publishing. */
		uint8_t is_local_available: 1;
		/** The last published location over periodic publication. */
		uint8_t was_last_local: 1;
	} pub_state;
	/** Pointer to a handler structure. */
	const struct bt_mesh_loc_srv_handlers *const handlers;
};

/** @brief Publish the Global Location state.
 *
 * Asynchronously publishes a Global Location status message with the
 * configured publish parameters.
 *
 * @note This API is only used for publishing unprompted status messages.
 * Response messages for get and set messages are handled internally.
 *
 * @note The server will only publish one state at the time. Calling this
 * function will terminate any publishing of the Local Location state.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 * @param[in] global Current global location.
 *
 * @retval 0 Successfully published a Global Location status message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_loc_srv_global_pub(struct bt_mesh_loc_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_loc_global *global);

/** @brief Publish the Local Location state.
 *
 * Asynchronously publishes a Local Location status message with the configured
 * publish parameters.
 *
 * @note This API is only used for publishing unprompted status messages.
 * Response messages for get and set messages are handled internally.
 *
 * @note The server will only publish one state at the time. Calling this
 * function will terminate any publishing of the Global Location state.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 * @param[in] local Current local location.
 *
 * @retval 0 Successfully published a Local Location status message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_loc_srv_local_pub(struct bt_mesh_loc_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_loc_local *local);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_loc_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_loc_setup_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_loc_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_LOC_SRV_H__ */

/** @} */
