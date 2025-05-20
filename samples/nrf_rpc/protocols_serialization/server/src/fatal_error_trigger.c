/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <dk_buttons_and_leds.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(nrf_ps_server, CONFIG_NRF_PS_SERVER_LOG_LEVEL);

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if (button_state & DK_BTN1_MSK) {
		LOG_ERR("Fatal error button pressed - triggering k_oops");
		k_oops();
	}
}

static int button_handler_init(void)
{
	return dk_buttons_init(button_handler);
}

SYS_INIT(button_handler_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
