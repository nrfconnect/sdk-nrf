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

static void cfun_callback(int mode)
{
	STRUCT_SECTION_FOREACH(nrf_modem_lib_at_cfun_cb, e) {
		LOG_DBG("CFUN callback %p", e->callback);
		e->callback(mode, e->context);
	}
}

static int nrf_modem_lib_cfun_hooks_init(void)
{
	nrf_modem_at_cfun_handler_set(cfun_callback);

	return 0;
}

SYS_INIT(nrf_modem_lib_cfun_hooks_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
