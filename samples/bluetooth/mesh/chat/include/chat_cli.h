/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_chat_cli
 * @{
 * @brief API for the Bluetooth Mesh Chat Client model.
 */

#ifndef BT_MESH_CHAT_CLI_H__
#define BT_MESH_CHAT_CLI_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* .. include_startingpoint_chat_cli_rst_1 */
/** Company ID of the Bluetooth Mesh Chat Client model. */
#define BT_MESH_CHAT_CLI_VENDOR_COMPANY_ID    CONFIG_BT_COMPANY_ID_NORDIC

/** Model ID of the Bluetooth Mesh Chat Client model. */
#define BT_MESH_CHAT_CLI_VENDOR_MODEL_ID      0x000A

/** Non-private message opcode. */
#define BT_MESH_CHAT_CLI_OP_MESSAGE BT_MESH_MODEL_OP_3(0x0A, \
				       BT_MESH_CHAT_CLI_VENDOR_COMPANY_ID)

/** Private message opcode. */
#define BT_MESH_CHAT_CLI_OP_PRIVATE_MESSAGE BT_MESH_MODEL_OP_3(0x0B, \
				       BT_MESH_CHAT_CLI_VENDOR_COMPANY_ID)

/** Message reply opcode. */
#define BT_MESH_CHAT_CLI_OP_MESSAGE_REPLY BT_MESH_MODEL_OP_3(0x0C, \
				       BT_MESH_CHAT_CLI_VENDOR_COMPANY_ID)

/** Presence message opcode. */
#define BT_MESH_CHAT_CLI_OP_PRESENCE BT_MESH_MODEL_OP_3(0x0D, \
				       BT_MESH_CHAT_CLI_VENDOR_COMPANY_ID)

/** Presence get message opcode. */
#define BT_MESH_CHAT_CLI_OP_PRESENCE_GET BT_MESH_MODEL_OP_3(0x0E, \
				       BT_MESH_CHAT_CLI_VENDOR_COMPANY_ID)
/* .. include_endpoint_chat_cli_rst_1 */

#define BT_MESH_CHAT_CLI_MSG_MINLEN_MESSAGE 1
#define BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE (\
				     CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH \
				     + 1) /* + \0 */
#define BT_MESH_CHAT_CLI_MSG_LEN_MESSAGE_REPLY 0
#define BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE 1
#define BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE_GET 0

/** Bluetooth Mesh Chat Client Presence values. */
enum bt_mesh_chat_cli_presence {
	BT_MESH_CHAT_CLI_PRESENCE_AVAILABLE,
	BT_MESH_CHAT_CLI_PRESENCE_AWAY,
	BT_MESH_CHAT_CLI_PRESENCE_INACTIVE,
	BT_MESH_CHAT_CLI_PRESENCE_DO_NOT_DISTURB
};

/* Forward declaration of the Bluetooth Mesh Chat Client model context. */
struct bt_mesh_chat_cli;

/* .. include_startingpoint_chat_cli_rst_2 */
/** @def BT_MESH_MODEL_CHAT_CLI
 *
 * @brief Bluetooth Mesh Chat Client model composition data entry.
 *
 * @param[in] _chat Pointer to a @ref bt_mesh_chat_cli instance.
 */
#define BT_MESH_MODEL_CHAT_CLI(_chat)                                          \
		BT_MESH_MODEL_VND_CB(BT_MESH_CHAT_CLI_VENDOR_COMPANY_ID,       \
			BT_MESH_CHAT_CLI_VENDOR_MODEL_ID,                      \
			_bt_mesh_chat_cli_op, &(_chat)->pub,                   \
			BT_MESH_MODEL_USER_DATA(struct bt_mesh_chat_cli,       \
						_chat),                        \
			&_bt_mesh_chat_cli_cb)
/* .. include_endpoint_chat_cli_rst_2 */

