/*
 * Copyright (c) 2022 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_CMD_H_
#define AT_CMD_H_

/**
 * @file at_host.h
 *
 * @defgroup at_host AT command interface driver to register custom handler
 *
 * @{
 *
 * @brief Public APIs for the AT custom command interface driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef at_cmd_custom_handler_t
 *
 * This type defines a custom handler which will manage the custom commands
 * received by the modem.
 *
 * @param cmd Pointer to null terminated AT command string
 * @param buf Buffer to put the response in. NULL pointer is allowed, see
 *            behaviour explanation for @p buf_len equals 0.
 * @param buf_len Length of response buffer. 0 length is allowed and will send
 *                the command, process the return code from the modem, but
 *                any returned data will be dropped.
 * @retval -ENOBUFS is returned if AT_CMD_RESPONSE_MAX_LEN is not large enough
 *         to hold the data returned from the modem.
 * @retval -ENOEXEC is returned if the modem returned ERROR.
 * @retval -EMSGSIZE is returned if the supplied buffer is to small or NULL.
 */
typedef int (*at_cmd_custom_handler_t)(const char *const cmd,
				       char *buf,
				       size_t buf_len,
				       enum at_cmd_state *state);

/**
 * @brief Sets the handler called when a custom commands is received.
 *
 * @note This function requires the CONFIG_AT_HOST_CUSTOM_COMMANDS to be set to
 * have any effect.
 *
 * @param handler Pointer to a notification handler function of type
 *                @ref at_cmd_custom_handler_t.
 */
void at_cmd_set_custom_handler(at_cmd_custom_handler_t handler);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_CMD_H_ */
