/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Idle application core main for nRF9251 sysbuild (NFC runs on CPUPPR).
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(writable_ndef_msg, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("Application core idle. The NFC Writable NDEF sample runs on CPUPPR");

	while (true) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