/** Bluetooth Mesh Chat Client model handlers. */
struct bt_mesh_chat_cli_handlers {
	/** @brief Called after the node has been provisioned, or after all
	 * mesh data has been loaded from persistent storage.
	 *
	 * @param[in] cli Chat Client instance that has been started.
	 */
	void (*const start)(struct bt_mesh_chat_cli *chat);

	/** @brief Handler for a presence message.
	 *
	 * @param[in] cli Chat client instance that received the text message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] presence Presence of a Chat Client that published
	 * the message.
	 */
	void (*const presence)(struct bt_mesh_chat_cli *chat,
			       struct bt_mesh_msg_ctx *ctx,
			       enum bt_mesh_chat_cli_presence presence);

	/** @brief Handler for a non-private text message.
	 *
	 * @param[in] cli Chat client instance that received the text message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] msg Pointer to a received text message terminated with
	 * a null character, '\0'.
	 */
	void (*const message)(struct bt_mesh_chat_cli *chat,
			      struct bt_mesh_msg_ctx *ctx,
			      const uint8_t *msg);

	/** @brief Handler for a private message.
	 *
	 * @param[in] cli Chat client that received the text message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] msg Pointer to a received text message terminated with
	 * a null character, '\0'.
	 */
	void (*const private_message)(struct bt_mesh_chat_cli *chat,
				      struct bt_mesh_msg_ctx *ctx,
				      const uint8_t *msg);

	/** @brief Handler for a reply on a private message.
	 *
	 * @param[in] cli Chat client instance that received the reply.
	 * @param[in] ctx Context of the incoming message.
	 */
	void (*const message_reply)(struct bt_mesh_chat_cli *chat,
				    struct bt_mesh_msg_ctx *ctx);
};

/* .. include_startingpoint_chat_cli_rst_3 */
/**
 * Bluetooth Mesh Chat Client model context.
 */
struct bt_mesh_chat_cli {
	/** Access model pointer. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Publication message. */
	struct net_buf_simple pub_msg;
	/** Publication message buffer. */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(BT_MESH_CHAT_CLI_OP_MESSAGE,
					  BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE)];
	/** Handler function structure. */
	const struct bt_mesh_chat_cli_handlers *handlers;
	/** Current Presence value. */
	enum bt_mesh_chat_cli_presence presence;
};
/* .. include_endpoint_chat_cli_rst_3 */

/** @brief Set the client presence and publish the presence to the mesh network.
 *
 * @param[in] chat     Chat Client model instance to set presence on.
 * @param[in] presence Presence status to be published.
 *
 * @retval 0 Successfully set the preceive and sent the message.
 * @retval -EADDRNOTAVAIL Publishing is not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_chat_cli_presence_set(struct bt_mesh_chat_cli *chat,
				  enum bt_mesh_chat_cli_presence presence);

/** @brief Get current presence value of a chat client.
 *
 * @param[in] chat Chat Client model instance to send the message.
 * @param[in] addr Address of the chat client to get presence value of.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EINVAL The model is not bound to an application key.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_chat_cli_presence_get(struct bt_mesh_chat_cli *chat,
				  uint16_t addr);

/** @brief Send a text message.
 *
 * @param[in] cli Chat Client model instance to send the message.
 * @param[in] msg Pointer to a text message to send. Must be terminated with
 * a null character, '\0'.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL Publishing is not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_chat_cli_message_send(struct bt_mesh_chat_cli *chat,
				  const uint8_t *msg);

/** @brief Send a text message to a specified destination.
 *
 * @param[in] cli  Chat Client model instance to send the message.
 * @param[in] addr Address of the chat client to send message to.
 * @param[in] msg  Pointer to a text message to send. Must be terminated with
 * a null character, '\0'.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EINVAL The model is not bound to an application key.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_chat_cli_private_message_send(struct bt_mesh_chat_cli *chat,
					  uint16_t addr,
					  const uint8_t *msg);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_chat_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_chat_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_CHAT_CLI_H__ */

/** @} */
