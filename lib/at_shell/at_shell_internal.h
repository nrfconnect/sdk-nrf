/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_SHELL_INTERNAL_H__
#define AT_SHELL_INTERNAL_H__

#include <zephyr/kernel.h>
#include <stdbool.h>

struct shell;

/* AT command mode (if enabled) */
#if defined(CONFIG_AT_SHELL_CMD_MODE)
enum at_shell_cmd_mode_termination {
	AT_SHELL_CMD_MODE_TERM_NULL = 0,
	AT_SHELL_CMD_MODE_TERM_CR = 1,
	AT_SHELL_CMD_MODE_TERM_LF = 2,
	AT_SHELL_CMD_MODE_TERM_CR_LF = 3,
};

struct at_shell_cmd_mode_config {
	enum at_shell_cmd_mode_termination termination;
	bool echo;
};

struct k_work_q *at_shell_work_q_get(void);
void at_shell_submit_at_cmd_mode_work(struct k_work *work);

int at_shell_cmd_mode_start(const struct shell *sh, const struct at_shell_cmd_mode_config *cfg);
#endif /* CONFIG_AT_SHELL_CMD_MODE */

#endif /* AT_SHELL_INTERNAL_H__ */
