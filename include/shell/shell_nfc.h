/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SHELL_NFC_H_
#define SHELL_NFC_H_

#include <zephyr/shell/shell.h>

/**
 * @file
 * @defgroup shell_nfc NFC shell transport
 * @{
 * @brief NFC shell transport API.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function provides a pointer to the shell NFC backend instance.
 *
 * The function returns a pointer to the shell NFC instance. This instance can then be
 * used with the shell_execute_cmd function to test the behavior of the command.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_nfc_get_ptr(void);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* SHELL_NFC_H_ */
