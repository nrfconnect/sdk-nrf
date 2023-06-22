/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_at.h>
#include <nrf_errno.h>
#include <modem/location.h>
#include <cJSON.h>
#include <stdio.h>

#include "nrf_cloud_codec_internal.h"
#include "connection.h"
#include "at_commands.h"

LOG_MODULE_REGISTER(at_cmd_execution, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

/* AT command request error handling */
#define AT_CMD_REQUEST_ERR_FORMAT "Error while processing AT command request: %d"
#define AT_CMD_REQUEST_ERR_MAX_LEN (sizeof(AT_CMD_REQUEST_ERR_FORMAT) + 20)
BUILD_ASSERT(CONFIG_AT_CMD_REQUEST_RESPONSE_BUFFER_LENGTH >= AT_CMD_REQUEST_ERR_MAX_LEN,
	     "Not enough AT command response buffer for printing error events.");

/* Buffer to contain modem responses when performing AT command requests */
static char at_cmd_resp_buf[CONFIG_AT_CMD_REQUEST_RESPONSE_BUFFER_LENGTH];

char *execute_at_cmd_request(const char *const cmd)
{
	LOG_DBG("Modem AT command requested: %s", cmd);

	/* Clear the response buffer */
	memset(at_cmd_resp_buf, 0, sizeof(at_cmd_resp_buf));

	/* Pass the command off to the modem.
	 *
	 * We must pass the command in using a format specifier it might contain special characters
	 * such as %.
	 *
	 * We subtract 1 from the passed-in response buffer length to ensure that the response is
	 * always null-terminated, even when the response is longer than the response buffer size.
	 */

	int err = nrf_modem_at_cmd(at_cmd_resp_buf, sizeof(at_cmd_resp_buf) - 1, "%s", cmd);

	/* Post-process the response */
	LOG_DBG("Modem AT command response (%d, %d): %s",
		nrf_modem_at_err_type(err), nrf_modem_at_err(err), at_cmd_resp_buf);

	/* Trim \r\n from modem response for better readability in the portal. */
	at_cmd_resp_buf[MAX(0, strlen(at_cmd_resp_buf) - 2)] = '\0';

	/* If an error occurred with the request, report it */
	if (err < 0) {
		/* Negative error codes indicate an error with the modem lib itself, so the
		 * response buffer will be empty (or filled with junk). Thus, we can print the
		 * error message directly into it.
		 */
		snprintf(at_cmd_resp_buf, sizeof(at_cmd_resp_buf), AT_CMD_REQUEST_ERR_FORMAT, err);
		LOG_ERR("%s", at_cmd_resp_buf);
	}

	/* Return a pointer to the start of the command response buffer (which is now populated with
	 * the AT command response)
	 */
	return &at_cmd_resp_buf[0];
}
