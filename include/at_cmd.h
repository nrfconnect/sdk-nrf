/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef AT_CMD_H_
#define AT_CMD_H_

/**
 * @file at_cmd.h
 *
 * @defgroup at_cmd AT command interface driver
 *
 * @{
 *
 * @brief Public APIs for the AT command interface driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <stddef.h>

/**
 * @brief AT command return codes
 */
enum at_cmd_state {
	AT_CMD_OK,
	AT_CMD_ERROR,
	AT_CMD_ERROR_CMS,
	AT_CMD_ERROR_CME,
	AT_CMD_NOTIFICATION,
};

/**
 * @typedefs at_cmd_handler_t
 *
 * Because this driver let multiple threads share the same socket, it must make
 * sure that the correct thread gets the correct data returned from the AT
 * interface. The at_cmd_write_with_callback() function let the user specify
 * the handler that will process the data from a specific AT command call.
 * Notifications will be handled by the global handler defined using the
 * at_cmd_set_notification_handler() function. Both handlers are of the type
 * @ref at_cmd_handler_t.
 *
 * @param response     Null terminated string containing the modem message
 *
 */
typedef void (*at_cmd_handler_t)(const char *response);

/**@brief Initialize AT command driver.
 *
 * @return Zero on success, non-zero otherwise.
 */
int at_cmd_init(void);

/**
 * @brief Function to send an AT command to the modem, any data from the modem
 *        will trigger the callback defined by the handler parameter in the
 *        function prototype.
 *
 * @param cmd     Pointer to null terminated AT command string.
 * @param handler Pointer to handler that will process any returned data.
 *                NULL pointer is allowed, which means that any returned data
 *                will not processed other than the return code (OK, ERROR, CMS
 *                or CME).
 * @param state   Pointer to @ref enum at_cmd_state variable that can hold
 *                the error state returned by the modem. If the return state
 *                is a CMS or CME errors will the error code be returned in the
 *                the function return code as a positive value. NULL pointer is
 *                allowed.
 *
 * @retval 0 If command execution was successful (same as OK returned from
 *           modem). Error codes returned from the driver or by the socket are
 *           returned as negative values, CMS and CME errors are returned as
 *           positive values, the state parameter will indicate if it's a CME
 *           or CMS error. ERROR will return ENOEXEC (positve).
 *
 * @retval -ENOBUFS is returned if AT_CMD_RESPONSE_MAX_LEN is not large enough
 *         to hold the data returned from the modem.
 * @retval ENOEXEC is returned if the modem returned ERROR.
 * @retval -ENOMEM is returned if allocation of callback worker failed.
 * @retval -EIO is returned if the function failed to send the command.
 */
int at_cmd_write_with_callback(const char *const cmd,
					  at_cmd_handler_t  handler,
					  enum at_cmd_state *state);

/**
 * @brief Function to send an AT command and receive response immediately
 *
 * This function should be used if the response from the modem should be
 * returned in a user supplied buffer. This function will return an empty buffer
 * if nothing is returned by the modem.
 *
 * @param cmd Pointer to null terminated AT command string
 * @param buf Buffer to put the response in. NULL pointer is allowed, see
 *            behaviour explanation for @ref buf_len equals 0.
 * @param buf_len Length of response buffer. 0 length is allowed and will send
 *                the command, process the return code from the modem, but
 *                any returned data will be dropped.
 * @param state   Pointer to @ref enum at_cmd_state variable that can hold
 *                the error state returned by the modem. If the return state
 *                is a CMS or CME errors will the error code be returned in the
 *                the function return code as a positive value. NULL pointer is
 *                allowed.
 *
 * @note It is allowed to use the same buffer for both, @ref cmd and @ref buf
 *       parameters in order to save RAM. The function will not modify @ref buf
 *       contents until the entire @ref cmd is sent.
 *
 * @retval 0 If command execution was successful (same as OK returned from
 *           modem). Error codes returned from the driver or by the socket are
 *           returned as negative values, CMS and CME errors are returned as
 *           positive values, the state parameter will indicate if it's a CME
 *           or CMS error. ERROR will return ENOEXEC (positve).
 *
 * @retval -ENOBUFS is returned if AT_CMD_RESPONSE_MAX_LEN is not large enough
 *         to hold the data returned from the modem.
 * @retval ENOEXEC is returned if the modem returned ERROR.
 * @retval -EMSGSIZE is returned if the supplied buffer is to small or NULL.
 * @retval -EIO is returned if the function failed to send the command.
 *
 */
int at_cmd_write(const char *const cmd,
		 char *buf,
		 size_t buf_len,
		 enum at_cmd_state *state);

/**
 * @brief Function to set AT command global notification handler
 *
 * @param handler Pointer to a received notification handler function of type
 *                @ref at_cmd_handler_t.
 */
void at_cmd_set_notification_handler(at_cmd_handler_t handler);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_CMD_H_ */
