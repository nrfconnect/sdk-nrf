/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_SHELL_H__
#define AT_SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif

struct shell;
struct k_work_q;

/**
 * @file at_shell.h
 *
 * @defgroup at_shell AT shell
 * @{
 *
 * @brief Public APIs for the AT shell library.
 */

/**
 * @brief Configuration for the AT shell library.
 */
struct at_shell_config {
	/**
	 * Work queue used by AT command mode when sending commands.
	 *
	 * AT command mode is an interactive "shell bypass" mode
	 * where raw input is forwarded as AT commands.
	 *
	 * Required when @kconfig{CONFIG_AT_SHELL_CMD_MODE} is enabled.
	 * Must be non-NULL when passing a non-NULL config.
	 */
	struct k_work_q *at_cmd_mode_work_q;
};

/**
 * @brief Optional initialization for the AT shell library.
 *
 * @param cfg Configuration (may be NULL for defaults).
 *
 * @retval 0 on success.
 * @retval -EINVAL if configuration is invalid.
 */
int at_shell_init(const struct at_shell_config *cfg);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_SHELL_H__ */
