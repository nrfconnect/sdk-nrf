/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SHELL_IPC_H_
#define SHELL_IPC_H_

#include <zephyr/shell/shell.h>

/**
 * @file
 * @defgroup shell_ipc IPC service shell transport
 * @{
 * @brief IPC Service shell transport API.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function provides a pointer to the shell IPC Service backend instance.
 *
 * The function returns a pointer to the shell IPC Service instance. This instance can then be
 * used with the shell_execute_cmd function to test the behavior of the command.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_ipc_get_ptr(void);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* SHELL_IPC_H_ */
