/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include <zephyr/kernel.h>

#include <modem/at_shell.h>

#include "at_shell_internal.h"

#if defined(CONFIG_AT_SHELL_CMD_MODE)
static struct k_work_q *at_shell_work_q;
#endif

#if defined(CONFIG_AT_SHELL_CMD_MODE)
struct k_work_q *at_shell_work_q_get(void)
{
	return at_shell_work_q;
}
#endif

#if defined(CONFIG_AT_SHELL_CMD_MODE)
void at_shell_submit_at_cmd_mode_work(struct k_work *work)
{
	(void)k_work_submit_to_queue(at_shell_work_q, work);
}
#endif /* CONFIG_AT_SHELL_CMD_MODE */

int at_shell_init(const struct at_shell_config *cfg)
{
	if (cfg == NULL) {
#if defined(CONFIG_AT_SHELL_CMD_MODE)
		at_shell_work_q = NULL;
#endif
		return 0;
	}

#if defined(CONFIG_AT_SHELL_CMD_MODE)
	if (cfg->at_cmd_mode_work_q == NULL) {
		return -EINVAL;
	}
	at_shell_work_q = cfg->at_cmd_mode_work_q;
#endif

	return 0;
}
