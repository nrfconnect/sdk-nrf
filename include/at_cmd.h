/**
 * @file at_cmd.h
 *
 * @brief Public APIs for the AT command driver.
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef ZEPHYR_INCLUDE_AT_CMD_H_
#define ZEPHYR_INCLUDE_AT_CMD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

#define CMD_MAX_LEN		CONFIG_AT_CMD_CMD_MAX_LEN
#define RESPONSE_MAX_LEN	CONFIG_AT_CMD_RESPONSE_MAX_LEN

typedef void (*at_cmd_handler_t)(char *cmd, size_t cmd_len,
				 char *response, size_t response_len);

struct at_cmd_client {
	u32_t id;
	at_cmd_handler_t handler;
	struct k_work work;
	char cmd[CMD_MAX_LEN];
	size_t cmd_len;
	char response[RESPONSE_MAX_LEN];
	size_t response_len;
	bool cmd_pending;
};

struct at_cmd_msg {
	struct at_cmd_client *client;
	u32_t timestamp;
	char *buf;
	size_t len;
};

struct at_cmd_client *at_cmd_client_init(at_cmd_handler_t handler);
void at_cmd_client_uninit(struct at_cmd_client *client);
int at_cmd_send(struct at_cmd_client *client, struct at_cmd_msg *cmd);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_AT_CMD_H_ */
