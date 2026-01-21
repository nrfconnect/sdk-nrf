/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(idle);

int main(void)
{
	LOG_INF("GRTC reset test");
	k_msleep(10);
	LOG_INF("k_cycle_get_32 = %u", k_cycle_get_32());
	LOG_INF("k_uptime_get = %llu", k_uptime_get());
	return 0;
}
