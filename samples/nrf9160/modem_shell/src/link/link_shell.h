/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LINK_SHELL_H
#define MOSH_LINK_SHELL_H

#include <shell/shell.h>
#include <modem/lte_lc.h>

int link_shell(const struct shell *shell, size_t argc, char **argv);

int link_shell_get_and_print_current_system_modes(
	const struct shell *shell, enum lte_lc_system_mode *sys_mode_current,
	enum lte_lc_system_mode_preference *sys_mode_preferred,
	enum lte_lc_lte_mode *currently_active_mode);

#endif /* MOSH_LINK_SHELL_H */
