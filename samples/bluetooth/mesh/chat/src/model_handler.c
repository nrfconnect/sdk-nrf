/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include "chat_cli.h"
#include "model_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(chat);

static const struct shell *chat_shell;

/******************************************************************************/
/*************************** Health server setup ******************************/
/******************************************************************************/
/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;

static void attention_blink(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
		BIT(0) | BIT(1),
		BIT(1) | BIT(2),
		BIT(2) | BIT(3),
		BIT(3) | BIT(0),
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	} else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static void attention_on(const struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(const struct bt_mesh_model *mod)
{
	/* Will stop rescheduling blink timer */
	attention = false;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

/******************************************************************************/
/***************************** Chat model setup *******************************/
/******************************************************************************/
struct presence_cache {
	uint16_t addr;
	enum bt_mesh_chat_cli_presence presence;
};

/* Cache of Presence values of other chat clients. */
static struct presence_cache presence_cache[
			    CONFIG_BT_MESH_CHAT_SAMPLE_PRESENCE_CACHE_SIZE];

static const uint8_t *presence_string[] = {
	[BT_MESH_CHAT_CLI_PRESENCE_AVAILABLE] = "available",
	[BT_MESH_CHAT_CLI_PRESENCE_AWAY] = "away",
	[BT_MESH_CHAT_CLI_PRESENCE_DO_NOT_DISTURB] = "dnd",
	[BT_MESH_CHAT_CLI_PRESENCE_INACTIVE] = "inactive",
};

/**
 * Returns true if the specified address is an address of the local element.
 */
static bool address_is_local(const struct bt_mesh_model *mod, uint16_t addr)
{
	return bt_mesh_model_elem(mod)->rt->addr == addr;
}

/**
 * Returns true if the provided address is unicast address.
 */
static bool address_is_unicast(uint16_t addr)
{
	return (addr > 0) && (addr <= 0x7FFF);
}

/**
 * Returns true if the node is new or the presence status is different from
 * the one stored in the cache.
 */
static bool presence_cache_entry_check_and_update(uint16_t addr,
				       enum bt_mesh_chat_cli_presence presence)
{
	static size_t presence_cache_head;
	size_t i;

	/* Find address in cache. */
	for (i = 0; i < ARRAY_SIZE(presence_cache); i++) {
		if (presence_cache[i].addr == addr) {
			if (presence_cache[i].presence == presence) {
				return false;
			}

			/* Break since the node in the cache. */
			break;
		}
	}

	/* Not in cache. */
	if (i == ARRAY_SIZE(presence_cache)) {
		for (i = 0; i < ARRAY_SIZE(presence_cache); i++) {
			if (!presence_cache[i].addr) {
				break;
			}
		}

		/* Cache is full. */
		if (i == ARRAY_SIZE(presence_cache)) {
			i = presence_cache_head;
			presence_cache_head = (presence_cache_head + 1)
			% CONFIG_BT_MESH_CHAT_SAMPLE_PRESENCE_CACHE_SIZE;
		}
	}

	/* Update cache. */
	presence_cache[i].addr = addr;
	presence_cache[i].presence = presence;

	return true;
}

static void print_client_status(void);

static void handle_chat_start(struct bt_mesh_chat_cli *chat)
{
	print_client_status();
}

static void handle_chat_presence(struct bt_mesh_chat_cli *chat,
				 struct bt_mesh_msg_ctx *ctx,
				 enum bt_mesh_chat_cli_presence presence)
{
	if (address_is_local(chat->model, ctx->addr)) {
		if (address_is_unicast(ctx->recv_dst)) {
			shell_print(chat_shell, "<you> are %s",
				    presence_string[presence]);
		}
	} else {
		if (address_is_unicast(ctx->recv_dst)) {
			shell_print(chat_shell, "<0x%04X> is %s", ctx->addr,
				    presence_string[presence]);
		} else if (presence_cache_entry_check_and_update(ctx->addr,
								 presence)) {
			shell_print(chat_shell, "<0x%04X> is now %s",
				    ctx->addr,
				    presence_string[presence]);
		}
	}
}

static void handle_chat_message(struct bt_mesh_chat_cli *chat,
				struct bt_mesh_msg_ctx *ctx,
				const uint8_t *msg)
{
	/* Don't print own messages. */
	if (address_is_local(chat->model, ctx->addr)) {
		return;
	}

	shell_print(chat_shell, "<0x%04X>: %s", ctx->addr, msg);
}

static void handle_chat_private_message(struct bt_mesh_chat_cli *chat,
					struct bt_mesh_msg_ctx *ctx,
					const uint8_t *msg)
{
	/* Don't print own messages. */
	if (address_is_local(chat->model, ctx->addr)) {
		return;
	}

	shell_print(chat_shell, "<0x%04X>: *you* %s", ctx->addr, msg);
}

static void handle_chat_message_reply(struct bt_mesh_chat_cli *chat,
				      struct bt_mesh_msg_ctx *ctx)
{
	shell_print(chat_shell, "<0x%04X> received the message", ctx->addr);
}

static const struct bt_mesh_chat_cli_handlers chat_handlers = {
	.start = handle_chat_start,
	.presence = handle_chat_presence,
	.message = handle_chat_message,
	.private_message = handle_chat_private_message,
	.message_reply = handle_chat_message_reply,
};

/* .. include_startingpoint_model_handler_rst_1 */
static struct bt_mesh_chat_cli chat = {
	.handlers = &chat_handlers,
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(
		1,
		BT_MESH_MODEL_LIST(
			BT_MESH_MODEL_CFG_SRV,
			BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
		BT_MESH_MODEL_LIST(BT_MESH_MODEL_CHAT_CLI(&chat))),
};
/* .. include_endpoint_model_handler_rst_1 */

static void print_client_status(void)
{
	if (!bt_mesh_is_provisioned()) {
		shell_print(chat_shell,
			    "The mesh node is not provisioned. Please provision the mesh node before using the chat.");
	} else {
		shell_print(chat_shell,
			    "The mesh node is provisioned. The client address is 0x%04x.",
			    bt_mesh_model_elem(chat.model)->rt->addr);
	}

	shell_print(chat_shell, "Current presence: %s",
		    presence_string[chat.presence]);
}

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

/******************************************************************************/
/******************************** Chat shell **********************************/
/******************************************************************************/
static int cmd_status(const struct shell *shell, size_t argc, char *argv[])
{
	print_client_status();

	return 0;
}

static int cmd_message(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	err = bt_mesh_chat_cli_message_send(&chat, argv[1]);
	if (err) {
		LOG_WRN("Failed to send message: %d", err);
	}

	/* Print own messages in the chat. */
	shell_print(shell, "<you>: %s", argv[1]);

	return 0;
}

static int cmd_private_message(const struct shell *shell, size_t argc,
			       char *argv[])
{
	uint16_t addr;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	addr = strtol(argv[1], NULL, 0);

	/* Print own message to the chat. */
	shell_print(shell, "<you>: *0x%04X* %s", addr, argv[2]);

	err = bt_mesh_chat_cli_private_message_send(&chat, addr, argv[2]);
	if (err) {
		LOG_WRN("Failed to publish message: %d", err);
	}

	return 0;
}

static int cmd_presence_set(const struct shell *shell, size_t argc,
			    char *argv[])
{
	size_t i;

	if (argc < 2) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(presence_string); i++) {
		if (!strcmp(argv[1], presence_string[i])) {
			enum bt_mesh_chat_cli_presence presence;
			int err;

			presence = i;

			err = bt_mesh_chat_cli_presence_set(&chat, presence);
			if (err) {
				LOG_WRN("Failed to update presence: %d", err);
			}

			/* Print own presence in the chat. */
			shell_print(shell, "You are now %s",
				    presence_string[presence]);

			return 0;
		}
	}

	shell_print(shell,
		    "Unknown presence status: %s. Possible presence statuses:",
		    argv[1]);
	for (i = 0; i < ARRAY_SIZE(presence_string); i++) {
		shell_print(shell, "%s", presence_string[i]);
	}

	return 0;
}

static int cmd_presence_get(const struct shell *shell, size_t argc,
			    char *argv[])
{
	uint16_t addr;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	addr = strtol(argv[1], NULL, 0);

	err = bt_mesh_chat_cli_presence_get(&chat, addr);
	if (err) {
		LOG_WRN("Failed to publish message: %d", err);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(presence_cmds,
	SHELL_CMD_ARG(set, NULL,
		      "Set presence of the current client <presence: available, away, dnd or inactive>",
		      cmd_presence_set, 2, 0),
	SHELL_CMD_ARG(get, NULL,
		      "Get presence status of the remote node <node>",
		      cmd_presence_get, 2, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_presence(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	if (argc != 3) {
		return -EINVAL;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(chat_cmds,
	SHELL_CMD_ARG(status, NULL, "Print client status", cmd_status, 1, 0),
	SHELL_CMD(presence, &presence_cmds, "Presence commands", cmd_presence),
	SHELL_CMD_ARG(private, NULL,
		      "Send a private text message to a client <node> <message>",
		      cmd_private_message, 3, 0),
	SHELL_CMD_ARG(msg, NULL, "Send a text message to the chat <message>",
		      cmd_message, 2, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_chat(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(chat, &chat_cmds, "Bluetooth Mesh Chat Client commands",
		       cmd_chat, 1, 1);

/******************************************************************************/
/******************************** Public API **********************************/
/******************************************************************************/
const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);

	chat_shell = shell_backend_uart_get_ptr();

	return &comp;
}
