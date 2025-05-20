/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _AT_CMD_REQUESTS_H_
#define _AT_CMD_REQUESTS_H_

/**
 * @brief Forwards an AT command string to the modem, and returns a pointer
 * to a buffer containing the modem's response, or a human readable error.
 *
 * If CONFIG_NRF_MODEM_LIB is not enabled, the function is not compiled and always
 * returns an error string.
 *
 * Please note that the returned response buffer becomes unsafe to use as soon as
 * the next AT command request is made, so be sure to copy or handle the response before performing
 * another request.
 *
 * Do not modify the string in the returned response buffer.
 *
 * @param cmd - The AT command to execute.
 * @return char* a pointer into a NULL-terminated buffer containing the command response.
 */
#if defined(CONFIG_NRF_MODEM_LIB)
char *execute_at_cmd_request(const char *const cmd);
#else /* CONFIG_NRF_MODEM_LIB */
#define execute_at_cmd_request(...) "AT command requests are not supported by this build!"
#endif /* CONFIG_NRF_MODEM_LIB */

#endif /* _AT_CMD_REQUESTS_H_ */
