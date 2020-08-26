/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *  @defgroup bt_mesh_thingy52_mod Thingy52 model
 *  @{
 *  @brief API for the Thingy52 Model.
 */

#ifndef BT_MESH_THINGY52_MOD_H__
#define BT_MESH_THINGY52_MOD_H__

#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** RGB colors */
struct bt_mesh_thingy52_rgb {
	/** Color Red */
	uint8_t red;
	/** Color Green */
	uint8_t green;
	/** Color Blue */
	uint8_t blue;
};

/** Thingy52 RGB message */
struct bt_mesh_thingy52_rgb_msg {
	/** Remaining hops of the message */
	uint8_t ttl;
	/** Duration assosiated with the message in milliseconds*/
	int32_t duration;
	/** Color of the message. */
	struct bt_mesh_thingy52_rgb color;
	/** Speaker status of the message. */
	bool speaker_on;
};

struct bt_mesh_thingy52_mod;

/** @def BT_MESH_THINGY52_MOD_INIT
 *
 * @brief Init parameters for a @ref bt_mesh_thingy52_mod instance.
 *
 * @param[in] _cb_handlers Callback handler to use in the model instance.
 */
#define BT_MESH_THINGY52_MOD_INIT(_cb_handlers)                                \
	{                                                                      \
		.msg_callbacks = _cb_handlers,                                 \
		.pub = {                                                       \
			.msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(           \
				BT_MESH_ONOFF_OP_STATUS,                       \
				BT_MESH_ONOFF_MSG_MAXLEN_STATUS)),             \
		},                                                             \
	}

/** @def BT_MESH_MODEL_THINGY52_MOD
 *
 * @brief Thingy:52 Model model composition data entry.
 *
 * @param[in] _mod Pointer to a @ref bt_mesh_thingy52_mod instance.
 */
#define BT_MESH_MODEL_THINGY52_MOD(_mod)                                       \
	BT_MESH_MODEL_VND_CB(                                                  \
		BT_MESH_NORDIC_SEMI_COMPANY_ID, BT_MESH_MODEL_ID_THINGY52_MOD, \
		_bt_mesh_thingy52_mod_op, &(_mod)->pub,                        \
		BT_MESH_MODEL_USER_DATA(struct bt_mesh_thingy52_mod, _mod),    \
		&_bt_mesh_thingy52_mod_cb)

/** Handler callbacks for the Thingy52 Model. */
struct bt_mesh_thingy52_cb {
	/** @brief RGB message handler.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] mod Model the message arrived on.
	 * @param[in] ctx Message context.
	 * @param[in] rgb Parameters of the RGB message.
	 */
	void (*const rgb_set_handler)(struct bt_mesh_thingy52_mod *mod,
				      struct bt_mesh_msg_ctx *ctx,
				      struct bt_mesh_thingy52_rgb_msg rgb);
};

/**
 * Thingy:52 Model instance. Should primarily be initialized with the
 * @ref BT_MESH_THINGY52_MOD_INIT macro.
 */
struct bt_mesh_thingy52_mod {
	/** Message callbacks */
	const struct bt_mesh_thingy52_cb *msg_callbacks;
	/** Access model pointer. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
};

/** @brief Send a RGB message.
 *
 * Asynchronously publishes a RGB message with the configured
 * publish parameters.
 *
 * @param[in] mod Model instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 * @param[in] rgb RGB message.
 *
 * @retval 0 Successfully published the message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_thingy52_mod_rgb_set(struct bt_mesh_thingy52_mod *mod,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_thingy52_rgb_msg *rgb);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_thingy52_mod_op[];
extern const struct bt_mesh_model_cb _bt_mesh_thingy52_mod_cb;

#define BT_MESH_NORDIC_SEMI_COMPANY_ID 0x0059
#define BT_MESH_MODEL_ID_THINGY52_MOD 0x0005
#define BT_MESH_MODEL_ID_THINGY52_CLI 0x0006

#define BT_MESH_THINGY52_OP_RGB_SET                                            \
	BT_MESH_MODEL_OP_3(0xC3, BT_MESH_NORDIC_SEMI_COMPANY_ID)

#define BT_MESH_THINGY52_MSG_LEN_RGB_SET 5
/** @endcond */

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* BT_MESH_THINGY52_MOD_H__ */
