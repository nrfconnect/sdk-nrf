/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_CUSTOM_CMD_H_
#define AT_CUSTOM_CMD_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file at_custom_cmd.h
 *
 * @defgroup at_custom_cmd Custom AT commands
 *
 * @{
 *
 * @brief Public API for adding custom AT commands using filters in the Modem library with
 * application callbacks.
 *
 */

#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain/common.h>
#include <nrf_modem_at.h>

/**
 * @brief Fill response buffer without overflowing the buffer.
 *
 * @note For the modem library to accept the response in @c buf as a success, the response must
 *       contain "OK\\r\\n". If the response is an error, use "ERROR\\r\\n" or appropriate CMS/CME
 *       responses, e.g. "+CMS ERROR: <errorcode>".
 *
 * @param buf Buffer to put response into.
 * @param buf_size Size of the response buffer.
 * @param response Response format.
 * @param ... Format arguments.
 *
 * @retval 0 on success.
 * @retval -NRF_EFAULT if no buffer provided.
 * @retval -NRF_E2BIG if the provided buffer is too small for the response.
 */
int at_custom_cmd_respond(char *buf, size_t buf_size,
		const char *response, ...);

/* Custom command filter is paused. */
#define AT_CUSTOM_CMD_PAUSED 1
/* Custom command filter is active, default. */
#define AT_CUSTOM_CMD_ACTIVE 0

/**
 * @brief Define an custom AT command callback.
 *
 * @param entry The entry name.
 * @param _filter The (partial) AT command on which the callback should trigger.
 * @param _callback The AT command callback function.
 * @param ... Optional initial state ( @c AT_CUSTOM_CMD_PAUSED or @c AT_CUSTOM_CMD_ACTIVE ).
 *	      The default is @c AT_CUSTOM_CMD_ACTIVE .
 */
#define AT_CUSTOM_CMD(entry, _filter, _callback, ...)                                              \
	static int _callback(char *buf, size_t len, char *at_cmd);                                 \
	static STRUCT_SECTION_ITERABLE(nrf_modem_at_cmd_filter, entry) = {                         \
		.cmd = _filter,                                                                    \
		.callback = _callback,                                                             \
		COND_CODE_1(__VA_ARGS__, (.paused = __VA_ARGS__,), ())                             \
	}

/**
 * @brief Pause a custom AT command.
 *
 * @param entry Pointer to the custom AT command to pause.
 */
void at_custom_cmd_pause(struct nrf_modem_at_cmd_filter *entry);

/**
 * @brief Resume a custom AT command.
 *
 * @param entry Pointer to the custom AT command to resume.
 */
void at_custom_cmd_resume(struct nrf_modem_at_cmd_filter *entry);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_CMD_H_ */
