/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

LOG_MODULE_DECLARE(nrf_modem, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

NRF_MODEM_LIB_ON_INIT(cfun_init_hook, on_modem_init, NULL);

static void cfun_callback(int mode)
{
	STRUCT_SECTION_FOREACH(nrf_modem_lib_at_cfun_cb, e) {
		LOG_DBG("CFUN callback %p", e->callback);
		e->callback(mode, e->context);
	}
}

static void on_modem_init(int err, void *ctx)
{
	nrf_modem_at_cfun_handler_set(cfun_callback);
}
