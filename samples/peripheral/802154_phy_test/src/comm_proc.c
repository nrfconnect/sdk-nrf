/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: Communication interface for library */

#include <stdint.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(comm);

#include "comm_proc.h"
#include "ptt_uart_api.h"

static char cmd_buf[COMM_MAX_TEXT_DATA_SIZE];
static const struct shell *shell;

static int cmd_custom(const struct shell *shell, size_t argc, char **argv)
{
	size_t len = 0;

	memset(cmd_buf, 0, sizeof(cmd_buf));

	for (int i = 0; i < argc; i++) {
		len = COMM_MAX_TEXT_DATA_SIZE - strlen(cmd_buf);
		if (len < strlen(argv[i])) {
			shell_error(shell, "the command is too long");
			return 0;
		}

		strncat(cmd_buf, argv[i], len);

		if (i < (argc - 1)) {
			strncat(cmd_buf, " ", 1);
		}
	}

	LOG_DBG("Assembled command: %s", cmd_buf);
	ptt_uart_push_packet((uint8_t *)cmd_buf, strlen(cmd_buf));

	return 0;
}
SHELL_CMD_REGISTER(custom, NULL, "PHY test tool custom commands", cmd_custom);

static int32_t comm_send_cb(const uint8_t *pkt, ptt_pkt_len_t len, bool add_crlf)
{
	/*
	 * add_crlf argument is used only by prompt print routine. It is used here as an
	 * indicator that a prompt is being printed. The prompt is deliberately not being printed.
	 */
	if (add_crlf) {
		shell_print(shell, "%s", pkt);
	}

	return len;
}

void comm_init(void)
{
	/* won't initialize UART if the device has DUT mode only */
	if (IS_ENABLED(CONFIG_PTT_DUT_MODE)) {
		LOG_INF("Device is in DUT mode only.");
		return;
	}

	shell = shell_backend_uart_get_ptr();
	ptt_uart_init(comm_send_cb);
}
