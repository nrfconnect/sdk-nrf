/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

static const u32_t beef = 0xBEEF;

void main(void)
{
	u32_t x = 0xDEADCAFE;

	while (true) {
		LOG_ERR("Hello World!");
		LOG_WRN("beef: %x", beef);
		LOG_INF("beef: %x    x: %x", beef, x);
		LOG_DBG("beef: %x %d x: %x", beef, beef, x);
		LOG_INF("beef: %x %d x: %x %d", beef, beef, x, x);
		LOG_HEXDUMP_WRN((uint8_t *)&x, sizeof(x), "");
		k_sleep(1000);
	}
}
