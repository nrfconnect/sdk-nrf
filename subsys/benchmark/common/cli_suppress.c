/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <shell/shell.h>

static bool suppress_enabled;
static struct shell *p_shell;

void cli_suppress_init(struct shell *_shell)
{
	suppress_enabled = false;
	p_shell = _shell;
}

void cli_suppress_enable(void)
{
	shell_info(p_shell, "Suppressing CLI");

	suppress_enabled = true;

	shell_stop(p_shell);
}

void cli_suppress_disable(void)
{
	suppress_enabled = false;

	shell_start(p_shell);

	shell_info(p_shell, "Enabling CLI");
}

bool cli_suppress_is_enabled(void)
{
	return suppress_enabled;
}
