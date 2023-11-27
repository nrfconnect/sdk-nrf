/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "chat_cli.h"
#include "mesh/net.h"
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(chat);

BUILD_ASSERT(BT_MESH_MODEL_BUF_LEN(BT_MESH_CHAT_CLI_OP_MESSAGE,
				   BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE) <=
		    BT_MESH_RX_SDU_MAX,
	     "The message must fit inside an application SDU.");
BUILD_ASSERT(BT_MESH_MODEL_BUF_LEN(BT_MESH_CHAT_CLI_OP_MESSAGE,
				   BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE) <=
		    BT_MESH_TX_SDU_MAX,
	     "The message must fit inside an application SDU.");

static void encode_presence(struct net_buf_simple *buf,
			    enum bt_mesh_chat_cli_presence presence)
{
	bt_mesh_model_msg_init(buf, BT_MESH_CHAT_CLI_OP_PRESENCE);
	net_buf_simple_add_u8(buf, presence);
}

static const uint8_t *extract_msg(struct net_buf_simple *buf)
{
	buf->data[buf->len - 1] = '\0';
	return net_buf_simple_pull_mem(buf, buf->len);
}

static int handle_message(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;
	const uint8_t *msg;

	msg = extract_msg(buf);

	if (chat->handlers->message) {
		chat->handlers->message(chat, ctx, msg);
	}

	return 0;
}

/* .. include_startingpoint_chat_cli_rst_1 */
static void send_message_reply(struct bt_mesh_chat_cli *chat,
			     struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_CHAT_CLI_OP_MESSAGE_REPLY,
				 BT_MESH_CHAT_CLI_MSG_LEN_MESSAGE_REPLY);
	bt_mesh_model_msg_init(&msg, BT_MESH_CHAT_CLI_OP_MESSAGE_REPLY);

	(void)bt_mesh_model_send(chat->model, ctx, &msg, NULL, NULL);
}

static int handle_private_message(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;
	const uint8_t *msg;

	msg = extract_msg(buf);

	if (chat->handlers->private_message) {
		chat->handlers->private_message(chat, ctx, msg);
	}

	send_message_reply(chat, ctx);
	return 0;
}
/* .. include_endpoint_chat_cli_rst_1 */

static int handle_message_reply(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;

	if (chat->handlers->message_reply) {
		chat->handlers->message_reply(chat, ctx);
	}

	return 0;
}

static int handle_presence(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;
	enum bt_mesh_chat_cli_presence presence;

	presence = net_buf_simple_pull_u8(buf);

	if (chat->handlers->presence) {
		chat->handlers->presence(chat, ctx, presence);
	}

	return 0;
}

static int handle_presence_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_CHAT_CLI_OP_PRESENCE,
				 BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE);

	encode_presence(&msg, chat->presence);

	(void)bt_mesh_model_send(chat->model, ctx, &msg, NULL, NULL);

	return 0;
}

/* .. include_startingpoint_chat_cli_rst_2 */
const struct bt_mesh_model_op _bt_mesh_chat_cli_op[] = {
	{
		BT_MESH_CHAT_CLI_OP_MESSAGE,
		BT_MESH_LEN_MIN(BT_MESH_CHAT_CLI_MSG_MINLEN_MESSAGE),
		handle_message
	},
	{
		BT_MESH_CHAT_CLI_OP_PRIVATE_MESSAGE,
		BT_MESH_LEN_MIN(BT_MESH_CHAT_CLI_MSG_MINLEN_MESSAGE),
		handle_private_message
	},
	{
		BT_MESH_CHAT_CLI_OP_MESSAGE_REPLY,
		BT_MESH_LEN_EXACT(BT_MESH_CHAT_CLI_MSG_LEN_MESSAGE_REPLY),
		handle_message_reply
	},
	{
		BT_MESH_CHAT_CLI_OP_PRESENCE,
		BT_MESH_LEN_EXACT(BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE),
		handle_presence
	},
	{
		BT_MESH_CHAT_CLI_OP_PRESENCE_GET,
		BT_MESH_LEN_EXACT(BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE_GET),
		handle_presence_get
	},
	BT_MESH_MODEL_OP_END,
};
/* .. include_endpoint_chat_cli_rst_2 */

static int bt_mesh_chat_cli_update_handler(const struct bt_mesh_model *model)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;

	/* Continue publishing current presence. */
	encode_presence(model->pub->msg, chat->presence);

	return 0;
}

