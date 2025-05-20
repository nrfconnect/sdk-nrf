/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(compressed_app, LOG_LEVEL_DBG);

int main(void)
{
	LOG_INF("Compressed image running");

	return 0;
}
