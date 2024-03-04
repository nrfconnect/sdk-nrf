/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: Communication interface for library */

#include <stdint.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include "comm_proc.h"
#include "ptt_uart_api.h"

LOG_MODULE_REGISTER(comm);

static char cmd_buf[COMM_MAX_TEXT_DATA_SIZE];
static const struct shell *shell;

static int cmd_custom(const struct shell *shell_inst, size_t argc, char **argv)
{
	size_t max_append_len = 0;

	memset(cmd_buf, 0, sizeof(cmd_buf));

	for (int i = 0; i < argc; i++) {
		/* cmd_buf should contain space for the command and a NULL character.
		 * Hence we reduce 1 here to compute the max_append_len of the maximum allowed
		 * length of string that can be appended.
		 */
		max_append_len = COMM_MAX_TEXT_DATA_SIZE - strlen(cmd_buf) - 1;
		if (i < (argc - 1)) {
			/* For every command except the last one, we append a space character. So we
			 * need to ensure that there is space for that as well.
			 */
			max_append_len = max_append_len - 1;
		}
		if (COMM_MAX_TEXT_DATA_SIZE < (strlen(cmd_buf) + max_append_len)) {
			shell_error(shell_inst, "the command is too long");
			return 0;
		}
		strncat(cmd_buf, argv[i], max_append_len);

		if (i < (argc - 1)) {
			strcat(cmd_buf, " ");
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

	size_t cnt = 0;

	STRUCT_SECTION_COUNT(shell, &cnt);

	/*
	 * The assumption is that this sample always uses only  a single shell backend.
	 * That is why it assumed, that the first available shell backend should be used
	 * for communication callback messages.
	 */
	if (cnt >= 1) {
		STRUCT_SECTION_GET(shell, 0, &shell);
	}

	ptt_uart_init(comm_send_cb);
}
