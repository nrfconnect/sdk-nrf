/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file nrf_modem_at.h
 *
 * @defgroup nrf_modem_at AT commands interface
 * @{
 * @brief Modem library AT commands interface.
 */
#ifndef NRF_MODEM_AT_H__
#define NRF_MODEM_AT_H__

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Modem response type for 'ERROR' responses. */
#define NRF_MODEM_AT_ERROR 1
/** @brief Modem response type for '+CME ERROR' responses. */
#define NRF_MODEM_AT_CME_ERROR 2
/** @brief Modem response type for '+CMS ERROR' responses. */
#define NRF_MODEM_AT_CMS_ERROR 3

/**
 * @brief AT Notification handler prototype.
 *
 * @note This handler is executed in an interrupt service routine.
 * Offload any intensive operation as necessary.
 *
 * @param notif The AT notification.
 */
typedef void (*nrf_modem_at_notif_handler_t)(const char *notif);

/**
 * @brief AT response handler prototype.
 *
 * @note This handler is executed in an interrupt service routine.
 * Offload any intensive operations as necessary.
 *
 * @param resp The AT command response.
 */
typedef void (*nrf_modem_at_resp_handler_t)(const char *resp);

/** @brief AT filter callback function format
 *
 * @note This declaration is used for the callback functions
 * of AT commands in the nRF SDK.
 *
 * @param buf Buffer to receive the response into.
 * @param len Buffer length.
 * @param at_cmd AT command.
 *
 * @retval  0 On "OK" responses.
 * @returns A positive value On "ERROR", "+CME ERROR", and "+CMS ERROR" responses.
 *	    The type of error can be distinguished using @c nrf_modem_at_err_type.
 *	    The error value can be retrieved using @c nrf_modem_at_err.
 * @retval -NRF_EPERM The Modem library is not initialized.
 * @retval -NRF_EFAULT @c buf or @c fmt are @c NULL.
 * @retval -NRF_ENOMEM Not enough shared memory for this request.
 * @retval -NRF_E2BIG The response is larger than the supplied buffer @c buf.
 */
typedef int (*nrf_modem_at_cmd_handler_t)(char *buf, size_t len, char *at_cmd);

/* Struct for AT filter
 * Contains string for which the AT commands are compared
 * and a function pointer for the function to call on detection.
 */
struct nrf_modem_at_cmd_filter {
	const char * const cmd;
	const nrf_modem_at_cmd_handler_t callback;
	/** Whether filter is paused. */
	bool paused;
};

/**
 * @brief Set a handler function for AT notifications.
 *
 * @note The callback is executed in an interrupt service routine.
 *	 Take care to offload any processing as appropriate.
 *
 * @param callback The AT notification callback. Use @c NULL to unset handler.
 *
 * @retval 0 On success.
 */
int nrf_modem_at_notif_handler_set(nrf_modem_at_notif_handler_t callback);

/**
 * @brief Send a formatted AT command to the modem.
 *
 * Supports all format specifiers of printf() as implemented by the selected C library.
 * This function can return a negative value, zero or a positive value.
 *
 * @param fmt Command format.
 * @param ... Format arguments.
 *
 * @retval  0 On "OK" responses.
 * @returns A positive value On "ERROR", "+CME ERROR", and "+CMS ERROR" responses.
 *	    The type of error can be distinguished using @c nrf_modem_at_err_type.
 *	    The error value can be retrieved using @c nrf_modem_at_err.
 * @retval -NRF_EPERM The Modem library is not initialized.
 * @retval -NRF_EFAULT @c fmt is @c NULL.
 * @retval -NRF_EINVAL Bad format @c fmt.
 * @retval -NRF_ENOMEM Not enough shared memory for this request.
 * @retval -NRF_ESHUTDOWN If modem was shut down.
 */
int nrf_modem_at_printf(const char *fmt, ...);

/**
 * @brief Send an AT command to the modem and read the formatted response
 *	  into the supplied argument list.
 *
 * Supports all the format specifiers of scanf() as implemented by the selected C library.
 * This function does not support retrieving the modem response beyond reading
 * the formatted response into the argument list.
 *
 * @param cmd AT command.
 * @param fmt Response format.
 * @param ... Variable argument list.
 *
 * @returns The number of arguments matched.
 * @retval -NRF_EPERM The Modem library is not initialized.
 * @retval -NRF_EFAULT @c cmd or @c fmt are @c NULL.
 * @retval -NRF_EBADMSG No arguments were matched.
 * @retval -NRF_ENOMEM Not enough shared memory for this request.
 * @retval -NRF_ESHUTDOWN If the modem was shut down.
 */
int nrf_modem_at_scanf(const char *cmd, const char *fmt, ...);

/**
 * @brief Send a formatted AT command to the modem
 *	  and receive the response into the supplied buffer.
 *
 * @param buf Buffer to receive the response into.
 * @param len Buffer length.
 * @param fmt Command format.
 * @param ... Format arguments.
 *
 * @retval  0 On "OK" responses.
 * @returns A positive value On "ERROR", "+CME ERROR", and "+CMS ERROR" responses.
 *	    The type of error can be distinguished using @c nrf_modem_at_err_type.
 *	    The error value can be retrieved using @c nrf_modem_at_err.
 * @retval -NRF_EPERM The Modem library is not initialized.
 * @retval -NRF_EFAULT @c buf or @c fmt are @c NULL.
 * @retval -NRF_EINVAL Bad format @c fmt, or @c len is zero.
 * @retval -NRF_ENOMEM Not enough shared memory for this request.
 * @retval -NRF_E2BIG The response is larger than the supplied buffer @c buf.
 * @retval -NRF_ESHUTDOWN If the modem was shut down.
 */
int nrf_modem_at_cmd(void *buf, size_t len, const char *fmt, ...);

/**
 * @brief Send a formatted AT command to the modem
 *	  and receive the response asynchronously via a callback.
 *
 * This function waits for the Modem to acknowledge the AT command
 * but will return without waiting for the command execution.
 *
 * @note The callback is executed in an interrupt context.
 *	 Take care to offload any processing as appropriate.
 *
 * @param callback Callback to receive the response.
 * @param fmt Command format.
 * @param ... Format arguments.
 *
 * @retval  0 On "OK" responses.
 * @retval -NRF_EPERM The Modem library is not initialized.
 * @retval -NRF_EFAULT @c callback or @c fmt are @c NULL.
 * @retval -NRF_EINVAL Bad format @c fmt.
 * @retval -NRF_EINPROGRESS An asynchrounous request is already in progress.
 * @retval -NRF_ENOMEM Not enough shared memory for this request.
 * @retval -NRF_ESHUTDOWN If the modem was shut down.
 */
int nrf_modem_at_cmd_async(nrf_modem_at_resp_handler_t callback, const char *fmt, ...);

/**
 * @brief Return the error type represented by the return value of
 *	  @c nrf_modem_at_printf and @c nrf_modem_at_cmd.
 *
 * @param error The return value of @c nrf_modem_at_printf or @c nrf_modem_at_cmd.
 *
 * @retval NRF_MODEM_AT_ERROR If the modem response was 'ERROR'.
 * @retval NRF_MODEM_AT_CME_ERROR If the modem response was '+CME ERROR'.
 * @retval NRF_MODEM_AT_CMS_ERROR If the modem response was '+CMS ERROR'.
 */
int nrf_modem_at_err_type(int error);

/**
 * @brief Retrieve the specific CME or CMS error from the return value of
 *	  a @c nrf_modem_at_printf or @c nrf_modem_at_cmd call.
 *
 * @param error The return value of a @c nrf_modem_at_printf or @c nrf_modem_at_cmd call.
 *
 * @returns int The CME or CMS error code.
 */
int nrf_modem_at_err(int error);

/**
 * @brief Set a list of AT commands to be filtered by @c nrf_modem_at_cmd.
 *
 * When a filter list is set, AT commands sent via @c nrf_modem_at_cmd that match any
 * AT command in the filter will be redirected to the filter callback function instead
 * of being sent to the modem.
 *
 * @note The filter is disabled by passing NULL to the @c filters and
 * 0 to the @c len.
 *
 * @param filters AT filter list.
 * @param len AT filter list size.
 *
 * @retval  0 On a success.
 * @retval -NRF_EINVAL On invalid parameters.
 */
int nrf_modem_at_cmd_filter_set(struct nrf_modem_at_cmd_filter *filters,
		size_t len);

#ifdef __cplusplus
}
#endif

#endif /* NRF_MODEM_AT_H__ */

/** @} */
