/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_light_sat_srv Light Saturation Server model
 * @{
 * @brief API for the Light Saturation Server model.
 */

#ifndef BT_MESH_LIGHT_SAT_SRV_H__
#define BT_MESH_LIGHT_SAT_SRV_H__

#include <bluetooth/mesh/light_hsl.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/gen_lvl_srv.h>
#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
#include "emds/emds.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_sat_srv;

/** @def BT_MESH_LIGHT_SAT_SRV_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_light_sat_srv
 * instance.
 *
 * @param[in] _handlers HSL Saturation Server callbacks.
 */
#define BT_MESH_LIGHT_SAT_SRV_INIT(_handlers)                                  \
	{                                                                      \
		.handlers = _handlers,                                         \
		.lvl = BT_MESH_LVL_SRV_INIT(                                   \
			&_bt_mesh_light_sat_srv_lvl_handlers),                 \
		.range = BT_MESH_LIGHT_HSL_OP_RANGE_DEFAULT,                   \
	}

/** @def BT_MESH_MODEL_LIGHT_SAT_SRV
 *
 * @brief HSL Saturation Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_light_sat_srv instance.
 */
#define BT_MESH_MODEL_LIGHT_SAT_SRV(_srv)                                      \
	BT_MESH_MODEL_LVL_SRV(&(_srv)->lvl),                                   \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_SAT_SRV,                   \
			 _bt_mesh_light_sat_srv_op, &(_srv)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_sat_srv, \
						 _srv),                        \
			 &_bt_mesh_light_sat_srv_cb)

/** HSL Saturation Server state access handlers. */
struct bt_mesh_light_sat_srv_handlers {
	/** @brief Set the Saturation state.
	 *
	 * When a set message is received, the model publishes a status message, with the response
	 * set to @c rsp. When an acknowledged set message is received, the model also sends a
	 * response back to a client. If a state change is non-instantaneous, for example when
	 * @ref bt_mesh_model_transition_time returns a nonzero value, the application is
	 * responsible for publishing a value of the Saturation state at the end of the transition.
	 *
	 *  @note This handler is mandatory.
	 *
	 *  @param[in]  srv Light Saturation server.
	 *  @param[in]  ctx Message context, or NULL if the callback was not
	 *                  triggered by a mesh message.
	 *  @param[in]  set Parameters of the state change.
	 *  @param[out] rsp Response structure to be filled.
	 */
	void (*const set)(struct bt_mesh_light_sat_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_sat *set,
			  struct bt_mesh_light_sat_status *rsp);

	/** @brief Get the Saturation state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in]  srv Light Saturation server.
	 * @param[in]  ctx Message context, or NULL if the callback was not
	 *                 triggered by a mesh message.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const get)(struct bt_mesh_light_sat_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_sat_status *rsp);

	/** @brief The default saturation value has been updated.
	 *
	 *  @param[in] srv         Light Saturation server.
	 *  @param[in] ctx         Message context, or NULL if the callback was
	 *                         not triggered by a mesh message
	 *  @param[in] old_default Default value before the update.
	 *  @param[in] new_default Default value after the update.
	 */
	void (*const default_update)(struct bt_mesh_light_sat_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     uint16_t old_default,
				     uint16_t new_default);

	/** @brief The valid saturation range has been updated.
	 *
	 *  @param[in] srv       Light Saturation server.
	 *  @param[in] ctx       Message context, or NULL if the callback was
	 *                       not triggered by a mesh message
	 *  @param[in] old_range Range before the update.
	 *  @param[in] new_range Range after the update.
	 */
	void (*const range_update)(
		struct bt_mesh_light_sat_srv *srv,
		struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_hsl_range *old_range,
		const struct bt_mesh_light_hsl_range *new_range);
};

/**
 * HSL Saturation Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_SAT_SRV_INIT.
 */
struct bt_mesh_light_sat_srv {
	/** Light Level Server instance. */
	struct bt_mesh_lvl_srv lvl;
	/** Handler function structure. */
	const struct bt_mesh_light_sat_srv_handlers *handlers;

	/** Model entry. */
	struct bt_mesh_model *model;
	/** Pointer to the corresponding HSL server, if it has one.
	 *  Is set automatically by the HSL server.
	 */
	const struct bt_mesh_light_hsl_srv *hsl;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Publication buffer */
	struct net_buf_simple buf;
	/** Publication buffer data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_SAT_OP_STATUS,
		BT_MESH_LIGHT_HSL_MSG_MAXLEN_SAT_STATUS)];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;

	/** Saturation range */
	struct bt_mesh_light_hsl_range range;
	/** Default Saturation level */
	uint16_t dflt;
	struct __packed {
		/** Last known Saturation level */
		uint16_t last;
	} transient;

#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
	/** Dynamic entry to be stored with EMDS */
	struct emds_dynamic_entry emds_entry;
#endif
};

/** @brief Publish the current HSL Saturation status.
 *
 * Asynchronously publishes a HSL Saturation status message with the configured
 * publish parameters.
 *
 * @note This API is only used publishing unprompted status messages. Response
 * messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 * @param[in] status Status parameters.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_sat_srv_pub(struct bt_mesh_light_sat_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_light_sat_status *status);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_sat_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_sat_srv_cb;
extern const struct bt_mesh_lvl_srv_handlers
	_bt_mesh_light_sat_srv_lvl_handlers;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_SAT_SRV_H__ */

/** @} */
