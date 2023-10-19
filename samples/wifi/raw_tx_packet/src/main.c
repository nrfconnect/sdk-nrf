/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Raw Tx Packet sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(raw_tx_packet, CONFIG_LOG_DEFAULT_LEVEL);

#include "net_private.h"
#include "wifi_connection.h"

int main(void)
{
	int status;

	status = try_wifi_connect();
	if (status < 0) {
		return status;
	}

	return 0;
}
