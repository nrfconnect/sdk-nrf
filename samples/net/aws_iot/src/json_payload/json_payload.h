/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

/* Structure used to populate and describe the JSON payload sent to AWS IoT. */
struct payload {
	struct {
		struct {
			const char *app_version;
			const char *modem_version;
			uint32_t uptime;
		} reported;
	} state;
};

/* @brief Construct a JSON message string.
 *
 * @param[out] message Pointer to a buffer that the JSON string is written to.
 * @param[in]  size    Size of the output buffer, message.
 * @param[in]  payload Pointer to a payload structure that will be used
 *	       to populate the JSON message.
 *
 * @return 0 on success, otherwise a negative value is returned.
 */
int json_payload_construct(char *message, size_t size, struct payload *payload);