/* .. include_startingpoint_chat_cli_rst_3 */
#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_chat_cli_settings_set(const struct bt_mesh_model *model,
					 const char *name,
					 size_t len_rd,
					 settings_read_cb read_cb,
					 void *cb_arg)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;

	if (name) {
		return -ENOENT;
	}

	ssize_t bytes = read_cb(cb_arg, &chat->presence,
				sizeof(chat->presence));
	if (bytes < 0) {
		return bytes;
	}

	if (bytes != 0 && bytes != sizeof(chat->presence)) {
		return -EINVAL;
	}

	return 0;
}
#endif
/* .. include_endpoint_chat_cli_rst_3 */

/* .. include_startingpoint_chat_cli_rst_4 */
static int bt_mesh_chat_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;

	chat->model = model;

	net_buf_simple_init_with_data(&chat->pub_msg, chat->buf,
				      sizeof(chat->buf));
	chat->pub.msg = &chat->pub_msg;
	chat->pub.update = bt_mesh_chat_cli_update_handler;

	return 0;
}
/* .. include_endpoint_chat_cli_rst_4 */

/* .. include_startingpoint_chat_cli_rst_5 */
static int bt_mesh_chat_cli_start(const struct bt_mesh_model *model)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;

	if (chat->handlers->start) {
		chat->handlers->start(chat);
	}

	return 0;
}
/* .. include_endpoint_chat_cli_rst_5 */

/* .. include_startingpoint_chat_cli_rst_6 */
static void bt_mesh_chat_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_chat_cli *chat = model->rt->user_data;

	chat->presence = BT_MESH_CHAT_CLI_PRESENCE_AVAILABLE;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void) bt_mesh_model_data_store(model, true, NULL, NULL, 0);
	}
}
/* .. include_endpoint_chat_cli_rst_6 */

/* .. include_startingpoint_chat_cli_rst_7 */
const struct bt_mesh_model_cb _bt_mesh_chat_cli_cb = {
	.init = bt_mesh_chat_cli_init,
	.start = bt_mesh_chat_cli_start,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = bt_mesh_chat_cli_settings_set,
#endif
	.reset = bt_mesh_chat_cli_reset,
};
/* .. include_endpoint_chat_cli_rst_7 */

/* .. include_startingpoint_chat_cli_rst_8 */
int bt_mesh_chat_cli_presence_set(struct bt_mesh_chat_cli *chat,
			 enum bt_mesh_chat_cli_presence presence)
{
	if (presence != chat->presence) {
		chat->presence = presence;

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			(void) bt_mesh_model_data_store(chat->model, true,
						NULL, &presence,
						sizeof(chat->presence));
		}
	}

	encode_presence(chat->model->pub->msg, chat->presence);

	return bt_mesh_model_publish(chat->model);
}
/* .. include_endpoint_chat_cli_rst_8 */

int bt_mesh_chat_cli_presence_get(struct bt_mesh_chat_cli *chat,
				  uint16_t addr)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = chat->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_CHAT_CLI_OP_PRESENCE_GET,
				 BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_CHAT_CLI_OP_PRESENCE_GET);

	return bt_mesh_model_send(chat->model, &ctx, &buf, NULL, NULL);
}

int bt_mesh_chat_cli_message_send(struct bt_mesh_chat_cli *chat,
				  const uint8_t *msg)
{
	struct net_buf_simple *buf = chat->model->pub->msg;

	bt_mesh_model_msg_init(buf, BT_MESH_CHAT_CLI_OP_MESSAGE);

	net_buf_simple_add_mem(buf, msg,
			       strnlen(msg,
				       CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH));
	net_buf_simple_add_u8(buf, '\0');

	return bt_mesh_model_publish(chat->model);
}

/* .. include_startingpoint_chat_cli_rst_9 */
int bt_mesh_chat_cli_private_message_send(struct bt_mesh_chat_cli *chat,
					  uint16_t addr,
					  const uint8_t *msg)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = chat->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_CHAT_CLI_OP_PRIVATE_MESSAGE,
				 BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE);
	bt_mesh_model_msg_init(&buf, BT_MESH_CHAT_CLI_OP_PRIVATE_MESSAGE);

	net_buf_simple_add_mem(&buf, msg,
			       strnlen(msg,
				       CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH));
	net_buf_simple_add_u8(&buf, '\0');

	return bt_mesh_model_send(chat->model, &ctx, &buf, NULL, NULL);
}
/* .. include_endpoint_chat_cli_rst_9 */
